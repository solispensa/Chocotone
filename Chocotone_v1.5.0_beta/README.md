# Chocotone v1.5.0 BETA Source Code

> ⚠️ **DEVELOPMENT BRANCH** - This is the beta version for v1.5.0 feature development.
> For stable release code, see the `Chocotone` folder (v1.4.0).

This directory contains the Arduino source code for the ESP32 MIDI Controller.

## Features Implemented in v1.5.0
- [x] Analog input (expression pedal support, up to 16 via multiplexer)
- [x] 128x128 TFT color display (ST7735 SPI)
- [x] 128x32 OLED support
- [x] ESP32-S3 Native USB MIDI
- [x] Battery monitoring (auto-calibrating ADC)
- [x] SysEx scroll parameters (AMP GAIN, PITCH LOW, RVB MIX, etc.)
- [x] Device profiles and templates

## File Structure

| File | Purpose |
|------|---------|
| `Chocotone_v1.5.0_beta.ino` | Main Arduino sketch with setup() and loop() |
| `Config.h` | Pin definitions and compile-time constants |
| `Globals.h/cpp` | Global variables, objects, and data structures |
| `BleMidi.h/cpp` | BLE MIDI client/server + USB MIDI implementation |
| `Input.h/cpp` | Button, encoder, and action handling |
| `Storage.h/cpp` | NVS (non-volatile storage) persistence |
| `UI_Display.h/cpp` | OLED/TFT display rendering + NeoPixel LED management |
| `WebInterface.h/cpp` | Web server, USB serial config API |
| `WebEditorHTML.h` | Embedded HTML for the web editor interface |
| `AnalogInput.h/cpp` | Analog inputs: expression pedals, pots, FSR, piezo |
| `GP5Protocol.h/cpp` | Valeton GP-5 SysEx sync protocol |
| `DeviceProfiles.h/cpp` | Device-specific presets and templates |
| `DefaultPresets.h` | Factory default presets |
| `SysexScrollData.h` | SysEx scroll parameter engine |
| `SysexScroll*.h` | Individual scroll parameters (AmpGain, PitchLow, etc.) |
| `sys_ex_data.h/cpp` | Generated SysEx lookup tables |
| `delay_time_sysex.h` | Tap tempo delay time calculations |

## Module Responsibilities

### Core System (Chocotone.ino)
- System initialization
- Main loop coordination
- Mode switching (preset vs menu)

### Configuration (Config.h)
- Hardware pin assignments
- System constants (screen size, button count, etc.)
- Default WiFi AP credentials

### BLE + USB MIDI (BleMidi.h/cpp)
- BLE client initialization and device scanning
- BLE server mode for DAW connections
- USB MIDI (ESP32-S3) via TinyUSB
- MIDI message transmission (Note, CC, PC, SysEx)
- SysEx scroll parameter engine
- Auto-reconnection logic

### Input Handling (Input.h/cpp)
- Button press/release detection
- Debounce handling
- Rotary encoder state tracking
- Tap tempo implementation
- Menu navigation

### Storage (Storage.h/cpp)
- NVS namespace management
- Preset save/load
- System settings persistence
- Factory reset functionality

### Display (UI_Display.h/cpp)
- OLED screen rendering
- Button label display
- Menu rendering
- Tap tempo mode display
- LED color management

### Web Interface (WebInterface.h/cpp)
- HTTP server routes
- JSON configuration API
- HTML interface generation
- Import/export functionality
- WiFi AP management

## Data Flow

```
User Input → Input.cpp → MIDI Message → BleMidi.cpp → BLE Device / USB DAW
                ↓                           ↓
         UI_Display.cpp              Storage.cpp (save state)
                ↓
         OLED/TFT Display + LEDs
```

### Additional Modules
- **AnalogInput.h/cpp** — Reads expression pedals, pots, FSR, piezo with smoothing and custom curves
- **GP5Protocol.h/cpp** — Valeton GP-5 SysEx sync (effect state read/write)
- **DeviceProfiles.h/cpp** — Device-specific preset templates

## Key Global Objects

From `Globals.cpp`:

