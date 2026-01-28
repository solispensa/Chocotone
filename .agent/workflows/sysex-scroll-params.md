---
description: How to add new SysEx Scroll parameter lists to the firmware
---

# Adding SysEx Scroll Parameters

This workflow guides you through adding new SysEx parameter lists (like PITCH - HIGH, DRV - GAIN) to the Chocotone firmware. Parameters are device-agnostic and work with any compatible pedal (Pocket Master, Valeton GP-5, etc.).

## Prerequisites
- PocketEdit or similar tool to capture SysEx messages
- The parameter you want to control must send discrete SysEx messages for each value

## Step 1: Capture SysEx Messages from PocketEdit

1. Open PocketEdit and connect to your SPM/GP-5
2. Open the Communication Log
3. Move the parameter slider through all values (e.g., 0-24 for PITCH, 0-100 for GAIN)
4. Copy the "Sending:" lines from the log

**Example output:**
```
Sending: 8080F0010B00010000000E01010408000600000000000000000000000000000000000000000000F7
Sending: 8080F0010000010000000E0101040800060000000000000000000000000000000000000800030FF7
...
```

## Step 2: Add Parameter ID to Firmware

Edit `SysexScrollData.h`, add your new parameter ID to the enum:

```cpp
enum SysexScrollParamId : uint8_t {
  SYSEX_PARAM_NONE = 0,
  SYSEX_PARAM_PITCH_HIGH = 1,
  SYSEX_PARAM_DRV_GAIN = 2,
  SYSEX_PARAM_YOUR_NEW_PARAM = 3  // <-- Add here with next available ID
};
```

## Step 3: Create SysEx Data Header File

For large parameter lists (50+ messages), create a separate header file to keep the code organized:

**File: `SysexScrollYourParam.h`**
```cpp
// YOUR_PARAM SysEx data - Auto-generated
// Description of this parameter

#define YOUR_PARAM_LIST_SIZE 101  // Number of values (e.g., 0-100)
#define YOUR_PARAM_MSG_LEN 40     // Length of EACH message (EXACT byte count)

const uint8_t PROGMEM YOUR_PARAM_DATA[YOUR_PARAM_LIST_SIZE * YOUR_PARAM_MSG_LEN] = {
  // 0: 0x80,0x80,F0,01,0B,...F7
  0x80,0x80,0xF0,0x01,...,0xF7, 
  // 1: ...
  // Repeat for all values. 
};
```

> [!IMPORTANT]
> **File Encoding**: The header file MUST be saved with **ASCII** or **UTF-8** encoding. If saved as UTF-16 (default for some Windows tools), the compiler will throw "stray '#'" errors.

**Conversion tip:** Use this pattern to convert hex strings:
- `8080F001...F7` → `0x80,0x80,0xF0,0x01,...,0xF7`
- Each pair of hex chars = one byte
- **DO NOT** add any trailing `0x00` padding after the `0xF7` terminator. The message length MUST be the exact number of bytes from the first `0x80` to the `0xF7`. 
- Every message in the array must be identical in length (`YOUR_PARAM_MSG_LEN`).

**Important:**
- Each message MUST be exactly `YOUR_PARAM_MSG_LEN` bytes
- The firmware finds the actual F7 terminator when sending

## Step 3a: Verify Message Length

Before proceeding, double-check your data:
1.  Count the bytes in one of your hex strings (e.g., 40 bytes).
2.  Ensure your `YOUR_PARAM_MSG_LEN` definition matches this EXACT count.
3.  Ensure your array data does **not** have trailing `0x00` bytes unless they are part of the actual SysEx message before the F7.

> [!WARNING]
> Mismatched lengths or extra padding bytes (like `0xF7, 0x00`) will cause the firmware to send malformed SysEx messages.

## Step 4: Include and Register the List

Edit `SysexScrollData.h`:

1. Include your new header after existing data:
```cpp
// Include YOUR_PARAM data
#include "SysexScrollYourParam.h"
```

2. Add to the `sysexScrollLists[]` array:
```cpp
const SysexScrollList sysexScrollLists[] = {
    {SYSEX_PARAM_PITCH_HIGH, PITCH_HIGH_LIST_SIZE, PITCH_HIGH_MSG_LEN, PITCH_HIGH_DATA},
    {SYSEX_PARAM_DRV_GAIN, DRV_GAIN_LIST_SIZE, DRV_GAIN_MSG_LEN, DRV_GAIN_DATA},
    {SYSEX_PARAM_YOUR_NEW_PARAM, YOUR_PARAM_LIST_SIZE, YOUR_PARAM_MSG_LEN, YOUR_PARAM_DATA}  // <-- Add
};
```

## Step 5: Add to Editor

Edit `web_tools/devices_database.js`, add your parameter to `SYSEX_SCROLL_PARAMS`:

```javascript
const SYSEX_SCROLL_PARAMS = {
    "PITCH - HIGH": new Array(25).fill(""),    // Firmware ID: 1
    "DRV - GAIN": new Array(101).fill(""),     // Firmware ID: 2
    "YOUR NEW PARAM": new Array(101).fill("")  // Firmware ID: 3  <-- Add here
};
```

Edit `chocotone_midi_editor_v1.5_beta.html`:

1. Update the **single global** `sysexParamToId` mapping (found shortly after `fillEmptyPresets()`):

```javascript
var sysexParamToId = {
    'PITCH - HIGH': 1,
    'DRV - GAIN': 2,
    'DLY - FBK': 3,
    'YOUR NEW PARAM': 4  // <-- Add here, must match firmware enum
};
```

> [!TIP]
> This map is now used globally by `updMsg` and `updAnalogMsg` to synchronize parameter IDs during live edits.

2. Update the `idToParam` maps inside `normalizeConfigData`. There are **two** identical locations (one for buttons, one for analog inputs) that ensure existing configs import correctly:

```javascript
var idToParam = { 1: 'PITCH - HIGH', 2: 'DRV - GAIN', 3: 'DLY - FBK', 4: 'YOUR NEW PARAM' };
```

## Step 6: Recompile and Test

1. Recompile firmware with Arduino IDE
2. Flash to ESP32
3. Hard refresh editor (Ctrl+Shift+R)
4. Configure an analog input with SYSEX_SCROLL type
5. Select your new parameter from the dropdown
6. Export config and send to device
7. Test with your analog input!

## Troubleshooting

- **Message not recognized**: Check that all bytes are correct, especially the F7 terminator
- **Wrong parameter triggered**: Verify the sysexParamToId in editor matches the enum value in firmware
- **No response from pedal**: Ensure BLE is connected and check serial monitor for debug output
- **Data1 shows 0 in debug**: The sysexParam wasn't set; make sure dropdown selection is saved

## Current Parameters

| Parameter | Firmware ID | Message Count | Description |
|-----------|-------------|---------------|-------------|
| PITCH - HIGH | 1 | 25 (0-24) | Pitch shifter semitones |
| DRV - GAIN | 2 | 101 (0-100) | Drive/Distortion gain level |
| DLY - FBK | 3 | 101 (0-100) | Delay Feedback |
| FX1 - RATE | 4 | 100 (0.1-10) | FX1 Modulation Rate |
| RVB - MIX | 5 | 101 (0-100) | Reverb Mix level |
| AMP - GAIN | 6 | 101 (0-100) | Amplifier Gain level |
| PITCH - LOW | 7 | 25 (-24 to 0) | Low Pitch Shifter |
