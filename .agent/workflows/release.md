---
description: Release checklist for publishing a new version
---

# Release Checklist

When updating the version number of Chocotone, make sure to update ALL of the following locations:

## Version Locations

1. **CHANGELOG.md** - Add new version entry at the top
   - Format: `## [vX.Y] - YYYY-MM-DD`

2. **Chocotone.ino** - OLED startup screen version badge (line ~58)
   - `display.print(F("vX.Y"));`
   - Also update the comment: `// === Chocotone vX.Y Loading Screen ===`

3. **BleMidi.cpp** - BLE GET_VERSION response (line ~1092)
   - `sendBleConfigResponse("Chocotone vX.Y");`

## Git Commands

```bash
git add -A
git commit -m "vX.Y: [Brief description of changes]"
git push
```

## Reminder
Always update all 3 locations when bumping the version number!
