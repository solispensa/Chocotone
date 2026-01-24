/**
 * OpenMIDI Device Database for Chocotone
 * Hybrid loader: bundled data + optional online updates from OpenMIDI
 * 
 * Data format follows OpenMIDI YAML structure converted to JS
 */

const DEVICE_DATABASE = {
    // ===================================================================
    // SONICAKE POCKET MASTER (SPM)
    // ===================================================================
    "Sonicake Pocket Master": {
        brand: "Sonicake",
        model: "Pocket Master",
        midi_in: "USB",
        midi_channel_default: 1,
        categories: {
            "Effect Modules": [
                { name: "NR On/Off", value: 43, description: "Noise Reducer toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "FX1 On/Off", value: 44, description: "FX1 module toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "DRV On/Off", value: 45, description: "Drive module toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "AMP On/Off", value: 46, description: "Amp simulation toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "IR On/Off", value: 47, description: "IR/Cabinet sim toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "EQ On/Off", value: 48, description: "EQ module toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "FX2 On/Off", value: 49, description: "FX2 module toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "DLY On/Off", value: 50, description: "Delay module toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "RVB On/Off", value: 51, description: "Reverb module toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Navigation": [
                { name: "Preset Select", value: 1, description: "1-50: User P01-P50, 51-100: Factory F01-F50", type: "Parameter", min: 1, max: 100 },
                { name: "Bank Down", value: 22, description: "Decrease preset bank", type: "System", min: 0, max: 127 },
                { name: "Bank Up", value: 23, description: "Increase preset bank", type: "System", min: 0, max: 127 },
                { name: "Preset Down", value: 24, description: "Previous preset", type: "System", min: 0, max: 127 },
                { name: "Preset Up", value: 25, description: "Next preset", type: "System", min: 0, max: 127 }
            ],
            "Volume Controls": [
                { name: "Master Volume", value: 6, description: "Global master volume", type: "Parameter", min: 0, max: 100 },
                { name: "Preset Volume", value: 7, description: "Current preset volume", type: "Parameter", min: 0, max: 100 }
            ],
            "Utilities": [
                { name: "Tuner On/Off", value: 58, description: "Toggle tuner mode", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Looper": [
                { name: "Looper On/Off", value: 59, description: "Toggle looper mode", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Looper Record", value: 60, description: "Start/stop recording", type: "System", min: 0, max: 127 },
                { name: "Looper Play/Stop", value: 62, description: "Play or stop loop", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Looper Delete", value: 64, description: "Delete current loop", type: "System", min: 0, max: 127 },
                { name: "Looper Rec Volume", value: 65, description: "Recording input volume", type: "Parameter", min: 0, max: 100 },
                { name: "Looper Play Volume", value: 66, description: "Playback volume", type: "Parameter", min: 0, max: 100 },
                { name: "Looper Placement", value: 67, description: "0-63: Post, 64-127: Pre", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Drum Machine": [
                { name: "Drum Menu On/Off", value: 92, description: "Toggle drum menu", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Drum Play/Stop", value: 93, description: "Play or stop drums", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Drum Rhythm", value: 94, description: "Select rhythm (0-9)", type: "Parameter", min: 0, max: 9 },
                { name: "Drum Volume", value: 95, description: "Drum volume level", type: "Parameter", min: 0, max: 100 }
            ]
        },
        // Flattened CC list for quick lookup
        get cc() {
            const all = [];
            for (const cat of Object.values(this.categories)) {
                all.push(...cat);
            }
            return all;
        },
        // Default template preset for STOMP mode
        templates: {
            "STOMP (Default)": [
                { name: "NR", cc: 43, color: "#ffffff" },
                { name: "FX1", cc: 44, color: "#3f67ff" },
                { name: "DRV", cc: 45, color: "#fc2c00" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" },
                { name: "EQ", cc: 48, color: "#0af500" },
                { name: "FX2", cc: 49, color: "#11f3ff" },
                { name: "DLY", cc: 50, color: "#332aff" },
                { name: "RVB", cc: 51, color: "#8400f7" }
            ],
            "Full Signal Chain": [
                { name: "NR", cc: 43, color: "#888888" },
                { name: "FX1", cc: 44, color: "#3f67ff" },
                { name: "DRV", cc: 45, color: "#fc2c00" },
                { name: "AMP", cc: 46, color: "#ff8800" },
                { name: "IR", cc: 47, color: "#ffcc00" },
                { name: "EQ", cc: 48, color: "#0af500" },
                { name: "FX2", cc: 49, color: "#11f3ff" },
                { name: "DLY", cc: 50, color: "#332aff" },
                { name: "RVB", cc: 51, color: "#8400f7" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" }
            ],
            "Bank Selector": [
                { name: "B1", cc: 1, d2: 1, color: "#ffffff" },
                { name: "B2", cc: 1, d2: 2, color: "#ffffff" },
                { name: "B3", cc: 1, d2: 3, color: "#ffffff" },
                { name: "B4", cc: 1, d2: 4, color: "#ffffff" },
                { name: "B5", cc: 1, d2: 5, color: "#0af500" },
                { name: "B6", cc: 1, d2: 6, color: "#0af500" },
                { name: "B7", cc: 1, d2: 7, color: "#0af500" },
                { name: "B8", cc: 1, d2: 8, color: "#0af500" }
            ],
            "Wisut (10-btn)": [
                { name: "P1", cc: 1, d2: 1, color: "#ffffff" },
                { name: "P2", cc: 1, d2: 2, color: "#ffffff" },
                { name: "P3", cc: 1, d2: 3, color: "#ffffff" },
                { name: "P4", cc: 1, d2: 4, color: "#ffffff" },
                { name: "P5", cc: 1, d2: 5, color: "#0af500" },
                { name: "FX1", cc: 44, color: "#3f67ff" },
                { name: "DRV", cc: 45, color: "#fc2c00" },
                { name: "FX2", cc: 49, color: "#11f3ff" },
                { name: "DLY", cc: 50, color: "#332aff" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" }
            ]
        }
    },

    // ===================================================================
    // VALETON GP-5 (Corrected CC mappings from TonexOneController)
    // ===================================================================
    "Valeton GP-5": {
        brand: "Valeton",
        model: "GP-5",
        midi_in: "USB-C / Bluetooth",
        midi_channel_default: 1,
        categories: {
            "Effect Modules (Enable/Disable)": [
                { name: "NR On/Off", value: 0, description: "Noise Reduction / Gate toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "PRE On/Off", value: 1, description: "Pre-effect (Comp/Boost/Wah/Pitch) toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "DST On/Off", value: 2, description: "Distortion toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "AMP On/Off", value: 3, description: "Amp model toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "CAB On/Off", value: 4, description: "Cabinet/IR toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "EQ On/Off", value: 5, description: "EQ toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "MOD On/Off", value: 6, description: "Modulation toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "DLY On/Off", value: 7, description: "Delay toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "RVB On/Off", value: 8, description: "Reverb toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Snaptone On/Off", value: 9, description: "NAM/Snaptone toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Effect Type Selection": [
                { name: "NR Type", value: 10, description: "Noise Reduction type (0=Gate, 1=NR, 2=NRG)", type: "Parameter", min: 0, max: 2 },
                { name: "PRE Type", value: 11, description: "Pre-effect type (0-23)", type: "Parameter", min: 0, max: 23 },
                { name: "DST Type", value: 12, description: "Distortion type (0-23)", type: "Parameter", min: 0, max: 23 },
                { name: "AMP Type", value: 13, description: "Amp model (0-31)", type: "Parameter", min: 0, max: 31 },
                { name: "CAB Type", value: 14, description: "Cabinet/IR (0-19)", type: "Parameter", min: 0, max: 19 },
                { name: "EQ Type", value: 15, description: "EQ type (0-4)", type: "Parameter", min: 0, max: 4 },
                { name: "MOD Type", value: 16, description: "Modulation type (0-14)", type: "Parameter", min: 0, max: 14 },
                { name: "DLY Type", value: 17, description: "Delay type (0-9)", type: "Parameter", min: 0, max: 9 },
                { name: "RVB Type", value: 18, description: "Reverb type (0-9)", type: "Parameter", min: 0, max: 9 },
                { name: "Snaptone Type", value: 19, description: "NAM model (0-79)", type: "Parameter", min: 0, max: 79 }
            ],
            "Navigation": [
                { name: "Preset Select", value: 127, description: "Direct patch select 0-99", type: "Parameter", min: 0, max: 99 },
                { name: "Preset Down", value: 116, description: "Previous patch", type: "Trigger", min: 0, max: 127 },
                { name: "Preset Up", value: 117, description: "Next patch", type: "Trigger", min: 0, max: 127 },
                { name: "BPM", value: 118, description: "Set tempo BPM (0=40 to 127=300)", type: "Parameter", min: 0, max: 127 }
            ],
            "Volume Controls": [
                { name: "Patch Volume", value: 20, description: "Current patch volume", type: "Parameter", min: 0, max: 127 },
                { name: "Input Level", value: 120, description: "Input gain", type: "Parameter", min: 0, max: 127 },
                { name: "CAB Bypass", value: 121, description: "Global CAB bypass", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Master Volume", value: 122, description: "Global master volume", type: "Parameter", min: 0, max: 127 },
                { name: "Record Level", value: 123, description: "USB record level", type: "Parameter", min: 0, max: 127 },
                { name: "Monitor Level", value: 124, description: "Monitor output level", type: "Parameter", min: 0, max: 127 },
                { name: "Bluetooth Level", value: 125, description: "Bluetooth audio level", type: "Parameter", min: 0, max: 127 }
            ],
            "NR Parameters": [
                { name: "NR Param 0", value: 21, description: "NR parameter 0", type: "Parameter", min: 0, max: 127 },
                { name: "NR Param 1", value: 23, description: "NR parameter 1", type: "Parameter", min: 0, max: 127 },
                { name: "NR Param 2", value: 24, description: "NR parameter 2", type: "Parameter", min: 0, max: 127 }
            ],
            "PRE Parameters": [
                { name: "PRE Param 0", value: 30, description: "Pre-effect parameter 0", type: "Parameter", min: 0, max: 127 },
                { name: "PRE Param 1", value: 31, description: "Pre-effect parameter 1", type: "Parameter", min: 0, max: 127 },
                { name: "PRE Param 2", value: 32, description: "Pre-effect parameter 2", type: "Parameter", min: 0, max: 127 }
            ],
            "DST Parameters": [
                { name: "DST Param 0 (Gain)", value: 38, description: "Distortion gain", type: "Parameter", min: 0, max: 127 },
                { name: "DST Param 1 (Bass)", value: 39, description: "Distortion bass", type: "Parameter", min: 0, max: 127 },
                { name: "DST Param 2 (Mid)", value: 40, description: "Distortion mid", type: "Parameter", min: 0, max: 127 },
                { name: "DST Param 3 (Treble)", value: 41, description: "Distortion treble", type: "Parameter", min: 0, max: 127 },
                { name: "DST Param 4 (Level)", value: 42, description: "Distortion level", type: "Parameter", min: 0, max: 127 }
            ],
            "AMP Parameters": [
                { name: "AMP Param 0 (Gain)", value: 46, description: "Amp gain", type: "Parameter", min: 0, max: 127 },
                { name: "AMP Param 1 (Bass)", value: 47, description: "Amp bass", type: "Parameter", min: 0, max: 127 },
                { name: "AMP Param 2 (Mid)", value: 48, description: "Amp mid", type: "Parameter", min: 0, max: 127 },
                { name: "AMP Param 3 (Treble)", value: 49, description: "Amp treble", type: "Parameter", min: 0, max: 127 },
                { name: "AMP Param 4 (Presence)", value: 50, description: "Amp presence", type: "Parameter", min: 0, max: 127 },
                { name: "AMP Param 5 (Master)", value: 51, description: "Amp master volume", type: "Parameter", min: 0, max: 127 }
            ],
            "MOD Parameters": [
                { name: "MOD Param 0 (Rate)", value: 70, description: "Modulation rate", type: "Parameter", min: 0, max: 127 },
                { name: "MOD Param 1 (Depth)", value: 71, description: "Modulation depth", type: "Parameter", min: 0, max: 127 },
                { name: "MOD Param 2", value: 72, description: "Modulation parameter 2", type: "Parameter", min: 0, max: 127 },
                { name: "MOD Param 3 (Mix)", value: 73, description: "Modulation mix", type: "Parameter", min: 0, max: 127 }
            ],
            "DLY Parameters": [
                { name: "DLY Param 0 (Time)", value: 78, description: "Delay time", type: "Parameter", min: 0, max: 127 },
                { name: "DLY Param 1 (Feedback)", value: 79, description: "Delay feedback", type: "Parameter", min: 0, max: 127 },
                { name: "DLY Param 2 (Mix)", value: 80, description: "Delay mix", type: "Parameter", min: 0, max: 127 }
            ],
            "RVB Parameters": [
                { name: "RVB Param 0 (Decay)", value: 86, description: "Reverb decay", type: "Parameter", min: 0, max: 127 },
                { name: "RVB Param 1 (Damping)", value: 87, description: "Reverb damping", type: "Parameter", min: 0, max: 127 },
                { name: "RVB Param 2 (Mix)", value: 88, description: "Reverb mix", type: "Parameter", min: 0, max: 127 }
            ],
            "Utilities": [
                { name: "Tuner On/Off", value: 69, description: "Toggle tuner mode", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ]
        },
        get cc() {
            const all = [];
            for (const cat of Object.values(this.categories)) {
                all.push(...cat);
            }
            return all;
        },
        templates: {
            "STOMP Mode (CC)": [
                { name: "NR", cc: 0, color: "#888888" },
                { name: "PRE", cc: 1, color: "#3f67ff" },
                { name: "DST", cc: 2, color: "#fc2c00" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" },
                { name: "EQ", cc: 5, color: "#0af500" },
                { name: "MOD", cc: 6, color: "#ff00ff" },
                { name: "DLY", cc: 7, color: "#332aff" },
                { name: "RVB", cc: 8, color: "#8400f7" }
            ],
            "Full Chain (CC)": [
                { name: "NR", cc: 0, color: "#888888" },
                { name: "PRE", cc: 1, color: "#3f67ff" },
                { name: "DST", cc: 2, color: "#fc2c00" },
                { name: "AMP", cc: 3, color: "#ff8800" },
                { name: "CAB", cc: 4, color: "#ffcc00" },
                { name: "EQ", cc: 5, color: "#0af500" },
                { name: "MOD", cc: 6, color: "#ff00ff" },
                { name: "DLY", cc: 7, color: "#332aff" },
                { name: "RVB", cc: 8, color: "#8400f7" },
                { name: "NAM", cc: 9, color: "#11f3ff" }
            ],
            "Bank Selector": [
                { name: "P1", cc: 127, d2: 0, color: "#ffffff" },
                { name: "P2", cc: 127, d2: 1, color: "#ffffff" },
                { name: "P3", cc: 127, d2: 2, color: "#ffffff" },
                { name: "P4", cc: 127, d2: 3, color: "#ffffff" },
                { name: "P5", cc: 127, d2: 4, color: "#0af500" },
                { name: "P6", cc: 127, d2: 5, color: "#0af500" },
                { name: "P7", cc: 127, d2: 6, color: "#0af500" },
                { name: "P8", cc: 127, d2: 7, color: "#0af500" }
            ],
            // GP-5 SysEx toggle template - uses raw SysEx for effect control (legacy)
            "GP5 SysEx (Legacy)": [
                { name: "PRE", sysexOn: "f0000f00010000000a0101040900010000000000000001000000000000f7", sysexOff: "f0010900010000000a0101040900010000000000000000000000000000f7", color: "#888888" },
                { name: "DST", sysexOn: "f0030a00010000000a0101040900020000000000000001000000000000f7", sysexOff: "f0020c00010000000a0101040900020000000000000000000000000000f7", color: "#fc2c00" },
                { name: "AMP", sysexOn: "f0020900010000000a0101040900030000000000000001000000000000f7", sysexOff: "f0030f00010000000a0101040900030000000000000000000000000000f7", color: "#ff8800" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" },
                { name: "NS", sysexOn: "f0090700010000000a0101040900090000000000000001000000000000f7", sysexOff: "f0080100010000000a0101040900090000000000000000000000000000f7", color: "#666666" },
                { name: "EQ", sysexOn: "f0040300010000000a0101040900050000000000000001000000000000f7", sysexOff: "f0050500010000000a0101040900050000000000000000000000000000f7", color: "#0af500" },
                { name: "DLY", sysexOn: "f0060500010000000a0101040900070000000000000001000000000000f7", sysexOff: "f0070300010000000a0101040900070000000000000000000000000000f7", color: "#332aff" },
                { name: "RVB", sysexOn: "f0080400010000000a0101040900080000000000000001000000000000f7", sysexOff: "f0090200010000000a0101040900080000000000000000000000000000f7", color: "#8400f7" }
            ]
        }
    },

    // ===================================================================
    // HOTONE AMPERO 2 STOMP
    // ===================================================================
    "Hotone Ampero 2 Stomp": {
        brand: "Hotone",
        model: "Ampero 2 Stomp",
        midi_in: "USB / MIDI TRS",
        midi_channel_default: 1,
        categories: {
            "Effect Slots": [
                { name: "Slot A1 On/Off", value: 48, description: "Effect slot A1 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot A2 On/Off", value: 49, description: "Effect slot A2 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot A3 On/Off", value: 50, description: "Effect slot A3 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot A4 On/Off", value: 51, description: "Effect slot A4 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot A5 On/Off", value: 52, description: "Effect slot A5 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot A6 On/Off", value: 53, description: "Effect slot A6 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot B1 On/Off", value: 54, description: "Effect slot B1 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot B2 On/Off", value: 55, description: "Effect slot B2 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot B3 On/Off", value: 56, description: "Effect slot B3 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot B4 On/Off", value: 57, description: "Effect slot B4 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot B5 On/Off", value: 58, description: "Effect slot B5 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Slot B6 On/Off", value: 59, description: "Effect slot B6 toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Navigation": [
                { name: "Bank Select MSB", value: 0, description: "Bank 0-2 for patch ranges", type: "Parameter", min: 0, max: 2 },
                { name: "Bank Down", value: 22, description: "Previous bank", type: "System", min: 0, max: 127 },
                { name: "Bank Up", value: 23, description: "Next bank", type: "System", min: 0, max: 127 },
                { name: "Pre-Select Menu", value: 24, description: "Open pre-select menu", type: "System", min: 0, max: 127 },
                { name: "Scene Select", value: 25, description: "Select scene 1-5", type: "Parameter", min: 1, max: 5 },
                { name: "Patch Down", value: 26, description: "Previous patch", type: "System", min: 0, max: 127 },
                { name: "Patch Up", value: 27, description: "Next patch", type: "System", min: 0, max: 127 }
            ],
            "Volume Controls": [
                { name: "Patch Volume", value: 7, description: "Patch volume 0-100", type: "Parameter", min: 0, max: 100 }
            ],
            "Expression": [
                { name: "Expression 1/2", value: 11, description: "Expression pedal EXP1/2", type: "Parameter", min: 0, max: 127 },
                { name: "EXP 1/2 Switch", value: 13, description: "Switch between EXP1 and EXP2", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Quick Access": [
                { name: "Quick Access 1", value: 16, description: "Quick access param 1", type: "Parameter", min: 0, max: 127 },
                { name: "Quick Access 1 Step", value: 17, description: "Step through QA1", type: "System", min: 0, max: 127 },
                { name: "Quick Access 2", value: 18, description: "Quick access param 2", type: "Parameter", min: 0, max: 127 },
                { name: "Quick Access 2 Step", value: 19, description: "Step through QA2", type: "System", min: 0, max: 127 },
                { name: "Quick Access 3", value: 20, description: "Quick access param 3", type: "Parameter", min: 0, max: 127 },
                { name: "Quick Access 3 Step", value: 21, description: "Step through QA3", type: "System", min: 0, max: 127 }
            ],
            "Mode": [
                { name: "Unit Mode", value: 28, description: "Patch/Stomp mode switch", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Display Mode", value: 29, description: "Main display mode", type: "System", min: 0, max: 127 }
            ],
            "Drum Machine": [
                { name: "Drum On/Off", value: 36, description: "Drum machine toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Drum Play/Stop", value: 37, description: "Play or stop drums", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Drum Rhythm", value: 38, description: "Select rhythm type", type: "Parameter", min: 0, max: 127 },
                { name: "Drum Volume", value: 39, description: "Drum volume", type: "Parameter", min: 0, max: 100 }
            ]
        },
        get cc() {
            const all = [];
            for (const cat of Object.values(this.categories)) {
                all.push(...cat);
            }
            return all;
        },
        templates: {
            "6-Slot STOMP": [
                { name: "A1", cc: 48, color: "#ff6b6b" },
                { name: "A2", cc: 49, color: "#feca57" },
                { name: "A3", cc: 50, color: "#48dbfb" },
                { name: "A4", cc: 51, color: "#1dd1a1" },
                { name: "A5", cc: 52, color: "#5f27cd" },
                { name: "A6", cc: 53, color: "#ff9ff3" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" },
                { name: "DRUM", cc: 37, color: "#54a0ff" }
            ],
            "Scene Selector": [
                { name: "SC1", cc: 25, d2: 1, color: "#ff6b6b" },
                { name: "SC2", cc: 25, d2: 2, color: "#feca57" },
                { name: "SC3", cc: 25, d2: 3, color: "#48dbfb" },
                { name: "SC4", cc: 25, d2: 4, color: "#1dd1a1" },
                { name: "SC5", cc: 25, d2: 5, color: "#5f27cd" },
                { name: "BK-", cc: 22, color: "#888888" },
                { name: "BK+", cc: 23, color: "#888888" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" }
            ],
            "Full 12 Slots": [
                { name: "A1", cc: 48, color: "#ff6b6b" },
                { name: "A2", cc: 49, color: "#feca57" },
                { name: "A3", cc: 50, color: "#48dbfb" },
                { name: "A4", cc: 51, color: "#1dd1a1" },
                { name: "A5", cc: 52, color: "#5f27cd" },
                { name: "A6", cc: 53, color: "#ff9ff3" },
                { name: "B1", cc: 54, color: "#ff6b6b" },
                { name: "B2", cc: 55, color: "#feca57" },
                { name: "B3", cc: 56, color: "#48dbfb" },
                { name: "B4", cc: 57, color: "#1dd1a1" }
            ]
        }
    },

    // ===================================================================
    // GENERIC / CUSTOM DEVICE (Fallback)
    // ===================================================================
    "Generic MIDI Device": {
        brand: "Generic",
        model: "Custom",
        midi_in: "Any",
        midi_channel_default: 1,
        categories: {
            "Standard CCs": [
                { name: "Modulation", value: 1, type: "Parameter", min: 0, max: 127 },
                { name: "Breath", value: 2, type: "Parameter", min: 0, max: 127 },
                { name: "Foot Controller", value: 4, type: "Parameter", min: 0, max: 127 },
                { name: "Volume", value: 7, type: "Parameter", min: 0, max: 127 },
                { name: "Balance", value: 8, type: "Parameter", min: 0, max: 127 },
                { name: "Pan", value: 10, type: "Parameter", min: 0, max: 127 },
                { name: "Expression", value: 11, type: "Parameter", min: 0, max: 127 },
                { name: "Sustain Pedal", value: 64, type: "Toggle", min: 0, max: 127 },
                { name: "Portamento", value: 65, type: "Toggle", min: 0, max: 127 },
                { name: "Sostenuto", value: 66, type: "Toggle", min: 0, max: 127 },
                { name: "Soft Pedal", value: 67, type: "Toggle", min: 0, max: 127 },
                { name: "Legato", value: 68, type: "Toggle", min: 0, max: 127 },
                { name: "Hold 2", value: 69, type: "Toggle", min: 0, max: 127 }
            ]
        },
        get cc() {
            const all = [];
            for (const cat of Object.values(this.categories)) {
                all.push(...cat);
            }
            return all;
        },
        templates: {}
    }
};

// ===========================================================================
// DATABASE MANAGEMENT FUNCTIONS
// ===========================================================================

// Current selected device
let currentDevice = "Sonicake Pocket Master";

// Get list of all available devices
function getDeviceList() {
    return Object.keys(DEVICE_DATABASE);
}

// Get device data by name
function getDevice(name) {
    return DEVICE_DATABASE[name] || DEVICE_DATABASE["Generic MIDI Device"];
}

// Get all CCs for current device
function getDeviceCCs(deviceName) {
    const device = getDevice(deviceName || currentDevice);
    return device.cc || [];
}

// Get CC by value for current device
function getCCByValue(ccValue, deviceName) {
    const ccs = getDeviceCCs(deviceName);
    return ccs.find(cc => cc.value === ccValue);
}

// Get CC label for display (e.g., "NR On/Off (43)")
function getCCLabel(ccValue, deviceName) {
    const cc = getCCByValue(ccValue, deviceName);
    if (cc) {
        return `${cc.name} (${cc.value})`;
    }
    return `CC ${ccValue}`;
}

// Get templates for a device
function getDeviceTemplates(deviceName) {
    const device = getDevice(deviceName || currentDevice);
    return device.templates || {};
}

// Build CC select options HTML - shows ALL devices organized by device name
function buildCCSelectOptions(selectedValue, deviceName) {
    let html = '<option value="">-- Select CC --</option>';

    // Show CCs from ALL devices, organized by device
    for (const [devName, device] of Object.entries(DEVICE_DATABASE)) {
        // Skip Generic MIDI Device as it's just a fallback
        if (devName === 'Generic MIDI Device') continue;

        // Device-level optgroup
        html += `<optgroup label="── ${devName} ──">`;
        for (const [category, ccs] of Object.entries(device.categories)) {
            for (const cc of ccs) {
                const selected = cc.value === selectedValue ? ' selected' : '';
                const shortName = devName === 'Sonicake Pocket Master' ? 'SPM' :
                    devName === 'Valeton GP-5' ? 'GP5' : devName;
                html += `<option value="${cc.value}"${selected}>[${shortName}] ${cc.name} (CC${cc.value})</option>`;
            }
        }
        html += '</optgroup>';
    }

    // Allow custom CC values
    html += '<optgroup label="── Custom CC ──">';
    for (let i = 0; i <= 127; i++) {
        // Only show if not already defined in any device
        let exists = false;
        for (const device of Object.values(DEVICE_DATABASE)) {
            if (device.categories) {
                for (const ccs of Object.values(device.categories)) {
                    if (ccs.some(cc => cc.value === i)) { exists = true; break; }
                }
            }
            if (exists) break;
        }
        if (!exists) {
            const selected = i === selectedValue ? ' selected' : '';
            html += `<option value="${i}"${selected}>CC ${i}</option>`;
        }
    }
    html += '</optgroup>';

    return html;
}

// Set current device
function setCurrentDevice(deviceName) {
    currentDevice = deviceName;
    // Save to localStorage for persistence
    try {
        localStorage.setItem('chocotone_target_device', deviceName);
    } catch (e) { /* ignore storage errors */ }
}

// Load saved device preference
function loadSavedDevice() {
    try {
        const saved = localStorage.getItem('chocotone_target_device');
        if (saved && DEVICE_DATABASE[saved]) {
            currentDevice = saved;
        }
    } catch (e) { /* ignore storage errors */ }
    return currentDevice;
}

// ===========================================================================
// ONLINE UPDATE FUNCTIONS (Future Enhancement)
// ===========================================================================

// OpenMIDI GitHub raw URL base
const OPENMIDI_BASE = 'https://raw.githubusercontent.com/Morningstar-Engineering/openmidi/main/data/brands';

// Try to fetch updated device data (background, non-blocking)
async function tryUpdateFromOpenMIDI() {
    // This will be implemented when we add online sync
    // For now, just use bundled data
    console.log('[OpenMIDI] Using bundled device database');
    return DEVICE_DATABASE;
}

// Export for use in offline_editor.html
if (typeof window !== 'undefined') {
    window.DEVICE_DATABASE = DEVICE_DATABASE;
    window.getDeviceList = getDeviceList;
    window.getDevice = getDevice;
    window.getDeviceCCs = getDeviceCCs;
    window.getCCByValue = getCCByValue;
    window.getCCLabel = getCCLabel;
    window.getDeviceTemplates = getDeviceTemplates;
    window.buildCCSelectOptions = buildCCSelectOptions;
    window.setCurrentDevice = setCurrentDevice;
    window.loadSavedDevice = loadSavedDevice;
    window.currentDevice = currentDevice;
}

// ===========================================================================
// FULL CONFIGURATION TEMPLATES (Complete presets + system config)
// ===========================================================================
const FULL_CONFIGS = {
    "ESP32-S3 Default (8-btn)": {
        "description": "Recommended pinout for ESP32-S3 (N16R8) - Safe for WiFi/BLE",
        "configName": "ESP32S3 TEMPLATE",
        "lastModified": "2026-01-24 11:59:37",
        "presets": [
            {
                "name": "STOMP",
                "syncMode": "SPM",
                "buttons": [
                    { "name": "NR", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 43, "data2": 127, "rgb": "#ffffff" }, { "action": "2ND_PRESS", "type": "CC", "data1": 43, "rgb": "#ffffff" }] },
                    { "name": "FX1", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 44, "data2": 127, "rgb": "#3f67ff" }, { "action": "2ND_PRESS", "type": "CC", "data1": 44, "rgb": "#3f67ff" }] },
                    { "name": "DRV", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 45, "data2": 127, "rgb": "#fc2c00" }, { "action": "2ND_PRESS", "type": "CC", "data1": 45, "rgb": "#fc2c00" }] },
                    { "name": "TAP", "messages": [{ "action": "PRESS", "type": "TAP_TEMPO", "data2": 127, "rgb": "#ffffff" }] },
                    { "name": "EQ", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 48, "data2": 127, "rgb": "#0af500" }, { "action": "2ND_PRESS", "type": "CC", "data1": 48, "rgb": "#0af500" }] },
                    { "name": "FX2", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 49, "data2": 127, "rgb": "#11f3ff" }, { "action": "2ND_PRESS", "type": "CC", "data1": 49, "rgb": "#11f3ff" }] },
                    { "name": "DLY", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 50, "data2": 127, "rgb": "#332aff" }, { "action": "2ND_PRESS", "type": "CC", "data1": 50, "rgb": "#332aff" }] },
                    { "name": "RVB", "ledMode": "TOGGLE", "messages": [{ "action": "PRESS", "type": "CC", "data1": 51, "data2": 127, "rgb": "#8400f7" }, { "action": "2ND_PRESS", "type": "CC", "data1": 51, "rgb": "#8400f7" }] }
                ]
            },
            {
                "name": "BANKS 1-8",
                "presetLedMode": "SELECTION",
                "buttons": [
                    { "name": "B1", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 1, "rgb": "#ffffff" }] },
                    { "name": "B2", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 2, "rgb": "#ffffff" }] },
                    { "name": "B3", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 3, "rgb": "#ffffff" }] },
                    { "name": "B4", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 4, "rgb": "#ffffff" }] },
                    { "name": "B5", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 5, "rgb": "#0af500" }] },
                    { "name": "B6", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 6, "rgb": "#0af500" }] },
                    { "name": "B7", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 7, "rgb": "#0af500" }, { "action": "COMBO", "partner": 7, "type": "WIFI_TOGGLE" }] },
                    { "name": "B8", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 8, "rgb": "#0af500" }] }
                ]
            },
            {
                "name": "BANKS 9-16",
                "presetLedMode": "SELECTION",
                "buttons": [
                    { "name": "B9", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 9, "rgb": "#11f3ff" }] },
                    { "name": "B10", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 10, "rgb": "#11f3ff" }] },
                    { "name": "B11", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 11, "rgb": "#11f3ff" }] },
                    { "name": "B12", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 12, "rgb": "#11f3ff" }] },
                    { "name": "B13", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 13, "rgb": "#aa00ff" }] },
                    { "name": "B14", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 14, "rgb": "#aa00ff" }] },
                    { "name": "B15", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 15, "rgb": "#aa00ff" }, { "action": "COMBO", "partner": 7, "type": "WIFI_TOGGLE" }] },
                    { "name": "B16", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 16, "rgb": "#aa00ff" }] }
                ]
            },
            {
                "name": "Note",
                "presetLedMode": "SELECTION",
                "buttons": [
                    { "name": "1st", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 40, "rgb": "#fd0000" }] },
                    { "name": "2nd", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 41, "rgb": "#fd0000" }] },
                    { "name": "3rd", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 42, "rgb": "#fd0000" }] },
                    { "name": "4th", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 43, "rgb": "#fd0000" }] },
                    { "name": "5th", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 44, "rgb": "#fd0000" }] },
                    { "name": "6th", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 45, "rgb": "#fd0000" }] },
                    { "name": "7th", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 46, "rgb": "#fd0000" }] },
                    { "name": "8up", "messages": [{ "action": "PRESS", "type": "CC", "data1": 1, "data2": 47, "rgb": "#fd0000" }] }
                ]
            }
        ],
        "currentPreset": 0,
        "presetCount": 4,
        "system": {
            "bleDeviceName": "CHOCOTONE S3",
            "apSSID": "CHOCOTONE_S3",
            "apPassword": "12345678",
            "buttonCount": 8,
            "buttonPins": "38,39,40,41,42,21,8,9",
            "ledPin": 48,
            "ledMap": "0,1,2,3,4,5,6,7",
            "encoderA": 16,
            "encoderB": 17,
            "encoderBtn": 18,
            "bleMode": "CLIENT",
            "brightness": 220,
            "brightnessDim": 20,
            "brightnessTap": 240,
            "analogInputCount": 4,
            "batteryAdcPin": 3,
            "oled": {
                "type": "128x128",
                "sdaPin": -1,
                "sclPin": -1,
                "csPin": 10,
                "dcPin": 13,
                "rstPin": 14,
                "mosiPin": 11,
                "sclkPin": 12,
                "ledPin": 15,
                "screens": {
                    "main": {
                        "topRowY": 4,
                        "titleY": 59,
                        "statusY": 44,
                        "bpmY": 32,
                        "bottomRowY": 117,
                        "topRowMap": "5,6,7,8",
                        "bottomRowMap": "1,2,3,4",
                        "showColorStrips": true,
                        "statusAlign": 1,
                        "showBattery": true,
                        "batteryX": 57,
                        "batteryY": 86
                    }
                }
            },
            "globalSpecialActions": [
                { "action": "LONG_PRESS", "type": "PRESET_DOWN", "channel": 1, "data1": 0, "data2": 0, "holdMs": 700, "label": "", "enabled": true, "partner": -1, "index": 4 },
                { "action": "LONG_PRESS", "type": "PRESET_UP", "channel": 1, "data1": 0, "data2": 0, "holdMs": 700, "label": "", "enabled": true, "partner": -1, "index": 7 }
            ]
        },
        "analogInputs": [
            { "index": 0, "pin": 4, "name": "A1", "rgb": "#f59e0b", "messages": [{ "type": "SYSEX_SCROLL", "data1": 1 }] },
            { "index": 1, "pin": 5, "name": "A2", "rgb": "#22c55e", "messages": [{ "type": "SYSEX_SCROLL", "data1": 2 }] },
            { "index": 2, "pin": 7, "name": "A3", "rgb": "#3b82f6", "messages": [{ "type": "CC", "data1": 11 }] },
            { "index": 3, "pin": 33, "name": "A4", "rgb": "#a855f7", "messages": [{ "type": "CC", "data1": 11 }] }
        ]
    },
    "Default (8-btn SPM)": {
        description: "Standard 8-button config for Sonicake Pocket Master",
        system: {
            bleDeviceName: "CHOCOTONE",
            apSSID: "CHOCOTONE",
            apPassword: "12345678",
            buttonCount: 8,
            buttonPins: "14,27,26,25,33,32,16,17",
            ledPin: 15,
            ledsPerButton: 1,
            ledMap: "0,1,2,3,7,6,5,4,8,9",
            encoderA: 18,
            encoderB: 19,
            encoderBtn: 23,
            bleMode: "CLIENT",
            brightness: 220,
            brightnessDim: 20
        },
        presets: [
            {
                name: "STOMP", presetLedMode: "NORMAL", syncMode: "SPM", buttons: [
                    { name: "NR", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 43, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 43, data2: 0, rgb: "#ffffff" }] },
                    { name: "FX1", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 44, data2: 127, rgb: "#3f67ff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 44, data2: 0, rgb: "#3f67ff" }] },
                    { name: "DRV", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 45, data2: 127, rgb: "#fc2c00" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 45, data2: 0, rgb: "#ff0000" }] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 13, data2: 127, rhythmPrev: 0, rhythmNext: 4, tapLock: 7, rgb: "#ffffff" }] },
                    { name: "EQ", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 48, data2: 127, rgb: "#0af500" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 48, data2: 0, rgb: "#0af500" }, { action: "LONG_PRESS", type: "PRESET_DOWN", channel: 1, data1: 0, data2: 0, holdMs: 700 }] },
                    { name: "FX2", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 49, data2: 127, rgb: "#11f3ff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 49, data2: 0, rgb: "#11f3ff" }] },
                    { name: "DLY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 50, data2: 127, rgb: "#332aff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 50, data2: 0, rgb: "#332aff" }] },
                    { name: "RVB", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 51, data2: 127, rgb: "#8400f7" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 51, data2: 0, rgb: "#8400f7" }, { action: "LONG_PRESS", type: "PRESET_UP", channel: 1, data1: 0, data2: 0, holdMs: 700 }] }
                ]
            },
            {
                name: "BANKS 1-8", presetLedMode: "SELECTION", buttons: [
                    { name: "B1", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 1, rgb: "#ffffff" }] },
                    { name: "B2", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 2, rgb: "#ffffff" }] },
                    { name: "B3", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 3, rgb: "#ffffff" }] },
                    { name: "B4", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 4, rgb: "#ffffff" }] },
                    { name: "B5", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 5, rgb: "#0af500" }, { action: "LONG_PRESS", type: "PRESET_DOWN", channel: 1, data1: 0, data2: 0, holdMs: 700 }] },
                    { name: "B6", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 6, rgb: "#0af500" }] },
                    { name: "B7", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 7, rgb: "#0af500" }] },
                    { name: "B8", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 8, rgb: "#0af500" }, { action: "LONG_PRESS", type: "PRESET_UP", channel: 1, data1: 0, data2: 0, holdMs: 700 }] }
                ]
            },
            {
                name: "BANKS 9-16", presetLedMode: "SELECTION", buttons: [
                    { name: "B9", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 9, rgb: "#11f3ff" }] },
                    { name: "B10", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 10, rgb: "#11f3ff" }] },
                    { name: "B11", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 11, rgb: "#11f3ff" }] },
                    { name: "B12", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 12, rgb: "#11f3ff" }] },
                    { name: "B13", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 13, rgb: "#aa00ff" }, { action: "LONG_PRESS", type: "PRESET_DOWN", channel: 1, data1: 0, data2: 0, holdMs: 700 }] },
                    { name: "B14", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 14, rgb: "#aa00ff" }] },
                    { name: "B15", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 15, rgb: "#aa00ff" }] },
                    { name: "B16", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 16, rgb: "#aa00ff" }, { action: "LONG_PRESS", type: "PRESET_UP", channel: 1, data1: 0, data2: 0, holdMs: 700 }] }
                ]
            },
            {
                name: "Note", presetLedMode: "SELECTION", buttons: [
                    { name: "1st", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 40, rgb: "#fd0000" }] },
                    { name: "2nd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 41, rgb: "#fd0000" }] },
                    { name: "3rd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 42, rgb: "#fd0000" }] },
                    { name: "4th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 43, rgb: "#fd0000" }] },
                    { name: "5th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 44, rgb: "#fd0000" }, { action: "LONG_PRESS", type: "PRESET_DOWN", channel: 1, data1: 0, data2: 0, holdMs: 700 }] },
                    { name: "6th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 45, rgb: "#fd0000" }] },
                    { name: "7th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 46, rgb: "#fd0000" }] },
                    { name: "8up", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 47, rgb: "#fd0000" }, { action: "LONG_PRESS", type: "PRESET_UP", channel: 1, data1: 0, data2: 0, holdMs: 700 }] }
                ]
            }
        ]
    },
    "Wisut (10-btn SPM)": {
        description: "10-button config for Sonicake Pocket Master",
        system: {
            bleDeviceName: "CHOCOTONE",
            apSSID: "CHOCOTONE",
            apPassword: "12345678",
            buttonCount: 10,
            buttonPins: "14,27,26,25,33,32,16,17,4,2",
            ledPin: 15,
            ledsPerButton: 1,
            ledMap: "0,1,2,3,4,9,8,7,6,5",
            encoderA: 18,
            encoderB: 19,
            encoderBtn: 23,
            bleMode: "CLIENT"
        },
        presets: [
            {
                name: "BANK 1", presetLedMode: "SELECTION", buttons: [
                    { name: "P1", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 1, rgb: "#ffffff" }] },
                    { name: "P2", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 2, rgb: "#ffffff" }] },
                    { name: "P3", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 3, rgb: "#ffffff" }] },
                    { name: "P4", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 4, rgb: "#ffffff" }] },
                    { name: "P5", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 5, rgb: "#0af500" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "FX1", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 44, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 44, data2: 0, rgb: "#3f67ff" }] },
                    { name: "DRV", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 45, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 45, data2: 0, rgb: "#ff0000" }] },
                    { name: "FX2", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 49, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 49, data2: 0, rgb: "#11f3ff" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] },
                    { name: "DLY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 50, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 50, data2: 0, rgb: "#332aff" }] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 13, data2: 127, rhythmPrev: 0, rhythmNext: 4, tapLock: 7, rgb: "#ffffff" }] }
                ]
            },
            {
                name: "BANK 2", presetLedMode: "SELECTION", buttons: [
                    { name: "P6", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 6, rgb: "#ffffff" }] },
                    { name: "P7", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 7, rgb: "#ffffff" }] },
                    { name: "P8", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 8, rgb: "#ffffff" }] },
                    { name: "P9", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 9, rgb: "#ffffff" }] },
                    { name: "P10", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 10, rgb: "#0af500" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "FX1", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 44, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 44, data2: 0, rgb: "#3f67ff" }] },
                    { name: "DRV", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 45, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 45, data2: 0, rgb: "#ff0000" }] },
                    { name: "FX2", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 49, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 49, data2: 0, rgb: "#11f3ff" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] },
                    { name: "DLY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 50, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 50, data2: 0, rgb: "#332aff" }] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 13, data2: 127, rhythmPrev: 0, rhythmNext: 4, tapLock: 7, rgb: "#ffffff" }] }
                ]
            },
            {
                name: "GP5", presetLedMode: "SELECTION", buttons: [
                    { name: "T1", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#ffffff", sysex: "f0060c000100000006010104030000000000000000f7" }] },
                    { name: "T2", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#ffffff", sysex: "f0070a000100000006010104030001000000000000f7" }] },
                    { name: "T3", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#ffffff", sysex: "f00400000100000006010104030002000000000000f7" }] },
                    { name: "T4", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#ffffff", sysex: "f00506000100000006010104030003000000000000f7" }] },
                    { name: "T5", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#0af500", sysex: "f00304000100000006010104030004000000000000f7" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "PRE", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#888888", sysex: "f0000f00010000000a0101040900010000000000000001000000000000f7" }, { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#888888", sysex: "f0010900010000000a0101040900010000000000000000000000000000f7" }] },
                    { name: "DST", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#fc2c00", sysex: "f0030a00010000000a0101040900020000000000000001000000000000f7" }, { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#fc2c00", sysex: "f0020c00010000000a0101040900020000000000000000000000000000f7" }] },
                    { name: "MOD", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#ff00ff", sysex: "f0070600010000000a0101040900060000000000000001000000000000f7" }, { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#ff00ff", sysex: "f0060000010000000a0101040900060000000000000000000000000000f7" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] },
                    { name: "DLY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#332aff", sysex: "f0060500010000000a0101040900070000000000000001000000000000f7" }, { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 0, rgb: "#332aff", sysex: "f0070300010000000a0101040900070000000000000000000000000000f7" }] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 13, data2: 127, rhythmPrev: 0, rhythmNext: 4, tapLock: 7, rgb: "#ffffff" }] }
                ]
            },
            {
                name: "Note", presetLedMode: "SELECTION", buttons: [
                    { name: "1st", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 40, rgb: "#fd0000" }] },
                    { name: "2nd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 41, rgb: "#fd0000" }] },
                    { name: "3rd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 42, rgb: "#fd0000" }] },
                    { name: "4th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 43, rgb: "#fd0000" }] },
                    { name: "5th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 44, rgb: "#fd0000" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "6th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 45, rgb: "#fd0000" }, { action: "COMBO", partner: 6, type: "WIFI_TOGGLE" }] },
                    { name: "7th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 46, rgb: "#fd0000" }] },
                    { name: "8up", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 47, rgb: "#fd0000" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] },
                    { name: "9th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 48, rgb: "#fd0000" }] },
                    { name: "OCT", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 52, rgb: "#ffff00" }] }
                ]
            }
        ]
    },
    "GP5 Wisut Profile": {
        description: "10-button config for Valeton GP-5 with hybrid preset/stomp banks",
        system: {
            bleDeviceName: "CHOCOTONE",
            apSSID: "CHOCOTONE",
            apPassword: "12345678",
            buttonCount: 10,
            buttonPins: "14,27,26,25,33,32,16,17",
            ledPin: 15,
            ledsPerButton: 1,
            ledMap: "0,1,2,3,7,6,5,4,8,9",
            encoderA: 18,
            encoderB: 19,
            encoderBtn: 23,
            bleMode: "CLIENT",
            brightness: 70,
            brightnessDim: 10,
            brightnessTap: 70
        },
        presets: [
            {
                name: "BANK-1", presetLedMode: "HYBRID", syncMode: "GP5", buttons: [
                    {
                        name: "P1", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 1, rgb: "#01becb", label: "", partner: -16, sysex: "f0070a000100000006010104030001000000000000f7" },
                            { action: "LONG_PRESS", type: "PRESET_1", channel: 1, data1: 43, data2: 0, rgb: "#ffffff", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P2", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 2, rgb: "#4bca07", label: "", partner: -16, sysex: "f00400000100000006010104030002000000000000f7" },
                            { action: "LONG_PRESS", type: "PRESET_2", channel: 1, data1: 44, data2: 0, rgb: "#3f67ff", label: "", partner: 0, holdMs: 0 }
                        ]
                    },
                    {
                        name: "P3", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 3, rgb: "#3d00ad", label: "", partner: -16, sysex: "f00506000100000006010104030003000000000000f7" },
                            { action: "LONG_PRESS", type: "PRESET_3", channel: 1, data1: 45, data2: 0, rgb: "#fc2c00", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P4", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 4, rgb: "#dc04a2", label: "", partner: -16, sysex: "f00304000100000006010104030004000000000000f7" },
                            { action: "LONG_PRESS", type: "PRESET_4", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P5", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 0, data2: 5, rgb: "#f50000", label: "", partner: -16, sysex: "f00202000100000006010104030005000000000000f7" }
                        ]
                    },
                    {
                        name: "BOOT", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 49, data2: 127, rgb: "#abe60a", label: "", partner: -16, sysex: "f0000f00010000000a0101040900010000000000000001000000000000f7" },
                            { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 49, data2: 0, rgb: "#abe60a", label: "", partner: -16, sysex: "f0010900010000000a0101040900010000000000000000000000000000f7" }
                        ]
                    },
                    {
                        name: "OD", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 50, data2: 127, rgb: "#e6740a", label: "", partner: -16, sysex: "f0030a00010000000a0101040900020000000000000001000000000000f7" },
                            { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 50, data2: 0, rgb: "#e6740a", label: "", partner: -16, sysex: "f0020c00010000000a0101040900020000000000000000000000000000f7" }
                        ]
                    },
                    {
                        name: "MODU", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 55, data2: 127, rgb: "#8400f7", label: "", partner: -16, sysex: "f0070600010000000a0101040900060000000000000001000000000000f7" },
                            { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 55, data2: 0, rgb: "#8400f7", label: "", partner: -16, sysex: "f0060000010000000a0101040900060000000000000000000000000000f7" }
                        ]
                    },
                    {
                        name: "DELY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "SYSEX", channel: 1, data1: 56, data2: 127, rgb: "#0923ec", label: "", partner: -16, sysex: "f0060500010000000a0101040900070000000000000001000000000000f7" },
                            { action: "2ND_PRESS", type: "SYSEX", channel: 1, data1: 56, data2: 127, rgb: "#0923ec", label: "", partner: -16, sysex: "f0070300010000000a0101040900070000000000000000000000000000f7" }
                        ]
                    },
                    {
                        name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 0, data2: 127, rgb: "#26c3f7", label: "", partner: 0, rhythmPrev: 0, rhythmNext: 4, tapLock: 7 }
                        ]
                    }
                ]
            },
            {
                name: "BANK-2", presetLedMode: "HYBRID", syncMode: "GP5", buttons: [
                    {
                        name: "P6", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 0, data2: 6, rgb: "#01d7f4", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_1", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P7", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 0, data2: 7, rgb: "#270ced", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_2", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P8", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 0, data2: 8, rgb: "#14f518", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_3", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P9", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 0, data2: 9, rgb: "#e18e19", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_4", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P10", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 0, data2: 10, rgb: "#f50000", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "BOOT", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 49, data2: 127, rgb: "#87f500", label: "", partner: 0 },
                            { action: "2ND_PRESS", type: "CC", channel: 1, data1: 49, data2: 0, rgb: "#87f500", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "OD", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 50, data2: 127, rgb: "#f54900", label: "", partner: 0 },
                            { action: "2ND_PRESS", type: "CC", channel: 1, data1: 50, data2: 0, rgb: "#f54900", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "MODU", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 55, data2: 127, rgb: "#f500ed", label: "", partner: 0 },
                            { action: "2ND_PRESS", type: "CC", channel: 1, data1: 55, data2: 0, rgb: "#f500ed", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "DELY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 56, data2: 127, rgb: "#1a40ff", label: "", partner: 0 },
                            { action: "2ND_PRESS", type: "CC", channel: 1, data1: 56, data2: 0, rgb: "#1a40ff", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 0, data2: 127, rgb: "#02f2e2", label: "", partner: 0, rhythmPrev: 0, rhythmNext: 4, tapLock: 7 }
                        ]
                    }
                ]
            },
            {
                name: "BANK-3", presetLedMode: "HYBRID", syncMode: "GP5", buttons: [
                    {
                        name: "P11", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 9, rgb: "#11f3ff", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_1", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P12", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 10, rgb: "#11f3ff", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_2", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P13", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 11, rgb: "#11f3ff", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_3", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P14", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 12, rgb: "#11f3ff", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_4", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P15", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 13, rgb: "#aa00ff", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "BOOT", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 14, rgb: "#aa00ff", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "OD", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 15, rgb: "#aa00ff", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "MODU", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 16, rgb: "#aa00ff", label: "", partner: 0 }
                        ]
                    },
                    { name: "DELY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [] }
                ]
            },
            {
                name: "BANK-4", presetLedMode: "HYBRID", syncMode: "GP5", buttons: [
                    {
                        name: "P16", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 60, data2: 127, rgb: "#fd0000", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_1", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P17", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 62, data2: 127, rgb: "#fd0000", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_2", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P18", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 64, data2: 127, rgb: "#fd0000", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_3", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P19", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 65, data2: 127, rgb: "#fd0000", label: "", partner: 0 },
                            { action: "LONG_PRESS", type: "PRESET_4", channel: 1, data1: 0, data2: 127, rgb: "#bb86fc", label: "", partner: -12, holdMs: 500 }
                        ]
                    },
                    {
                        name: "P20", ledMode: "MOMENTARY", inSelectionGroup: true, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 67, data2: 127, rgb: "#fd0000", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "BOOT", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 69, data2: 127, rgb: "#fd0000", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "OD", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 71, data2: 127, rgb: "#fd0000", label: "", partner: 0 }
                        ]
                    },
                    {
                        name: "MODU", ledMode: "TOGGLE", inSelectionGroup: false, messages: [
                            { action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 72, data2: 127, rgb: "#fd0000", label: "", partner: 0 }
                        ]
                    },
                    { name: "DELY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [] }
                ]
            }
        ]
    }
};