```cpp
Adafruit_SSD1306 display      // OLED display controller (128x64 or 128x32)
Adafruit_ST7735 tft           // TFT color display controller (128x128)
Adafruit_NeoPixel strip       // NeoPixel LED controller  
ESP32Encoder encoder          // Rotary encoder
Preferences systemPrefs       // NVS storage
WebServer server(80)          // HTTP server
BLEClient* pClient            // BLE MIDI client
USBMIDI usbMidi               // USB MIDI (ESP32-S3 only)
```

## Build Configuration

### Required Libraries
See [../README.md](../README.md#software-dependencies) for library versions.

### Compilation Settings
- **Arduino ESP32 Core: v3.1.2** (by Espressif Systems — newer versions may break BLE/NeoPixel compatibility)
- Board: ESP32 Dev Module (or ESP32-S3 Dev Module for S3 boards)
- Upload Speed: 921600 (or lower if upload fails)
- Flash Frequency: 80MHz
- Partition Scheme: Default (or "No OTA" if space constrained)

## Customization Points

### Change Pin Assignments
Edit `Config.h` - update GPIO definitions:
```cpp
#define ENCODER_A_PIN 18        // Encoder A pin (verified)
#define ENCODER_B_PIN 19        // Encoder B pin (verified)
#define ENCODER_BUTTON_PIN 23   // Encoder button (verified)
#define OLED_SDA_PIN 21         // I2C SDA
#define OLED_SCL_PIN 22         // I2C SCL
#define NEOPIXEL_PIN 5          // NeoPixel data pin
// etc.
```

### Change Button Layout
Configure button count (4-10) via web interface System Config, or edit defaults in `Config.h`:
```cpp
// Default 8 buttons, supports 4-10 dynamically via web interface
#define MAX_BUTTONS 10              // Maximum supported
#define DEFAULT_BUTTON_COUNT 8      // Default active buttons
const uint8_t DEFAULT_BUTTON_PINS[MAX_BUTTONS] = {14, 27, 26, 25, 33, 32, 16, 17, 0, 0};
// Note: Set valid GPIO pins for buttons 9-10 via web editor if using more than 8
```

### Change LED Mapping
Edit `Globals.cpp` - update ledMap array:
```cpp
const int ledMap[NUM_LEDS] = {0, 1, 2, 3, 4, 5, 6, 7};
```

### Change Default Settings
Edit `Config.h` - update defaults:
```cpp
#define DEFAULT_BLE_NAME "ESP32 MIDI Controller"
#define DEFAULT_AP_SSID "ESP32_MIDI_Config"  
#define DEFAULT_AP_PASS "12345678"
```

## Development Tips

### Serial Debugging
Enable verbose logging in `Chocotone.ino`:
```cpp
Serial.begin(115200);
```

Monitor output for:
- BLE connection status
- Button press events
- MIDI message details
- Storage operations

### Testing Without Hardware
- Comment out hardware initialization for component testing
- Use Serial output to verify logic flow
- Test web interface without BLE/OLED

### Adding New Features

1. **New MIDI Message Type**
   - Add to `MidiCommandType` enum in `Globals.h`
   - Update encoding in `Input.cpp`
   - Add transmission in `BleMidi.cpp`
   - Update web interface in `WebInterface.cpp`

2. **New Menu Item**
   - Add to `menuItems[]` array in `UI_Display.cpp`
   - Handle in `handleMenuSelection()` in `Input.cpp`
   - Update `loop_menuMode()` for navigation

3. **New System Setting**
   - Add variable to `Globals.h/cpp`
   - Add save/load in `Storage.cpp`
   - Add web interface field in `WebInterface.cpp`

## Code Style

- Follow existing naming conventions
- Use camelCase for variables/functions
- Use UPPER_CASE for constants/defines
- Add comments for complex logic
- Keep functions focused (<100 lines ideally)

## Related Documentation

- [Main README](../README.md) - Project overview
- [Hardware Guide](../docs/HARDWARE.md) - Component specifications
- [Contributing](../CONTRIBUTING.md) - Contribution guidelines
