---
description: Release checklist for publishing a new version
---

# Release Checklist

When releasing a new version of Chocotone, follow these steps:

## 1. Update Version Numbers

Update ALL of the following locations:

| File | Location | Format |
|------|----------|--------|
| `CHANGELOG.md` | Top of file | `## [vX.Y] - YYYY-MM-DD` |
| `Chocotone.ino` | Line ~58 | `display.print(F("vX.Y"));` |
| `BleMidi.cpp` | Line ~1092 | `sendBleConfigResponse("Chocotone vX.Y");` |
| `manifest.json` | Line 3 | `"version": "X.Y.0",` |

## 2. Push to GitHub

// turbo-all
```bash
git add -A
git commit -m "vX.Y: [Brief description]"
git tag vX.Y
git push origin main --tags
```

## 3. Automatic Build (GitHub Actions)

After pushing, GitHub Actions will automatically:
- ✅ Compile the firmware
- ✅ Create `firmware/chocotone_vX.Y.0.bin`
- ✅ Update `firmware/index.json` with new version
- ✅ Commit the binary back to the repo

**Wait ~5 minutes** for the workflow to complete.

## 4. Create GitHub Release (Optional)

1. Go to GitHub → Releases → Draft new release
2. Choose tag: `vX.Y`
3. Title: `Chocotone vX.Y`
4. Description: Copy from CHANGELOG.md
5. The .bin is already in the repo, no need to attach
6. Publish release

## Quick Checklist

- [ ] CHANGELOG.md updated
- [ ] Chocotone.ino version updated
- [ ] BleMidi.cpp version updated  
- [ ] manifest.json version updated
- [ ] Code pushed to GitHub
- [ ] GitHub Actions completed successfully
- [ ] Installer shows new version in dropdown
