---
description: How to update templates with new editor features
---
# Updating Templates Workflow

When new features are added to the editor (e.g., new message properties, new LED modes), follow these steps to update the templates in `devices_database.js`:

## 1. Identify New Properties

Check what new properties have been added to:
- **Button config**: `ledMode`, `inSelectionGroup`, etc.
- **Message config**: `action`, `type`, `channel`, `data1`, `data2`, `rgb`, `label`, `partner`, `sysex`, `holdMs`, etc.
- **Preset config**: `presetLedMode`, `syncMode`
- **System config**: `bleMode`, `brightness`, etc.

## 2. Update FULL_CONFIGS Templates

Location: `web_tools/devices_database.js` → `FULL_CONFIGS` object (around line 542)

For each full configuration template:
1. Add any new required properties with appropriate default values
2. Ensure all `messages[]` entries have the new properties
3. If a property is optional, only add it where relevant

### Example: Adding a new `label` property to messages
```javascript
// OLD
{ action: "PRESS", type: "CC", channel: 1, data1: 43, data2: 127, rgb: "#ffffff" }

// NEW (with label property)
{ action: "PRESS", type: "CC", channel: 1, data1: 43, data2: 127, rgb: "#ffffff", label: "" }
```

## 3. Update Device Partial Templates

Location: `web_tools/devices_database.js` → `DEVICE_DATABASE[device].templates` 

Partial templates are simpler (name, cc, color, special) so they rarely need updates.

## 4. Test Retrocompatibility 

Ensure `migrateOldData()` in the editor HTML handles old configs:
- Location: `chocotone_midi_editor_v1.5_beta.html` → search for `migrateOldData`
- Add fallback defaults for any new properties

### Example: Adding migration for new `partner` property
```javascript
if (msg.partner === undefined) msg.partner = 0;
```

## 5. Validate

1. Load the editor
2. Try loading each template via Templates card
3. Verify all properties appear correctly
4. Export JSON and verify structure

## Current Template List

**Full Configurations** (in FULL_CONFIGS):
- Default (8-btn SPM)
- Wisut (10-btn SPM)
- GP5 Wisut Profile

**Device Partial Templates** (per-device):
- Sonicake Pocket Master: STOMP, Full Signal Chain, Bank Selector, Wisut (10-btn)
- Valeton GP-5: STOMP Mode (CC), Full Chain (CC), Bank Selector, GP5 SysEx (Legacy)
- Hotone Ampero 2 Stomp: 6-Slot STOMP, Scene Selector, Full 12 Slots
- Generic MIDI Device: (none)
