/**
 * @file breathing.h
 * @brief Breathing motion detection, digital filtering, and pause state machine
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#ifndef BREATHING_H
#define BREATHING_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "config.h"

class BreathingDetector {
public:
    BreathingDetector();

    /**
     * @brief Initialize MPU-6050 sensor on shared I2C bus.
     * @param sdaPin I2C SDA pin
     * @param sclPin I2C SCL pin
     * @return true if initialized successfully, false otherwise.
     */
    bool begin(int sdaPin = PIN_I2C_SDA, int sclPin = PIN_I2C_SCL);

    /**
     * @brief Update breathing detection pipeline (sample at ~25 Hz).
     * @param outAlertTriggered Set to true if a pause event just transitioned.
     */
    void update(bool &outAlertTriggered);

    /**
     * @brief Synthetic sample injector used when DEMO_MODE = 1.
     * @param syntheticAccZ Simulated acceleration value
     * @param outAlertTriggered Set to true if a pause event just transitioned.
     */
    void updateSimulated(float syntheticAccZ, BodyPosition simPos, bool &outAlertTriggered);

    // Getters for telemetry
    float getFilteredMotion() const { return _filteredSignal; }
    float getRawMotion() const { return _rawMotion; }
    uint8_t getBreathingRate() const { return _breathingRateBpm; }
    bool isPauseActive() const { return _pauseActive; }
    uint16_t getPauseDurationSeconds() const;
    BodyPosition getBodyPosition() const { return _currentPosition; }
    const char* getBodyPositionString() const;

    // Direct access to last transition alert packet
    const PauseAlertPacket& getLastAlertPacket() const { return _lastAlert; }

private:
    Adafruit_MPU6050 _mpu;
    bool _sensorInitialized;

    // Kinematic and filter states
    float _rawMotion;
    float _dcBaseline;
    float _filteredSignal;
    float _previousFiltered;

    // Peak and zero-crossing detection state
    uint32_t _lastPeakTimeMs;
    uint32_t _lastBreathTimeMs;
    float _peakCandidateValue;
    bool _isRising;

    // Rolling window buffer for rate calculation
    uint32_t _breathIntervals[BREATH_WINDOW_SIZE];
    uint8_t _intervalIndex;
    uint8_t _intervalCount;
    uint8_t _breathingRateBpm;

    // Breathing Pause Alert (prototype) state machine
    bool _pauseActive;
    uint32_t _pauseStartTimeMs;
    uint32_t _pauseTotalDurationSec;
    PauseAlertPacket _lastAlert;

    // Body position orientation
    BodyPosition _currentPosition;

    // Helper functions
    void processSample(float sampleMagnitude, float accX, float accY, float accZ, bool &outAlertTriggered);
    void updateBodyPosition(float accX, float accY, float accZ);
    void registerBreathCycle(uint32_t nowMs);
    void checkPauseState(uint32_t nowMs, bool &outAlertTriggered);
};

#endif // BREATHING_H
