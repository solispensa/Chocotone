# Changelog

All notable changes to the Chocotone MIDI Controller project will be documented in this file.

## [v1.4] - 2024-12-21

### Added
- **Labels for All Action Types** - Every action can now have a custom OLED label, not just COMBO
  - When label is blank, button name is displayed (for CC, PC, NOTE, SYSEX types)
  - Special types still have auto-labels: TAP_TEMPO→"TAP", PRESET_UP→">", PRESET_DOWN→"<", etc.
  - Label field moved outside COMBO-only section in both editors
- **LED Tap Tempo Brightness** - Separate brightness control for tap tempo LED blink (default: 240)

### Changed
- **TAP_TEMPO UI Improvements**:
  - Renamed fields: R.Prev→"Rhythm <", R.Next→"Rhythm >", Lock→"Lock Btn"
  - Changed button numbering from 0-9 to 1-10 (1-indexed display)
  - Hidden D1/D2 fields when TAP_TEMPO type is selected (not applicable)
- **System Config Button Color** - Fixed active state to use palette colors instead of hardcoded orange

### Fixed
- **USB Reconnection** - Can now reconnect USB serial without page refresh
  - Properly releases reader/writer locks before closing port
  - Added error handling to prevent stuck connections

---

## [v1.3] - 2024-12-19

### Added
- **SPM Effect State Sync** - Real-time synchronization of effect ON/OFF states with Sonicake Pocket Master
  - Per-preset "Sync SPM" option to enable/disable sync behavior
  - Automatic LED state updates when SPM preset changes
  - Works with SPM's 32-byte state update SysEx messages
- **New Action Types** - Extended action system with alternate variants:
  - `2ND_RELEASE` - Triggered on release after using 2ND_PRESS (fallback to RELEASE if not defined)
  - `2ND_LONG_PRESS` - Triggered on long-press after using 2ND_PRESS (fallback to LONG_PRESS if not defined)
- **SysEx Picker** - Dropdown menu with pre-configured SPM SysEx commands:
  - Reverb Type (10 types: Air, Room, Hall, Church, Plate L, Plate, Spring, N-Star, Deepsea, Sweet Space)
  - Reverb ON/OFF toggle
  - Delay Time (100-500ms common values)
  - Delay Mix (0-100% in 10% increments)
  - Delay Feedback (0-100% in 10% increments)
  - Reverb Decay (10-100% common values)
- **Increased SysEx Buffer** - Extended from 16 to 48 bytes to support longer SPM commands
- **USB Serial Configuration** - Full read/write support for configuration via USB
  - `GET_CONFIG` command returns full JSON configuration
  - `SET_CONFIG_START/CHUNK/END` protocol for uploading configurations
  - Offline Editor v2 now supports direct USB connection

### Fixed
- **SysEx Save/Load** - SysEx commands now correctly persist through USB serial round-trips
- **SPM Sync Race Conditions** - Disabled faulty 32-byte SysEx parser that caused state corruption
- **LED State Conflicts** - LED toggle state now updates after action execution for SPM-synced presets

### Changed
- Offline Editor v2 now includes USB Serial connection panel
- SysEx field now has both picker dropdown and manual hex input

---

## [v1.2] - 2024-12-16

### Fixed
- **Dynamic Button Count** - LED updates and OLED display now properly support 4-10 buttons
  - `updateLeds()` now uses `systemConfig.buttonCount` instead of hardcoded 8
  - OLED layout adapts: 4-8 buttons = 4 per row (4-char labels), 9-10 buttons = 5 per row (3-char labels)
  - All buttons 9-10 now properly light up and display on OLED when configured

### Changed
- OLED display dynamically adjusts column width and label length based on button count

---

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
