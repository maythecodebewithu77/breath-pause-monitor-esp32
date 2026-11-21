# Custom Chest-Strap PCB Design Notes (KiCad)

Project: Wearable Breathing Pause Monitor (learning prototype)  
Author: Vigneshwar S, 2nd-year ECE student at SRM Institute of Science and Technology  
Tool: KiCad EDA (v7 / v8)

---

## 1. Mechanical & Form Factor Requirements

```text
       +-------------------------------------------------------+
       | [SLOT 1]                                     [SLOT 2] |
       | +-----+                                       +-----+ |
       | | 20x |     +--------------------------+      | 20x | |
       | | 3mm |     |    ESP32-WROOM-32E       |      | 3mm | |
       | +-----+     |    [RF Antenna Zone]     |      +-----+ |
       |             +--------------------------+              |
       |             +------------+ +-----------+              |
       |             |  MPU-6050  | |  TP4056   |              |
       |             +------------+ +-----------+              |
       |             +------------+ [USB-C Port]|              |
       |             |  MAX30102  | ===[==]==== |              |
       |             +------------+                            |
       +-------------------------------------------------------+
       <--------------------- 45.0 mm ------------------------->
```

- **Target Dimensions**: $45\text{ mm} \times 32\text{ mm}$ rectangular outline.
- **Corner Radius**: $3.0\text{ mm}$ rounded corners to prevent irritation against the chest wall.
- **Strap Mounting Slots**:
  - Dual lateral cutouts: $20.0\text{ mm} \times 3.0\text{ mm}$ with rounded edges on the left and right edges of the PCB.
  - Allows an adjustable 20 mm medical-grade elastic chest band to loop through directly, eliminating bulky external casing.
- **Board Thickness**: $1.2\text{ mm}$ or $1.6\text{ mm}$ standard FR-4 (1.2 mm reduces weight on the sternum).

---

## 2. Layer Stackup & Power Planes

A **2-layer PCB stackup** is selected for cost-efficiency and quick fabrication at Indian prototyping houses (e.g., LionCircuits, JLCPCB):

```text
Layer 1 (Top Layer):
  ├── Primary component placement (ESP32-WROOM, MPU-6050, Passives, LDO)
  ├── High-speed RF traces and crystal routing
  ├── I2C signal traces (0.25 mm width)
  └── Local copper ground pours stitched to bottom

Dielectric Core:
  └── FR-4, ~1.2 mm or 1.5 mm thickness

Layer 2 (Bottom Layer):
  ├── MAX30102 Optical Sensor (aperture facing chest skin)
  ├── Solid, continuous Ground Return Plane (0V)
  ├── Wide Battery & 3.3V Power Traces (0.6 mm - 1.0 mm width)
  └── USB-C Connector (recessed or edge-mounted)
```

---

## 3. Power Architecture & Battery Charging Section

1. **Li-ion / LiPo Charging Circuit**:
   - IC: **TP4056** (or micro-footprint **MCP73831T-2ACI/OT** in SOT-23-5).
   - Programming Resistor ($R_{prog}$): Set charging current to $250\text{ mA} - 300\text{ mA}$ for a 600 mAh LiPo cell:
     $$R_{prog} = \frac{1000\text{ V}}{I_{chg}} \approx \frac{1000}{0.25\text{ A}} = 4.0\text{ k}\Omega \quad (\text{Use } 3.9\text{ k}\Omega \text{ or } 4.7\text{ k}\Omega).$$
   - USB-C Connector: 16-pin or 6-pin Type-C receptacle.
     - **Mandatory**: Place two $5.1\text{ k}\Omega \pm 1\%$ pulldown resistors on pins `CC1` and `CC2` to GND. This ensures standard USB-PD chargers negotiate and deliver 5V.
2. **3.3V Low-Dropout Voltage Regulator (LDO)**:
   - IC: **AP2112K-3.3** (SOT-23-5) or **ME6211C33M5G**.
   - Current Capability: $\ge 600\text{ mA}$ continuous, low quiescent current ($< 55\ \mu\text{A}$).
   - Dropout Voltage: $< 250\text{ mV}$ at $500\text{ mA}$. This allows clean 3.3V regulation even when the LiPo discharges down to 3.55V.
   - Decoupling: $10\ \mu\text{F}$ low-ESR ceramic capacitor on input and output.
