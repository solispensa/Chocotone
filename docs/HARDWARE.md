# Chocotone MIDI Controller - Hardware Documentation

This document provides detailed hardware specifications and assembly instructions for the Chocotone MIDI Controller.

## Bill of Materials (BOM)

Two hardware variants are available depending on your use case:
- **Variant A: Desktop Version** - Tactile buttons for studio/desktop use
- **Variant B: Floor Pedal Version** - Footswitches for live performance/floor use

---

### Variant A: Desktop Version (Tactile Buttons)

| Qty | Component | Specification | Example Part | Approx. Cost |
|-----|-----------|---------------|--------------|--------------|
| 1 | ESP32 Development Board | ESP32 or ESP32-S3 (for Native USB) | ESP32-DevKitC, ESP32-S3-DevKit-1 | $5-15 |
| 1 | Display | 128x64 OLED (I2C) or 128x128 TFT (SPI) | 0.96" OLED or 1.44" TFT | $3-8 |
| 1 | Rotary Encoder | EC11 type with push button | EC11 Encoder | $1-2 |
| 8 | Tactile Buttons | 6x6mm or 12x12mm momentary | Standard tactile switches | $2-4 |
| 8 | NeoPixel LEDs | WS2812B individual or strip | WS2812B LED Strip/Ring | $3-8 |
| - | Analog Inputs | Pots, Expression Pedals, FSR, Piezo | 10k Linear Pot, FSR402 | $2-15 |
| 2 | 100kΩ Resistors | 1/4W, voltage divider | For battery monitoring (ESP32-S3) | $0.50 |
| 2 | 10kΩ Resistors | 1/4W | For GPIO 34/35 pull-ups (ESP32) | $0.50 |
| - | Jumper Wires | 22-24 AWG, various lengths | Breadboard jumpers or custom | $2-5 |
| 1 | Breadboard/PCB | For prototyping or permanent | Half+ breadboard or custom PCB | $2-10 |
| 1 | Power Supply | USB 5V, 2A minimum | USB power supply | $3-5 |

**Total Estimated Cost:** $20-50 USD

---

### Variant B: Floor Pedal Version (Footswitches)

| Qty | Component | Specification | Example Part | Approx. Cost |
|-----|-----------|---------------|--------------|--------------|
| 1 | ESP32 Development Board | Any ESP32 with BLE support | ESP32-DevKitC, ESP32-WROOM-32 | $5-10 |
| 1 | OLED Display | 128x64, I2C, SSD1306 driver | 0.96" I2C OLED | $3-5 |
| 1 | Rotary Encoder | EC11 type with push button | EC11 Encoder | $1-2 |
| 8 | Footswitches | SPST Momentary Soft Touch, Normally Open | Soft-touch foot switches | $15-30 |
| 8 | 1/4" Mono Jacks | Panel mount (optional) | For remote footswitch mounting | $5-10 |
| 8 | NeoPixel LEDs | WS2812B individual or strip | WS2812B LED Strip/Ring | $3-8 |
| 2 | 10kΩ Resistors | 1/4W | For GPIO 34/35 pull-ups (recommended) | $0.50 |
| 1 | 470Ω Resistor | 1/4W | Optional: NeoPixel data line | $0.10 |
| - | Wire | 22-24 AWG, shielded for footswitches | For footswitch connections | $3-8 |
| 1 | Enclosure | Rugged, floor-suitable | Custom or project box | $10-30 |
| 1 | Power Supply | USB 5V, 2A minimum or battery | USB adapter or LiPo battery | $5-15 |

**Total Estimated Cost:** $50-120 USD

> [!NOTE]
> The floor pedal version requires more robust construction and wiring. Consider using shielded cable for footswitch connections to reduce noise in stage environments.

## Detailed Component Specifications

### ESP32 Development Board

**Recommended Models:**
- **ESP32-S3 DevKitC-1**: Best for modern builds with Native USB MIDI.
- **ESP32-DevKitC v4**: Classic choice for BLE-only or Serial MIDI.
- **ESP32-WROOM-32**: Standard module for custom PCBs.

