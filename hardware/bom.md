# Bill of Materials (BOM) & Sourcing Guide

Project: Wearable Breathing Pause Monitor (learning prototype)  
Author: Vigneshwar S, SRM Institute of Science and Technology  
Target Region: Chennai, Tamil Nadu, India

> [!NOTE]
> All prices listed are approximate Indian Rupee (INR ₹) retail estimates as of late 2026. Local shop prices at Ritchie Street, Chennai may vary slightly depending on bargaining and stock availability.

---

## 1. Component Parts List

| S.No | Component | Specification / Package | Qty | Approx. Price (INR) | Primary Source / Vendor | Sourcing Link / Notes |
| :---: | :--- | :--- | :---: | :---: | :--- | :--- |
| 1 | **ESP32 DevKit V1** | ESP32-WROOM-32 (30-pin, CP2102 or CH340 USB) | 1 | ₹380 | Robu.in / Ritchie Street | Common NodeMCU-32S style board. Ritchie Street: Radio Warehouse. |
| 2 | **MPU-6050 Module** | GY-521 3-axis gyro + 3-axis accelerometer | 1 | ₹150 | Robu.in / ElectronicsComp | Ritchie Street: Supreme Electronics. Ensure 3.3V logic jumper intact. |
| 3 | **MAX30102 Module** | Heart Rate & SpO2 breakout (optical sensor) | 1 | ₹320 | Robu.in / QuartzComponents | Check for 3.3V regulator on breakout (RC filter mod recommended if noisy). |
| 4 | **LiPo Battery (1S)** | 3.7 V 600 mAh – 1000 mAh with JST-PH 2.0 connector & protection | 1 | ₹280 | Robu.in / QuartzComponents | 25C or standard capacity pouch cell. Ensure built-in PCM protection. |
| 5 | **TP4056 Charger** | Type-C 1S Li-ion charging board with DW01A protection | 1 | ₹45 | Robu.in / Ritchie Street | Type-C preferred over micro-USB. Includes over-discharge protection. |
| 6 | **100 kΩ Resistors** | 1/4W, 1% tolerance metal film resistors | 2 | ₹4 | Local Electronics Shop | Ritchie Street or local college hardware lab. Used for battery divider. |
| 7 | **100 nF Capacitor** | 50V ceramic disc / MLCC capacitor (code 104) | 1 | ₹3 | Local Electronics Shop | Noise filter for ADC pin GPIO34. |
| 8 | **Slide Switch** | Miniature SPDT slide switch (horizontal or vertical) | 1 | ₹15 | Local Electronics Shop | Main battery power on/off switch. |
| 9 | **Elastic Chest Strap** | 25 mm – 38 mm adjustable elastic belt with clip | 1 | ₹120 | Amazon India / Decathlon | Fitness/heart-rate strap replacement band (velcro or buckle). |
| 10 | **Prototyping Board** | Single-sided dotted zero PCB (5 cm x 7 cm) | 1 | ₹30 | Ritchie Street / Robu.in | For breadboard-free, robust soldered prototype. |
| 11 | **Jumper Wires** | 28–30 AWG flexible silicone multi-strand wire (1m) | 1 set | ₹60 | Robu.in / Local shop | Multi-colored silicone wire allows movement without snapping traces. |
| 12 | **Headers & Sockets** | 2.54 mm female and male breakaway headers | 1 strip | ₹25 | Local Electronics Shop | For modular sensor disconnection. |
| **TOTAL**| | | | **~ ₹1,432** | | *(Excludes optional shipping fees)* |

---

## 2. Indian Vendor Sourcing Guide

### A. Local Chennai Sourcing (Ritchie Street, Mount Road)
Ritchie Street is Asia’s second-largest electronics market and the ideal place for students in Chennai to inspect parts before buying:
1. **Radio Warehouse (Wallers Road / Narayana Mudali Street)**:
   - Best for genuine ESP32 DevKit boards, breadboards, headers, and standard passives.
2. **Supreme Electronics (Ritchie Street Main)**:
   - Reliable stock of sensor breakout modules (GY-521 MPU-6050, MAX30102, TP4056 modules).
3. **Mohan Electronics (Meerand Sahib Street)**:
   - Great for LiPo batteries, JST connectors, miniature toggle switches, and silicone wires.
4. **General Tips for Ritchie Street**:
   - Ask for "ESP-WROOM-32 30-pin board".
   - Verify the MAX30102 board has the clean optical window without flux residue.
   - Purchase extra 100k metal film resistors (buy a pack of 10 for ₹10).

### B. Online Indian Stores
- **Robu.in (Pune, ships across India)**:
  - High stock reliability, quick dispatch to Chennai (typically 2–3 days via Bluedart/Delhivery).
  - Best for: LiPo batteries with safety certificates, genuine SparkFun/Adafruit pin-compatible modules.
- **QuartzComponents.com**:
  - Very affordable prices for basic passives, breadboards, and battery chargers.
- **ElectronicsComp.com**:
  - Good alternative for discounted bundle orders of sensors and dev boards.
- **Amazon.in**:
  - Useful for fast 1-day delivery if in a pinch, though per-module prices are typically 20–30% higher.

---

## 3. Alternative & Substitute Components

| Original Part | Approved Alternative | Engineering Consideration |
| :--- | :--- | :--- |
| **ESP32-WROOM-32 (30-pin)** | ESP32-WROOM-32 (38-pin) | Pinout varies slightly; ensure GPIO21, GPIO22, and GPIO34 correspond to headers. |
| **MAX30102** | MAX30100 | *Caution*: MAX30100 runs on 1.8V/3.3V dual rails and suffers from well-documented I2C level shift bugs on older breakouts. Stick with MAX30102 if possible. |
| **TP4056 Board** | MCP73831 Micro-Charger | MCP73831 is smaller and ideal for the custom PCB phase, but requires custom breakout. |
| **100kΩ / 100kΩ Resistors** | 47kΩ / 47kΩ Resistors | Consumes slightly more quiescent divider current (~40 µA vs 20 µA), but works identically. |