3. **Battery Monitoring Divider**:
   - Dual $100\text{ k}\Omega \pm 1\%$ 0603 SMD resistors.
   - $100\text{ nF}$ ceramic decoupling capacitor immediately adjacent to ESP32 pin GPIO34.

---

## 4. Sensor Placement & Thermal / Noise Isolation

1. **MPU-6050 (Motion / Kinematics)**:
   - Place as close to the mechanical center of the board as possible.
   - Chest expansion acts along the Z-axis (normal to the sternum) and Y-axis (vertical spine axis).
   - Place four $0.3\text{ mm}$ thermal-relief stitching vias around the ground pad.
2. **MAX30102 (Photoplethysmography)**:
   - **Crucial Placement Rule**: Place on the **Bottom Layer** facing the skin.
   - Keep the optical aperture clear of solder mask and copper. Add a circular silkscreen keepout ring of diameter $8\text{ mm}$ around the LED/photodiode lens.
   - Decoupling: Place $1.0\ \mu\text{F}$ and $0.1\ \mu\text{F}$ capacitors within $2\text{ mm}$ of the sensor power pins (`VDD` and `VLED`).
3. **I2C Bus Integrity**:
   - Provide footprints for two $4.7\text{ k}\Omega$ 0603 pull-up resistors to the 3.3V rail.
   - Route `SDA` and `SCL` as parallel differential-like traces with $\ge 0.3\text{ mm}$ clearance from switching power lines.

---

## 5. RF & Antenna Keepout Zone

```text
    +-----------------------------+
    |   ANTENNA COPPER KEEPOUT    |  <-- NO copper on Top, Bottom,
    |    (Min. 15 mm x 7 mm)      |      or Inner layers.
    |                             |      Must overhang PCB edge!
    +=============================+
    |     ESP32-WROOM-32 Module   |
    |     [Metal Shield Can]      |
    +-----------------------------+
```

- **Keep-out Rule**: The inverted-F PCB antenna on the ESP32-WROOM-32 must project over the board edge or rest on an area completely free of copper pours, traces, or mounting hardware across all layers.
- Minimum keepout distance: $15\text{ mm} \times 8\text{ mm}$.
- Failure to enforce this keepout will degrade BLE communication range from $> 15\text{ m}$ down to $< 1\text{ m}$.

---

## 6. Test Points & In-Circuit Programming

Provide six $1.0\text{ mm}$ circular test pads (or a $1 \times 6$ $2.54\text{ mm}$ unpopulated header) for initial firmware flashing and debugging using an external USB-to-UART adapter (CP2102 / CH340):

| Test Point | Signal | Purpose |
| :---: | :--- | :--- |
| **TP1** | `GND` | Ground reference for UART and scope. |
| **TP2** | `3V3` | External 3.3V power injection during bench tests. |
| **TP3** | `TXD0` (GPIO1) | Serial output (firmware logs, 115200 baud). |
| **TP4** | `RXD0` (GPIO3) | Serial flashing input. |
| **TP5** | `IO0` | Bootloader strap pin (pull to GND to enter download mode). |
| **TP6** | `EN` (CHIP_PU) | Reset line (with $10\text{ k}\Omega$ pullup and $1\ \mu\text{F}$ timing cap to GND). |
| **TP7** | `GPIO34` | ADC battery divider test point. |
| **TP8** | `SDA` | I2C data line observation point for logic analyzer. |
| **TP9** | `SCL` | I2C clock line observation point. |

---

## 7. Wearable Safety & Environmental Sealing

1. **Sweat & Moisture Barrier**: Perspiration contains saline and acts as an electrolyte. After assembling and verifying the PCB, apply silicone conformal coating (or acrylic protective lacquer) to all components *except* the optical window of the MAX30102 and the USB-C port.
2. **Skin Contact Ring**: Design a 3D-printed TPU (flexible) or silicone gasket around the MAX30102 to create gentle, light-tight skin contact and prevent chest strap friction burns.
