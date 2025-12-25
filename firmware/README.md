# Firmware Directory

This directory contains pre-compiled firmware binaries for the Chocotone MIDI Controller.

## Files

- `chocotone.bin` - Merged firmware binary for web flashing

## Creating the .bin File

To generate the merged binary for web flashing:

### Using Arduino IDE

1. Open `Chocotone/Chocotone.ino` in Arduino IDE
2. Go to **Sketch → Export Compiled Binary**
3. Find the `.bin` file in the sketch folder
4. Use esptool to create a merged binary:

```bash
esptool.py --chip esp32 merge_bin \
  -o firmware/chocotone.bin \
  --flash_mode dio \
  --flash_freq 40m \
  --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 Chocotone.ino.bin
```

### Using PlatformIO

```bash
pio run
# Creates .pio/build/esp32dev/firmware.bin
```

Then merge with esptool as shown above.

## Web Installer

The `chocotone.bin` file is used by the [Web Installer](../installer.html) for browser-based flashing.
