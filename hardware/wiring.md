# Hardware Wiring & Interconnect Guide

Project: Wearable Breathing Pause Monitor (learning prototype)  
Author: Vigneshwar S, SRM Institute of Science and Technology

> [!IMPORTANT]
> Non-Medical Educational Device. This hardware configuration is designed strictly for academic demonstration and wearable sensor prototyping.

---

## 1. Complete Pin Assignment Table

All connections run from the central ESP32-WROOM-32 DevKit. Both I2C sensors (MPU-6050 and MAX30102) share the primary hardware I2C bus.

| ESP32 Pin | Target Device | Device Pin | Signal Type | Wire Color (Rec.) | Description & Operational Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **3V3** | MPU-6050 | VCC | Power (3.3V) | Red | Common regulated 3.3V logic & sensor rail. |
| **3V3** | MAX30102 | VIN | Power (3.3V) | Red | Sensor logic and LED power. |
| **GND** | MPU-6050 | GND | Ground | Black | Common reference ground. |
| **GND** | MAX30102 | GND | Ground | Black | Common reference ground. |
| **GND** | MPU-6050 | AD0 | Logic In | Black | Tied to GND to set I2C address to `0x68`. |
| **GPIO21** | MPU-6050 | SDA | I2C Data | Blue / Green | Shared I2C Serial Data line. |
| **GPIO21** | MAX30102 | SDA | I2C Data | Blue / Green | Shared I2C Serial Data line. |
| **GPIO22** | MPU-6050 | SCL | I2C Clock | Yellow | Shared I2C Serial Clock line (400 kHz). |
| **GPIO22** | MAX30102 | SCL | I2C Clock | Yellow | Shared I2C Serial Clock line (400 kHz). |
| **GPIO34** | Battery Divider | Midpoint | Analog In (ADC1) | Orange | Voltage divider junction ($R_1 / R_2$) from LiPo $V_{bat}$. |
| **GND** | Battery Divider | Bottom ($R_2$) | Ground | Black | Connects $R_2$ and 100 nF capacitor to GND. |
| **VIN / 5V** | TP4056 Module | OUT+ | Power In (3.7–4.2V) | Red | Power output from TP4056 to ESP32 onboard LDO. |
| **GND** | TP4056 Module | OUT- | Ground | Black | System ground from battery protection circuit. |
| *(None)* | LiPo Battery | (+) Terminal | Battery Power | Red | Direct connection to TP4056 **B+** pad. |
| *(None)* | LiPo Battery | (-) Terminal | Battery Ground | Black | Direct connection to TP4056 **B-** pad. |

---

## 2. Battery Voltage Divider Network

To monitor single-cell LiPo voltage (nominal 3.7 V, peak 4.2 V, cutoff ~3.2 V) without exceeding the ESP32 ADC rating (0 to 3.3 V), a 1:2 precision divider is used:

```text
    LiPo Vbat (3.0V - 4.2V)
          │
          ├───[ Slide Switch (SPDT) ]
          │
         [R1] 100kΩ (1% Metal Film)
          │
          ├───► To ESP32 GPIO34 (ADC1_CH6)
          │
         [R2] 100kΩ (1% Metal Film)
          │
         [C1] 100nF Ceramic (Low-pass filter capacitor)
          │
         GND
```

### Key Engineering Details:
- **Voltage Division Ratio**: $V_{adc} = V_{bat} \times \frac{R_2}{R_1 + R_2} = \frac{V_{bat}}{2}$.
  - Maximum input voltage at 4.2V battery charge = 2.10 V (safely within ESP32 ADC 11 dB attenuation range).
  - Cutoff voltage at 3.2V battery charge = 1.60 V.
- **Pin Selection**: **GPIO34** is selected specifically because it belongs to **ADC1**. ESP32 **ADC2** cannot be used simultaneously with the active BLE stack due to shared internal RF silicon resources. GPIO34 is input-only, eliminating any risk of driving accidental outputs.
- **Filtering**: A 100 nF ceramic capacitor across GPIO34 and GND suppresses high-frequency switching noise from the ESP32 DC rails.

---

## 3. Circuit Wiring Schematic (ASCII)

