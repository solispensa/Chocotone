# Chocotone MIDI Controller - Release Notes

## Version 1.1 (December 2024)

**Status:** Stable Release  
**Type:** Maintenance & Stability Improvements

### Overview
This release focuses on stability improvements, OLED display refinements, and BLE connection reliability. The firmware is production-ready with solid WiFi/BLE coexistence and robust web interface.

---

## ✨ New Features

### BLE Improvements
- **Automatic Bond Clearing**: Clears old BLE bonds on boot when max connections reached
- **WiFi/BLE Coexistence**: BLE pauses when WiFi active, prevents conflicts
- **Improved Scanning**: Better connection handling with `doScan` flag

### Web Interface Enhancements
- **Deferred Display Updates**: Moved display/LED updates to main loop from web handlers
- **Stability**: Added `yield()` calls in long loops to prevent watchdog timeouts
- **Better Responsiveness**: Smoother web interface during configuration

### OLED Display
- **Menu Refinements**: Renamed "Wi-Fi Config" to "Wi-Fi Editor"
- **Branding**: Changed header to "Menu CHOCOTONE"
- **Loading Screen**: Updated to "Chocotone MIDI v1.0 Loading...."
- **Fixed Boot Menu**: "WiFi on at Boot" now properly selectable

### Configuration
- **CHOCOTONE Branding**: Default BLE name updated from "ESP32 MIDI Controller"
- **WiFi SSID**: Default changed to reflect project name

---

## 🐛 Bug Fixes

### Critical
- Fixed device freezing when BLE client disconnects during WiFi activity
- Resolved display corruption from concurrent web handler updates
- Fixed unselectable menu items in OLED interface

### Stability
- Prevented WiFi/BLE interference through proper mutual exclusion
- Added safety checks in web handlers to prevent crashes
- Improved NVS storage reliability

---

## 📋 Features (Existing)

### MIDI Capabilities
- 8 configurable buttons
- 4 presets (Note, STOMP, Banks 1-8, Banks 9-16)
- Message types: CC, Note (Momentary/Toggle), PC, Tap Tempo
- Dual messages per button (primary/alternate)
- RGB LED feedback per button

### Connectivity
- **BLE MIDI**: Wireless MIDI over Bluetooth
- **WiFi**: Web-based configuration interface
- **Auto-connect**: Remembers last BLE device

### Display
- 128x64 OLED (SH1106)
- Preset visualization
- Menu system
- Button status indicators

### Configuration
- Web-based editor (WiFi AP mode)
- JSON import/export
- Preset management
- System settings

---

## 🔧 Technical Details

### Hardware Support
- **MCU**: ESP32 (tested on NodeMCU32s)
- **Buttons**: 8 buttons (GPIO: 32,33,25,26,27,14,12,13)
- **LEDs**: WS2812B RGB strip (GPIO 5)
- **Display**: SH1106 OLED (I2C)
- **Power**: USB or battery (rechargeable)

### Memory Usage
- Flash: ~1.2MB
- RAM: ~40KB (idle)
- NVS: ~8KB (config storage)

### Dependencies
- ESP32_BLE_Arduino
- Adafruit_GFX
- Adafruit_SH110X
- Adafruit_NeoPixel

---

## 📦 Files Included

### Firmware
- `Chocotone.ino` - Main sketch
- `Globals.h/cpp` - Global variables, extern declarations
- `Config.h` - Data structures
- `BleMidi.cpp/h` - BLE MIDI client
- `Input.cpp/h` - Button handling
- `Display.cpp/h` - OLED display
- `WebInterface.cpp/h` - Web server
- `Storage.cpp/h` - NVS persistence

### Web Tools
- `web_tools/web_interface.html` - Embedded web editor
- `web_tools/offline_editor.html` - Standalone configuration tool (v2 with advanced features)

### Documentation
- `README.md` - Project overview
- `docs/HARDWARE.md` - Hardware guide
- `CHANGELOG.md` - Version history

---

## 🚀 Upgrade Notes

### From v1.0
1. Upload new firmware via Arduino IDE
2. Existing configurations preserved in NVS
3. BLE bonds may be auto-cleared on first boot (expected)
4. No breaking changes to configuration format

### First-Time Installation
1. Upload firmware
2. Connect to "CHOCOTONE" WiFi network (password: 12345678)
3. Open browser to 192.168.4.1
4. Configure presets and buttons
5. Export configuration for backup

---

## 🔮 What's Next (v2.0 Roadmap)

The following features are in development in the offline editor prototype:

- **Advanced MIDI**: SysEx support, 7 new internal commands
- **Press & Hold**: Long-press actions with configurable threshold
- **Button Combos**: Dual-button combinations
- **Global Controls**: Preset-independent shortcuts
- **Dynamic Hardware**: 4-10 button support, configurable pins
- **Enhanced Validation**: Pin safety warnings

These will be integrated into firmware in v2.0.

---

## 📝 Known Issues

- None reported in this release

---

## 🙏 Credits

**Developer**: André Solis  
**Project**: Chocotone MIDI Controller  
**License**: MIT

---

**Download**: [GitHub Releases](https://github.com/YourRepo/Chocotone/releases/tag/v1.1)  
**Documentation**: [GitHub Wiki](https://github.com/YourRepo/Chocotone/wiki)  
**Support**: [GitHub Issues](https://github.com/YourRepo/Chocotone/issues)
