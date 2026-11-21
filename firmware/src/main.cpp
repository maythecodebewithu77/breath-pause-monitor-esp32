/**
 * @file main.cpp
 * @brief Firmware orchestration, non-blocking sampling schedulers, and debug dispatch
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 * 
 * Non-Medical Educational Prototype.
 */

#include <Arduino.h>
#include "config.h"
#include "breathing.h"
#include "spo2_pulse.h"
#include "battery.h"
#include "ble_service.h"

// Subsystem Instances
BreathingDetector breathingDetector;
Spo2PulseSensor   spo2Sensor;
BatteryMonitor    batteryMonitor(PIN_BATTERY_ADC);
BleTelemetryServer bleServer;

// Execution Schedulers
uint32_t lastBreathSampleMs = 0;
uint32_t lastPpgSampleMs    = 0;
uint32_t lastTelemetryMs    = 0;
uint32_t lastBatteryMs      = 0;

#if DEMO_MODE
// Synthetic physiological generator variables
float demoPhase = 0.0f;
uint32_t demoCycleStartMs = 0;
#endif

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000); // Settle UART

    #if !SERIAL_DEBUG_PLOTTER
    Serial.println();
    Serial.println(F("=================================================================="));
    Serial.println(F(" Wearable Breathing Pause Monitor (Learning Prototype)"));
    Serial.println(F(" Author: Vigneshwar S (SRM Institute of Science and Technology)"));
    Serial.println(F(" NOTICE: NOT A MEDICAL DEVICE. EDUCATIONAL PROTOTYPE ONLY."));
    #if DEMO_MODE
    Serial.println(F(" >>> DEMO MODE ACTIVE: GENERATING SYNTHETIC PHYSIOLOGICAL DATA <<<"));
    #endif
    Serial.println(F("=================================================================="));
    #endif

    // Initialize subsystems
    #if !DEMO_MODE
    breathingDetector.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    spo2Sensor.begin();
    batteryMonitor.begin();
    #else
    Serial.println(F("[SYSTEM] Hardware sensor init bypassed (DEMO_MODE enabled)."));
    #endif

    bleServer.begin();

    #if SERIAL_DEBUG_PLOTTER
    // Header for Arduino Serial Plotter
    Serial.println(F("Raw_Accel,Filtered_Breath,Rate_BPM,Pause_Active"));
    #endif
}

void loop() {
    uint32_t nowMs = millis();
    bool pauseAlertTriggered = false;

    // -------------------------------------------------------------------------
    // 1. BREATHING SAMPLING LOOP (~25 Hz / every 40 ms)
    // -------------------------------------------------------------------------
    if (nowMs - lastBreathSampleMs >= BREATH_SAMPLE_INTERVAL_MS) {
        lastBreathSampleMs = nowMs;

        #if !DEMO_MODE
        breathingDetector.update(pauseAlertTriggered);
        #else
        // In Demo Mode: generate a synthetic breathing sine wave (14 breaths/min = 0.233 Hz)
        // Introduce a programmed 12-second pause every 35 seconds to simulate a breathing pause.
        uint32_t demoTimeInCycle = (nowMs - demoCycleStartMs) % 35000U;
        float synthAccZ = 9.81f; // Baseline gravity
        if (demoTimeInCycle < 23000U) {
            // Normal breathing phase
            demoPhase += (2.0f * 3.14159f * 0.233f) * (BREATH_SAMPLE_INTERVAL_MS / 1000.0f);
            synthAccZ += 0.12f * sinf(demoPhase); // Chest displacement
        } else {
            // Simulated breathing pause phase (motion ceases)
            synthAccZ += 0.005f * sinf(nowMs * 0.01f); // Tiny ambient noise only
        }
        breathingDetector.updateSimulated(synthAccZ, POSITION_SUPINE, pauseAlertTriggered);
        #endif

        // Fire immediate BLE alert if pause transitioned (started or cleared)
        if (pauseAlertTriggered) {
            bleServer.sendPauseAlert(breathingDetector.getLastAlertPacket());
        }

        #if SERIAL_DEBUG_PLOTTER
        Serial.print(breathingDetector.getRawMotion());
        Serial.print(F(","));
        Serial.print(breathingDetector.getFilteredMotion());
        Serial.print(F(","));
        Serial.print(breathingDetector.getBreathingRate());
        Serial.print(F(","));
        Serial.println(breathingDetector.isPauseActive() ? 1.0f : 0.0f);
        #endif
    }

    // -------------------------------------------------------------------------
    // 2. PHOTOPLETHYSMOGRAPHY (PPG) SAMPLING LOOP (~50 Hz / every 20 ms)
    // -------------------------------------------------------------------------
    if (nowMs - lastPpgSampleMs >= PPG_SAMPLE_INTERVAL_MS) {
        lastPpgSampleMs = nowMs;

        #if !DEMO_MODE
        spo2Sensor.update();
        #else
        // Demo Mode: simulated 72 BPM pulse, 98% SpO2, Optimal quality
        spo2Sensor.updateSimulated(72, 98, SIGNAL_OPTIMAL);
        #endif
    }

    // -------------------------------------------------------------------------
    // 3. BATTERY SAMPLING LOOP (Every 5 seconds)
    // -------------------------------------------------------------------------
    if (nowMs - lastBatteryMs >= BATTERY_CHECK_INTERVAL_MS) {
        lastBatteryMs = nowMs;

        #if !DEMO_MODE
        batteryMonitor.update();
        #else
        // Demo Mode: simulate 3.95 V / 78% charge
        batteryMonitor.updateSimulated(3.95f, 78);
        #endif
    }

    // -------------------------------------------------------------------------
    // 4. BLE TELEMETRY NOTIFICATION LOOP (1 Hz / every 1000 ms)
    // -------------------------------------------------------------------------
    if (nowMs - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = nowMs;

        TelemetryPacket packet;
        packet.timestamp_ms       = nowMs;
        packet.breath_wave        = breathingDetector.getFilteredMotion();
        packet.breathing_rate_bpm = breathingDetector.getBreathingRate();
        packet.pause_active       = breathingDetector.isPauseActive() ? 1 : 0;
        packet.pause_duration_s   = breathingDetector.getPauseDurationSeconds();
        packet.heart_rate_bpm     = spo2Sensor.getHeartRate();
        packet.spo2_percent       = spo2Sensor.getSpO2();
        packet.signal_quality     = (uint8_t)spo2Sensor.getSignalQuality();
        packet.body_position      = (uint8_t)breathingDetector.getBodyPosition();
        packet.battery_percent    = batteryMonitor.getPercentage();
        #if DEMO_MODE
        packet.is_simulated       = 1;
        #else
        packet.is_simulated       = 0;
        #endif

        bleServer.sendTelemetry(packet);

        #if !SERIAL_DEBUG_PLOTTER
        Serial.printf("[STATUS %lu ms] BreathRate: %u BPM | Pause: %s (%u s) | HR: %u BPM | SpO2: %u%% [%s] | Pos: %s | Bat: %u%%\n",
                      nowMs,
                      packet.breathing_rate_bpm,
                      packet.pause_active ? "ACTIVE" : "NONE",
                      packet.pause_duration_s,
                      packet.heart_rate_bpm,
                      packet.spo2_percent,
                      spo2Sensor.getSignalQualityString(),
                      breathingDetector.getBodyPositionString(),
                      packet.battery_percent);
        #endif
    }

    // Small yield to FreeRTOS scheduler to manage background BLE stacks smoothly
    yield();
}
