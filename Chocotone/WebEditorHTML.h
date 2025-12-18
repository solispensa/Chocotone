const char EDITOR_HTML[] PROGMEM = R"raw(<!DOCTYPE html>
<html>

<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>Chocotone MIDI Editor v2</title>
    <style>
        :root {
            --bg: #0a0a0f;
            --card: #16161e;
            --card-hover: #1e1e2a;
            --text: #e4e4e7;
            --text-dim: #71717a;
            --acc: #a78bfa;
            --acc-dim: #7c3aed;
            --inp: #27272a;
            --brd: #3f3f46;
            --success: #22c55e;
            --warning: #f59e0b;
            --danger: #ef4444;
            --radius: 10px;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: var(--bg);
            color: var(--text);
            min-height: 100vh;
            padding: 20px;
            max-width: 900px;
            margin: 0 auto;
        }

        /* Header */
        .header {
            text-align: center;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid var(--brd);
        }

        .header h1 {
            font-size: 1.5rem;
            font-weight: 700;
            background: linear-gradient(135deg, #fff, var(--acc));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }

        .header .sub {
            color: var(--text-dim);
            font-size: 0.8rem;
            margin-top: 4px;
        }

        /* Section Cards */
        .section {
            background: var(--card);
            border-radius: var(--radius);
            border: 1px solid var(--brd);
            padding: 16px;
            margin-bottom: 16px;
        }

        .section-title {
            font-size: 0.75rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: var(--acc);
            margin-bottom: 12px;
            font-weight: 600;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        /* Row layouts - CSS Grid */
        .row {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 12px;
            align-items: start;
            margin-bottom: 10px;
        }

        /* Form Fields */
        .field {
            display: flex;
            align-items: center;
            gap: 8px;
            width: 100%;
        }

        .field label {
            width: 55px;
            font-size: 0.8rem;
            color: var(--text-dim);
            flex-shrink: 0;
            text-align: right;
        }

        /* Inputs */
        .field input,
        .field select {
            width: 100%;
            min-width: 0;
            background: var(--inp);
            border: 1px solid var(--brd);
            color: var(--text);
            padding: 8px 10px;
            border-radius: 6px;
            font-size: 0.85rem;
            outline: none;
            transition: border 0.1s;
        }

        .field input:focus,
        .field select:focus {
            border-color: var(--acc);
        }

        .field input[type="color"] {
            height: 34px;
            padding: 2px;
            cursor: pointer;
        }

        .field input[type="checkbox"] {
            width: 18px;
            height: 18px;
            cursor: pointer;
            flex: none;
        }

        .field.disabled {
            opacity: 0.4;
            pointer-events: none;
        }

        /* Button Quick Menu - 2 rows layout */
        .btn-grid {
            display: grid;
            gap: 8px;
            margin-bottom: 8px;
        }

        .btn-tile {
            background: var(--inp);
            border: 2px solid var(--brd);
            border-radius: 8px;
            padding: 8px 4px;
            text-align: center;
            cursor: pointer;
            transition: all 0.15s;
            overflow: hidden;
        }

        .btn-tile:hover {
            background: var(--card-hover);
            border-color: var(--acc-dim);
        }

        .btn-tile.active {
            border-color: var(--acc);
            background: var(--card-hover);
        }

        .btn-tile .num {
            font-size: 0.65rem;
            color: var(--text-dim);
        }

        .btn-tile .name {
            font-size: 0.8rem;
            font-weight: 600;
            margin: 2px 0;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }

        .btn-tile .info {
            font-size: 0.6rem;
            color: var(--text-dim);
        }

        .btn-tile .colors {
            display: flex;
            justify-content: center;
            gap: 2px;
            margin-top: 4px;
        }

        .btn-tile .colors .dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            border: 1px solid #fff3;
        }

        /* System Tile - Full width, outside cards */
        .sys-tile-wrap {
            margin: 16px 0;
        }

        .sys-tile {
            background: var(--card);
            border: 1px solid var(--brd);
            border-radius: var(--radius);
            padding: 12px 16px;
            text-align: center;
            cursor: pointer;
            transition: all 0.15s;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            width: 100%;
        }

        .sys-tile:hover {
            border-color: var(--acc);
            background: var(--card-hover);
        }

        .sys-tile.active {
            border-color: var(--acc);
            background: linear-gradient(135deg, #3700b3, #7c3aed);
        }

        .sys-tile .icon {
            font-size: 1.2rem;
        }

        .sys-tile .label {
            font-size: 0.85rem;
            font-weight: 600;
            color: #fff;
        }

        /* Message Box */
        .msg-box {
            background: var(--inp);
            border: 1px solid var(--brd);
            border-radius: 8px;
            padding: 12px;
            margin-bottom: 12px;
            position: relative;
        }

        .msg-title {
            font-size: 0.7rem;
            text-transform: uppercase;
            color: var(--acc);
            margin-bottom: 10px;
            font-weight: 600;
            letter-spacing: 0.5px;
            border-bottom: 1px solid var(--brd);
            padding-bottom: 6px;
        }

        /* Delete button */
        .msg-box .remove-btn {
            position: absolute;
            top: 10px;
            right: 10px;
            background: rgba(239, 68, 68, 0.2);
            color: var(--danger);
            border: 1px solid var(--danger);
            width: 24px;
            height: 24px;
            border-radius: 4px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 16px;
            transition: all 0.2s;
            z-index: 10;
        }

        .msg-box .remove-btn:hover {
            background: var(--danger);
            color: white;
        }

        /* Action Buttons */
        .actions {
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }

        .btn {
            padding: 10px 18px;
            border: none;
            border-radius: 6px;
            font-size: 0.85rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            white-space: nowrap;
        }

        .btn.primary {
            background: var(--acc);
            color: #fff;
        }

        .btn.primary:hover {
            background: var(--acc-dim);
        }

        .btn.secondary {
            background: var(--inp);
            color: var(--text);
            border: 1px solid var(--brd);
        }

        .btn.success {
            background: var(--success);
            color: #fff;
        }

        .btn.warning {
            background: var(--warning);
            color: #000;
        }

        .btn.sm {
            padding: 4px 10px;
            font-size: 0.75rem;
        }

        /* JSON Area */
        .json-area {
            width: 100%;
            height: 120px;
            background: var(--inp);
            border: 1px solid var(--brd);
            color: var(--text);
            padding: 10px;
            border-radius: 6px;
            font-family: monospace;
            font-size: 0.8rem;
            resize: vertical;
        }

        /* Responsive */
        @media (max-width: 600px) {
            body {
                padding: 10px;
            }

            .row {
                grid-template-columns: 1fr;
            }

            .header h1 {
                font-size: 1.2rem;
            }
        }
    </style>
    <script>
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

    // ===================================================================
    // VALETON GP-5
    // ===================================================================
    "Valeton GP-5": {
        brand: "Valeton",
        model: "GP-5",
        midi_in: "USB-C",
        midi_channel_default: 1,
        categories: {
            "Effect Modules": [
                { name: "PRE On/Off", value: 49, description: "Pre-effect toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Distortion On/Off", value: 50, description: "Distortion toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Amp On/Off", value: 51, description: "Amp sim toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "IR On/Off", value: 52, description: "IR/Cab toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "NS On/Off", value: 53, description: "Noise Suppressor toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "EQ On/Off", value: 54, description: "EQ toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Modulation On/Off", value: 55, description: "Modulation toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Delay On/Off", value: 56, description: "Delay toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "Reverb On/Off", value: 57, description: "Reverb toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 },
                { name: "FX Loop On/Off", value: 58, description: "FX Loop toggle", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
            ],
            "Navigation": [
                { name: "Patch Select", value: 0, description: "Select patch 0-99", type: "Parameter", min: 0, max: 99 },
                { name: "Bank Down", value: 23, description: "Scroll bank down (-10 patches)", type: "System", min: 0, max: 127 },
                { name: "Bank Up", value: 24, description: "Scroll bank up (+10 patches)", type: "System", min: 0, max: 127 },
                { name: "Patch Down", value: 25, description: "Previous patch", type: "System", min: 0, max: 127 },
                { name: "Patch Up", value: 26, description: "Next patch", type: "System", min: 0, max: 127 }
            ],
            "Volume Controls": [
                { name: "Patch Volume", value: 7, description: "Current patch volume", type: "Parameter", min: 0, max: 100 }
            ],
            "Expression": [
                { name: "Wah Position", value: 29, description: "Wah pedal position", type: "Parameter", min: 0, max: 127 },
                { name: "Volume Pedal", value: 30, description: "Volume pedal position", type: "Parameter", min: 0, max: 127 }
            ],
            "Utilities": [
                { name: "Tuner On/Off", value: 69, description: "Toggle tuner", type: "Toggle", min: 0, max: 127, on: 127, off: 0 }
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
            "STOMP Mode": [
                { name: "PRE", cc: 49, color: "#888888" },
                { name: "DIST", cc: 50, color: "#fc2c00" },
                { name: "AMP", cc: 51, color: "#ff8800" },
                { name: "TAP", cc: null, special: "TAP_TEMPO", color: "#ffffff" },
                { name: "EQ", cc: 54, color: "#0af500" },
                { name: "MOD", cc: 55, color: "#ff00ff" },
                { name: "DLY", cc: 56, color: "#332aff" },
                { name: "RVB", cc: 57, color: "#8400f7" }
            ],
            "Full Chain": [
                { name: "PRE", cc: 49, color: "#888888" },
                { name: "DIST", cc: 50, color: "#fc2c00" },
                { name: "AMP", cc: 51, color: "#ff8800" },
                { name: "IR", cc: 52, color: "#ffcc00" },
                { name: "NS", cc: 53, color: "#666666" },
                { name: "EQ", cc: 54, color: "#0af500" },
                { name: "MOD", cc: 55, color: "#ff00ff" },
                { name: "DLY", cc: 56, color: "#332aff" },
                { name: "RVB", cc: 57, color: "#8400f7" },
                { name: "LOOP", cc: 58, color: "#11f3ff" }
            ],
            "Bank Selector": [
                { name: "P1", cc: 0, d2: 1, color: "#ffffff" },
                { name: "P2", cc: 0, d2: 2, color: "#ffffff" },
                { name: "P3", cc: 0, d2: 3, color: "#ffffff" },
                { name: "P4", cc: 0, d2: 4, color: "#ffffff" },
                { name: "P5", cc: 0, d2: 5, color: "#0af500" },
                { name: "P6", cc: 0, d2: 6, color: "#0af500" },
                { name: "P7", cc: 0, d2: 7, color: "#0af500" },
                { name: "P8", cc: 0, d2: 8, color: "#0af500" }
            ],
            // GP-5 SysEx toggle template - uses raw SysEx for effect control
            "GP5 Sysex": [
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

// Build CC select options HTML
function buildCCSelectOptions(selectedValue, deviceName) {
    const device = getDevice(deviceName || currentDevice);
    let html = '<option value="">-- Select CC --</option>';

    // Group by category
    for (const [category, ccs] of Object.entries(device.categories)) {
        html += `<optgroup label="${category}">`;
        for (const cc of ccs) {
            const selected = cc.value === selectedValue ? ' selected' : '';
            html += `<option value="${cc.value}"${selected}>${cc.name} (${cc.value})</option>`;
        }
        html += '</optgroup>';
    }

    // Allow custom CC values
    html += '<optgroup label="Custom">';
    for (let i = 0; i <= 127; i++) {
        const existingCC = getCCByValue(i, deviceName);
        if (!existingCC) {
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
            bleMode: "CLIENT"
        },
        presets: [
            {
                name: "STOMP", presetLedMode: "NORMAL", buttons: [
                    { name: "NR", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 43, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 43, data2: 0, rgb: "#ffffff" }] },
                    { name: "FX1", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 44, data2: 127, rgb: "#3f67ff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 44, data2: 0, rgb: "#3f67ff" }] },
                    { name: "DRV", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 45, data2: 127, rgb: "#fc2c00" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 45, data2: 0, rgb: "#ff0000" }] },
                    { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 13, data2: 127, rhythmPrev: 0, rhythmNext: 4, tapLock: 7, rgb: "#ffffff" }] },
                    { name: "EQ", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 48, data2: 127, rgb: "#0af500" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 48, data2: 0, rgb: "#0af500" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "FX2", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 49, data2: 127, rgb: "#11f3ff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 49, data2: 0, rgb: "#11f3ff" }] },
                    { name: "DLY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 50, data2: 127, rgb: "#332aff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 50, data2: 0, rgb: "#332aff" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                    { name: "RVB", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 51, data2: 127, rgb: "#8400f7" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 51, data2: 0, rgb: "#8400f7" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
                ]
            },
            {
                name: "BANKS 1-8", presetLedMode: "SELECTION", buttons: [
                    { name: "B1", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 1, rgb: "#ffffff" }] },
                    { name: "B2", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 2, rgb: "#ffffff" }] },
                    { name: "B3", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 3, rgb: "#ffffff" }] },
                    { name: "B4", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 4, rgb: "#ffffff" }] },
                    { name: "B5", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 5, rgb: "#0af500" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "B6", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 6, rgb: "#0af500" }] },
                    { name: "B7", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 7, rgb: "#0af500" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                    { name: "B8", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 8, rgb: "#0af500" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
                ]
            },
            {
                name: "BANKS 9-16", presetLedMode: "SELECTION", buttons: [
                    { name: "B9", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 9, rgb: "#11f3ff" }] },
                    { name: "B10", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 10, rgb: "#11f3ff" }] },
                    { name: "B11", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 11, rgb: "#11f3ff" }] },
                    { name: "B12", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 12, rgb: "#11f3ff" }] },
                    { name: "B13", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 13, rgb: "#aa00ff" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "B14", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 14, rgb: "#aa00ff" }] },
                    { name: "B15", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 15, rgb: "#aa00ff" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                    { name: "B16", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 16, rgb: "#aa00ff" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
                ]
            },
            {
                name: "Note", presetLedMode: "SELECTION", buttons: [
                    { name: "1st", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 60, data2: 127, rgb: "#fd0000" }] },
                    { name: "2nd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 62, data2: 127, rgb: "#fd0000" }] },
                    { name: "3rd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 64, data2: 127, rgb: "#fd0000" }] },
                    { name: "4th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 65, data2: 127, rgb: "#fd0000" }] },
                    { name: "5th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 67, data2: 127, rgb: "#fd0000" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                    { name: "6th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 69, data2: 127, rgb: "#fd0000" }] },
                    { name: "7th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 71, data2: 127, rgb: "#fd0000" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                    { name: "8up", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 72, data2: 127, rgb: "#fd0000" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
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

</script>
</head>

<body>
    <div class="header">
        <h1>Chocotone MIDI Editor</h1>
        <div class="sub">v2.0 - Hybrid Action System</div>
    </div>

    <div id="app"></div>

    <script>
        // ===============================================================
        // STATE
        // ===============================================================
        var selectedBtn = 0;
        var showSystem = false;

        var actionTypes = ['NO_ACTION', 'PRESS', '2ND_PRESS', 'RELEASE', 'LONG_PRESS', 'DOUBLE_TAP', 'COMBO'];
        var midiTypes = ['OFF', 'NOTE_MOMENTARY', 'NOTE_ON', 'NOTE_OFF', 'CC', 'PC', 'SYSEX', 'TAP_TEMPO', 'PRESET_UP', 'PRESET_DOWN', 'PRESET_1', 'PRESET_2', 'PRESET_3', 'PRESET_4', 'CLEAR_BLE_BONDS', 'WIFI_TOGGLE'];

        // ===============================================================
        // DATA
        // ===============================================================
        var presetData = {
            presets: [
                {
                    name: "STOMP", presetLedMode: "NORMAL", buttons: [
                        { name: "NR", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 43, data2: 127, rgb: "#ffffff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 43, data2: 0, rgb: "#ffffff" }] },
                        { name: "FX1", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 44, data2: 127, rgb: "#3f67ff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 44, data2: 0, rgb: "#3f67ff" }] },
                        { name: "DRV", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 45, data2: 127, rgb: "#fc2c00" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 45, data2: 0, rgb: "#ff0000" }] },
                        { name: "TAP", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "TAP_TEMPO", channel: 1, data1: 13, data2: 127, rhythmPrev: 0, rhythmNext: 4, tapLock: 7, rgb: "#ffffff" }] },
                        { name: "EQ", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 48, data2: 127, rgb: "#0af500" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 48, data2: 0, rgb: "#0af500" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN", channel: 1, data1: 0, data2: 0, label: "" }] },
                        { name: "FX2", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 49, data2: 127, rgb: "#11f3ff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 49, data2: 0, rgb: "#11f3ff" }] },
                        { name: "DLY", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 50, data2: 127, rgb: "#332aff" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 50, data2: 0, rgb: "#332aff" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE", channel: 1, data1: 0, data2: 0, label: "" }] },
                        { name: "RVB", ledMode: "TOGGLE", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 51, data2: 127, rgb: "#8400f7" }, { action: "2ND_PRESS", type: "CC", channel: 1, data1: 51, data2: 0, rgb: "#8400f7" }, { action: "COMBO", partner: 3, type: "PRESET_UP", channel: 1, data1: 0, data2: 0, label: "" }] }
                    ]
                },
                {
                    name: "BANKS 1-8", presetLedMode: "SELECTION", buttons: [
                        { name: "B1", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 1, rgb: "#ffffff" }] },
                        { name: "B2", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 2, rgb: "#ffffff" }] },
                        { name: "B3", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 3, rgb: "#ffffff" }] },
                        { name: "B4", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 4, rgb: "#ffffff" }] },
                        { name: "B5", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 5, rgb: "#0af500" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                        { name: "B6", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 6, rgb: "#0af500" }] },
                        { name: "B7", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 7, rgb: "#0af500" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                        { name: "B8", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 8, rgb: "#0af500" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
                    ]
                },
                {
                    name: "BANKS 9-16", presetLedMode: "SELECTION", buttons: [
                        { name: "B9", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 9, rgb: "#11f3ff" }] },
                        { name: "B10", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 10, rgb: "#11f3ff" }] },
                        { name: "B11", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 11, rgb: "#11f3ff" }] },
                        { name: "B12", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 12, rgb: "#11f3ff" }] },
                        { name: "B13", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 13, rgb: "#aa00ff" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                        { name: "B14", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 14, rgb: "#aa00ff" }] },
                        { name: "B15", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 15, rgb: "#aa00ff" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                        { name: "B16", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "CC", channel: 1, data1: 1, data2: 16, rgb: "#aa00ff" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
                    ]
                },
                {
                    name: "Note", presetLedMode: "SELECTION", buttons: [
                        { name: "1st", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 60, data2: 127, rgb: "#fd0000" }] },
                        { name: "2nd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 62, data2: 127, rgb: "#fd0000" }] },
                        { name: "3rd", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 64, data2: 127, rgb: "#fd0000" }] },
                        { name: "4th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 65, data2: 127, rgb: "#fd0000" }] },
                        { name: "5th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 67, data2: 127, rgb: "#fd0000" }, { action: "COMBO", partner: 0, type: "PRESET_DOWN" }] },
                        { name: "6th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 69, data2: 127, rgb: "#fd0000" }] },
                        { name: "7th", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 71, data2: 127, rgb: "#fd0000" }, { action: "COMBO", partner: 7, type: "WIFI_TOGGLE" }] },
                        { name: "8up", ledMode: "MOMENTARY", inSelectionGroup: false, messages: [{ action: "PRESS", type: "NOTE_MOMENTARY", channel: 1, data1: 72, data2: 127, rgb: "#fd0000" }, { action: "COMBO", partner: 3, type: "PRESET_UP" }] }
                    ]
                }
            ],
            currentPreset: 0,
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
                bleMode: "CLIENT"
            }
        };

        function fillEmptyPresets() {
            for (let p = 0; p < presetData.presets.length; p++) {
                if (!presetData.presets[p].buttons.length) {
                    for (let i = 0; i < 8; i++) presetData.presets[p].buttons.push(createDefaultButton(i));
                }
            }
        }
        fillEmptyPresets();

        // ===============================================================
        // OPEN MIDI FUNCTIONS
        // ===============================================================
        function getDeviceSelectOptions() {
            var devices = typeof getDeviceList === 'function' ? getDeviceList() : ['Generic MIDI Device'];
            var current = typeof loadSavedDevice === 'function' ? loadSavedDevice() : 'Sonicake Pocket Master';
            return devices.map(function (d) {
                return '<option value="' + d + '"' + (d === current ? ' selected' : '') + '>' + d + '</option>';
            }).join('');
        }
        function getTemplateSelectOptions() {
            var current = typeof loadSavedDevice === 'function' ? loadSavedDevice() : 'Sonicake Pocket Master';
            var templates = typeof getDeviceTemplates === 'function' ? getDeviceTemplates(current) : {};
            var fullConfigs = typeof getFullConfigList === 'function' ? getFullConfigList() : [];

            var html = '<option value="">-- Select Template --</option>';

            // Full Configurations (load everything)
            if (fullConfigs.length > 0) {
                html += '<optgroup label="[Full Configurations]">';
                for (var i = 0; i < fullConfigs.length; i++) {
                    html += '<option value="FULL:' + fullConfigs[i] + '">' + fullConfigs[i] + '</option>';
                }
                html += '</optgroup>';
            }

            // Device-specific partial templates
            html += '<optgroup label="[Button Layouts: ' + current + ']">';
            for (var name in templates) {
                html += '<option value="' + name + '">' + name + '</option>';
            }
            html += '</optgroup>';

            return html;
        }
        function getCCPickerOptions(selectedVal) {
            if (typeof buildCCSelectOptions === 'function') {
                return buildCCSelectOptions(selectedVal);
            }
            // Fallback: simple 0-127
            var html = '';
            for (var i = 0; i <= 127; i++) {
                html += '<option value="' + i + '"' + (i === selectedVal ? ' selected' : '') + '>CC ' + i + '</option>';
            }
            return html;
        }
        function loadDeviceTemplate() {
            var sel = document.getElementById('tmplSelect');
            if (!sel || !sel.value) return alert('Please select a template');
            var tmplName = sel.value;

            // Check if this is a full configuration
            if (tmplName.startsWith('FULL:')) {
                var configName = tmplName.substring(5);
                var fullConfig = typeof getFullConfig === 'function' ? getFullConfig(configName) : null;
                if (!fullConfig) return alert('Full configuration not found');

                // Load system config
                if (fullConfig.system) {
                    for (var key in fullConfig.system) {
                        presetData.system[key] = fullConfig.system[key];
                    }
                }

                // Load all presets
                if (fullConfig.presets) {
                    for (var p = 0; p < fullConfig.presets.length && p < 4; p++) {
                        var srcPreset = fullConfig.presets[p];
                        presetData.presets[p].name = srcPreset.name;
                        presetData.presets[p].presetLedMode = srcPreset.presetLedMode || 'NORMAL';
                        presetData.presets[p].buttons = JSON.parse(JSON.stringify(srcPreset.buttons));
                    }
                }

                selectedBtn = 0;
                render();
                alert('Full configuration "' + configName + '" loaded!\n\nIncludes:\n- System config (10 buttons)\n- All 4 presets');
                return;
            }

            // Regular partial template (only loads buttons for current preset)
            var currentDevice = typeof loadSavedDevice === 'function' ? loadSavedDevice() : 'Sonicake Pocket Master';
            var templates = typeof getDeviceTemplates === 'function' ? getDeviceTemplates(currentDevice) : {};
            var tmpl = templates[tmplName];
            if (!tmpl) return alert('Template not found');

            var p = presetData.currentPreset;
            var count = Math.min(tmpl.length, presetData.system.buttonCount);
            for (var i = 0; i < count; i++) {
                var src = tmpl[i];
                var btn = presetData.presets[p].buttons[i];
                btn.name = src.name;
                btn.messages = [];

                // Check for SysEx template (has sysexOn/sysexOff)
                if (src.sysexOn && src.sysexOff) {
                    // SysEx toggle button - PRESS sends ON, 2ND_PRESS sends OFF
                    btn.messages.push({
                        action: 'PRESS',
                        type: 'SYSEX',
                        channel: 1,
                        data1: 0,
                        data2: 0,
                        rgb: src.color,
                        sysex: src.sysexOn  // Hex string
                    });
                    btn.messages.push({
                        action: '2ND_PRESS',
                        type: 'SYSEX',
                        channel: 1,
                        data1: 0,
                        data2: 0,
                        rgb: src.color,
                        sysex: src.sysexOff  // Hex string
                    });
                    btn.ledMode = 'TOGGLE';
                } else if (src.special) {
                    // Special action (TAP_TEMPO, etc.)
                    var msg1 = { action: 'PRESS', type: src.special, channel: 1, data1: 0, data2: 127, rgb: src.color };
                    btn.messages.push(msg1);
                    btn.ledMode = 'MOMENTARY';
                } else {
                    // Regular CC toggle
                    var msg1 = { action: 'PRESS', type: 'CC', channel: 1, data1: src.cc, data2: (src.d2 !== undefined ? src.d2 : 127), rgb: src.color };
                    btn.messages.push(msg1);
                    btn.messages.push({ action: '2ND_PRESS', type: 'CC', channel: 1, data1: src.cc, data2: 0, rgb: src.color });
                    btn.ledMode = 'TOGGLE';
                }
            }
            render();
            alert('Template "' + tmplName + '" loaded!');
        }
        function changeTargetDevice(d) {
            if (typeof setCurrentDevice === 'function') setCurrentDevice(d);
            render();
        }

        // ===============================================================
        // RENDER UI
        // ===============================================================
        function render() {
            var p = presetData.currentPreset;
            var preset = presetData.presets[p];
            var btn = preset.buttons[selectedBtn];
            var presetMode = preset.presetLedMode || 'NORMAL';
            var showLedMode = (presetMode === 'NORMAL' || presetMode === 'HYBRID');
            var showSelGroup = (presetMode === 'HYBRID');
            var html = '';

            // --- PRESET CONFIG -------------------------------------------
            html += '<div class="section">';
            html += '<div class="section-title">Preset Configuration</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>Preset</label><select onchange="chgPreset(this.value)">';
            for (var i = 0; i < 4; i++) html += '<option value="' + i + '"' + (i === p ? ' selected' : '') + '>' + presetData.presets[i].name + '</option>';
            html += '</select></div>';
            html += '<div class="field"><label>Name</label><input type="text" value="' + preset.name + '" maxlength="20" onchange="preset.name=this.value"></div>';
            html += '<div class="field"><label>LED Mode</label><select onchange="chgPresetLedMode(this.value)">';
            html += '<option value="NORMAL"' + (presetMode === 'NORMAL' ? ' selected' : '') + '>Normal</option>';
            html += '<option value="SELECTION"' + (presetMode === 'SELECTION' ? ' selected' : '') + '>Selection</option>';
            html += '<option value="HYBRID"' + (presetMode === 'HYBRID' ? ' selected' : '') + '>Hybrid</option>';
            html += '</select></div>';
            html += '</div></div>';

            // --- BUTTONS (2 ROWS) ----------------------------------------
            html += '<div class="section">';
            html += '<div class="section-title">Buttons</div>';
            var btnCount = presetData.system.buttonCount || 8;
            var cols = Math.ceil(btnCount / 2); // 2 rows = half columns
            html += '<div class="btn-grid" style="grid-template-columns: repeat(' + cols + ', 1fr)">';
            // Render second half first (top row: 5,6,7,8), then first half (bottom row: 1,2,3,4)
            var half = Math.ceil(btnCount / 2);
            var order = [];
            for (var i = half; i < btnCount; i++) order.push(i); // Top row: indices 4,5,6,7
            for (var i = 0; i < half; i++) order.push(i);        // Bottom row: indices 0,1,2,3
            for (var idx = 0; idx < order.length; idx++) {
                var b = order[idx];
                var bt = preset.buttons[b];
                if (!bt) continue;
                var isActive = (b === selectedBtn && !showSystem);
                var msg1 = (bt.messages && bt.messages[0]) ? bt.messages[0] : { type: 'OFF', data1: 0, rgb: '#555' };

                html += '<div class="btn-tile' + (isActive ? ' active' : '') + '" onclick="selBtn(' + b + ')">';
                html += '<div class="num">BTN ' + (b + 1) + '</div>';
                html += '<div class="name">' + bt.name + '</div>';
                html += '<div class="info">' + msg1.type + ' ' + msg1.data1 + '</div>';
                html += '<div class="colors">';
                html += '<div class="dot" style="background:' + msg1.rgb + '"></div>';
                html += '</div></div>';
            }
            html += '</div>';
            html += '</div>'; // Close Buttons section

            // System Tile - OUTSIDE the Buttons card
            html += '<div class="sys-tile-wrap">';
            html += '<div class="sys-tile' + (showSystem ? ' active' : '') + '" onclick="toggleSystem()">';
            html += '<span class="label">System Configuration</span>';
            html += '</div></div>';

            // --- MAIN CONTENT --------------------------------------------
            if (showSystem) {
                html += renderSystemConfig();
            } else {
                html += renderButtonDetail(btn, selectedBtn, showLedMode, showSelGroup);
            }

            // --- JSON ----------------------------------------------------
            html += '<div class="section">';
            html += '<div class="section-title">Import / Export</div>';
            html += '<textarea class="json-area" id="jsonArea" placeholder="Paste JSON here..."></textarea>';
            html += '<div class="actions" style="margin-top:10px">';
            html += '<button class="btn success" onclick="exportJSON()">Export</button>';
            html += '<button class="btn secondary" onclick="importJSON()">Import</button>';
            html += '<button class="btn secondary" onclick="copyJSON()">Copy</button>';
            html += '<button class="btn warning" onclick="downloadJSON()">Download</button>';
            html += '</div></div>';

            html += '<div style="text-align:center;color:#555;font-size:0.7rem;margin-top:10px">Chocotone v2.0 - Hybrid Editor</div>';
            document.getElementById('app').innerHTML = html;
        }

        function renderButtonDetail(btn, idx, showLedMode, showSelGroup) {
            var html = '<div class="section">';
            html += '<div class="section-title">Button ' + (idx + 1) + ': ' + btn.name + '</div>';

            html += '<div class="row">';
            html += '<div class="field"><label>Label</label><input type="text" value="' + btn.name + '" maxlength="20" onchange="updBtn(\'name\',this.value)"></div>';
            if (showLedMode) {
                html += '<div class="field"><label>LED</label><select onchange="updBtn(\'ledMode\',this.value)">';
                html += '<option value="MOMENTARY"' + (btn.ledMode === 'MOMENTARY' ? ' selected' : '') + '>Momentary</option>';
                html += '<option value="TOGGLE"' + (btn.ledMode === 'TOGGLE' ? ' selected' : '') + '>Toggle</option>';
                html += '</select></div>';
            }
            if (showSelGroup) {
                html += '<div class="field"><label>Sel.Grp</label><input type="checkbox"' + (btn.inSelectionGroup ? ' checked' : '') + ' onchange="updBtn(\'inSelectionGroup\',this.checked)"></div>';
            }
            html += '</div>';

            html += '<div class="section-title" style="margin-top:16px; border-bottom:1px solid #333; padding-bottom:4px">Action List <button class="btn primary sm" style="margin-left:auto" onclick="addMsg()">+</button></div>';

            var msgs = btn.messages || [];
            for (var i = 0; i < msgs.length; i++) html += renderActionCard(msgs[i], i);

            // Save Changes Button
            html += '<div class="actions" style="margin-top:16px; justify-content:center">';
            html += '<button class="btn success" onclick="saveChanges()">Save Changes</button>';
            html += '</div>';

            html += '</div>';
            return html;
        }

        function renderActionCard(msg, i) {
            var html = '<div class="msg-box">';
            html += '<div class="remove-btn" onclick="delMsg(' + i + ')">×</div>';

            // Action Type
            html += '<div class="field" style="margin-bottom:8px; padding-right:30px"><label style="font-weight:600;color:var(--acc)">ACTION</label><select style="font-weight:600" onchange="updMsg(' + i + ',\'action\',this.value)">' + actionOpts(msg.action) + '</select></div>';

            // Conditional fields
            if (msg.action === 'COMBO') {
                html += '<div class="row">';
                html += '<div class="field"><label>Partner</label><select onchange="updMsg(' + i + ',\'partner\',parseInt(this.value))">';
                var cnt = presetData.system.buttonCount;
                for (let b = 0; b < cnt; b++) {
                    if (b !== selectedBtn) html += '<option value="' + b + '"' + (msg.partner === b ? ' selected' : '') + '>BTN ' + (b + 1) + '</option>';
                }
                html += '</select></div>';
                html += '<div class="field"><label>Label</label><input type="text" maxlength="6" value="' + (msg.label || '') + '" placeholder="Auto" onchange="updMsg(' + i + ',\'label\',this.value)"></div>';
                html += '</div>';
            }
            else if (msg.action === 'LONG_PRESS') {
                html += '<div class="row">';
                html += '<div class="field"><label>Hold ms</label><input type="number" min="200" max="3000" step="100" value="' + (msg.holdMs || 500) + '" onchange="updMsg(' + i + ',\'holdMs\',parseInt(this.value))"></div>';
                html += '</div>';
            }

            // MIDI Content
            html += '<div class="row">';
            html += '<div class="field"><label>Type</label><select onchange="updMsg(' + i + ',\'type\',this.value)">' + typeOpts(msg.type) + '</select></div>';
            html += '<div class="field"><label>Ch</label><input type="number" min="1" max="16" value="' + (msg.channel || 1) + '" onchange="updMsg(' + i + ',\'channel\',parseInt(this.value))"></div>';
            // D1 as CC Picker dropdown for CC type
            if (msg.type === 'CC') {
                html += '<div class="field"><label>CC</label><select onchange="updMsg(' + i + ',\'data1\',parseInt(this.value))">' + getCCPickerOptions(msg.data1 || 0) + '</select></div>';
            } else {
                html += '<div class="field"><label>D1</label><input type="number" min="0" max="127" value="' + (msg.data1 || 0) + '" onchange="updMsg(' + i + ',\'data1\',parseInt(this.value))"></div>';
            }
            html += '<div class="field"><label>D2</label><input type="number" min="0" max="127" value="' + (msg.data2 || 0) + '" onchange="updMsg(' + i + ',\'data2\',parseInt(this.value))"></div>';
            html += '<div class="field"><label>RGB</label><input type="color" value="' + (msg.rgb || '#bb86fc') + '" onchange="updMsg(' + i + ',\'rgb\',this.value)"></div>';
            html += '</div>';

            if (msg.type === 'TAP_TEMPO') {
                html += '<div class="row" style="margin-top:8px">';
                html += '<div class="field"><label>R.Prev</label><input type="number" min="0" max="9" value="' + (msg.rhythmPrev || 0) + '" onchange="updMsg(' + i + ',\'rhythmPrev\',parseInt(this.value))"></div>';
                html += '<div class="field"><label>R.Next</label><input type="number" min="0" max="9" value="' + (msg.rhythmNext || 4) + '" onchange="updMsg(' + i + ',\'rhythmNext\',parseInt(this.value))"></div>';
                html += '<div class="field"><label>Lock</label><input type="number" min="0" max="9" value="' + (msg.tapLock || 7) + '" onchange="updMsg(' + i + ',\'tapLock\',parseInt(this.value))"></div>';
                html += '</div>';
            }

            // SysEx hex input field
            if (msg.type === 'SYSEX') {
                html += '<div class="row" style="margin-top:8px">';
                html += '<div class="field" style="flex:1"><label>SysEx Hex (e.g. f00f0f00...f7)</label><input type="text" style="font-family:monospace;font-size:11px" placeholder="f0...f7" value="' + (msg.sysex || '') + '" onchange="updMsg(' + i + ',\'sysex\',this.value.toLowerCase().replace(/[^0-9a-f]/g,\'\'))"></div>';
                html += '</div>';
            }

            html += '</div>';
            return html;
        }

        function renderSystemConfig() {
            var sys = presetData.system;
            var html = '<div class="section">';
            html += '<div class="section-title">System Configuration</div>';

            // Templates
            html += '<div class="msg-box"><div class="msg-title">Templates</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>Device</label><select onchange="changeTargetDevice(this.value)">' + getDeviceSelectOptions() + '</select></div>';
            html += '</div><div class="row">';
            html += '<div class="field"><label>Template</label><select id="tmplSelect">' + getTemplateSelectOptions() + '</select></div>';
            html += '<div class="field" style="max-width:100px"><button class="btn primary sm" onclick="loadDeviceTemplate()">Load</button></div>';
            html += '</div></div>';

            html += '<div class="msg-box"><div class="msg-title">Bluetooth / WiFi</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>BLE Name</label><input type="text" value="' + sys.bleDeviceName + '" maxlength="20" onchange="updSys(\'bleDeviceName\',this.value)"></div>';
            html += '<div class="field"><label>BLE Mode</label><select onchange="updSys(\'bleMode\',this.value)">';
            html += '<option value="CLIENT"' + (sys.bleMode === 'CLIENT' ? ' selected' : '') + '>Client</option>';
            html += '<option value="SERVER"' + (sys.bleMode === 'SERVER' ? ' selected' : '') + '>Server</option>';
            html += '<option value="DUAL"' + (sys.bleMode === 'DUAL' ? ' selected' : '') + '>Dual</option>';
            html += '</select></div>';
            html += '</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>WiFi SSID</label><input type="text" value="' + sys.apSSID + '" onchange="updSys(\'apSSID\',this.value)"></div>';
            html += '<div class="field"><label>Password</label><input type="text" value="' + sys.apPassword + '" onchange="updSys(\'apPassword\',this.value)"></div>';
            html += '</div></div>';

            html += '<div class="msg-box"><div class="msg-title">Hardware</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>Buttons #</label><input type="number" min="4" max="10" value="' + sys.buttonCount + '" onchange="updBtnCount(parseInt(this.value))"></div>';
            html += '<div class="field"><label>Btn Pins</label><input type="text" value="' + sys.buttonPins + '" onchange="updSys(\'buttonPins\',this.value)"></div>';
            html += '</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>LED Pin</label><input type="number" min="0" max="39" value="' + sys.ledPin + '" onchange="updSys(\'ledPin\',parseInt(this.value))"></div>';
            html += '<div class="field"><label>LEDs/Btn</label><input type="number" min="1" max="32" value="' + sys.ledsPerButton + '" onchange="updSys(\'ledsPerButton\',parseInt(this.value))"></div>';
            html += '<div class="field"><label>LED Map</label><input type="text" value="' + sys.ledMap + '" onchange="updSys(\'ledMap\',this.value)"></div>';
            html += '</div></div>';

            html += '<div class="msg-box"><div class="msg-title">Encoder</div>';
            html += '<div class="row">';
            html += '<div class="field"><label>Enc A</label><input type="number" min="0" max="39" value="' + sys.encoderA + '" onchange="updSys(\'encoderA\',parseInt(this.value))"></div>';
            html += '<div class="field"><label>Enc B</label><input type="number" min="0" max="39" value="' + sys.encoderB + '" onchange="updSys(\'encoderB\',parseInt(this.value))"></div>';
            html += '<div class="field"><label>Enc Btn</label><input type="number" min="0" max="39" value="' + sys.encoderBtn + '" onchange="updSys(\'encoderBtn\',parseInt(this.value))"></div>';
            html += '</div></div>';

            html += '</div>';
            return html;
        }

        function typeOpts(sel) { return midiTypes.map(function (t) { return '<option value="' + t + '"' + (t === sel ? ' selected' : '') + '>' + t + '</option>'; }).join(''); }
        function actionOpts(sel) { return actionTypes.map(function (t) { return '<option value="' + t + '"' + (t === sel ? ' selected' : '') + '>' + t.replace('_', ' ') + '</option>'; }).join(''); }

        function chgPreset(v) { presetData.currentPreset = parseInt(v); selectedBtn = 0; showSystem = false; render(); fetch('/preset?p=' + v).catch(e => { }); }
        function chgPresetLedMode(v) { presetData.presets[presetData.currentPreset].presetLedMode = v; render(); }
        function selBtn(b) { selectedBtn = b; showSystem = false; render(); }
        function toggleSystem() { showSystem = !showSystem; render(); }

        function updBtn(k, v) { presetData.presets[presetData.currentPreset].buttons[selectedBtn][k] = v; render(); }
        function updMsg(idx, k, v) {
            presetData.presets[presetData.currentPreset].buttons[selectedBtn].messages[idx][k] = v;
            if (k === 'type' || k === 'action') render();
        }
        function addMsg() {
            var p = presetData.currentPreset;
            var btn = presetData.presets[p].buttons[selectedBtn];
            if (!btn.messages) btn.messages = [];
            btn.messages.push({ action: 'PRESS', type: 'CC', channel: 1, data1: 0, data2: 127, rgb: '#bb86fc' });
            render();
        }
        function delMsg(idx) {
            var p = presetData.currentPreset;
            presetData.presets[p].buttons[selectedBtn].messages.splice(idx, 1);
            render();
        }
        function updSys(k, v) { presetData.system[k] = v; }
        function saveChanges() { try { var json = JSON.stringify(presetData); document.getElementById('jsonArea').value = JSON.stringify(presetData, null, 2); console.log('Saving config, JSON length: ' + json.length + ' bytes'); var blob = new Blob([json], { type: 'application/json' }); var formData = new FormData(); formData.append('config', blob, 'config.json'); var xhr = new XMLHttpRequest(); xhr.open('POST', '/import', true); xhr.timeout = 30000; xhr.onreadystatechange = function() { if (xhr.readyState === 4) { if (xhr.status === 200) { alert('SUCCESS: ' + xhr.responseText + '\\n\\nReconnect to WiFi after device reboots.'); } else if (xhr.status === 400) { alert('ERROR from device:\\n' + xhr.responseText); } else if (xhr.status === 0) { alert('Device is rebooting...\\n\\nReconnect to WiFi to continue editing.'); } else { alert('Unexpected response: ' + xhr.status + ' ' + xhr.responseText); } } }; xhr.onerror = function() { alert('Device is rebooting...\\n\\nReconnect to WiFi to continue editing.'); }; xhr.ontimeout = function() { alert('Request timed out.\\nDevice may be rebooting or unresponsive.'); }; xhr.send(formData); } catch(e) { alert('JS Error: ' + e.message); } }
        function updBtnCount(count) {
            presetData.system.buttonCount = count;
            for (var p = 0; p < presetData.presets.length; p++) {
                var btns = presetData.presets[p].buttons;
                while (btns.length < count) btns.push(createDefaultButton(btns.length));
                while (btns.length > count) btns.pop();
            }
            if (selectedBtn >= count) selectedBtn = count - 1;
            render();
        }
        function createDefaultButton(idx) {
            var colors = ['#ff6b6b', '#feca57', '#48dbfb', '#1dd1a1', '#5f27cd', '#ff9ff3', '#54a0ff', '#00d2d3'];
            var c = colors[idx % colors.length];
            return {
                name: 'BTN' + (idx + 1), ledMode: 'MOMENTARY', inSelectionGroup: false,
                messages: [{ action: 'PRESS', type: 'CC', channel: 1, data1: 20 + idx, data2: 127, rgb: c }]
            };
        }

        function exportJSON() { document.getElementById('jsonArea').value = JSON.stringify(presetData, null, 2); }
        function importJSON() { try { var data = JSON.parse(document.getElementById('jsonArea').value); if (data.presets) { presetData = data; selectedBtn = 0; showSystem = false; render(); alert('Imported!'); } else alert('Invalid format'); } catch (e) { alert('Error: ' + e.message); } }
        function copyJSON() { exportJSON(); navigator.clipboard.writeText(document.getElementById('jsonArea').value); alert('Copied!'); }
        function downloadJSON() { exportJSON(); var blob = new Blob([document.getElementById('jsonArea').value], { type: 'application/json' }); var a = document.createElement('a'); a.href = URL.createObjectURL(blob); a.download = 'chocotone_config.json'; a.click(); }

        function testPost() { var xhr = new XMLHttpRequest(); xhr.open('POST', '/test', true); xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded'); xhr.onreadystatechange = function() { if (xhr.readyState === 4) { alert('Test: ' + xhr.status + ' ' + xhr.responseText); } }; xhr.send('json_data=test123'); } window.addEventListener('DOMContentLoaded', () => { fetch('/export').then(r => r.json()).then(d => { if(d && d.presets) { presetData = d; if(!d.system.ledMap) d.system.ledMap = [0,1,2,3,7,6,5,4,8,9]; render(); } else { render(); } }).catch(e => { console.error('Fetch error', e); render(); }); });
    </script>
</body>

</html>)raw";