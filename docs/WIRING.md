# Wiring Diagram

Visual pin connection guide for the Chocotone MIDI Controller.

## Quick Reference Table

| Component | Component Pin | ESP32 GPIO | Notes |
|-----------|---------------|------------|-------|
| **OLED Display** | | | **I2C Mode (Default)** |
| | VCC | 3.3V | |
| | GND | GND | |
| | SCL | GPIO 22 | |
| | SDA | GPIO 21 | |
| **TFT Display** | | | **SPI Mode (128x128)** |
| | VCC | 3.3V | |
| | GND | GND | |
| | SCL (SCLK) | GPIO 18 | *Default Encoder A is 18 - Conflict* |
| | SDA (MOSI) | GPIO 23 | *Default Encoder Btn is 23 - Conflict* |
| | RES (RST) | GPIO 4 | |
| | DC | GPIO 2 | *Default Button 6 is 2 - Conflict* |
| | CS | GPIO 15 | *Default Button 5 is 15 - Conflict* |
| | BLK (LED) | GPIO 32 | *Default Button 6 is 32 - Conflict* |
| **Rotary Encoder** | | | |
| | VCC/+ | 3.3V / 5V | |
| | GND | GND | |
| | CLK (A) | GPIO 18 | **Conflict with TFT**. Use free pin (e.g., 25) |
| | DT (B) | GPIO 19 | |
| | SW (Btn) | GPIO 23 | **Conflict with TFT**. Use free pin (e.g., 26) |
| **NeoPixel Strip** | | | |
| | VCC | 5V | Buffer logic level if unstable |
| | GND | GND | |
| | DIN | GPIO 5 | |
| **Analog Input** | | | **Expression Pedal / Pot** |
| | VCC | 3.3V | |
| | GND | GND | |
| | Signal | GPIO 36/39 | Only use INPUT-ONLY pins (34-39) |

## Display Wiring

### Option A: I2C OLED (Default)
Most common for 128x64 or 128x32 OLEDs.
- **SDA**: GPIO 21
- **SCL**: GPIO 22

### Option B: SPI TFT (128x128)
For color ST7735 displays. **Note:** This requires more pins and creates conflicts with the default encoder/button layout.
- **MOSI (SDA)**: GPIO 23
- **SCLK (SCK)**: GPIO 18
- **CS**: GPIO 15
- **DC (A0)**: GPIO 2
- **RST**: GPIO 4
- **LED (BLK)**: GPIO 32

**⚠️ IMPORTANT: SPI Pin Conflicts**
When using the TFT display, pins 18, 23, 15, 2, and 32 are occupied. You must move any overlapping components to other free pins:
- **Encoder A (18)** -> Move to e.g., GPIO 25
- **Encoder Btn (23)** -> Move to e.g., GPIO 26
- **Button 6 (2)** -> Move to e.g., GPIO 13
- **Button 5 (15)** -> Move to e.g., GPIO 12

*The firmware will attempt to auto-resolve conflicts, but it is best to wire them to free pins manually.*

## Analog Input Recommendation

For adding an Expression Pedal or Potentiometer input (e.g., for Volume, Wah), follow this circuit:

**Recommended Components:**
- **Potentiometer:** 10kΩ Linear (B-Taper)
- **Resistor:** 1kΩ (Series protection)

**Wiring:**
```
       3.3V
         │
         │
      ┌──┴──┐
      │ Pot │ 10kΩ Linear (B)
      │     │
      └──┬──┘
         │      1kΩ Resistor
   Wiper ├───────[////]───────→ ESP32 Analog Pin (e.g., GPIO 36, 39)
         │
         │
        GND
```
*The 1kΩ resistor protects the GPIO from high current if the pin is accidentally configured as output.*

### Notes on Pins
- **Use Input-Only Pins:** GPIO 34, 35, 36, 39 are best for Analog Inputs as they lack internal pull-ups and are input-only, reducing interference.
- **Avoid:** GPIO 0, 2, 12, 15 (Strapping pins) for inputs if possible, or ensure they are not held low/high at boot.

## GPIO Pin Layout (ESP32-DevKitC)

```
                     ESP32-DevKitC
          ┌───────────────────────────────┐
          │                               │
      3V3 │●                            ●│ GND
      EN  │●                            ●│ GPIO 23  ← MOSI (TFT) / Enc Btn
GPIO 36   │●← Analog In                 ●│ GPIO 22  ← OLED SCL
GPIO 39   │●← Analog In                 ●│ TXD0
GPIO 34   │●← Button 7 (Pull-up needed)●│ RXD0
GPIO 35   │●← Button 8 (Pull-up needed)●│ GPIO 21  ← OLED SDA
GPIO 32   │●← TFT LED / Btn 6          ●│ GND
GPIO 33   │●← Button 5                 ●│ GPIO 19  ← Encoder B
GPIO 25   │●                           ●│ GPIO 18  ← SCLK (TFT) / Enc A
GPIO 26   │●                           ●│ GPIO 5   ← NeoPixel DIN
GPIO 27   │●← Button 1                 ●│ GPIO 17
GPIO 14   │●← Button 2                 ●│ GPIO 16
GPIO 12   │●← Button 3                 ●│ GPIO 4   ← TFT RST
      GND │●                           ●│ GPIO 0
GPIO 13   │●← Button 4                 ●│ GPIO 2   ← TFT DC
     SHD  │●                           ●│ GPIO 15  ← TFT CS
     SCK  │●                           ●│ GND
          │                               │
          │         [USB Port]            │
          └───────────────────────────────┘
```

## ESP32-S3 Wiring Guide (N16R8 / DevKitM)

**Safe Pinout (WiFi/BLE Compatible)**

This layout matches the **"ESP32-S3 Default (8-btn)"** editor template.

| Component | Pin Function | ESP32-S3 GPIO | Notes |
|-----------|--------------|---------------|-------|
| **Buttons** | Button 1 | GPIO 38 | |
| | Button 2 | GPIO 39 | |
| | Button 3 | GPIO 40 | |
| | Button 4 | GPIO 41 | |
| | Button 5 | GPIO 42 | |
| | Button 6 | GPIO 21 | |
| | Button 7 | GPIO 8 | |
| | Button 8 | GPIO 9 | |
| **Encoder** | A | GPIO 16 | |
| | B | GPIO 17 | |
| | Btn | GPIO 18 | |
| **Display** | CS | GPIO 10 | SPI (FSPI) |
| (128x128) | MOSI | GPIO 11 | |
| | SCLK | GPIO 12 | |
| | DC | GPIO 13 | |
| | RST | GPIO 14 | |
| | LED | GPIO 15 | Backlight |
| **LED** | WS2812 DIN | GPIO 48 | Built-in RGB (DevKit) |
| **Power** | Battery ADC | GPIO 3 | ADC1 Ch3 |
| **Analog** | A1 | GPIO 4 | ADC1 Ch4 |
| | A2 | GPIO 5 | ADC1 Ch5 |
| | A3 | GPIO 7 | ADC1 Ch7 |
| | A4 | GPIO 33 | *Verify PSRAM conflict if N16R8* |

---

## Wiring Schematics

### V2 Wiring (Classic ESP32)
![V2 Wiring](../images/wiring_schematic_v2.png)

### V3 Wiring (ESP32-S3)
![V3 Wiring](../images/wiring_schematic_v3.png)