**Requirements:**
- BLE (Bluetooth Low Energy) support
- WiFi capability (for Web Config)
- **ESP32-S3**: Specifically required for Native USB MIDI and battery monitoring.
- Minimum 4MB flash (8MB+ recommended for S3 variants)
- USB programming interface

**Not Recommended:**
- ESP32-C3 (different GPIO layout, may need pin changes)
- ESP8266 (no BLE support)

### OLED Display (SSD1306) & TFT (ST7735)

**OLED (I2C):**
- Size: 0.96" (128x64 pixels) or 0.91" (128x32 pixels)
- Interface: I2C (4-pin: VCC, GND, SCL, SDA)
- Driver IC: SSD1306
- Operating Voltage: 3.3V

**TFT Color Display (SPI) - v1.5:**
- Size: 1.44" or 1.77" (128x128 pixels)
- Interface: SPI (MOSI, SCLK, CS, DC, RST)
- Driver IC: ST7735
- Features: Shows color-coded strips for each preset and button state.

### Rotary Encoder (EC11)

**Specifications:**
- Type: Incremental encoder
- Detents: 20-30 per revolution typical
- Button: Integrated push button
- Pins: 5 pins (VCC/+, CLK, DT, SW, GND)

**Wiring:**
- VCC/+ → ESP32 VIN (5V) **or** 3.3V (see note below)
- CLK → ESP32 GPIO 18
- DT → ESP32 GPIO 19
- SW → ESP32 GPIO 23
- GND → Common ground

> [!IMPORTANT]
> **Encoder Power Options:**
> - **5V Encoders** (5-pin with VCC): Connect VCC to ESP32 **VIN pin** (not USB 5V rail directly)
> - **3.3V Compatible**: Some encoders work at 3.3V - connect to ESP32 3.3V pin
> - **Passive Encoders** (3-pin, no VCC): No power needed, just CLK/DT/GND
>
> **If OLED stops working when encoder button is pressed:**
> 1. Add 0.1µF capacitor between encoder VCC and GND (very close to encoder)
> 2. Ensure all GND connections are solid (common ground)
> 3. Try powering encoder from 3.3V instead of 5V
> 4. Check that ESP32 can supply enough current (use external 5V if needed)

### NeoPixel LEDs (WS2812B)

**Specifications:**
- Type: WS2812B or compatible (SK6812, WS2811)
- Quantity: 8 individual LEDs
- Voltage: 5V recommended (3.3V works but reduced brightness)
- Current: ~60mA max per LED at full white

**Options:**
- 8-LED NeoPixel Ring
- Strip cut to 8 LEDs
- Individual WS2812B modules

**Power Considerations:**
- Max current (8 LEDs full white): ~480mA
- Ensure adequate 5V power supply (2A recommended)
- Consider separate power for LEDs if using many

### Buttons

**Desktop Variant - Tactile Switches:**
- Type: Momentary tactile switches
- Quantity: 8
- Size: 6x6mm or 12x12mm
- Force: 160-260gf typical
- Mounting: Through-hole (PCB) or breadboard

**Floor Pedal Variant - Footswitches:**
- Type: SPST Momentary Soft Touch Foot Switch, Normally Open
- Quantity: 8
- Actuation Force: Light touch (varies by model)
- Mounting: Panel mount or 1/4" jack connection
- Cable: 22-24 AWG shielded recommended for lengths >1m

**Notes on Analog Inputs (v1.5):**
- **Precision Signal Processing**: Includes oversampling and EMA smoothing to eliminate jitter from old pots.
- **Support for FSR/Piezo**: Use pressure-sensitive pads or drum triggers.
- **Response Curves**: Switch between Linear, Logarithmic, and Exponential for natural control (e.g., volume/wah).
- **ESP32 Restrictions**: Buttons on GPIO 34 and 35 **strongly benefit from external 10kΩ pull-up resistors** as these are input-only.
- **ESP32-S3 Note**: S3 internal pull-ups are generally stronger/better, but external ones are still safer for high-interference environments.

## Pin Assignment Reference

### ESP32 Pin Definitions

From `Chocotone/Config.h`:

```cpp
// Display (I2C)
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

// Encoder
#define ENCODER_A_PIN 18
#define ENCODER_B_PIN 19
#define ENCODER_BUTTON_PIN 23

// NeoPixels
#define NEOPIXEL_PIN 5
```

