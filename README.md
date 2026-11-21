# Wearable Breathing Pause Monitor (learning prototype)

**Project Identifier**: `breath-pause-monitor-esp32`  
**Author**: Vigneshwar S, 2nd-year ECE student at SRM Institute of Science and Technology  
**License**: MIT (Code) / CC BY 4.0 (Docs)

---

> [!CAUTION]
> ### NON-MEDICAL EDUCATIONAL PROTOTYPE NOTICE
> This project is an experimental engineering learning prototype developed to build practical skills in wearable sensor signal processing, low-power embedded firmware (ESP32), and Bluetooth Low Energy (Web Bluetooth API) communication.
> 
> **It is NOT a medical device and is NOT intended, certified, or tested to diagnose, detect, monitor, or treat sleep apnea, hypoxemia, respiratory arrest, or any physiological condition.**
> 
> The system implements an experimental **breathing pause alert (prototype)** strictly for educational demonstration of chest kinematics. Never rely on this device for health, life-safety, or medical decision-making.

---

## 1. Project Overview

The **Wearable Breathing Pause Monitor** is a chest-worn, battery-powered embedded prototype powered by an ESP32 microcontroller. The device captures mechanical chest displacement using an **MPU-6050** 6-DoF inertial measurement unit (IMU) to extract respiratory rate and detect long breathing pauses. In parallel, a **MAX30102** photoplethysmography (PPG) optical sensor captures photoplethysmographic signals to estimate pulse and blood oxygen saturation ($SpO_2$) with integrated signal quality assessment. Real-time telemetry is streamed wirelessly over Bluetooth Low Energy (BLE) to a standalone web dashboard featuring live graph visualization, pause event logging, and CSV export.

---

## 2. System Status & Verification Checklist

In accordance with transparent engineering principles, all subsystem statuses are honestly documented below:

| Subsystem / Feature | Implementation Status | Verification Stage |
| :--- | :--- | :--- |
| **Repository & Docs** | Completed | Clean structure, BOM, wiring, PCB notes |
| **Breathing Signal Filter** | Completed | 0.1–0.7 Hz band-pass filter, simulated test suites |
| **Breathing Pause Alert (prototype)** | Completed | State machine with configurable timeout (10 s default) |
| **Body Position Classifier** | Completed | Tilt quadrant estimation (Supine/Prone/Left/Right/Upright) |
| **Pulse & $SpO_2$ Estimator** | Completed | AC/DC ratio-of-ratios with signal quality validity flag |
| **Battery Fuel Gauge** | Completed | Voltage divider reading + LiPo discharge curve mapping |
| **BLE Custom GATT Service** | Completed | Binary & JSON notification characteristics |
| **Web Bluetooth Dashboard** | Completed | Single self-contained HTML/JS dashboard with Canvas graphs |
| **Hardware Bench Testing** | Pending | Not yet tested on physical human chest (to be performed by author) |
| **Battery Life Measurements** | Pending | Theoretical estimate documented; physical drain unmeasured |

---

## 3. Hardware Architecture

```text
       +-------------------------------------------------------+
       |               1S LiPo Battery (3.7V)                  |
       +---------------------------+---------------------------+
                                   |
                                   v
       +-------------------------------------------------------+
       |       TP4056 USB-C Charger & Battery Protection       |
       +---------------------------+---------------------------+
                                   |
                     +-------------+-------------+
                     |                           |
                     v                           v
              +--------------+            +--------------+
              | 100k/100k Div|            | ESP32 DevKit |
              |  To GPIO34   |            |  (3.3V Rail) |
              +--------------+            +-------+------+
                                                  | Shared I2C Bus
                                      +-----------+-----------+
                                      |                       |
                                      v                       v
                              +---------------+       +---------------+
                              |   MPU-6050    |       |   MAX30102    |
                              |  Addr: 0x68   |       |  Addr: 0x57   |
                              | (Chest Motion)|       | (PPG Sensor)  |
                              +---------------+       +---------------+
```

---

## 4. Repository Structure

```text
breath-pause-monitor-esp32/
├── README.md               # Main project documentation and setup guide
├── LICENSE                 # MIT (code) & CC BY 4.0 (documentation)
├── .gitignore              # PlatformIO, KiCad, editor, and OS ignore rules
├── platformio.ini          # PlatformIO build configuration for ESP32
├── firmware/
│   ├── src/
│   │   ├── main.cpp        # FreeRTOS setup, sensor loops, debug dispatch
│   │   ├── config.h        # Pin definitions, sampling rates, thresholds
│   │   ├── breathing.h/.cpp# MPU-6050 filter, rate, pause alert state machine
│   │   ├── spo2_pulse.h/.cpp# MAX30102 PPG driver, ratio-of-ratios SpO2
│   │   ├── ble_service.h/.cpp# Custom BLE GATT service & characteristics
│   │   └── battery.h/.cpp  # ADC voltage divider reading & LiPo curve
│   └── test/
│       └── test_breathing.cpp # Unit tests with synthetic breathing & pause arrays
├── dashboard/
│   └── index.html          # Web Bluetooth single-page live telemetry dashboard
├── hardware/
│   ├── wiring.md           # Pin tables, ASCII schematic & Mermaid diagram
│   ├── bom.md              # Parts list with Indian INR prices & Chennai sourcing
│   └── pcb-notes.md        # KiCad chest-strap PCB design specifications
└── docs/
    ├── design-notes.md     # Architecture, math models, BLE packet specs
    ├── test-plan.md        # Physical testing protocols for the author
    ├── results.md          # Empty benchmark tables for real hardware results
    └── images/             # Enclosure photos, bench wiring (.gitkeep)
```

---

## 5. Quick Start: Firmware Compilation

### Prerequisites
- [PlatformIO Core](https://platformio.org/) or PlatformIO IDE for VS Code.
- ESP32 development board (ESP32-WROOM-32).

### Building and Uploading
```bash
# Navigate to the project directory
cd breath-pause-monitor-esp32

# Build the firmware
pio run

# Flash to connected ESP32 (replace COM port on Windows if needed)
pio run --target upload --upload-port COM3

# Open serial monitor (115200 baud)
pio run --target monitor
```

---

## 6. Web Dashboard Setup

The telemetry dashboard is entirely self-contained in `dashboard/index.html` with zero npm dependencies or build steps.

1. Open `dashboard/index.html` in **Google Chrome** (Version 70+ on Windows, macOS, Linux, or Android).
2. Ensure Bluetooth is enabled on your host computer or phone.
3. Click the **"Connect Monitor"** button.
4. Select `BreathPause-ESP32` from the browser Bluetooth device picker.
5. Observe live chest movement, breathing rate, body position, $SpO_2$, pulse, and pause alerts.

---

## 7. Roadmap & Next Steps

- [x] Phase 1: Repository structure, hardware documentation, and KiCad PCB notes.
- [ ] Phase 2: Firmware implementation (digital filters, pause alerts, PPG, BLE, demo mode).
- [ ] Phase 3: Web Bluetooth HTML5 Canvas dashboard with CSV session export.
- [ ] Phase 4: Unit test execution, test protocols, and blank experimental tables.
- [ ] Phase 5: Git commit history, remote repository linking, and GitHub publishing.
- [ ] Future Hardware: 2-layer custom KiCad PCB fabricated and assembled on elastic strap.