// Get full config list
function getFullConfigList() {
    return Object.keys(FULL_CONFIGS);
}

// Get a full config by name
function getFullConfig(name) {
    return FULL_CONFIGS[name] || null;
}

// Export full config functions
if (typeof window !== 'undefined') {
    window.FULL_CONFIGS = FULL_CONFIGS;
    window.getFullConfigList = getFullConfigList;
    window.getFullConfig = getFullConfig;
}

// ===========================================================================
// SYSEX SCROLL PARAMETER LISTS (v1.5.5)
// Maps analog input values to SysEx messages for parameter control
// Device-agnostic - works with Pocket Master, Valeton GP-5, and compatible devices
// ===========================================================================
const SYSEX_SCROLL_PARAMS = {
    // Note: Actual SysEx data is stored in firmware (PROGMEM)
    // These entries define parameter names and message counts for the editor UI
    "PITCH - HIGH": new Array(25).fill(""),   // 25 values (0-24), Firmware ID: 1
    "DRV - GAIN": new Array(101).fill("")     // 101 values (0-100), Firmware ID: 2
};

// Get all available SysEx scroll parameter names
function getSysexScrollParamList() {
    return Object.keys(SYSEX_SCROLL_PARAMS);
}

// Get message count for a parameter
function getSysexScrollParamCount(paramName) {
    return SYSEX_SCROLL_PARAMS[paramName] ? SYSEX_SCROLL_PARAMS[paramName].length : 0;
}

// Export SysEx scroll functions
if (typeof window !== 'undefined') {
    window.SYSEX_SCROLL_PARAMS = SYSEX_SCROLL_PARAMS;
    window.getSysexScrollParamList = getSysexScrollParamList;
    window.getSysexScrollParamCount = getSysexScrollParamCount;
}
