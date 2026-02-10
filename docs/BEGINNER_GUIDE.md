# Chocotone Beginner's Guide

**A step-by-step tutorial for first-time ESP32 users**

Welcome! This guide will walk you through building and setting up your Chocotone MIDI controller, even if you've never worked with electronics or code before.

---

## 📋 What You'll Need

### Hardware Shopping List

| Item | What to Search | Approx. Cost |
|------|----------------|--------------|
| ESP32 Board | "ESP32 DevKit" (Classic) or "ESP32-S3 DevKit" (Best!) | $5-15 |
| Display | "0.96 OLED I2C" or "1.44 TFT SPI ST7735" | $3-8 |
| Rotary Encoder | "KY-040 Rotary Encoder" or "EC11" | $1-2 |
| Tactile Buttons | "6x6mm tactile button" (get 10+) | $2-3 |
| NeoPixel LEDs | "WS2812B LED strip" (8+ LEDs) | $3-8 |
| Jumper Wires | "Dupont jumper wires" | $2-3 |
| Breadboard | "400 point breadboard" | $2-3 |
| USB Cable | "Micro USB data cable" | $2-3 |

**Total: ~$20-40 USD**

> 💡 **Tip**: Search on AliExpress, Amazon, or your local electronics store. Most items ship from China and take 2-4 weeks.

---

## 🤔 What is ESP32?

**ESP32** is a tiny, affordable computer chip that powers your Chocotone controller. Think of it as the "brain" of your device.

**Key features:**
- 🔵 **Bluetooth** - Connects wirelessly to your amp/pedal
- 📶 **WiFi** - Configure settings from your phone/computer
- ⚡ **Fast** - Responds instantly to button presses.
- 🔌 **USB MIDI (S3 Only)** - The newer **ESP32-S3** version acts as a true MIDI device on your computer!
- 💰 **Cheap** - Only ~$5-10!

The **firmware** is the software that runs on the ESP32 — it's what makes your Chocotone work. We've already written all the code for you!

---

## 🔧 Hardware Assembly

### Step 1: Prepare Your Workspace

You'll need:
- A flat, well-lit surface
- Your components
- A computer with USB ports

### Step 2: Identify Your ESP32 Pins

Your ESP32 board has many numbered pins. Here's what we'll use:

```
ESP32 Pin → Component
─────────────────────
GPIO 21   → OLED SDA (data)
GPIO 22   → OLED SCL (clock)
GPIO 18   → Encoder CLK
GPIO 19   → Encoder DT
GPIO 23   → Encoder Button
GPIO 5    → NeoPixel Data
GPIO 14   → Button 1
GPIO 27   → Button 2
GPIO 26   → Button 3
GPIO 25   → Button 4
GPIO 33   → Button 5
GPIO 32   → Button 6
GPIO 16   → Button 7
GPIO 17   → Button 8
3.3V      → Power for components
GND       → Ground (shared by all)
```

### Step 3: Connect the OLED Display

The OLED has 4 pins:
1. **VCC** → Connect to ESP32 **3.3V**
2. **GND** → Connect to ESP32 **GND**
3. **SCL** → Connect to ESP32 **GPIO 22**
4. **SDA** → Connect to ESP32 **GPIO 21**

### Step 4: Connect the Rotary Encoder

The encoder has 5 pins:
1. **GND** → Connect to ESP32 **GND**
2. **+** (VCC) → Connect to ESP32 **3.3V**
3. **SW** → Connect to ESP32 **GPIO 23**
4. **DT** → Connect to ESP32 **GPIO 19**
5. **CLK** → Connect to ESP32 **GPIO 18**

### Step 5: Connect the Buttons

Each button has 2 legs. Connect:
- **One leg** → ESP32 GPIO pin (see table above)
- **Other leg** → ESP32 **GND**

Repeat for all 8 buttons (or however many you want, 4-10 supported).

### Step 6: Connect the NeoPixel LEDs

The LED strip has 3 wires:
1. **5V/VCC** (red) → Connect to ESP32 **VIN** or **5V** pin
2. **GND** (black/white) → Connect to ESP32 **GND**
3. **DIN** (green) → Connect to ESP32 **GPIO 5**

### Step 7: Connect Analog Inputs (Expression Pedals)

If you want to use an expression pedal (like a volume or wah pedal):
- **VCC** → Connect to ESP32 **3.3V**
- **GND** → Connect to ESP32 **GND**
- **Signal** → Connect to an Analog Pin (e.g., **GPIO 36 or 39**)

> 💡 **Tip**: We support **Custom Response Curves** (Linear, Log, Exp) so even a cheap pedal can feel like a high-end one!

---

## 💾 Installing the Firmware

This is the easiest part! We've created a web-based installer that works right in your browser.

### Method 1: Web Installer (Recommended)

1. **Open the installer page**: [Chocotone Web Installer](../installer.html)
   
2. **Connect your ESP32** to your computer with a USB cable

3. **Click "Install Chocotone"** and select your ESP32 from the list

4. **Wait ~30 seconds** for the firmware to install

5. **Done!** Your OLED should show "Chocotone" when complete

> 🌐 **Browser Support**: Use Chrome, Edge, or Brave. Firefox and Safari won't work.
> 🔌 **ESP32-S3 Tip**: If you are using the S3 board, it will show up as "ESP32-S3 USB MIDI" on your computer once flashed!

### Method 2: Arduino IDE (Advanced)

If the web installer doesn't work, you can use Arduino IDE:

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)