```text
               +----------------------------------------------------+
               |                1S LiPo Battery (3.7V)              |
               |             (+) Red             (-) Black          |
               +----------------+--------------------+--------------+
                                |                    |
                                v                    v
                     +---------------------------------------+
                     |         TP4056 Charging Module        |
                     |  [USB-C]   B+              B-         |
                     |            OUT+           OUT-        |
                     +--------------+--------------+---------+
                                    |              |
                      +--[Switch]---+              |
                      |                            |
       +--------------+--------------+             |
       |                             |             |
       v                             v             v
 [R1: 100k]                   +--------------+-------------+
       |                      |     ESP32 DevKit Board     |
       +---> GPIO34           | 5V/VIN                 GND |<----+ (Common GND)
       |                      | 3V3                        |     |
 [R2: 100k]                   | GPIO21 (SDA)  GPIO22 (SCL) |     |
       |                      +------+--------------+------+     |
      GND                            |              |            |
                                     |              |            |
                      +--------------+              |            |
                      |              |              |            |
                      |   +----------+--------------+            |
                      |   |                                      |
                      v   v                                      |
               +--------------+                                  |
               |   MPU-6050   | VCC <--- 3V3                     |
               | (0x68 AD0=0) | GND -----------------------------+
               |              | SDA <--- GPIO21                  |
               |              | SCL <--- GPIO22                  |
               +--------------+                                  |
                                                                 |
               +--------------+                                  |
               |   MAX30102   | VIN <--- 3V3                     |
               | (0x57 Addr)  | GND -----------------------------+
               |              | SDA <--- GPIO21
               |              | SCL <--- GPIO22
               +--------------+
```

---

## 4. Architectural Block Diagram (Mermaid)

```mermaid
flowchart TD
    subgraph PowerSystem ["Power Subsystem"]
        BATT["1S LiPo Battery\n3.7V nominal / 4.2V peak"]
        TP["TP4056 Charger &\nDW01A Protection"]
        SW["SPDT Slide Switch"]
        VD["Voltage Divider\n100k / 100k + 100nF"]
        
        BATT -->|B+ / B-| TP
        TP -->|OUT+| SW
        SW -->|V_Raw| VD
        SW -->|VIN Rail| ESP_REG["ESP32 Onboard\n3.3V LDO"]
    end

    subgraph Controller ["ESP32-WROOM-32 Controller"]
        ADC_CH["GPIO34 (ADC1_CH6)\nBattery Sensing"]
        I2C_BUS["Shared I2C Bus\nGPIO21 (SDA) / GPIO22 (SCL)"]
        BLE_CORE["ESP32 Dual-Core CPU\n& BLE 4.2 Subsystem"]
    end

    subgraph Sensors ["Wearable Sensor Array"]
        MPU["MPU-6050 (0x68)\nChest Kinematics & Accelerometer"]
        MAX["MAX30102 (0x57)\nPulse & SpO2 Optical Sensor"]
    end

    VD -->|Filtered V_adc| ADC_CH
    ESP_REG -->|3.3V Clean| MPU
    ESP_REG -->|3.3V Clean| MAX
    ESP_REG -->|3.3V Clean| Controller

    I2C_BUS <==>|Shared I2C (400kHz)| MPU
    I2C_BUS <==>|Shared I2C (400kHz)| MAX
    
    Controller -->|BLE Notifications\n(GATT Service)| DASH["Web Bluetooth Dashboard\n(Desktop / Mobile Chrome)"]
```

---

## 5. Critical Hardware Notes & Precautions

1. **Shared I2C Bus Pull-Ups**:
   - Both the MPU-6050 breakout (GY-521) and MAX30102 breakout typically feature onboard $4.7\text{ k}\Omega$ or $10\text{ k}\Omega$ pull-up resistors to their supply rail.
   - When connecting both modules on the same bus, the parallel combination yields roughly $2.3\text{ k}\Omega$ to $3.3\text{ k}\Omega$, which is within standard I2C Fast-Mode specs (400 kHz).
   - Ensure both modules are powered strictly from **3.3 V** (never 5 V) to protect the ESP32 GPIO inputs from overvoltage.
2. **MPU-6050 AD0 Address Pin**:
   - Pin AD0 controls the 7-bit I2C address. Connecting AD0 to **GND** sets the address to `0x68`.
   - If left floating, internal noise can cause the address to flip spuriously between `0x68` and `0x69`. Ground it firmly.
3. **MAX30102 Optical Contact**:
   - The MAX30102 optical surface requires steady, gentle contact against skin. Excessive pressure restricts local microvascular blood flow (leading to flatline signals), while loose contact introduces ambient light interference.
4. **Common Ground Integrity**:
   - The TP4056 OUT-, ESP32 GND, MPU-6050 GND, MAX30102 GND, and the divider capacitor ground must all tie back to a single star ground point to prevent ground bounce during BLE transmissions.
