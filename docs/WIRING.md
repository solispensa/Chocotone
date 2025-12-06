# Wiring Diagram

Visual pin connection guide for the ESP32 MIDI Controller.

## Quick Reference Table

| Component | Component Pin | ESP32 GPIO | Wire Color Suggestion |
|-----------|---------------|------------|----------------------|
| **OLED Display** | | | |
| | VCC | 3.3V | Red |
| | GND | GND | Black |
| | SCL | GPIO 22 | Yellow |
| | SDA | GPIO 21 | Green |
| **Rotary Encoder** | | | |
| | VCC/+ | 5V (VIN) or 3.3V | See encoder power note below |
| | CLK (A) | GPIO 18 | White |
| | DT (B) | GPIO 19 | Gray |
| | SW (Button) | GPIO 23 | Blue |
| | GND | GND | Black |
| **NeoPixel Strip** | | | |
| | VCC | 5V | Red |
| | GND | GND | Black |
| | DIN | GPIO 5 | Orange |
| **Button 1** | One side | GPIO 27 | - |
| | Other side | GND | Black |
| **Button 2** | One side | GPIO 14 | - |
| | Other side | GND | Black |
| **Button 3** | One side | GPIO 12 | - |
| | Other side | GND | Black |
| **Button 4** | One side | GPIO 13 | - |
| | Other side | GND | Black |
| **Button 5** | One side | GPIO 15 | - |
| | Other side | GND | Black |
| **Button 6** | One side | GPIO 2 | - |
| | Other side | GND | Black |
| **Button 7** | One side | GPIO 34 | - |
| | Other side | GND | Black |
| | **10kΩ to 3.3V** | **Required** | **Red** |
| **Button 8** | One side | GPIO 35 | - |
| | Other side | GND | Black |
| | **10kΩ to 3.3V** | **Required** | **Red** |

## GPIO Pin Layout (ESP32-DevKitC)

```
                     ESP32-DevKitC
          ┌───────────────────────────────┐
          │                               │
      3V3 │●                            ●│ GND
      EN  │●                            ●│ GPIO 23  ← Encoder Button
GPIO 36   │●                            ●│ GPIO 22  ← OLED SCL
GPIO 39   │●                            ●│ TXD0
GPIO 34   │●← Button 7 (with pull-up)  ●│ RXD0
GPIO 35   │●← Button 8 (with pull-up)  ●│ GPIO 21  ← OLED SDA
GPIO 32   │●                            ●│ GND
GPIO 33   │●                            ●│ GPIO 19  ← Encoder B
GPIO 25   │●                            ●│ GPIO 18  ← Encoder A
GPIO 26   │●                            ●│ GPIO 5   ← NeoPixel DIN
GPIO 27   │●← Button 1                 ●│ GPIO 17
GPIO 14   │●← Button 2                 ●│ GPIO 16
GPIO 12   │●← Button 3                 ●│ GPIO 4
      GND │●                            ●│ GPIO 0
GPIO 13   │●← Button 4                 ●│ GPIO 2   ← Button 6
     SHD  │●                            ●│ GPIO 15  ← Button 5
     SCK  │●                            ●│ GND
          │                               │
          │         [USB Port]            │
          └───────────────────────────────┘
```

## Button Layout Recommendation

Physical button arrangement (looking at controller from user perspective):

```
Top Row:
┌────────┬────────┬────────┬────────┐
│ BTN 5  │ BTN 6  │ BTN 7  │ BTN 8  │
│ GPIO15 │ GPIO2  │ GPIO34*│ GPIO35*│
│ LED 4  │ LED 5  │ LED 6  │ LED 7  │
└────────┴────────┴────────┴────────┘

Bottom Row:
┌────────┬────────┬────────┬────────┐
│ BTN 1  │ BTN 2  │ BTN 3  │ BTN 4  │
│ GPIO27 │ GPIO14 │ GPIO12 │ GPIO13 │
│ LED 0  │ LED 1  │ LED 2  │ LED 3  │
└────────┴────────┴────────┴────────┘

Center:
        ┌─────────────┐
        │    OLED     │
        │   128x64    │
        └─────────────┘
              ◎  ← Rotary Encoder

* Buttons 7 & 8 require 10kΩ external pull-up resistors
```