### Button Pin Array

From `Chocotone/Globals.cpp`:

```cpp
int buttonPins[8] = {14, 27, 26, 25, 33, 32, 16, 17};
// Button mapping: [B1, B2, B3, B4, B5, B6, B7, B8]
```

### Pin Capabilities and Restrictions

| GPIO | Input | Output | ADC | Touch | Notes |
|------|-------|--------|-----|-------|-------|
| 2 | ✓ | ✓ | - | ✓ | Internal LED on some boards |
| 5 | ✓ | ✓ | - | - | Used for NeoPixel |
| 12 | ✓ | ✓ | ✓ | ✓ | Boot fails if HIGH |
| 13 | ✓ | ✓ | ✓ | ✓ | |
| 14 | ✓ | ✓ | ✓ | ✓ | |
| 15 | ✓ | ✓ | ✓ | ✓ | Boot fails if HIGH |
| 18 | ✓ | ✓ | - | - | |
| 19 | ✓ | ✓ | - | - | |
| 21 | ✓ | ✓ | - | - | I2C SDA |
| 22 | ✓ | ✓ | - | - | I2C SCL |
| 23 | ✓ | ✓ | - | - | |
| 27 | ✓ | ✓ | ✓ | ✓ | |
| 34 | ✓ | ✗ | ✓ | - | **Input only, no pull-up** |
| 35 | ✓ | ✗ | ✓ | - | **Input only, no pull-up** |

> [!NOTE]
> **GPIO 12 and 15** are boot-mode pins. Avoid holding these buttons pressed during power-on or reset to prevent boot issues. Normal operation after boot is unaffected.

## Assembly Instructions

### Step 1: Breadboard Layout

1. **Mount ESP32** on breadboard with access to both sides
2. **Position OLED** near ESP32 for short I2C wires
3. **Place encoder** within easy reach
4. **Arrange 8 buttons** in desired layout (2 rows of 4 recommended)
5. **Position NeoPixels** aligned with buttons

### Step 2: Power Distribution

1. Connect ESP32 **GND** to breadboard ground rail
2. Connect ESP32 **3.3V** to breadboard 3.3V rail (for logic)
3. Connect **5V supply** to breadboard 5V rail (for NeoPixels)
4. Ensure common ground between all power supplies

### Step 3: I2C Connections (OLED)

```
OLED VCC → ESP32 3.3V (or 5V if display is 5V tolerant)
OLED GND → GND
OLED SCL → GPIO 22
OLED SDA → GPIO 21
```

No pull-up resistors needed (internal pull-ups enabled in code).

### Step 4: Encoder Connections

```
Encoder CLK → GPIO 18
Encoder DT  → GPIO 19
Encoder SW  → GPIO 23
Encoder GND → GND
```

### Step 5: Button Connections

For each button:

**Desktop Variant (Tactile Buttons):**

**Buttons 1-6 (GPIO 27, 14, 12, 13, 15, 2):**
```
One pin → ESP32 GPIO
Other pin → GND
```

**Buttons 7-8 (GPIO 34, 35) - Pull-ups strongly recommended:**
```
One pin → ESP32 GPIO
Other pin → GND
10kΩ resistor from GPIO pin to 3.3V (strongly recommended)
```

**Floor Pedal Variant (Footswitches):**

If using 1/4" jacks:
```
Jack Tip → ESP32 GPIO
Jack Sleeve → GND
Footswitch connects Tip to Sleeve when pressed
```

For GPIO 34/35, add 10kΩ pull-up to 3.3V as above.

### Step 6: NeoPixel Strip/Ring

```
NeoPixel VCC → 5V rail
NeoPixel GND → GND
NeoPixel DIN → GPIO 5
```

**Optional:** Add 470Ω resistor between GPIO 5 and NeoPixel DIN for protection.

**Power Note:** If using more than 8 LEDs or running at high brightness, power NeoPixels from external 5V supply (not through ESP32).

## Schematic Overview

