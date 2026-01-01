# Chocotone FAQ

Frequently Asked Questions about the Chocotone MIDI Controller.

---

## Coming Soon

> These features are planned but **not yet implemented** (as of January 2025). Stay tuned for updates!

### Can I use MIDI USB?
**Not yet.** USB MIDI In/Out is on the roadmap. Currently, the controller only uses BLE MIDI, via Client, Server or Dual mode.

### Is there battery level monitoring?
**Not yet.** Battery level display on the OLED is on the roadmap for battery-powered builds.

---

## Analog Inputs (v1.5+)

### Does the controller support analog inputs?
**Yes!** As of v1.5, Chocotone supports up to **4 analog inputs** (on GPIO 32-39). You can use:
- **Potentiometers** for expression control
- **Expression pedals** via TRS jack with 1MΩ resistor
- **FSR (Force Sensitive Resistors)** for velocity-sensitive pads
- **Piezo sensors** for drum pads with velocity detection
- **Switches** for digital input via analog pins

### How do I configure analog inputs?
In the web editor, go to **System Config** → **Analog Inputs** section. Set the count, configure pins, input modes, and MIDI messages for each input.

### How do I see live analog values?
Enable **Analog Debug** from the OLED menu. This shows raw ADC values on the display in real-time for calibration and testing.

### My analog input is jittery
Adjust the **Smoothing α** (lower = more smoothing) and **Hysteresis** (higher = less jitter) values in the analog input configuration.

---

## General

### What is Chocotone?
Chocotone is an open-source ESP32-based BLE MIDI controller. It's designed to wirelessly control devices like the Sonicake Pocket Master, Valeton GP-5, and other BLE MIDI-compatible equipment.

