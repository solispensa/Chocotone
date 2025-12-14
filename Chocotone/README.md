# Chocotone Source Code

This directory contains the Arduino source code for the ESP32 MIDI Controller.

## File Structure

| File | Purpose |
|------|---------|
| `Chocotone.ino` | Main Arduino sketch with setup() and loop() |
| `Config.h` | Pin definitions and compile-time constants |
| `Globals.h/cpp` | Global variables, objects, and data structures |
| `BleMidi.h/cpp` | BLE MIDI client implementation and SysEx handling |
| `Input.h/cpp` | Button and rotary encoder input handling |
| `Storage.h/cpp` | NVS (non-volatile storage) persistence |
| `UI_Display.h/cpp` | OLED display rendering functions |
| `WebInterface.h/cpp` | Web server and configuration interface |
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

### BLE MIDI (BleMidi.h/cpp)
- BLE client initialization
- Device scanning and connection
- MIDI message transmission
- SysEx data handling
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
User Input → Input.cpp → MIDI Message → BleMidi.cpp → BLE Device
                ↓                           ↓
         UI_Display.cpp              Storage.cpp (save state)
                ↓
         OLED Display + LEDs
```

## Key Global Objects

From `Globals.cpp`:

```cpp
Adafruit_SSD1306 display      // OLED display controller
Adafruit_NeoPixel strip       // NeoPixel LED controller  
ESP32Encoder encoder          // Rotary encoder
Preferences systemPrefs       // NVS storage
WebServer server(80)          // HTTP server
BLEClient* pClient            // BLE MIDI client
```

## Build Configuration

### Required Libraries
See [../README.md](../README.md#software-dependencies) for library versions.

### Compilation Settings
- Board: ESP32 Dev Module (or your specific ESP32 board)
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
Edit `Config.h` - verified button pins (8 buttons):
```cpp
// Buttons 1-8: verified physical layout
const uint8_t DEFAULT_BUTTON_PINS[MAX_BUTTONS] = {14, 27, 26, 25, 33, 32, 16, 17, 0, 0};
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