```
                    ESP32-DevKitC
                    ┌───────────┐
         ┌──────────┤ 3.3V  GND ├──────────┐
         │         ├───────────┤          │
         │   ┌──────┤ G21   G22 ├──────┐   │
         │   │      ├───────────┤      │   │
         │   │  ┌───┤ G18   G19 ├───┐  │   │
         │   │  │   ├───────────┤   │  │   │
         │   │  │ ┌─┤ G23       │   │  │   │
         │   │  │ │ └───────────┘   │  │   │
         │   │  │ │                 │  │   │
    OLED │   │  │ │                 │  │   │
    ┌────┴───┴┐ │ │    Encoder     │  │   │
    │ VCC  SCL├─┘ │    ┌───────┐   │  │   │
    │ GND  SDA├───┘    │ CLK  DT├───┘  │   │
    │ 0.96"   │        │ SW  GND│      │   │
    └─────────┘        └───┬───┴──────┘   │
                           └───────────────┘

    Buttons (x8)          NeoPixels
    GPIO 27,14,12,13,    GPIO 5 → DIN
    15,2,34*,35*         5V → VCC
    *with 10kΩ pull-up   GND → GND
```

## Testing Procedure

### 1. Power-On Test
- Connect USB power
- OLED should display "Starting..."
- LEDs should briefly initialize

### 2. Button Test
- Press each button
- Corresponding LED should brighten
- OLED should show button name
- Serial Monitor should show MIDI message

### 3. Encoder Test
- Rotate encoder → Menu selection should change
- Press encoder → Enter/exit menu

### 4. Display Test
- All text should be clearly visible
- No missing pixels or glitches

### 5. BLE Test
- Power on target MIDI device
- Check Serial Monitor for "Scanning for SPM..."
- OLED should show "SPM:Y" when connected

## Troubleshooting Hardware Issues

### OLED Not Working
- Check I2C address (try I2C scanner sketch)
- Verify 3.3V/5V compatibility
- Check wire connections (swap SCL/SDA if needed)
- **If display stops when encoder button pressed:**
  - Encoder drawing too much current from 3.3V rail
  - Try powering encoder from VIN (5V) instead
  - Add 0.1µF capacitor between encoder VCC and GND
  - Ensure solid ground connections

### Encoder Erratic Behavior
- Add 0.1µF capacitors across CLK and DT pins
- Check for loose connections
- Try different GPIO pins

### Buttons Not Responding
- Verify GPIO 34/35 have external pull-ups (strongly recommended for reliability)
- Check for shorts on breadboard
- Test with multimeter in continuity mode  
- For footswitches: Check cable integrity and shielding

### NeoPixels Not Lighting
- Ensure 5V power supply adequate
- Check data direction (DIN not DOUT)
- Add 470Ω resistor on data line
- Level shift 3.3V→5V if needed (usually not required)

### ESP32 Won't Boot
- Check if GPIO 12 or 15 are HIGH at boot
- Ensure no shorts on power rails
- Verify sufficient power supply current

## PCB Design Considerations

If designing a custom PCB:

1. **ESD Protection** - Add TVS diodes on USB lines
2. **Power Filtering** - 100nF caps near each IC
3. **NeoPixel Level Shifting** - Consider 74HCT125 buffer
4. **Mounting Holes** - Plan enclosure early
5. **GPIO 34/35 Pull-ups** - Include 10kΩ resistors on board
6. **Test Points** - Add for power rails and key signals

## Enclosure Options

**Desktop Version:**
- 3D printed custom case
- Hammond project box (recommend 150x100x50mm minimum)
- Laser-cut acrylic layers
- DIY wooden enclosure

**Floor Pedal Version:**
- Rugged metal enclosure with rubber feet
- Stomp-box style housing
- 1U rack-mount case
- Custom 3D printed with reinforced mounting

**Design Requirements:**
- Cutouts for buttons/footswitches (8x holes)
- OLED window
- Encoder shaft hole
- USB access port
- Ventilation for ESP32
- Cable glands for footswitch variant

## Related Documentation

- [Main README](../README.md) - Project overview
- [Wiring Diagram](WIRING.md) - Visual wiring guide
- [Software Setup](../README.md#installation--setup) - Firmware installation

---

**Note:** Always verify component specifications against your specific parts. Pin-compatible alternatives may have different voltage or current requirements.