2. Add ESP32 board support:
   - Go to **File → Preferences**
   - Add this URL to "Additional Boards Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools → Board → Boards Manager**
   - Search "ESP32" and install "**esp32 by Espressif Systems** version **3.1.2**"

   > [!IMPORTANT]
   > Install **v3.1.2 specifically**. Newer versions may cause LED/USB conflicts on ESP32-S3.

3. Install required libraries via **Sketch → Include Library → Manage Libraries**:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - Adafruit NeoPixel (v1.12.0+)
   - ESP32Encoder
   - ArduinoJson

4. Download the [Chocotone source code](https://github.com/solispensa/Chocotone)

5. Open `Chocotone/Chocotone.ino` in Arduino IDE

6. Select your board: **Tools → Board → ESP32 Dev Module**

7. Select your port: **Tools → Port → COM# (your ESP32)**

8. Click **Upload** (→ arrow button)

---

## 🎮 First-Time Setup

After flashing, your Chocotone will start in default mode:

### 1. Check the Display

You should see:
- Preset name (e.g., "PRESET 1")
- Button labels
- Connection status

### 2. Open the Chocotone Editor

1. Keep your ESP32 **connected via USB** to your computer
2. Open the **[Chocotone Editor](https://solispensa.github.io/Chocotone/chocotone_midi_editor.html)** in Chrome, Edge, or Brave
3. Click **Connect USB** and select your ESP32 from the list
4. Click **Read Config** to load the current configuration

### 3. Configure Your Controller

The editor lets you:
- **Rename buttons** (e.g., "DRIVE", "DELAY", "CHORUS")
- **Set MIDI messages** for each button
- **Change LED colors**
- **Adjust settings**

### 4. Save Your Configuration

1. Click **Write Config** to send changes to your Chocotone
2. Your controller will save and reboot automatically
3. Done! Your configuration is now stored on the device

---

## 🔗 Connecting to Your Amp/Pedal

Chocotone connects via **Bluetooth MIDI** and supports three connection modes:

### BLE Modes Explained

| Mode | Description | Use Case |
|------|-------------|----------|
| **CLIENT** | Chocotone connects TO your device | Controlling amps/pedals (e.g., Sonicake Pocket Master, Hotone Ampero) |
| **SERVER** | Other devices connect TO Chocotone | DAWs, mobile apps, or computers connecting to your controller |
| **DUAL** | Both modes active simultaneously | Control your pedal AND send MIDI to your DAW at the same time |

### Connecting to a Pedal/Amp (CLIENT Mode - Default)

1. Turn on your Bluetooth MIDI device (e.g., Sonicake Pocket Master)
2. Make sure it's in pairing mode
3. Chocotone will automatically scan and connect
4. When connected, you'll see **"SPM:Y"** on the display

Now press buttons on your Chocotone — they'll control your amp!

### Native USB MIDI (ESP32-S3 only)
If you have an **ESP32-S3**, you can use **Native USB MIDI** for the lowest latency connection to your DAW:

1. **Long-press encoder** to enter the menu
2. Navigate to **MIDI Mode** and select **USB**
3. Select **Save and Exit** (Chocotone will reboot)
4. Plug the USB cable into your computer
5. Your DAW will see **"CHOCOTONE USB"** as a MIDI device

> [!NOTE]
> LEDs are disabled in USB mode due to an RMT hardware conflict. BLE is also disabled in USB mode.

### Connecting to DAW/Apps (SERVER Mode)

1. Change BLE mode: **Long-press encoder** → Menu → **BLE Mode** → Select **SERVER**
2. Save and exit (Chocotone will reboot)
3. On your computer/phone, look for **"Chocotone"** in MIDI devices
4. Connect from your DAW or app
5. When connected, you'll see **"DAW:Y"** on the display

### Using Both (DUAL Mode)

1. Change BLE mode to **DUAL** from the menu
2. Save and exit (Chocotone will reboot)
3. Chocotone will connect to your pedal AND accept DAW connections
4. Perfect for live performance with backing tracks!

> 💡 **Tip**: Most users should start with **CLIENT** mode (default). Switch to DUAL if you need to control both your amp and a DAW.

---

## ❓ Troubleshooting

### ESP32 won't connect to computer
- Try a **different USB cable** (some are charge-only, no data)
- Try a **different USB port**
- Install [CP210x drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) if on Windows

### OLED screen doesn't turn on
- Double-check wiring (SDA ↔ GPIO 21, SCL ↔ GPIO 22)
- Make sure VCC is connected to 3.3V, not 5V
- Try swapping SDA and SCL wires

### Buttons aren't responding
- Check wiring to GND
- Make sure you're pressing the correct buttons
- Adjust debounce in menu: **Menu → Pad Debounce**

### LEDs don't light up
- Check you connected to **DIN**, not DOUT
- Make sure LED power is connected to **5V/VIN**
- Check the NeoPixel pin setting in web config

### Bluetooth won't connect
- Power cycle both devices
- Try **Menu → Clear BLE Bonds** on Chocotone
- Make sure the target device is in pairing mode

---

## 🎉 You Did It!

Congratulations on building your Chocotone controller! 

**Next steps:**
- Join our [Facebook Community](https://www.facebook.com/groups/1950814575859830/) for help and ideas
- Explore the [Web Editor](../chocotone_midi_editor.html) for advanced configuration
- Share photos of your build with the community!

---

**Happy MIDI controlling!** 🎵🎛️
