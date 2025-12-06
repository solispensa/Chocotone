# Changelog

All notable changes to the ESP32 MIDI Controller project will be documented in this file.

## [Unreleased] - Current Version

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
