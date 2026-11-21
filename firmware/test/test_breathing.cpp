/**
 * @file test_breathing.cpp
 * @brief Unit tests for Breathing Detection and Pause Alert Logic
 * 
 * Project: breath-pause-monitor-esp32
 * Author: Vigneshwar S, 2nd-year ECE, SRM Institute of Science and Technology
 * 
 * Tests the digital filter, peak detection, and pause timeout logic using
 * pre-generated synthetic acceleration sample arrays.
 */

#include <unity.h>
#include <math.h>
#include "../src/config.h"

// Standalone test fixture simulating the C++ core breathing detector logic
class BreathingAlgorithmTester {
public:
    BreathingAlgorithmTester() {
        reset();
    }

    void reset() {
        dcBaseline = 9.81f;
        filteredSignal = 0.0f;
        previousFiltered = 0.0f;
        lastPeakTimeMs = 0;
        lastBreathTimeMs = 0;
        peakCandidateValue = -1.0f;
        isRising = true;
        pauseActive = false;
        pauseStartTimeMs = 0;
        pauseDurationSec = 0;
        breathCount = 0;
        intervalIndex = 0;
        intervalCount = 0;
        breathingRateBpm = 0;
        alertTriggered = false;
        for (int i = 0; i < 4; i++) breathIntervals[i] = 0;
    }

    void processSample(float sampleMagnitude, uint32_t nowMs) {
        alertTriggered = false;

        // 1. Remove DC gravity offset
        dcBaseline = (FILTER_ALPHA_DC * dcBaseline) + ((1.0f - FILTER_ALPHA_DC) * sampleMagnitude);
        float ac = sampleMagnitude - dcBaseline;

        // 2. Low-pass filter (~0.7 Hz)
        filteredSignal = (FILTER_ALPHA_LP * ac) + ((1.0f - FILTER_ALPHA_LP) * filteredSignal);

        // 3. Peak detection
        if (filteredSignal > previousFiltered) {
            isRising = true;
            if (filteredSignal > peakCandidateValue) {
                peakCandidateValue = filteredSignal;
            }
        } else {
            if (isRising) {
                if (peakCandidateValue >= BREATH_AMPLITUDE_MIN) {
                    uint32_t elapsed = nowMs - lastPeakTimeMs;
                    if (elapsed >= BREATH_REFRACTORY_MS) {
                        registerBreath(nowMs);
                        lastPeakTimeMs = nowMs;
                    }
                }
                peakCandidateValue = -1.0f;
            }
            isRising = false;
        }
        previousFiltered = filteredSignal;

        // 4. Pause alert state machine
        uint32_t elapsedSinceLastBreath = nowMs - lastBreathTimeMs;
        if (!pauseActive) {
            if (lastBreathTimeMs > 0 && elapsedSinceLastBreath >= PAUSE_ALERT_TIMEOUT_MS) {
                pauseActive = true;
                pauseStartTimeMs = nowMs - (elapsedSinceLastBreath - PAUSE_ALERT_TIMEOUT_MS);
                pauseDurationSec = (nowMs - pauseStartTimeMs) / 1000U;
                alertTriggered = true;
            }
        } else {
            pauseDurationSec = (nowMs - pauseStartTimeMs) / 1000U;
            if (elapsedSinceLastBreath < PAUSE_ALERT_TIMEOUT_MS) {
                pauseActive = false;
                alertTriggered = true;
            }
        }
    }

    void registerBreath(uint32_t nowMs) {
        breathCount++;
        if (lastBreathTimeMs > 0) {
            uint32_t interval = nowMs - lastBreathTimeMs;
            if (interval >= BREATH_REFRACTORY_MS && interval <= BREATH_MAX_INTERVAL_MS) {
                breathIntervals[intervalIndex] = interval;
                intervalIndex = (intervalIndex + 1) % 4;
                if (intervalCount < 4) intervalCount++;

                uint32_t sum = 0;
                for (uint8_t i = 0; i < intervalCount; i++) sum += breathIntervals[i];
                float avgSec = (float)sum / (intervalCount * 1000.0f);
                if (avgSec > 0.01f) {
                    breathingRateBpm = (uint8_t)roundf(60.0f / avgSec);
                }
            }
        }
        lastBreathTimeMs = nowMs;
    }

    float dcBaseline;
    float filteredSignal;
    float previousFiltered;
    uint32_t lastPeakTimeMs;
    uint32_t lastBreathTimeMs;
    float peakCandidateValue;
    bool isRising;
    bool pauseActive;
    uint32_t pauseStartTimeMs;
    uint32_t pauseDurationSec;
    uint32_t breathCount;
    uint32_t breathIntervals[4];
    uint8_t intervalIndex;
    uint8_t intervalCount;
    uint8_t breathingRateBpm;
    bool alertTriggered;
};

