/**
 * @file spo2_pulse.cpp
 * @brief Implementation of MAX30102 PPG sampling, ratio-of-ratios SpO2, and signal validity
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#include "spo2_pulse.h"
#include <Wire.h>
#include <math.h>

Spo2PulseSensor::Spo2PulseSensor()
    : _sensorInitialized(false),
      _contactDetected(false),
      _redDc(0.0f),
      _irDc(0.0f),
      _redAc(0.0f),
      _irAc(0.0f),
      _lastPeakTimeMs(0),
      _pulseIntervalIdx(0),
      _pulseIntervalCount(0),
      _pulseRising(false),
      _prevIrAc(0.0f),
      _heartRateBpm(0),
      _spo2Percent(0),
      _quality(SIGNAL_INVALID) {
    memset(_pulseIntervals, 0, sizeof(_pulseIntervals));
}

bool Spo2PulseSensor::begin() {
    // Verify MAX30102 is reachable on shared I2C bus
    if (!_sensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println(F("[MAX30102] Error: Sensor not found at I2C address 0x57!"));
        _sensorInitialized = false;
        return false;
    }

    // Configure sensor parameters for continuous PPG acquisition
    // LED Brightness: 0x1F (~6.4mA) to prevent excessive tissue heating and battery drain
    byte ledBrightness = 0x1F;
    byte sampleAverage = 4;   // Average 4 samples per FIFO reading
    byte ledMode = 2;         // Red + IR dual LED mode for SpO2
    int sampleRate = 100;     // 100 samples per second
    int pulseWidth = 411;     // 411 us pulse width (18-bit ADC resolution)
    int adcRange = 4096;      // Full scale 4096 nA

    _sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    _sensorInitialized = true;
    Serial.println(F("[MAX30102] Initialized successfully. Dual LED mode (Red + IR)."));
    return true;
}

void Spo2PulseSensor::update() {
    if (!_sensorInitialized) {
        _quality = SIGNAL_INVALID;
        _heartRateBpm = 0;
        _spo2Percent = 0;
        return;
    }

    // Check if new samples are available in the FIFO
    _sensor.check();
    while (_sensor.available()) {
        uint32_t rawRed = _sensor.getFIFORed();
        uint32_t rawIr  = _sensor.getFIFOIR();
        _sensor.nextSample();

        processPpgSample(rawRed, rawIr);
    }
}

void Spo2PulseSensor::updateSimulated(uint8_t simHeartRate, uint8_t simSpo2, SignalQuality quality) {
    _heartRateBpm = simHeartRate;
    _spo2Percent = simSpo2;
    _quality = quality;
    _contactDetected = (quality != SIGNAL_INVALID);
}

void Spo2PulseSensor::processPpgSample(uint32_t rawRed, uint32_t rawIr) {
    uint32_t nowMs = millis();

    // 1. Contact Validation: Minimum light threshold
    if (rawIr < PPG_MIN_CONTACT_RAW_COUNT || rawRed < PPG_MIN_CONTACT_RAW_COUNT) {
        // No tissue / finger contact detected
        _contactDetected = false;
        _quality = SIGNAL_INVALID;
        _heartRateBpm = 0;
        _spo2Percent = 0;
        _redDc = 0.0f;
        _irDc = 0.0f;
        return;
    }

    _contactDetected = true;

    // 2. Exponential Moving Average to isolate DC baseline from AC cardiac pulse
    if (_irDc < 100.0f) {
        // Initialize baselines
        _irDc = (float)rawIr;
        _redDc = (float)rawRed;
    } else {
        const float alpha = 0.95f;
        _irDc = (alpha * _irDc) + ((1.0f - alpha) * (float)rawIr);
        _redDc = (alpha * _redDc) + ((1.0f - alpha) * (float)rawRed);
    }

    // AC pulsatile signal
    _irAc = (float)rawIr - _irDc;
    _redAc = (float)rawRed - _redDc;

    // 3. Peak Detection for Heart Rate on the IR AC channel
    evaluatePulsePeak(_irAc, nowMs);

    // 4. Compute SpO2 Ratio-of-Ratios
    computeSpo2Ratio();
}

void Spo2PulseSensor::evaluatePulsePeak(float currentIrAc, uint32_t nowMs) {
    // Cardiac peak detector: looking for local maximum above a noise floor
    const float minPulseAmplitude = 40.0f; // Minimum AC count to count as systolic wave

    if (currentIrAc > _prevIrAc) {
        _pulseRising = true;
    } else {
        if (_pulseRising && _prevIrAc > minPulseAmplitude) {
            // Potential systolic peak
            uint32_t elapsed = nowMs - _lastPeakTimeMs;
            // Refractory period: 300 ms (corresponds to max 200 BPM)
            // Timeout limit: 1500 ms (corresponds to min 40 BPM)
            if (elapsed >= 300U && elapsed <= 1500U) {
                _pulseIntervals[_pulseIntervalIdx] = elapsed;
                _pulseIntervalIdx = (_pulseIntervalIdx + 1) % 4;
                if (_pulseIntervalCount < 4) _pulseIntervalCount++;

                // Compute smoothed average heart rate
                uint32_t sum = 0;
                for (uint8_t i = 0; i < _pulseIntervalCount; i++) {
                    sum += _pulseIntervals[i];
                }
                float avgIntervalSec = (float)sum / (_pulseIntervalCount * 1000.0f);
                if (avgIntervalSec > 0.05f) {
                    uint8_t instantBpm = (uint8_t)roundf(60.0f / avgIntervalSec);
                    if (instantBpm >= HEART_RATE_MIN_BPM && instantBpm <= HEART_RATE_MAX_BPM) {
                        _heartRateBpm = instantBpm;
                    }
                }
                _lastPeakTimeMs = nowMs;
            }
        }
        _pulseRising = false;
    }
    _prevIrAc = currentIrAc;

    // Reset heart rate if no pulse detected for 2.5 seconds
    if (nowMs - _lastPeakTimeMs > 2500U) {
        _heartRateBpm = 0;
    }
}

void Spo2PulseSensor::computeSpo2Ratio() {
    if (!_contactDetected || _irDc <= 1.0f || _redDc <= 1.0f) {
        _quality = SIGNAL_INVALID;
        _spo2Percent = 0;
        return;
    }

    // Ratio-of-Ratios (Beer-Lambert Law approximation):
    // R = (AC_red / DC_red) / (AC_ir / DC_ir)
    float redRatio = fabsf(_redAc) / _redDc;
    float irRatio  = fabsf(_irAc) / _irDc;

    if (irRatio < 0.0001f) {
        _quality = SIGNAL_DEGRADED;
        return;
    }

    float R = redRatio / irRatio;

    // Standard empirical calibration curve for MAX30102:
    // SpO2 = 110 - 25 * R
    float calculatedSpo2 = 110.0f - (25.0f * R);

    // Sanity check and quality classification
    if (calculatedSpo2 >= (float)SPO2_MIN_PERCENT && calculatedSpo2 <= (float)SPO2_MAX_PERCENT) {
        _spo2Percent = (uint8_t)roundf(calculatedSpo2);
        _quality = (_heartRateBpm > 0) ? SIGNAL_OPTIMAL : SIGNAL_DEGRADED;
    } else if (calculatedSpo2 > 100.0f && calculatedSpo2 <= 104.0f) {
        _spo2Percent = 100;
        _quality = SIGNAL_OPTIMAL;
    } else {
        // Unphysical reading caused by optical displacement or motion artifact
        _quality = SIGNAL_DEGRADED;
        // Keep previous valid SpO2 or report 0 if prolonged
    }
}

const char* Spo2PulseSensor::getSignalQualityString() const {
    switch (_quality) {
        case SIGNAL_OPTIMAL:  return "Optimal";
        case SIGNAL_DEGRADED: return "Degraded (Motion/Weak)";
        default:              return "Invalid (No Contact)";
    }
}
