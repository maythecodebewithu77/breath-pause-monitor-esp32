/**
 * @file breathing.cpp
 * @brief Implementation of digital filtering, breath cycle detection, and pause alert logic
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#include "breathing.h"
#include <Wire.h>
#include <math.h>

BreathingDetector::BreathingDetector()
    : _sensorInitialized(false),
      _rawMotion(0.0f),
      _dcBaseline(9.81f),
      _filteredSignal(0.0f),
      _previousFiltered(0.0f),
      _lastPeakTimeMs(0),
      _lastBreathTimeMs(0),
      _peakCandidateValue(0.0f),
      _isRising(true),
      _intervalIndex(0),
      _intervalCount(0),
      _breathingRateBpm(0),
      _pauseActive(false),
      _pauseStartTimeMs(0),
      _pauseTotalDurationSec(0),
      _currentPosition(POSITION_UNKNOWN) {
    memset(_breathIntervals, 0, sizeof(_breathIntervals));
    memset(&_lastAlert, 0, sizeof(_lastAlert));
}

bool BreathingDetector::begin(int sdaPin, int sclPin) {
    // Join I2C bus if not already started
    Wire.begin(sdaPin, sclPin, I2C_FREQUENCY);

    if (!_mpu.begin(MPU6050_I2C_ADDR, &Wire)) {
        Serial.println(F("[MPU6050] Error: Failed to find MPU-6050 chip at 0x68!"));
        _sensorInitialized = false;
        return false;
    }

    // Configure MPU6050 for low-noise, wearable chest kinematics
    _mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    _mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    _sensorInitialized = true;
    _lastBreathTimeMs = millis();
    Serial.println(F("[MPU6050] Initialized successfully. Range ±2G, Bandwidth 21Hz."));
    return true;
}

void BreathingDetector::update(bool &outAlertTriggered) {
    outAlertTriggered = false;

    if (!_sensorInitialized) {
        checkPauseState(millis(), outAlertTriggered);
        return;
    }

    sensors_event_t a, g, temp;
    _mpu.getEvent(&a, &g, &temp);

    // Primary axis of chest expansion:
    // When worn on the sternum, the Z-axis (normal to chest) or vector magnitude
    // captures respiratory chest wall displacement.
    // Calculate total acceleration magnitude:
    float magnitude = sqrtf(a.acceleration.x * a.acceleration.x +
                            a.acceleration.y * a.acceleration.y +
                            a.acceleration.z * a.acceleration.z);

    processSample(magnitude, a.acceleration.x, a.acceleration.y, a.acceleration.z, outAlertTriggered);
}

void BreathingDetector::updateSimulated(float syntheticMotion, BodyPosition simPos, bool &outAlertTriggered) {
    outAlertTriggered = false;
    _currentPosition = simPos;
    processSample(syntheticMotion, 0.0f, 0.0f, 9.81f, outAlertTriggered);
}

void BreathingDetector::processSample(float sampleMagnitude, float accX, float accY, float accZ, bool &outAlertTriggered) {
    uint32_t nowMs = millis();
    _rawMotion = sampleMagnitude;

    // 1. Remove DC Gravity Offset using Leaky Integrator (High-pass filter at ~0.08 Hz)
    _dcBaseline = (FILTER_ALPHA_DC * _dcBaseline) + ((1.0f - FILTER_ALPHA_DC) * sampleMagnitude);
    float acComponent = sampleMagnitude - _dcBaseline;

    // 2. Low-Pass Smoothing Filter (~0.7 Hz cutoff at 25 Hz sampling rate)
    _filteredSignal = (FILTER_ALPHA_LP * acComponent) + ((1.0f - FILTER_ALPHA_LP) * _filteredSignal);

    // 3. Peak and Direction Detection for Breath Cycles
    if (_filteredSignal > _previousFiltered) {
        // Signal is rising
        _isRising = true;
        if (_filteredSignal > _peakCandidateValue) {
            _peakCandidateValue = _filteredSignal;
        }
    } else {
        // Signal peaked and is now descending
        if (_isRising) {
            // Check if peak candidate meets minimum amplitude and refractory threshold
            if (_peakCandidateValue >= BREATH_AMPLITUDE_MIN) {
                uint32_t elapsedSincePeak = nowMs - _lastPeakTimeMs;
                if (elapsedSincePeak >= BREATH_REFRACTORY_MS) {
                    registerBreathCycle(nowMs);
                    _lastPeakTimeMs = nowMs;
                }
            }
            _peakCandidateValue = -1.0f; // Reset candidate
        }
        _isRising = false;
    }
    _previousFiltered = _filteredSignal;

    // 4. Update Body Position Classification from static gravity vectors
    updateBodyPosition(accX, accY, accZ);

    // 5. Evaluate Breathing Pause Alert State Machine
    checkPauseState(nowMs, outAlertTriggered);
}

void BreathingDetector::registerBreathCycle(uint32_t nowMs) {
    if (_lastBreathTimeMs > 0) {
        uint32_t interval = nowMs - _lastBreathTimeMs;

        // Store interval if it represents a physically plausible breath duration (1.2s - 7.0s)
        if (interval >= BREATH_REFRACTORY_MS && interval <= BREATH_MAX_INTERVAL_MS) {
            _breathIntervals[_intervalIndex] = interval;
            _intervalIndex = (_intervalIndex + 1) % BREATH_WINDOW_SIZE;
            if (_intervalCount < BREATH_WINDOW_SIZE) {
                _intervalCount++;
            }

            // Calculate rolling average interval
            uint32_t sum = 0;
            for (uint8_t i = 0; i < _intervalCount; i++) {
                sum += _breathIntervals[i];
            }
            float avgIntervalSec = (float)sum / (_intervalCount * 1000.0f);
            if (avgIntervalSec > 0.01f) {
                _breathingRateBpm = (uint8_t)roundf(60.0f / avgIntervalSec);
            }
        }
    }

    _lastBreathTimeMs = nowMs;
}

void BreathingDetector::checkPauseState(uint32_t nowMs, bool &outAlertTriggered) {
    uint32_t elapsedSinceLastBreath = nowMs - _lastBreathTimeMs;

    if (!_pauseActive) {
        // Check if silence exceeded configured timeout (e.g., 10 seconds)
        if (elapsedSinceLastBreath >= PAUSE_ALERT_TIMEOUT_MS) {
            _pauseActive = true;
            _pauseStartTimeMs = nowMs - (elapsedSinceLastBreath - PAUSE_ALERT_TIMEOUT_MS);
            _pauseTotalDurationSec = (nowMs - _pauseStartTimeMs) / 1000U;

            // Trigger immediate pause start notification
            _lastAlert.timestamp_ms = nowMs;
            _lastAlert.event_type = 1; // 1 = PAUSE_STARTED
            _lastAlert.duration_s = (uint16_s)_pauseTotalDurationSec;
            _lastAlert.body_position = (uint8_t)_currentPosition;
            outAlertTriggered = true;

            #if !SERIAL_DEBUG_PLOTTER
            Serial.printf("[ALERT] Breathing pause alert (prototype) TRIGGERED! Duration: %u s\n", _pauseTotalDurationSec);
            #endif
        }
    } else {
        // Pause is currently active: track duration
        _pauseTotalDurationSec = (nowMs - _pauseStartTimeMs) / 1000U;

        // Check if breathing has resumed (a valid breath was detected recently)
        if (elapsedSinceLastBreath < PAUSE_ALERT_TIMEOUT_MS) {
            _pauseActive = false;

            // Trigger immediate pause cleared notification
            _lastAlert.timestamp_ms = nowMs;
            _lastAlert.event_type = 2; // 2 = PAUSE_CLEARED
            _lastAlert.duration_s = (uint16_t)_pauseTotalDurationSec;
            _lastAlert.body_position = (uint8_t)_currentPosition;
            outAlertTriggered = true;

            #if !SERIAL_DEBUG_PLOTTER
            Serial.printf("[ALERT] Breathing pause alert (prototype) CLEARED. Total duration: %u s\n", _pauseTotalDurationSec);
            #endif
        }
    }
}

uint16_t BreathingDetector::getPauseDurationSeconds() const {
    if (_pauseActive) {
        return (uint16_t)((millis() - _pauseStartTimeMs) / 1000U);
    }
    return (uint16_t)_pauseTotalDurationSec;
}

void BreathingDetector::updateBodyPosition(float accX, float accY, float accZ) {
    // Standard anatomical tilt classification based on 1g (9.81 m/s^2) projection:
    // +Z = Supine (face up)
    // -Z = Prone (face down)
    // -X = Left Lateral Decubitus
    // +X = Right Lateral Decubitus
    // -Y = Upright (sitting or standing)

    if (accZ > TILT_GRAVITY_THRESHOLD) {
        _currentPosition = POSITION_SUPINE;
    } else if (accZ < -TILT_GRAVITY_THRESHOLD) {
        _currentPosition = POSITION_PRONE;
    } else if (accX < -TILT_GRAVITY_THRESHOLD) {
        _currentPosition = POSITION_LEFT;
    } else if (accX > TILT_GRAVITY_THRESHOLD) {
        _currentPosition = POSITION_RIGHT;
    } else if (accY < -TILT_GRAVITY_THRESHOLD) {
        _currentPosition = POSITION_UPRIGHT;
    } else {
        // Diagonal tilt or intermediate posture
        _currentPosition = POSITION_UNKNOWN;
    }
}

const char* BreathingDetector::getBodyPositionString() const {
    switch (_currentPosition) {
        case POSITION_SUPINE:  return "Supine (Back)";
        case POSITION_PRONE:   return "Prone (Chest)";
        case POSITION_LEFT:    return "Left Lateral";
        case POSITION_RIGHT:   return "Right Lateral";
        case POSITION_UPRIGHT: return "Upright (Sitting/Standing)";
        default:               return "Intermediate / Unknown";
    }
}
