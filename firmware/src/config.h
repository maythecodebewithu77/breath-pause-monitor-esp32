/**
 * @file config.h
 * @brief Central Configuration and Threshold Parameters
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 * 
 * Non-Medical Educational Prototype.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// 1. OPERATION & DEBUG FLAGS
// =============================================================================

/**
 * @brief DEMO MODE FLAG
 * 0: Normal operation using real MPU-6050 and MAX30102 hardware sensors.
 * 1: Generates synthetic physiological data (breathing wave, heart rate, SpO2,
 *    and simulated breathing pauses) for testing dashboard without hardware.
 *    Any data generated will set the 'is_simulated' flag to TRUE.
 */
#define DEMO_MODE 0

/**
 * @brief SERIAL DEBUG PLOTTER
 * 0: Standard human-readable logging.
 * 1: Output formatted exclusively for Arduino IDE Serial Plotter:
 *    Format: "raw_accel,filtered_breath,zero_cross,pause_flag"
 */
#define SERIAL_DEBUG_PLOTTER 0

#define SERIAL_BAUD_RATE 115200

// =============================================================================
// 2. HARDWARE PIN DEFINITIONS (ESP32-WROOM-32)
// =============================================================================

// Shared Hardware I2C Bus
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define I2C_FREQUENCY 400000U // 400 kHz Fast-Mode

// Battery Voltage Monitoring
// Must be an ADC1 pin (GPIO 32-39) so it remains functional alongside BLE radio.
#define PIN_BATTERY_ADC 34

// I2C 7-Bit Slave Addresses
#define MPU6050_I2C_ADDR   0x68
#define MAX30102_I2C_ADDR  0x57

// =============================================================================
// 3. SAMPLING & TIMING PARAMETERS
// =============================================================================

#define BREATH_SAMPLE_RATE_HZ     25U                    // 25 Hz (40 ms period)
#define BREATH_SAMPLE_INTERVAL_MS (1000U / BREATH_SAMPLE_RATE_HZ)

#define PPG_SAMPLE_RATE_HZ        50U                    // 50 Hz (20 ms period)
#define PPG_SAMPLE_INTERVAL_MS    (1000U / PPG_SAMPLE_RATE_HZ)

#define TELEMETRY_INTERVAL_MS     1000U                  // 1 Hz BLE telemetry rate
#define BATTERY_CHECK_INTERVAL_MS 5000U                  // Check battery every 5s

// =============================================================================
// 4. BREATHING DETECTION ALGORITHM PARAMETERS
// =============================================================================

// Chest motion filter cutoffs: Normal resting respiration is 10-24 breaths/min
// (0.16 Hz to 0.40 Hz). We set bandpass filter roughly to 0.10 Hz - 0.70 Hz.
#define FILTER_ALPHA_DC           0.95f                  // Leaky integrator for gravity DC removal
#define FILTER_ALPHA_LP           0.25f                  // Low-pass smoothing factor (~0.7 Hz at 25 Hz Fs)

// Threshold for valid breathing movement on filtered displacement signal (in m/s^2 or g)
#define BREATH_AMPLITUDE_MIN      0.035f                 // Minimum peak amplitude threshold
#define BREATH_REFRACTORY_MS      1200U                  // Minimum refractory period (1.2s = max 50 BPM)
#define BREATH_MAX_INTERVAL_MS    7000U                  // Max interval considered a single continuous cycle

// Rolling window size for averaging breathing rate (number of breath intervals)
#define BREATH_WINDOW_SIZE        4U

/**
 * @brief BREATHING PAUSE ALERT (PROTOTYPE) TIMEOUT
 * If no valid breath cycle is detected for more than this timeout, a breathing
 * pause alert event is triggered and its duration is tracked until breathing resumes.
 */
#define PAUSE_ALERT_TIMEOUT_MS    10000U                 // 10 seconds default

// =============================================================================
// 5. BODY POSITION CLASSIFICATION THRESHOLDS
// =============================================================================

// Accelerometer gravity tilt thresholds (in m/s^2, 1g = 9.81 m/s^2)
#define TILT_GRAVITY_THRESHOLD    6.5f

