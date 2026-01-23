---
description: How to add new SysEx Scroll parameter lists to the firmware
---

# Adding SysEx Scroll Parameters

This workflow guides you through adding new SysEx parameter lists (like PITCH - HIGH) to the Chocotone firmware.

## Prerequisites
- PocketEdit or similar tool to capture SysEx messages
- The parameter you want to control must send discrete SysEx messages for each value

## Step 1: Capture SysEx Messages from PocketEdit

1. Open PocketEdit and connect to your SPM/GP-5
2. Open the Communication Log
3. Move the parameter slider through all values (e.g., 0-24 for PITCH - HIGH)
4. Copy the "Sending:" lines from the log

**Example output:**
```
Sending: 8080F0010B00010000000E01010408000600000000000000000000000000000000000000000000F7
Sending: 8080F0010000010000000E0101040800060000000000000000000000000000000000000800030FF7
...
```

## Step 2: Add Parameter ID to Firmware

Edit `SysexScrollData.h`, add your new parameter ID:

```cpp
enum SysexScrollParamId : uint8_t {
  SYSEX_PARAM_NONE = 0,
  SYSEX_PARAM_PITCH_HIGH = 1,
  SYSEX_PARAM_YOUR_NEW_PARAM = 2  // <-- Add here
};
```

## Step 3: Add SysEx Data Array

Add a new PROGMEM array with your captured messages:

```cpp
#define YOUR_PARAM_LIST_SIZE 25  // Number of values
#define YOUR_PARAM_MSG_LEN 41    // Message length (usually 41 bytes)

const uint8_t PROGMEM YOUR_PARAM_DATA[YOUR_PARAM_LIST_SIZE * YOUR_PARAM_MSG_LEN] = {
  // 0: <paste hex string here>
  0x80,0x80,0xF0,... // Convert hex string to bytes
  // 1: ...
  // Repeat for all values
};
```

**Conversion tip:** Use this pattern to convert hex strings:
- `8080F001...` → `0x80,0x80,0xF0,0x01,...`
- Each pair of hex chars = one byte

**Important:** 
- Each message MUST be exactly `YOUR_PARAM_MSG_LEN` bytes
- If a message is shorter, pad with `0x00` at the end
- The firmware finds the actual F7 terminator when sending

## Step 4: Register the List

Add your list to the `sysexScrollLists[]` array:

```cpp
const SysexScrollList sysexScrollLists[] = {
    {SYSEX_PARAM_PITCH_HIGH, PITCH_HIGH_LIST_SIZE, PITCH_HIGH_MSG_LEN, PITCH_HIGH_DATA},
    {SYSEX_PARAM_YOUR_NEW_PARAM, YOUR_PARAM_LIST_SIZE, YOUR_PARAM_MSG_LEN, YOUR_PARAM_DATA}  // <-- Add
};
```

## Step 5: Add to Editor

Edit `devices_database.js`, add your parameter:

```javascript
const SYSEX_SCROLL_PARAMS = {
  'Pocket Master': {
    'PITCH - HIGH': [...],
    'YOUR NEW PARAM': [...]  // <-- Add here
  }
};
```

Edit `chocotone_midi_editor_v1.5_beta.html`, update the `sysexParamToId` mapping:

```javascript
var sysexParamToId = {
    'Pocket Master|PITCH - HIGH': 1,
    'Pocket Master|YOUR NEW PARAM': 2  // <-- Add here
};
```

## Step 6: Recompile and Test

1. Recompile firmware with Arduino IDE
2. Flash to ESP32
3. In editor, select your new parameter for an analog input
4. Export config and send to device
5. Test with your analog input!

## Troubleshooting

- **Message not recognized**: Check that all bytes are correct, especially the F7 terminator
- **Wrong parameter triggered**: Verify the paramId in config matches the enum value
- **No response from pedal**: Ensure BLE is connected and check serial monitor for debug output
