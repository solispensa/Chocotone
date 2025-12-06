# Adding Photos and Screenshots to Your Repository

This guide shows you how to add photos of your hardware build and screenshots to the documentation.

## Step 1: Take Your Photos

### Photos to Take

**Hardware Photos:**
1. **Full build** - Complete assembled controller from front
2. **Top view** - Showing button layout and encoder
3. **Wiring** - Close-up of ESP32 connections (helpful for others)
4. **Components** - Individual components before assembly (optional)

**Screenshots:**
1. **OLED Display** - Normal preset mode showing button labels
2. **OLED Tap Tempo** - Tap tempo mode with BPM display
3. **OLED Menu** - Menu system
4. **Web Interface** - Main configuration page at http://192.168.4.1

### Photo Tips
- Use good lighting
- Keep background clean/plain
- Take photos horizontally (landscape mode)
- Resolution: 1920x1080 or lower is fine
- Format: JPG or PNG

## Step 2: Save Photos to Repository

### Create Images Directory

1. Open PowerShell in your project folder:
   ```powershell
   cd "path\to\your\project"
   New-Item -ItemType Directory -Path "docs\images" -ErrorAction SilentlyContinue
   ```

2. Copy your photos to `docs\images\` folder

### Recommended File Names

```
docs/images/
├── build-front.jpg          # Full assembled controller
├── build-top.jpg            # Top view with buttons
├── build-wiring.jpg         # Wiring close-up
├── oled-preset-mode.jpg     # OLED in preset mode
├── oled-tap-tempo.jpg       # OLED in tap tempo mode
├── oled-menu.jpg            # OLED menu
└── web-interface.png        # Web configuration page
```

## Step 3: Add Photos to README

Edit `README.md` and add an Images section after the feature list:

```markdown
## Gallery

### Hardware Build

![Chocotone Controller](docs/images/build-front.jpg)
*Complete assembled controller with 8 buttons, OLED display, and rotary encoder*

![Top View](docs/images/build-top.jpg)
*Button layout with NeoPixel LEDs*

### Display Modes

![Preset Mode](docs/images/oled-preset-mode.jpg)
*OLED showing preset name and button labels*

![Tap Tempo](docs/images/oled-tap-tempo.jpg)
*Tap tempo mode with BPM and rhythm pattern*

### Web Interface

![Web Configuration](docs/images/web-interface.png)
*Browser-based MIDI mapping configuration at 192.168.4.1*
```

## Step 4: Capture OLED Screenshots

Since the OLED is tiny, use these methods:

### Method 1: Photo with Phone
- Take clear photo of OLED screen
- Crop tightly around display
- Increase contrast/brightness in photo editor

### Method 2: Screen Capture (if using Serial Monitor)
If your code outputs screen content to Serial, screenshot that instead.

### Method 3: Edit Config and Photo
For clearest display photos:
1. Temporarily increase button name font size
2. Or create a special "demo" preset with clear labels
3. Take photo
4. Revert changes

## Step 5: Capture Web Interface Screenshot

1. Power on controller
2. Enable WiFi (long-press encoder → Wi-Fi Config)
3. Connect to `ESP32_MIDI_Config` WiFi
4. Open browser to `http://192.168.4.1`
5. Press `Windows + Shift + S` (Snipping Tool)
6. Capture web page
7. Save as `docs/images/web-interface.png`

## Step 6: Optional - Add Hero Image

For maximum impact, create a hero image at the top of README:

```markdown
# Chocotone - BLE MIDI Controller for SPM

![Chocotone Controller](docs/images/hero.jpg)

An ESP32-based BLE MIDI controller...
```

**Hero image tips:**
- Landscape orientation (16:9 ratio ideal)
- Show complete setup in action
- Good lighting, professional look
- Resize to ~1200px width for web

## Step 7: Commit Images to Git

```bash
git add docs/images/
git commit -m "Add hardware photos and screenshots"
```

## Image Optimization (Optional)

Before committing, optimize images to reduce repository size:

### Using PowerShell (ImageMagick)
```powershell
# Install ImageMagick first (optional)
# Then resize large images:
magick convert docs/images/build-front.jpg -resize 1200x900 docs/images/build-front.jpg
```

### Manual Optimization
- Use https://tinypng.com/ for PNG files
- Use https://compressjpeg.com/ for JPG files
- Target: <500KB per image

## Embedding Images in Markdown

```markdown
# Basic image
![Alt text](path/to/image.jpg)

# Image with caption
![Controller front view](docs/images/build-front.jpg)
*Caption text appears in italics below*

# Image with link
[![Controller](docs/images/build-front.jpg)](docs/images/build-front-hires.jpg)
*Click for high-res version*

# Resize in HTML (if needed)
<img src="docs/images/build-front.jpg" width="600" alt="Controller">
```

## What NOT to Include

❌ **Don't add:**
- Photos with personal information visible
- Images showing WiFi passwords or device names
- Photos of proprietary equipment internals
- Very large files (>2MB each)

## Example Gallery Layout

Here's a complete gallery section you can adapt:

```markdown
## Gallery

### 📸 Hardware Build

<div align="center">

![Chocotone Front](docs/images/build-front.jpg)

*Assembled Chocotone controller with 8 RGB buttons and OLED display*

</div>

---

### 🖥️ Display Modes

| Preset Mode | Tap Tempo | Menu |
|-------------|-----------|------|
| ![Preset](docs/images/oled-preset-mode.jpg) | ![Tap](docs/images/oled-tap-tempo.jpg) | ![Menu](docs/images/oled-menu.jpg) |

---

### 🌐 Web Configuration

![Web Interface](docs/images/web-interface.png)

*Full MIDI mapping editor accessible via WiFi at http://192.168.4.1*
```

## After Adding Photos

1. Update README with image links
2. Test images display correctly on GitHub (after pushing)
3. Update CHANGELOG.md mentioning visual documentation added
4. Consider adding to hardware guide as well

---

**Questions?** Just ask! I can help you format the markdown after you add your photos.