enum BodyPosition : uint8_t {
    POSITION_UNKNOWN = 0,
    POSITION_SUPINE  = 1,   // Lying on back (Z-axis +)
    POSITION_PRONE   = 2,   // Lying on chest (Z-axis -)
    POSITION_LEFT    = 3,   // Lying on left side (X-axis -)
    POSITION_RIGHT   = 4,   // Lying on right side (X-axis +)
    POSITION_UPRIGHT = 5    // Sitting or standing upright (Y-axis -)
};

// =============================================================================
// 6. MAX30102 PULSE & SPO2 PARAMETERS
// =============================================================================

// Minimum raw DC optical count to confirm finger/skin contact
#define PPG_MIN_CONTACT_RAW_COUNT 15000UL
#define PPG_SAMPLE_BUFFER_LEN     100U                   // 2-second buffer at 50 Hz

// Plausible physiological boundaries
#define HEART_RATE_MIN_BPM        40U
#define HEART_RATE_MAX_BPM        210U
#define SPO2_MIN_PERCENT          70U
#define SPO2_MAX_PERCENT          100U

enum SignalQuality : uint8_t {
    SIGNAL_INVALID  = 0,
    SIGNAL_DEGRADED = 1,
    SIGNAL_OPTIMAL  = 2
};

// =============================================================================
// 7. BATTERY MONITORING PARAMETERS
// =============================================================================

// 100k / 100k divider yields a scaling multiplier of 2.0
#define BATTERY_DIVIDER_RATIO     2.0f
#define ESP32_ADC_MAX_COUNTS      4095.0f
#define ESP32_ADC_VREF_VOLTS      3.30f
#define BATTERY_VOLTAGE_CALIB     1.025f                 // Slight calibration factor for ESP32 ADC non-linearity

#define BATTERY_VOLTAGE_MAX       4.20f                  // 100% full LiPo
#define BATTERY_VOLTAGE_NOMINAL   3.70f                  // ~30-40% charge
#define BATTERY_VOLTAGE_MIN       3.30f                  // 0% cutoff threshold
#define BATTERY_LOW_THRESHOLD_PCT 15U                    // Alert when <= 15%

// =============================================================================
// 8. BLUETOOTH LOW ENERGY (BLE) UUID DEFINITIONS
// =============================================================================

#define BLE_DEVICE_NAME           "BreathPause-ESP32"

// Custom GATT Service UUID
#define BLE_SERVICE_UUID          "19b10000-e8f2-537e-4f6c-d104768a1214"

// Primary Telemetry Characteristic (Notify, 1 Hz)
#define BLE_CHAR_TELEMETRY_UUID   "19b10001-e8f2-537e-4f6c-d104768a1214"

// Instant Pause Event Alert Characteristic (Notify on state transitions)
#define BLE_CHAR_PAUSE_ALERT_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"

// Configuration / Threshold Write Characteristic (Write/Read)
#define BLE_CHAR_CONFIG_UUID      "19b10003-e8f2-537e-4f6c-d104768a1214"

// =============================================================================
// 9. TELEMETRY PACKET STRUCTURE (Packed Binary for low-power BLE)
// =============================================================================

#pragma pack(push, 1)
struct TelemetryPacket {
    uint32_t timestamp_ms;       // Uptime in milliseconds
    float    breath_wave;        // Current filtered chest displacement signal
    uint8_t  breathing_rate_bpm; // Estimated breaths per minute
    uint8_t  pause_active;       // 1 if breathing pause currently active, else 0
    uint16_t pause_duration_s;   // Current/last pause duration in seconds
    uint8_t  heart_rate_bpm;     // Pulse rate (0 if invalid)
    uint8_t  spo2_percent;       // Blood oxygen (0 if invalid)
    uint8_t  signal_quality;     // SignalQuality enum: 0=Invalid, 1=Degraded, 2=Optimal
    uint8_t  body_position;      // BodyPosition enum
    uint8_t  battery_percent;    // 0 - 100 %
    uint8_t  is_simulated;       // 1 if DEMO_MODE, 0 if authentic hardware
};

struct PauseAlertPacket {
    uint32_t timestamp_ms;
    uint8_t  event_type;         // 1 = PAUSE_STARTED, 2 = PAUSE_CLEARED
    uint16_t duration_s;         // Total duration in seconds (for cleared events)
    uint8_t  body_position;
};
#pragma pack(pop)

#endif // CONFIG_H
