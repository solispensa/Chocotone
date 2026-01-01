---
description: Release checklist for publishing a new version
---

# Release Checklist

When releasing a new version of Chocotone:

## 1. Update Version Numbers

| File | Location | Format |
|------|----------|--------|
| `CHANGELOG.md` | Top of file | `## [vX.Y] - YYYY-MM-DD` |
| `Chocotone.ino` | Line ~58 | `display.print(F("vX.Y"));` |
| `BleMidi.cpp` | Line ~1092 | `sendBleConfigResponse("Chocotone vX.Y");` |
| `manifest.json` | Line 3 | `"version": "X.Y.0",` |

## 2. Build Firmware Locally

// turbo
```powershell
.\scripts\build-firmware.ps1
```

This will:
1. Guide you through Arduino IDE compilation
2. Copy the .bin to `firmware/chocotone_vX.Y.0.bin`
3. Update `firmware/index.json`

## 3. Push to GitHub

// turbo-all
```bash
git add -A
git commit -m "vX.Y: [Brief description]"
git push origin main
```

## Quick Checklist

- [ ] CHANGELOG.md updated
- [ ] Chocotone.ino version updated
- [ ] BleMidi.cpp version updated  
- [ ] manifest.json version updated
- [ ] `.\scripts\build-firmware.ps1` run
- [ ] Code pushed to GitHub
- [ ] Installer shows new version in dropdown
- [ ] Verify OLED layouts (128x64 & 128x32) in Editor Preview
