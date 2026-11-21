/**
 * @file spo2_pulse.h
 * @brief MAX30102 Photoplethysmography (PPG), Heart Rate, and SpO2 Estimation
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 * 
 * Non-Medical Educational Prototype.
 */

#ifndef SPO2_PULSE_H
#define SPO2_PULSE_H

#include <Arduino.h>
#include "MAX30105.h" // SparkFun MAX3010x library
#include "config.h"

class Spo2PulseSensor {
public:
    Spo2PulseSensor();

    /**
     * @brief Initialize MAX30102 sensor on shared I2C bus.
     * @return true if detected and configured, false otherwise.
     */
    bool begin();

    /**
     * @brief Sample optical data and update PPG processing at ~50 Hz.
     */
    void update();

    /**
     * @brief Inject simulated pulse/SpO2 values for DEMO_MODE = 1.
     */
    void updateSimulated(uint8_t simHeartRate, uint8_t simSpo2, SignalQuality quality);

    // Telemetry getters
    uint8_t getHeartRate() const { return _heartRateBpm; }
    uint8_t getSpO2() const { return _spo2Percent; }
    SignalQuality getSignalQuality() const { return _quality; }
    const char* getSignalQualityString() const;
    bool isContactDetected() const { return _contactDetected; }

private:
    MAX30105 _sensor;
    bool _sensorInitialized;
    bool _contactDetected;

    // Moving average baselines to extract AC pulsatile & DC ambient signals
    float _redDc;
    float _irDc;
    float _redAc;
    float _irAc;

    // Pulse peak tracking
    uint32_t _lastPeakTimeMs;
    uint32_t _pulseIntervals[4];
    uint8_t _pulseIntervalIdx;
    uint8_t _pulseIntervalCount;
    bool _pulseRising;
    float _prevIrAc;

    // Calibrated estimates
    uint8_t _heartRateBpm;
    uint8_t _spo2Percent;
    SignalQuality _quality;

    void processPpgSample(uint32_t rawRed, uint32_t rawIr);
    void evaluatePulsePeak(float currentIrAc, uint32_t nowMs);
    void computeSpo2Ratio();
};

#endif // SPO2_PULSE_H