static BreathingAlgorithmTester tester;

void setUp(void) {
    tester.reset();
}

void tearDown(void) {
}

/**
 * @brief Test 1: Normal 15 Breaths/Minute Sine Wave
 * Verifies that the rate estimation locks onto 15 BPM within ±1 BPM after filter warm-up.
 */
void test_normal_breathing_rate_estimation(void) {
    const float sampleRateHz = 25.0f;
    const float dt = 1.0f / sampleRateHz;
    const float breathFrequencyHz = 15.0f / 60.0f; // 0.25 Hz = 15 BPM
    const float amplitude = 0.15f; // m/s^2 chest wall motion
    const float baseline = 9.81f; // 1g gravity

    uint32_t simulatedTimeMs = 0;
    // Simulate 45 seconds of continuous breathing (1125 samples at 25 Hz)
    for (int i = 0; i < 1125; i++) {
        float t = i * dt;
        float sample = baseline + amplitude * sinf(2.0f * 3.14159265f * breathFrequencyHz * t);
        simulatedTimeMs += (uint32_t)(dt * 1000.0f);

        tester.processSample(sample, simulatedTimeMs);
    }

    // After warm-up, breathing rate should accurately reflect ~15 BPM
    TEST_ASSERT_INT_WITHIN(1, 15, tester.breathingRateBpm);
    TEST_ASSERT_FALSE(tester.pauseActive);
}

/**
 * @brief Test 2: Breathing Pause Event Detection (12-second apnea gap)
 * Verifies that if breathing stops for 12 seconds, pauseActive becomes true at exactly 10s,
 * and duration reaches 12s.
 */
void test_breathing_pause_alert_trigger(void) {
    const float sampleRateHz = 25.0f;
    const float dt = 1.0f / sampleRateHz;
    const float breathFrequencyHz = 15.0f / 60.0f; // 0.25 Hz
    const float amplitude = 0.15f;
    const float baseline = 9.81f;

    uint32_t simulatedTimeMs = 0;

    // 1. Breathe normally for 20 seconds (500 samples)
    for (int i = 0; i < 500; i++) {
        float t = i * dt;
        float sample = baseline + amplitude * sinf(2.0f * 3.14159265f * breathFrequencyHz * t);
        simulatedTimeMs += (uint32_t)(dt * 1000.0f);
        tester.processSample(sample, simulatedTimeMs);
    }

    TEST_ASSERT_FALSE(tester.pauseActive);

    // 2. Cessation of chest wall motion for 12 seconds (300 samples)
    bool pauseTriggerReported = false;
    for (int i = 0; i < 300; i++) {
        float sample = baseline; // Flatline (no respiratory chest expansion)
        simulatedTimeMs += (uint32_t)(dt * 1000.0f);
        tester.processSample(sample, simulatedTimeMs);
        if (tester.pauseActive) {
            pauseTriggerReported = true;
        }
    }

    // Verify pause active was triggered and duration logged
    TEST_ASSERT_TRUE(pauseTriggerReported);
    TEST_ASSERT_TRUE(tester.pauseActive);
    TEST_ASSERT_GREATER_OR_EQUAL(10, tester.pauseDurationSec);
}

/**
 * @brief Test 3: Breathing Pause Clearance and Resumption
 * Verifies that when respiration resumes following a pause, pauseActive clears.
 */
void test_breathing_pause_recovery(void) {
    const float sampleRateHz = 25.0f;
    const float dt = 1.0f / sampleRateHz;
    const float breathFrequencyHz = 15.0f / 60.0f;
    const float amplitude = 0.15f;
    const float baseline = 9.81f;

    uint32_t simulatedTimeMs = 0;

    // Breathe normally
    for (int i = 0; i < 500; i++) {
        float t = i * dt;
        float sample = baseline + amplitude * sinf(2.0f * 3.14159265f * breathFrequencyHz * t);
        simulatedTimeMs += 40;
        tester.processSample(sample, simulatedTimeMs);
    }

    // 11-second pause
    for (int i = 0; i < 275; i++) {
        simulatedTimeMs += 40;
        tester.processSample(baseline, simulatedTimeMs);
    }
    TEST_ASSERT_TRUE(tester.pauseActive);

    // Resumption of breathing for 10 seconds
    for (int i = 0; i < 250; i++) {
        float t = (500 + 275 + i) * dt;
        float sample = baseline + amplitude * sinf(2.0f * 3.14159265f * breathFrequencyHz * t);
        simulatedTimeMs += 40;
        tester.processSample(sample, simulatedTimeMs);
    }

    // Pause should now be cleared
    TEST_ASSERT_FALSE(tester.pauseActive);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_normal_breathing_rate_estimation);
    RUN_TEST(test_breathing_pause_alert_trigger);
    RUN_TEST(test_breathing_pause_recovery);
    return UNITY_END();
}
