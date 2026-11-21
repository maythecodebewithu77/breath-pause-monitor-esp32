/**
 * @file battery.cpp
 * @brief Implementation of LiPo battery ADC reading and discharge curve mapping
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 */

#include "battery.h"

BatteryMonitor::BatteryMonitor(uint8_t adcPin)
    : _adcPin(adcPin),
      _voltage(4.20f),
      _percentage(100),
      _isLow(false) {}

void BatteryMonitor::begin() {
    pinMode(_adcPin, INPUT);
    // Configure ESP32 ADC for 12-bit resolution (0-4095) and 11dB attenuation (up to 3.3V)
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    update();
}

float BatteryMonitor::readAdcVoltage() {
    // Perform multi-sample averaging (16 samples) to reduce high-frequency ADC noise
    uint32_t adcSum = 0;
    const uint8_t numSamples = 16;
    for (uint8_t i = 0; i < numSamples; i++) {
        adcSum += analogRead(_adcPin);
        delayMicroseconds(50);
    }
    float avgAdc = (float)adcSum / numSamples;

    // Convert raw ADC count to voltage at the divider midpoint
    float vMidpoint = (avgAdc / ESP32_ADC_MAX_COUNTS) * ESP32_ADC_VREF_VOLTS;

    // Multiply by divider ratio (2.0) and slight calibration factor
    float vBat = vMidpoint * BATTERY_DIVIDER_RATIO * BATTERY_VOLTAGE_CALIB;
    return vBat;
}

void BatteryMonitor::update() {
    _voltage = readAdcVoltage();
    _percentage = calculatePercentage(_voltage);
    _isLow = (_percentage <= BATTERY_LOW_THRESHOLD_PCT);
}

void BatteryMonitor::updateSimulated(float simVoltage, uint8_t simPercent) {
    _voltage = simVoltage;
    _percentage = simPercent;
    _isLow = (_percentage <= BATTERY_LOW_THRESHOLD_PCT);
}

uint8_t BatteryMonitor::calculatePercentage(float vbat) {
    // Piecewise-linear approximation of a 1S 3.7V Lithium-Polymer discharge curve:
    // >= 4.15 V -> 100%
    // 4.00 V    -> 85%
    // 3.85 V    -> 60%
    // 3.72 V    -> 30%
    // 3.60 V    -> 10%
    // <= 3.30 V -> 0%
    if (vbat >= 4.15f) return 100;
    if (vbat <= 3.30f) return 0;

    if (vbat >= 4.00f) {
        return 85 + (uint8_t)(((vbat - 4.00f) / 0.15f) * 15.0f);
    } else if (vbat >= 3.85f) {
        return 60 + (uint8_t)(((vbat - 3.85f) / 0.15f) * 25.0f);
    } else if (vbat >= 3.72f) {
        return 30 + (uint8_t)(((vbat - 3.72f) / 0.13f) * 30.0f);
    } else if (vbat >= 3.60f) {
        return 10 + (uint8_t)(((vbat - 3.60f) / 0.12f) * 20.0f);
    } else {
        return (uint8_t)(((vbat - 3.30f) / 0.30f) * 10.0f);
    }
}
