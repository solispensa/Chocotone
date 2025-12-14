# Changelog

All notable changes to the ESP32 MIDI Controller project will be documented in this file.

## [v1.1] - 2024-12-14

### Added
- **BLE Dual-Mode Support** - ESP32 can now act as both BLE Client (for SPM) AND BLE Server (for DAW/Apps) simultaneously
  - Three modes available: CLIENT (SPM only), DUAL (both), SERVER (DAW/Apps only)
  - Runtime mode selection via web interface or OLED menu
  - MIDI messages routed to both connected devices in Dual Mode
- **Hold Commands** - Configure long-press actions for any button (separate MIDI message after hold threshold)
- **Combo Commands** - Two-button simultaneous press actions with custom labels
- **Tap Tempo Addressable Buttons** - Assign any button as rhythm pattern navigator or tap mode lock
  - R.prev: Navigate to previous rhythm pattern
  - R.next: Navigate to next rhythm pattern  
  - Lock: Lock/unlock tap tempo mode (instant exit when unlocked)
- **Web Editor View Separation** - Organized into Main Messages / Special Actions / System Config pages
- **BLE Mode in OLED Menu** - Toggle between CLIENT/DUAL/SERVER modes directly from the controller
  - Changes apply on "Save and Exit" with automatic reboot
- **Web Editor Stability Improvements**
  - Request throttling to prevent crashes from double-clicks or rapid requests
  - Low memory protection with graceful error pages
  - Fixed memory leak in error page generation
- **Editable Pin Configuration** - Configure button GPIO pins directly in web editor System Config
- **Tap Tempo LED Blink** - All LEDs blink on tap with custom color feedback
- **Button Name Font Size** - Adjustable font size for OLED button name display (1-5)
- **Internal Preset Commands** - Switch to specific preset via PRESET_1/2/3/4 button actions
- **Offline Editor** - Standalone HTML file for editing configurations without device connection
- **Gradient Header** - Web editor now features a stylish gradient "Chocotone" title matching the presentation page
- **OLED Refresh on Preset Change** - Display updates when changing presets via web editor

### Fixed
- **BLE + WiFi Crash** - Properly stops BLE Server advertising when turning WiFi on (prevents radio conflicts)
- **Special Actions Save Bug** - Saving from Special Actions view no longer erases RGB values or main message settings
- **Alternate Mode Reset** - isAlternate checkbox preserved when saving from Special Actions view
- **Web Editor Crashes** - Added request throttling and heap checks to prevent crashes
- **Duplicate Variable Definition** - Resolved linker error for `serverConnected` variable

### Changed
- Web editor subtitle updated to "MIDI Editor - by André Solis - v1.1"
- Lowered OLED update heap threshold from 50KB to 25KB for better WiFi compatibility
- BLE Mode persists across reboots via NVS storage
- Increased WiFi startup delay for BLE resource release

## [v1.0] - 2024-12-13 (Initial Release)

### Added
- Web-based configuration interface for editing MIDI mappings
- WiFi access point mode for wireless configuration
- NVS (non-volatile storage) persistence for all settings
- Export/import functionality for preset configurations (including system settings)
- Tap tempo mode with rhythm pattern selection (1/8, 1/8d, 1/4, 1/2)
- Encoder-based BPM adjustment in tap tempo mode
- BLE client-only mode for SPM (Sonicake Pocket Master) connectivity
- Automatic BLE scanning and reconnection
- Button name display on OLED when pressed
- Configurable LED brightness (on/dim states)
- Configurable button debounce timing
- Font size customization for button names
- Factory reset functionality
- BLE bond clearing option
- WiFi on at boot configuration option
- Offline editor HTML file for configuration without device connection

### Fixed
- NVS persistence issues for system settings and presets
- BLE connection stability improvements
- Device freezing during problematic BLE connections
- Tap tempo BPM calculation and message sending
- WiFi on at Boot menu option now selectable (was hidden due to incorrect menu count)

### Changed
- BLE architecture: client-only mode (no server mode)
- Improved OLED display layout with connection status
- Enhanced menu system with scrolling support
- Web interface completely redesigned with modern dark theme
- Web interface now mobile-responsive with 2-column grid on small screens
- Menu header changed from "Menu SPM" to "Menu CHOCOTONE"
- Wi-Fi Config menu option renamed to "Wi-Fi Editor"
- Startup screen updated with "Chocotone MIDI v1.0" branding
- Web interface now supports JSON export/import of system settings (BLE, WiFi)
- Wi-Fi Editor menu option now displays current status (ON/OFF) in label
- WiFi On at Boot now toggles directly without submenu and displays current status
- WiFi On at Boot setting now properly auto-starts WiFi when device boots

## [v8] - chocotone_v8

### Added
- Enhanced web interface
- Multiple preset support (4 presets)
- Improved OLED menu system

## [v7] - chocotone_v7

### Added
- Initial web interface implementation
- Basic BLE MIDI functionality
- OLED display support
- NeoPixel LED feedback
- Rotary encoder navigation
