/**
 * @file battery.h
 * @brief LiPo battery voltage sensing and state-of-charge estimation
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "config.h"

class BatteryMonitor {
public:
    BatteryMonitor(uint8_t adcPin = PIN_BATTERY_ADC);

    /**
     * @brief Configure ADC resolution and attenuation.
     */
    void begin();

    /**
     * @brief Sample ADC voltage and update battery percentage.
     */
    void update();

    /**
     * @brief Inject simulated battery value for DEMO_MODE = 1.
     */
    void updateSimulated(float simVoltage, uint8_t simPercent);

    // Telemetry getters
    float getVoltage() const { return _voltage; }
    uint8_t getPercentage() const { return _percentage; }
    bool isLowBattery() const { return _isLow; }

private:
    uint8_t _adcPin;
    float _voltage;
    uint8_t _percentage;
    bool _isLow;

    float readAdcVoltage();
    uint8_t calculatePercentage(float vbat);
};

#endif // BATTERY_H