### Why "Chocotone"?
The name comes from a Brazilian Christmas dessert (a chocolate-filled Panettone). The project started as a "Chocolate Plus Pro Max" — an enhanced version of the M-VAVE Chocolate Plus footswitch. See the [About the Name](README.md#about-the-name-) section in the README for the full story!

### Is this compatible with my pedal/device?
Chocotone works with any BLE MIDI device that accepts standard MIDI messages. Tested devices include:
- Sonicake Pocket Master (full support including Tap Tempo)
- Valeton GP-5 (SysEx support)
- Hotone Ampero 2 Stomp
- Any DAW or app that accepts BLE MIDI (in SERVER mode)

---

## Hardware

### How many buttons can I use?
You can configure **4 to 10 buttons** via the web editor. The layout automatically adjusts on the OLED display. I plan adding more buttons in the future.

### What ESP32 board should I use?
Any ESP32 development board with BLE support works. The project was developed and tested with the ESP32 NodeMCU32s.

### Do I need pull-up resistors?
- Most GPIOs have internal pull-ups that work fine
- **GPIO 34 and 35** are input-only and don't have internal pull-ups — use external 10kΩ resistors if using these pins

### The OLED shows I2C errors. Is it broken?
Occasional I2C errors can happen due to:
- Loose wiring connections
- Long wires picking up electrical noise
- Power supply instability
- Missing or weak pull-up resistors on SDA/SCL

The firmware has auto-recovery built in. If errors are frequent, check your wiring and consider adding 4.7kΩ pull-up resistors to SDA and SCL lines.

### How do I wire the components?
See [docs/WIRING.md](docs/WIRING.md) for detailed wiring diagrams and pin assignments.

---

## Configuration

### How do I access the web editor?
1. Long-press the encoder to enter the menu
2. Navigate to "Wi-Fi LoadCfg" and enable it
3. Connect your phone/computer to WiFi network "CHOCOTONE" (password: 12345678)
4. Open `http://192.168.4.1` in your browser

### What's the difference between WiFi and BT Serial?
- **WiFi (Wi-Fi LoadCfg)**: Import/export configuration profiles via web browser. Connect to the controller's WiFi hotspot and access `http://192.168.4.1` to load or save JSON config files.
- **BT Serial (BL Serial)**: Full configuration editing via Bluetooth Serial connection — useful for real-time changes without WiFi.

### How do I change the button count?
In the web editor, go to **System Config** and change the "Buttons #" field. Click "Apply Changes" to update the layout.

### What are the different BLE modes?
- **CLIENT**: Connects to external BLE MIDI devices (like pedals)
- **SERVER**: Makes Chocotone visible to DAWs and apps
- **EDIT**: Pauses BLE scanning for configuration (web editor connections)
- **DUAL**: Both CLIENT and SERVER active simultaneously

### How do I save my configuration?
In the web editor, changes are saved when you click "Save to Device". From the OLED menu, select "Save and Exit" to persist settings.

### Can I backup my configuration?
Yes! In the web editor, use the "Export" button to download a JSON file with all your settings. Use "Import" to restore from a backup.

---

## MIDI & Actions

### What action types are available?
| Action | Description |
|--------|-------------|
| PRESS | Triggered on button press |
| 2ND_PRESS | Triggered on alternate press (toggle mode) |
| RELEASE | Triggered when button is released |
| 2ND_RELEASE | Alternate release action |
| LONG_PRESS | Triggered after holding button (configurable time) |
| 2ND_LONG_PRESS | Alternate long press |
| DOUBLE_TAP | Triggered on quick double press |
| COMBO | Triggered when two buttons are pressed together |

### What MIDI message types are supported?
- **CC (Control Change)**: Most common for effect on/off
- **Note On/Off**: For triggering notes or momentary presses
- **Program Change (PC)**: For selecting presets on external devices
- **SysEx**: For device-specific commands (up to 48 bytes)
- **TAP_TEMPO**: Special type for tap tempo with rhythm patterns

### How does Tap Tempo work?
1. Assign TAP_TEMPO type to a button
2. Press to enter tap tempo mode
3. Tap repeatedly to set the tempo
4. BPM is calculated from your tap intervals
5. Rotate encoder to fine-tune (±0.5 BPM per click)
6. Press encoder to cycle rhythm patterns (1/8, 1/8d, 1/4, 1/2)
7. Auto-exits after 3 seconds of inactivity

### What are the Tap Tempo rhythm patterns?
| Pattern | Multiplier | Use Case |
|---------|------------|----------|
| 1/8 | 0.5x | Eighth notes (default)|
| 1/8d | 0.75x | Dotted eighth (common for delay) |
| 1/4 | 1.0x | Quarter notes |
| 1/2 | 2.0x | Half notes |

### My SysEx commands aren't working!
SysEx commands can be up to **48 bytes**. Common issues:
- Ensure the hex string is valid (starts with F0, ends with F7)
- Check the BLE connection status on the OLED
- Monitor Serial output for "SYSEX sent (X bytes)" messages
- Make sure the device is in CLIENT mode and connected

### What are the Sync Device modes?
Sync modes allow the controller to synchronize button LED states with the actual effect states on your pedal.

| Mode | Description |
|------|-------------|
| **NONE** | No synchronization — LEDs only reflect local button state |
| **SPM** | Sonicake Pocket Master sync — controller reads CC responses from SPM to update LED states |
| **GP5** | Valeton GP-5 sync — reads SysEx responses to update effect states |

When connected to a synced device, pressing a button on the controller updates the LED to match the actual effect state reported by the device.

### What are the internal command types? (v1.5+)
These commands control the device itself rather than sending MIDI:

| Command | Description |
|---------|-------------|
| TAP_TEMPO | Enter tap tempo mode |
| PRESET_UP/DOWN | Navigate presets |
| PRESET_1-4 | Jump to specific preset |
| WIFI_TOGGLE | Toggle WiFi on/off |
| CLEAR_BLE_BONDS | Clear BLE bonding data |
| MENU_TOGGLE | Enter/exit menu mode |
| MENU_UP/DOWN | Navigate menu items |
| MENU_ENTER | Select menu item |

Use the **Internal Commands** optgroup in the editor's Type dropdown to access these.

---

## LED Modes

### What are the different LED modes?
- **MOMENTARY**: LED on while button is pressed, off when released
- **TOGGLE**: LED alternates between on/off with each press
- **TOGGLE_HOLD**: Same as toggle, but also lights up while held

### What are the preset LED modes?
- **NORMAL**: Each button controls its own LED state
- **SELECTION**: Only the last pressed button stays lit (bank selection)
- **HYBRID**: Combination of both behaviors

---

## Troubleshooting

### BLE won't connect to my device
1. From the menu, select "Clear BLE Bonds"
2. Reboot the controller
3. Put your target device in pairing mode
4. Make sure you're in CLIENT mode (not SERVER or EDIT)
5. Check Serial Monitor for connection logs

### Buttons are not responding correctly
1. Check button wiring
2. Adjust "Pad Debounce" in the menu (try 50-100ms)
3. Verify button configuration in web editor
4. Ensure buttons aren't assigned to GPIO 34/35 without external pull-ups

### Settings won't save
1. Wait for the "Saving..." message to complete
2. The controller may auto-reboot after saving — this is normal
3. If save fails, check Serial Monitor for NVS errors
4. Try Factory Reset if persistent issues occur

### The display shows wrong information
1. Check I2C wiring (SDA on GPIO 21, SCL on GPIO 22)
2. Verify OLED address is 0x3C (most common)
3. Check for I2C errors in Serial Monitor

### How do I Factory Reset?
1. Long-press encoder to enter menu
2. Navigate to "Factory Reset"
3. Confirm with "Yes, Reset" option
4. Controller will reboot with default settings

---

## Advanced

### Can I add custom devices to the editor?
Yes! Edit `web_tools/devices_database.js` to add new device templates with their CC mappings and default presets.

### How do I build a floor pedal version?
See [docs/HARDWARE.md](docs/HARDWARE.md) for the Bill of Materials for both desktop (tactile buttons) and floor pedal (footswitches) versions.

### Can I contribute to the project?
Absolutely! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. Pull requests and issues are welcome on GitHub.

---

## Need More Help?

- **GitHub Issues**: Report bugs or request features
- **GitHub Discussions**: Ask questions and share ideas
- **Serial Monitor**: Enable verbose logging for debugging

Happy MIDI controlling! 🎵🎛️