## Pull-up Resistor Configuration (GPIO 34 & 35)

```
For Button 7 (GPIO 34):
                    3.3V
                     │
                    ┌┴┐
                    │ │ 10kΩ
                    └┬┘
                     │
       Button 7      ├─────→ GPIO 34
         ┌───┐       │
         │   ├───────┤
         └───┘       │
                     ├─────→ GND
                     

For Button 8 (GPIO 35):
                    3.3V
                     │
                    ┌┴┐
                    │ │ 10kΩ
                    └┬┘
                     │
       Button 8      ├─────→ GPIO 35
         ┌───┐       │
         │   ├───────┤
         └───┘       │
                     ├─────→ GND
```

## NeoPixel LED Mapping

LEDs are mapped to match button positions:

```cpp
const int ledMap[8] = {0, 1, 2, 3, 4, 5, 6, 7};
```

Connect NeoPixels in order:
- LED 0 → Button 1 (GPIO 27)
- LED 1 → Button 2 (GPIO 14)
- LED 2 → Button 3 (GPIO 12)
- LED 3 → Button 4 (GPIO 13)
- LED 4 → Button 5 (GPIO 15)
- LED 5 → Button 6 (GPIO 2)
- LED 6 → Button 7 (GPIO 34)
- LED 7 → Button 8 (GPIO 35)

## Power Distribution

```
USB 5V ──┬──→ ESP32 VIN (5V input)
         │
         ├──→ NeoPixel VCC (via power rail)
         │
         └──→ (Optional) External components

ESP32 ───┬──→ 3.3V rail (for logic)
         │
         ├──→ OLED VCC
         ├──→ Button 7 pull-up (10kΩ to GPIO 34)
         └──→ Button 8 pull-up (10kΩ to GPIO 35)

Common ──┴──→ GND rail (all grounds connected)
```

## Breadboard Layout Example

```
Row indicators:  + = Power rail, - = Ground rail

+5V  ════════════════════════════════════════ Red rail
GND  ════════════════════════════════════════ Black rail
     ┌──────────────────────────────────────┐
   a │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   b │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   c │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   d │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   e │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
     ──ESP32──┐                               
   f │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   g │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   h │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   i │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
   j │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │
     └──────────────────────────────────────┘
GND  ════════════════════════════════════════ Black rail
3.3V ════════════════════════════════════════ Red rail
```

**Component Placement:**
- ESP32: Center, spanning rows e-f
- OLED: Upper left corner
- Encoder: Upper right corner
- Buttons: Two rows below ESP32
- NeoPixels: Aligned with buttons

## Testing Connections

### Continuity Test (Multimeter)
Before powering on:
1. Check **no shorts** between 5V and GND
2. Check **no shorts** between 3.3V and GND
3. Verify all button connections to ground
4. Confirm I2C connections (21, 22)

### Power Test
1. Connect USB only (no components)
2. Verify **3.3V** on ESP32 3.3V pin
3. Verify **5V** on ESP32 VIN pin
4. Check current draw (<100mA with no LEDs)

### Incremental Testing
Add components one at a time:
1. ESP32 only
2. Add OLED → Verify display
3. Add Encoder → Test rotation
4. Add 1-2 buttons → Test presses
5. Add remaining buttons
6. Add NeoPixels last

## Notes

- **Keep I2C wires short** (<15cm) for reliability
- **Twist I2C pairs** (SCL+GND, SDA+GND) to reduce noise
- **Add decoupling caps** (0.1µF near ESP32, OLED)
- **Use thicker wire** for power rails (22 AWG recommended)
- **Color code wires** for easier debugging

## Related Documentation

- [Hardware Specifications](HARDWARE.md) - Component details and BOM
- [Main README](../README.md) - Software setup and usage

---

**Always double-check connections before applying power!**
