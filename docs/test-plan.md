# Physical Test Plan & Experimental Protocol

Project: Wearable Breathing Pause Monitor (learning prototype)  
Author: Vigneshwar S, 2nd-year ECE student at SRM Institute of Science and Technology

> [!CAUTION]
> ### SAFETY PROTOCOL FOR VOLUNTARY BREATH-HOLD EXPERIMENTS
> 1. **Educational Prototype Only**: This device is an academic demonstration and not a clinical diagnostic tool.
> 2. **Seated Position Only**: All voluntary breath-hold tests must be conducted while seated comfortably on a chair or sofa. **Never** conduct tests while standing, walking, driving, or in water.
> 3. **Maximum Hold Duration**: Do not exceed a short, safe voluntary breath-hold of **10 to 15 seconds**. Discontinue the hold immediately if you feel any discomfort or lightheadedness.
> 4. **Second Person Observer**: A second person (e.g., lab partner, classmate, or friend) must be present to monitor you, observe a physical stopwatch, and record test observations.

---

## 1. Overview of Experimental Protocols

| Test ID | Test Objective | Target Parameters | Reference Standard |
| :---: | :--- | :--- | :--- |
| **TP-01** | Resting Respiration Rate Accuracy | Breaths per Minute (BPM) | Synchronized manual visual counting (1 min) |
| **TP-02** | Post-Movement Respiration Recovery | Breaths per Minute (BPM) | Manual counting after light exertion |
| **TP-03** | Breathing Pause Alert (prototype) Trigger | Elapsed silence to alert (s) | Calibrated digital stopwatch |
| **TP-04** | Pulse & SpO2 Optical Agreement | Pulse (BPM) & SpO2 (%) | Commercial FDA/CE fingertip pulse oximeter |
| **TP-05** | Anatomical Body Position Classification | Posture quadrant label | Known physical postures (5 states) |
| **TP-06** | Battery Fuel Gauge & Voltage Tracking | Voltage ($V_{bat}$) & SoC (%) | Digital Multimeter (DMM) at battery terminals |
| **TP-07** | BLE Wireless Range & Packet Delivery | RSSI (dBm) & Delivery (%) | Distance measurements (1m, 3m, 5m, 10m) |

---

## 2. Step-by-Step Test Procedures

### Test TP-01: Baseline Resting Breathing Rate
1. Fasten the prototype securely around the lower sternum using the elastic chest strap. Ensure the MPU-6050 is oriented facing forward and the MAX30102 optical window sits flush against the skin.
2. Sit quietly in a chair for 3 minutes to establish a resting baseline.
3. Open `dashboard/index.html` on a laptop or Android smartphone and click **"Connect Monitor"**.
4. The observer starts a 60-second stopwatch.
5. The observer counts chest expansions visually while the dashboard logs rolling BPM.
6. Record the manual count and the average dashboard BPM over the 60-second window in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-1-breathing-rate-validation-tp-01--tp-02). Repeat 3 times.

### Test TP-02: Post-Light-Movement Breathing Rate
1. Stand up and perform 20 light jumping jacks or 2 minutes of brisk pacing to naturally elevate respiration rate to ~20–28 BPM.
2. Immediately sit down and resume quiet monitoring.
3. Observe how quickly the digital bandpass filter dampens motion artifacts and locks onto the elevated respiratory rate.
4. Record manual count vs dashboard reported rate in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-1-breathing-rate-validation-tp-01--tp-02).

### Test TP-03: Breathing Pause Alert (prototype) Response Time
1. With the prototype connected and streaming normal resting breathing (12–16 BPM), the subject takes a normal breath and holds it voluntarily (safe breath-hold, max 12–15 seconds).
2. The observer starts the stopwatch at the exact moment chest movement ceases.
3. Note the exact stopwatch elapsed time when the red **"🚨 BREATHING PAUSE ALERT (PROTOTYPE) ACTIVE"** banner flashes on the dashboard.
4. Record the reported pause duration and elapsed time in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-2-breathing-pause-alert-response-times-tp-03).
5. The subject resumes normal breathing. Verify that the alert banner automatically clears within 1–2 breath cycles, and the event logs to the table. Repeat 3 times with 3-minute rest intervals.

### Test TP-04: Pulse & SpO2 vs Commercial Reference Oximeter
1. Attach a standard commercial fingertip pulse oximeter (e.g., Beurer, Dr. Trust, or ChoiceMMed) to the index finger.
2. Ensure the MAX30102 sensor on the chest strap has firm, gentle contact against clean skin.
3. Record simultaneous readings at 30-second intervals for 3 minutes at rest.
4. Verify whether the dashboard correctly reports **"Signal Invalid"** when the sensor is intentionally lifted away from skin contact.
5. Log the paired readings in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-3-pulse--spo2-optical-agreement-tp-04).

### Test TP-05: Body Position Classification
1. Lie down on a firm bed or exercise mat with the monitor strapped to the chest.
2. Cycle through the 5 standard body positions, holding each posture for 30 seconds:
   - **Supine**: Lying on back, facing ceiling (+Z axis).
   - **Prone**: Lying face-down on chest (-Z axis).
   - **Left Lateral**: Lying on left side (-X axis).
   - **Right Lateral**: Lying on right side (+X axis).
   - **Upright**: Sitting upright on a chair (-Y axis).
3. Confirm whether the dashboard card updates to the matching posture label. Log findings in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-4-body-position-classification-accuracy-tp-05).

### Test TP-06: Battery Life & Fuel Gauge Accuracy
1. Fully charge the 1S LiPo battery using the TP4056 module until the blue/green LED illuminates ($V_{bat} \approx 4.18\text{ V} - 4.20\text{ V}$).
2. Connect the battery to the prototype and power it on.
3. Measure the initial battery terminal voltage with a calibrated digital multimeter (DMM).
4. Connect the dashboard via Web Bluetooth and start a continuous monitoring session.
5. Record multimeter voltage, dashboard voltage, and battery percentage every 2 hours until the ESP32 powers down or reaches low battery cutoff (~3.3 V).
6. Fill in the discharge log in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-5-battery-discharge-and-run-time-log-tp-06).

### Test TP-07: BLE Range & Indoor Packet Delivery
1. Set up the laptop running the dashboard at a fixed location in an indoor room.
2. Move the chest-worn prototype to distances of 1 meter, 3 meters, 5 meters, and 10 meters line-of-sight.
3. Test communication through 1 standard interior brick/drywall partition at 5 meters.
4. Record connection stability, perceived latency, and packet drops in [docs/results.md](file:///d:/New%20folder%20(4)/breath-pause-monitor-esp32/docs/results.md#table-6-ble-wireless-range--indoor-packet-delivery-tp-07).
