#include "UI_Display.h"
#include "AnalogInput.h"
#include "SysexScrollData.h"
#include <SPI.h> // For TFT displays
#include <Wire.h>

// Color abstraction
#define DISPLAY_WHITE                                                          \
  ((oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160)          \
       ? ST7735_WHITE                                                          \
       : SSD1306_WHITE)
#define DISPLAY_BLACK                                                          \
  ((oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160)          \
       ? ST7735_BLACK                                                          \
       : SSD1306_BLACK)

// Helper to flush display (OLED needs this, TFT doesn't or at least not the
// same way)
void flushDisplay() {
  if (displayPtr == nullptr)
    return;
  if (oledConfig.type != TFT_128X128 && oledConfig.type != TFT_128X160) {
    static_cast<Adafruit_SSD1306 *>(displayPtr)->display();
  }
}

// Helper to clear display buffer
void clearDisplayBuffer() {
  if (displayPtr == nullptr)
    return;
  if (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160) {
    displayPtr->fillScreen(ST7735_BLACK);
  } else {
    static_cast<Adafruit_SSD1306 *>(displayPtr)->clearDisplay();
  }
}

// ============================================
// BATTERY MONITORING (v1.5)
// ============================================

// Read battery voltage and update percentage (0-100)
// Auto-calibrates by tracking the highest and lowest ADC readings seen.
// Percentage is linearly mapped between min and max.
void updateBatteryLevel() {
  if (systemConfig.batteryAdcPin == 0)
    return;
  if (millis() - lastBatteryRead < 5000)
    return; // Read every 5 seconds

  lastBatteryRead = millis();

  // Take multiple samples and average to reduce fluctuation
  long adcSum = 0;
  for (int i = 0; i < 16; i++) {
    adcSum += analogRead(systemConfig.batteryAdcPin);
    delayMicroseconds(500);
  }
  int rawAdc = adcSum / 16;

  // Auto-calibrate: track highest and lowest readings
  bool calibrationChanged = false;
  if (rawAdc > batteryAdcMax) {
    batteryAdcMax = rawAdc;
    calibrationChanged = true;
  }
  if (rawAdc < batteryAdcMin) {
    batteryAdcMin = rawAdc;
    calibrationChanged = true;
  }

  // Calculate percentage from calibrated range
  int range = batteryAdcMax - batteryAdcMin;
  if (range > 50) {
    // Enough spread for meaningful percentage
    batteryPercent = ((rawAdc - batteryAdcMin) * 100) / range;
    if (batteryPercent > 100)
      batteryPercent = 100;
    if (batteryPercent < 0)
      batteryPercent = 0;
  } else {
    // Not enough calibration data yet - show 100% until range builds up
    batteryPercent = 100;
  }

  // Save calibration to NVS periodically (every 5 minutes) to avoid flash wear
  static unsigned long lastCalibrationSave = 0;
  if (calibrationChanged && (millis() - lastCalibrationSave > 300000)) {
    lastCalibrationSave = millis();
    Preferences prefs;
    if (prefs.begin("midi_presets", false)) {
      prefs.putInt("s_batMax", batteryAdcMax);
      prefs.putInt("s_batMin", batteryAdcMin);
      prefs.end();
      Serial.printf("[BAT] Calibration saved: min=%d max=%d\n", batteryAdcMin,
                    batteryAdcMax);
    }
  }

  Serial.printf("[BAT] Pin:%d ADC:%d -> %d%% (range: %d-%d)\n",
                systemConfig.batteryAdcPin, rawAdc, batteryPercent,
                batteryAdcMin, batteryAdcMax);
}

// Draw battery icon at specified position with scale factor
// Base size: 14x7 pixels, 4 divisions with black partition lines
// Uses white for OLED and green for TFT displays
void drawBatteryIcon(int x, int y, int scale) {
  if (displayPtr == nullptr)
    return;
  if (scale < 1)
    scale = 1;
  if (scale > 3)
    scale = 3;

  int w = 13 * scale;   // Main body width
  int h = 7 * scale;    // Main body height
  int tipW = 2 * scale; // Tip width
  int tipH = 3 * scale; // Tip height
  int innerW = w - 2;   // Interior fill width
  int innerH = h - 2;   // Interior fill height

  // Use green for TFT, white for OLED
  uint16_t battColor =
      (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160)
          ? 0x07E0
          : DISPLAY_WHITE;

  // Battery outline
  displayPtr->drawRect(x, y, w, h, battColor);
  // Battery tip (positive terminal)
  displayPtr->fillRect(x + w, y + (h - tipH) / 2, tipW, tipH, battColor);

  // Calculate proportional fill width based on percentage (loading bar style)
  int fillW = (innerW * batteryPercent) / 100;
  if (fillW > innerW)
    fillW = innerW;
  if (fillW < 0)
    fillW = 0;

  // Draw filled portion (proportional loading bar)
  if (fillW > 0) {
    displayPtr->fillRect(x + 1, y + 1, fillW, innerH, battColor);
  }
}

// MIDI Note Names
const char *MIDI_NOTE_NAMES[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                 "F#", "G",  "G#", "A",  "A#", "B"};

void midiNoteNumberToString(char *b, size_t s, int n) {
  if (n < 0 || n > 127) {
    snprintf(b, s, "N/A");
    return;
  }
  snprintf(b, s, "%s%d", MIDI_NOTE_NAMES[n % 12], (n / 12 - 1));
}

// Get button summary from command type and data
void getButtonSummary(char *b, size_t s, MidiCommandType type, int data1) {
  char n[8];
  b[0] = '\0';
  switch (type) {
  case NOTE_MOMENTARY:
    midiNoteNumberToString(n, 8, data1);
    snprintf(b, s, "%.4s", n);
    break;
  case NOTE_ON:
    midiNoteNumberToString(n, 8, data1);
    snprintf(b, s, "^%.3s", n);
    break;
  case NOTE_OFF:
    midiNoteNumberToString(n, 8, data1);
    snprintf(b, s, "v%.3s", n);
    break;
  case CC:
    snprintf(b, s, "CC%d", data1);
    break;
  case PC:
    snprintf(b, s, "PC%d", data1);
    break;
  case TAP_TEMPO:
    strncpy(b, "TAP", s - 1);
    b[s - 1] = '\0';
    break;
  case PRESET_UP:
    strncpy(b, ">", s - 1);
    b[s - 1] = '\0';
    break;
  case PRESET_DOWN:
    strncpy(b, "<", s - 1);
    b[s - 1] = '\0';
    break;
  case PRESET_1:
    snprintf(b, s, "%.10s", presetNames[0]);
    break;
  case PRESET_2:
    snprintf(b, s, "%.10s", presetNames[1]);
    break;
  case PRESET_3:
    snprintf(b, s, "%.10s", presetNames[2]);
    break;
  case PRESET_4:
    snprintf(b, s, "%.10s", presetNames[3]);
    break;
  case WIFI_TOGGLE:
    strncpy(b, "WiFi", s - 1);
    b[s - 1] = '\0';
    break;
  case CLEAR_BLE_BONDS:
    strncpy(b, "xBLE", s - 1);
    b[s - 1] = '\0';
    break;
  case SYSEX:
    strncpy(b, "SYS", s - 1);
    b[s - 1] = '\0';
    break;
  case MENU_TOGGLE:
    strncpy(b, "MENU", s - 1);
    b[s - 1] = '\0';
    break;
  case MENU_UP:
    strncpy(b, "M.UP", s - 1);
    b[s - 1] = '\0';
    break;
  case MENU_DOWN:
    strncpy(b, "M.DN", s - 1);
    b[s - 1] = '\0';
    break;
  case MENU_ENTER:
    strncpy(b, "M.OK", s - 1);
    b[s - 1] = '\0';
    break;
  case MIDI_OFF:
    strncpy(b, "OFF", s - 1);
    b[s - 1] = '\0';
    break;
  default:
    break;
  }
}

// Logic to get a label string for an input ID (e.g. "1", "A1")
void getInputLabel(char *target, size_t targetSize, const char *labelId,
                   int maxCharsDisplay) {
  if (labelId[0] == 'A' || labelId[0] == 'a') {
    int aIdx = atoi(&labelId[1]) - 1;
    if (aIdx >= 0 && aIdx < MAX_ANALOG_INPUTS && analogInputs[aIdx].enabled) {
      snprintf(target, targetSize, "%.*s", maxCharsDisplay,
               analogInputs[aIdx].name);
    } else {
      snprintf(target, targetSize, "%.*s", maxCharsDisplay, labelId);
    }
  } else {
    int bIdx = atoi(labelId) - 1;
    if (bIdx >= 0 && bIdx < systemConfig.buttonCount) {
      const ButtonConfig &config = buttonConfigs[currentPreset][bIdx];
      char defaultName[21];
      snprintf(defaultName, sizeof(defaultName), "B%d", bIdx + 1);
      if (strncmp(config.name, defaultName, 20) == 0 ||
          strlen(config.name) == 0) {
        char summary[10];
        ActionMessage *firstAction = (config.messageCount > 0)
                                         ? (ActionMessage *)&config.messages[0]
                                         : nullptr;
        if (firstAction) {
          getButtonSummary(summary, sizeof(summary), firstAction->type,
                           firstAction->data1);
        } else {
          strncpy(summary, "---", sizeof(summary));
        }
        snprintf(target, targetSize, "%.*s", maxCharsDisplay, summary);
      } else {
        snprintf(target, targetSize, "%.*s", maxCharsDisplay, config.name);
      }
    } else {
      snprintf(target, targetSize, "%.*s", maxCharsDisplay, labelId);
    }
  }
}

void displayOLED() {
  // Skip display updates when heap is critically low (WiFi uses lots of memory)
  if (ESP.getFreeHeap() < 20000) {
    return;
  }

  // Skip if no display configured (OLED_NONE)
  if (displayPtr == nullptr || oledConfig.type == OLED_NONE) {
    return;
  }

  // Check if in tap tempo mode
  if (inTapTempoMode) {
    displayTapTempoMode();
    return;
  }

  if (buttonNameDisplayUntil > 0) {
    if (millis() < buttonNameDisplayUntil) {
      displayButtonName();
      return;
    } else {
      buttonNameDisplayUntil = 0;
    }
  }

  clearDisplayBuffer();

  // Get layout config from oledConfig (v1.5 - 128x32 support)
  uint8_t topRowY = oledConfig.main.topRowY;
  uint8_t titleY = oledConfig.main.titleY;
  uint8_t statusY = oledConfig.main.statusY;
  uint8_t bpmY = oledConfig.main.bpmY;
  uint8_t bottomRowY = oledConfig.main.bottomRowY;
  uint8_t labelSize = oledConfig.main.labelSize;
  uint8_t titleSize = oledConfig.main.titleSize;
  uint8_t statusSize = oledConfig.main.statusSize;
  uint8_t bpmSize = oledConfig.main.bpmSize;
  bool showBpm = oledConfig.main.showBpm;
  bool showTopRow = oledConfig.main.showTopRow;
  bool showBottomRow = oledConfig.main.showBottomRow;
  uint8_t titleAlign = oledConfig.main.titleAlign;
  uint8_t statusAlign = oledConfig.main.statusAlign;
  uint8_t bpmAlign = oledConfig.main.bpmAlign;

  // For 128x32, adjust bottomRowY if using default 128x64 value
  int screenHeight = (oledConfig.type == OLED_128X32)   ? 32
                     : (oledConfig.type == TFT_128X128) ? 128
                     : (oledConfig.type == TFT_128X160) ? 160
                                                        : 64;
  if (oledConfig.type == OLED_128X32 && bottomRowY > 24) {
    bottomRowY = 24; // Clamp for 128x32
  }

  displayPtr->setTextSize(labelSize);
  char summary[10];

  // Dynamic layout based on button count and orientation
  // Rotation index: 0=0°, 1=90°, 2=180°, 3=270°
  bool isVertical = (oledConfig.rotation == 1 || oledConfig.rotation == 3);
  int w = displayPtr->width();
  int h = displayPtr->height();

  // Text bounds variables
  int16_t bx, by;
  uint16_t bw, bh;

  // Top Row (conditional on showTopRow)
  if (showTopRow) {
    char tempMap[34];
    strncpy(tempMap, oledConfig.main.topRowMap, 33);
    tempMap[33] = '\0';
    int itemCount = 0;
    char *p = tempMap;
    while (*p) {
      if (*p == ',')
        itemCount++;
      p++;
    }
    if (strlen(tempMap) > 0)
      itemCount++;

    if (itemCount > 0) {
      if (isVertical) {
        // Vertical layout: top row items at the top
        int yOffset = topRowY;
        int maxVerticalChars = (w < 40) ? 4 : 8;
        char *token = strtok(tempMap, ",");
        while (token != NULL) {
          char label[11];
          getInputLabel(label, sizeof(label), token, maxVerticalChars);
          displayPtr->setCursor(1, yOffset);
          displayPtr->print(label);
          yOffset += labelSize * 8 + 2;
          token = strtok(NULL, ",");
          if (yOffset > h / 2)
            break; // Stop if too many items
        }
      } else {
        // Horizontal layout: row of items
        int dynColWidth = w / itemCount;
        int dynMaxChars = (itemCount > 4) ? 3 : 4;

        // For TFT color strips, need to re-parse to get button indices
        char parseMap[34];
        strncpy(parseMap, oledConfig.main.topRowMap, 33);
        parseMap[33] = '\0';

        // Use strtok_r (reentrant) to avoid interference between parsing loops
        char *saveptr1 = NULL;
        char *saveptr2 = NULL;
        char *token = strtok_r(tempMap, ",", &saveptr1);
        char *colorToken = strtok_r(parseMap, ",", &saveptr2);
        int i = 0;
        while (token != NULL) {
          char label[11];
          getInputLabel(label, sizeof(label), token, dynMaxChars);

          // Calculate text width and apply alignment within column
          int textWidth = strlen(label) * 6 * labelSize;
          int colStart = i * dynColWidth;
          int textX = colStart; // Default: left align
          if (oledConfig.main.topRowAlign == 1) {
            textX = colStart + (dynColWidth - textWidth) / 2; // Center
          } else if (oledConfig.main.topRowAlign == 2) {
            textX = colStart + dynColWidth - textWidth; // Right
          }
          displayPtr->setCursor(textX, topRowY);
          displayPtr->print(label);

          // Draw color strip below label for TFT when enabled
          if ((oledConfig.type == TFT_128X128 ||
               oledConfig.type == TFT_128X160) &&
              oledConfig.main.showColorStrips && colorToken != NULL) {
            int stripY = topRowY + (labelSize * 8) + 1;
            int stripH = oledConfig.main.colorStripHeight > 0
                             ? oledConfig.main.colorStripHeight
                             : 4;
            int maxStripW = dynColWidth - 2;

            // Check if this is an analog input (starts with 'A')
            if (colorToken[0] == 'A' || colorToken[0] == 'a') {
              int ainIdx = atoi(&colorToken[1]) - 1;
              if (ainIdx >= 0 && ainIdx < MAX_ANALOG_INPUTS &&
                  analogInputs[ainIdx].enabled) {
                uint8_t *rgb = analogInputs[ainIdx].rgb;
                uint16_t color = ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) |
                                 (rgb[2] >> 3);
                // Calculate fill width as loading bar (based on reading %)
                float percent = analogInputs[ainIdx].smoothedValue / 4095.0f;
                int stripW = (int)(maxStripW * percent);
                if (stripW < 1 && percent > 0.01f)
                  stripW = 1; // Min 1px if active
                displayPtr->fillRect(colStart, stripY, stripW, stripH, color);
              }
            } else {
              // Button - full-width strip
              int btnIdx = atoi(colorToken) - 1;
              if (btnIdx >= 0 && btnIdx < MAX_BUTTONS) {
                ButtonConfig &btn = buttonConfigs[currentPreset][btnIdx];
                if (btn.messageCount > 0) {
                  uint8_t *rgb = btn.messages[0].rgb;
                  uint16_t color = ((rgb[0] >> 3) << 11) |
                                   ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
                  displayPtr->fillRect(colStart, stripY, maxStripW, stripH,
                                       color);
                }
              }
            }
          }

          token = strtok_r(NULL, ",", &saveptr1);
          colorToken = colorToken ? strtok_r(NULL, ",", &saveptr2) : NULL;
          i++;
        }
      }
    }
  }

  // Bottom Row (conditional on showBottomRow)
  if (showBottomRow) {
    char tempMap[34];
    strncpy(tempMap, oledConfig.main.bottomRowMap, 33);
    tempMap[33] = '\0';
    int itemCount = 0;
    char *p = tempMap;
    while (*p) {
      if (*p == ',')
        itemCount++;
      p++;
    }
    if (strlen(tempMap) > 0)
      itemCount++;

    if (itemCount > 0) {
      if (isVertical) {
        // Vertical layout: bottom row items at the bottom
        int yOffset = h - (itemCount * (labelSize * 8 + 2)) - 2;
        if (yOffset < h / 2)
          yOffset = h / 2; // Prevent overlap
        int maxVerticalChars = (w < 40) ? 4 : 8;
        char *token = strtok(tempMap, ",");
        while (token != NULL) {
          char label[11];
          getInputLabel(label, sizeof(label), token, maxVerticalChars);
          displayPtr->setCursor(1, yOffset);
          displayPtr->print(label);
          yOffset += labelSize * 8 + 2;
          token = strtok(NULL, ",");
        }
      } else {
        // Horizontal layout: row of items
        int dynColWidth = w / itemCount;
        int dynMaxChars = (itemCount > 4) ? 3 : 4;

        // For TFT color strips, need to re-parse to get button indices
        char parseMap[34];
        strncpy(parseMap, oledConfig.main.bottomRowMap, 33);
        parseMap[33] = '\0';

        // Use strtok_r (reentrant) to avoid interference between parsing loops
        char *saveptr1 = NULL;
        char *saveptr2 = NULL;
        char *token = strtok_r(tempMap, ",", &saveptr1);
        char *colorToken = strtok_r(parseMap, ",", &saveptr2);
        int i = 0;
        while (token != NULL) {
          char label[11];
          getInputLabel(label, sizeof(label), token, dynMaxChars);

          // Calculate text width and apply alignment within column
          int textWidth = strlen(label) * 6 * labelSize;
          int colStart = i * dynColWidth;
          int textX = colStart; // Default: left align
          if (oledConfig.main.bottomRowAlign == 1) {
            textX = colStart + (dynColWidth - textWidth) / 2; // Center
          } else if (oledConfig.main.bottomRowAlign == 2) {
            textX = colStart + dynColWidth - textWidth; // Right
          }
          displayPtr->setCursor(textX, bottomRowY);
          displayPtr->print(label);

          // Draw color strip above label for TFT when enabled
          if ((oledConfig.type == TFT_128X128 ||
               oledConfig.type == TFT_128X160) &&
              oledConfig.main.showColorStrips && colorToken != NULL) {
            int stripH = oledConfig.main.colorStripHeight > 0
                             ? oledConfig.main.colorStripHeight
                             : 4;
            int stripY = bottomRowY - stripH - 1; // Strip grows upward
            int maxStripW = dynColWidth - 2;

            // Check if this is an analog input (starts with 'A')
            if (colorToken[0] == 'A' || colorToken[0] == 'a') {
              int ainIdx = atoi(&colorToken[1]) - 1;
              if (ainIdx >= 0 && ainIdx < MAX_ANALOG_INPUTS &&
                  analogInputs[ainIdx].enabled) {
                uint8_t *rgb = analogInputs[ainIdx].rgb;
                uint16_t color = ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) |
                                 (rgb[2] >> 3);
                // Calculate fill width as loading bar (based on reading %)
                float percent = analogInputs[ainIdx].smoothedValue / 4095.0f;
                int stripW = (int)(maxStripW * percent);
                if (stripW < 1 && percent > 0.01f)
                  stripW = 1; // Min 1px if active
                displayPtr->fillRect(colStart, stripY, stripW, stripH, color);
              }
            } else {
              // Button - full-width strip
              int btnIdx = atoi(colorToken) - 1;
              if (btnIdx >= 0 && btnIdx < MAX_BUTTONS) {
                ButtonConfig &btn = buttonConfigs[currentPreset][btnIdx];
                if (btn.messageCount > 0) {
                  uint8_t *rgb = btn.messages[0].rgb;
                  uint16_t color = ((rgb[0] >> 3) << 11) |
                                   ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
                  displayPtr->fillRect(colStart, stripY, maxStripW, stripH,
                                       color);
                }
              }
            }
          }

          token = strtok_r(NULL, ",", &saveptr1);
          colorToken = colorToken ? strtok_r(NULL, ",", &saveptr2) : NULL;
          i++;
        }
      }
    }
  }

  // Middle Area - Skip rest of rendering if vertical for now to avoid mess,
  // or implement clean vertical middle
  if (isVertical) {
    // Basic Vertical Middle Info
    int midY = h / 2 - (titleSize * 4);
    char truncatedName[9];
    snprintf(truncatedName, sizeof(truncatedName), "%.8s",
             presetNames[currentPreset]);
    displayPtr->setTextSize(titleSize);
    displayPtr->getTextBounds(truncatedName, 0, 0, &bx, &by, &bw, &bh);
    int titleX = (titleAlign == 0)   ? 0
                 : (titleAlign == 2) ? (w - bw)
                                     : (w - bw) / 2;
    displayPtr->setCursor(titleX, midY);
    displayPtr->print(truncatedName);

    if (showBpm) {
      displayPtr->setTextSize(bpmSize);
      char bpmStr[8];
      snprintf(bpmStr, sizeof(bpmStr), "%.1f", currentBPM);
      displayPtr->getTextBounds(bpmStr, 0, 0, &bx, &by, &bw, &bh);
      int bpmX = (bpmAlign == 0)   ? 0
                 : (bpmAlign == 2) ? (w - bw)
                                   : (w - bw) / 2;
      displayPtr->setCursor(bpmX, midY + (titleSize * 8) + 2);
      displayPtr->print(bpmStr);
    }
    flushDisplay();
    return;
  }

  // Middle Area - Layout differs for 128x32 vs 128x64
  displayPtr->setTextSize(titleSize);

  if (oledConfig.type == OLED_128X32) {
    // === 128x32 COMPACT HORIZONTAL LAYOUT ===
    char truncatedName[7];
    snprintf(truncatedName, sizeof(truncatedName), "%.6s",
             presetNames[currentPreset]);
    displayPtr->setTextSize(titleSize);
    displayPtr->getTextBounds(truncatedName, 0, 0, &bx, &by, &bw, &bh);
    int titleX = (titleAlign == 0)   ? 0
                 : (titleAlign == 2) ? (w - bw)
                                     : (w - bw) / 2;
    displayPtr->setCursor(titleX, titleY);
    displayPtr->print(truncatedName);

    // Status / Analog center or per alignment
    displayPtr->setTextSize(statusSize);
    const char *statusText = clientConnected ? "SYNC"
                             : (systemConfig.bleMode == BLE_SERVER_ONLY)
                                 ? "SRV"
                                 : "BLE";
    if (oledConfig.main.showAnalog) {
      statusText = "A1";
    }
    displayPtr->getTextBounds(statusText, 0, 0, &bx, &by, &bw, &bh);
    int statusX = (statusAlign == 0)   ? 0
                  : (statusAlign == 2) ? (w - bw)
                                       : (w - bw) / 2;
    displayPtr->setCursor(statusX, titleY);
    displayPtr->print(statusText);

    // BPM with alignment and bpmSize
    if (showBpm) {
      displayPtr->setTextSize(bpmSize);
      char bpmStr[8];
      snprintf(bpmStr, sizeof(bpmStr), "%.1f", currentBPM);
      displayPtr->getTextBounds(bpmStr, 0, 0, &bx, &by, &bw, &bh);
      int bpmX = (bpmAlign == 0)   ? 0
                 : (bpmAlign == 2) ? (w - bw - 2)
                                   : (w - bw) / 2;
      displayPtr->setCursor(bpmX, titleY);
      displayPtr->print(bpmStr);
    }
  } else {
    // === STANDARD 128x64 LAYOUT ===
    char truncatedName[11];
    snprintf(truncatedName, sizeof(truncatedName), "%.10s",
             presetNames[currentPreset]);
    displayPtr->getTextBounds(truncatedName, 0, 0, &bx, &by, &bw, &bh);
    // Apply title alignment (0=Left, 1=Center, 2=Right)
    int titleX = (titleAlign == 0)   ? 0
                 : (titleAlign == 2) ? (w - bw)
                                     : (w - bw) / 2;
    displayPtr->setCursor(titleX, titleY);
    displayPtr->print(truncatedName);

    // BPM Display (if enabled) - use bpmSize and bpmY
    if (showBpm) {
      displayPtr->setTextSize(bpmSize);
      char bpmStr[16];
      snprintf(bpmStr, sizeof(bpmStr), "%.1f BPM", currentBPM);
      displayPtr->getTextBounds(bpmStr, 0, 0, &bx, &by, &bw, &bh);
      // Apply BPM alignment (0=Left, 1=Center, 2=Right)
      int bpmX = (bpmAlign == 0)   ? 0
                 : (bpmAlign == 2) ? (w - bw)
                                   : (w - bw) / 2;
      displayPtr->setCursor(bpmX, bpmY);
      displayPtr->print(bpmStr);
    }
    displayPtr->setTextSize(statusSize); // Reset for status line
  }

  // Status Line - Show connection mode and state
  int statusLineY = statusY; // Use absolute statusY from config
  if (oledConfig.type == OLED_128X32) {
    // For 128x32, skip status line entirely (no room)
  } else if (oledConfig.main.showStatus) {
    displayPtr->setCursor(0, statusLineY);
    if (isWifiOn) {
      // WiFi config mode - centered message
      const char *wifiMsg = "- WIFI CONFIG -";
      displayPtr->getTextBounds(wifiMsg, 0, 0, &bx, &by, &bw, &bh);
      displayPtr->setCursor((w - bw) / 2, statusLineY);
      displayPtr->print(wifiMsg);
    } else if (isBtSerialOn) {
      // BT Serial config mode - centered message
      const char *btMsg = "- BL Serial -";
      displayPtr->getTextBounds(btMsg, 0, 0, &bx, &by, &bw, &bh);
      displayPtr->setCursor((w - bw) / 2, statusLineY);
      displayPtr->print(btMsg);
    } else {
      // Normal mode - show connection state based on MIDI mode
      char statusLine[32];

      // USB MIDI mode shows simple status
      if (systemConfig.bleMode == MIDI_USB_ONLY) {
        snprintf(statusLine, sizeof(statusLine), "USB MIDI");
      } else {
        // BLE modes - show BLE state
        const char *bleMode =
            bleConfigMode                             ? "EDIT"
            : systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT"
            : systemConfig.bleMode == BLE_SERVER_ONLY ? "SERVER"
                                                      : "DUAL";
        const char *bleState = clientConnected ? "Synced" : "Scanning";

        // For SERVER mode or EDIT mode, don't show scanning
        if (systemConfig.bleMode == BLE_SERVER_ONLY || bleConfigMode) {
          bleState = clientConnected ? "Synced" : "";
        }

        if (strlen(bleState) > 0) {
          snprintf(statusLine, sizeof(statusLine), "BLE:%s %s", bleMode,
                   bleState);
        } else {
          snprintf(statusLine, sizeof(statusLine), "BLE:%s", bleMode);
        }
      }

      // Apply status alignment (0=Left, 1=Center, 2=Right)
      displayPtr->getTextBounds(statusLine, 0, 0, &bx, &by, &bw, &bh);
      int statusX = (statusAlign == 0)   ? 0
                    : (statusAlign == 2) ? (w - bw)
                                         : (w - bw) / 2;
      displayPtr->setCursor(statusX, statusLineY);
      displayPtr->print(statusLine);
    }
  }

  // Battery indicator (v1.5) - configurable position and scale
  if (oledConfig.main.showBattery && systemConfig.batteryAdcPin > 0) {
    updateBatteryLevel();
    int battX = oledConfig.main.batteryX;
    int battY = oledConfig.main.batteryY;
    int battScale = oledConfig.main.batteryScale;
    if (battScale < 1)
      battScale = 1;
    drawBatteryIcon(battX, battY, battScale);
  }

  flushDisplay();
}

// Partial update for analog color strips only (no flicker)
// Only redraws the color strip areas for analog inputs, not the entire screen
void updateAnalogColorStrips() {
  if (displayPtr == nullptr)
    return;
  if (oledConfig.type != TFT_128X128 && oledConfig.type != TFT_128X160)
    return;
  if (!oledConfig.main.showColorStrips)
    return;

  // Get display dimensions
  int w = 128;
  int h = (oledConfig.type == TFT_128X160) ? 160 : 128;
  int labelSize = oledConfig.main.labelSize > 0 ? oledConfig.main.labelSize : 1;
  int stripH = oledConfig.main.colorStripHeight > 0
                   ? oledConfig.main.colorStripHeight
                   : 4;
  int topRowY = oledConfig.main.topRowY;
  int bottomRowY = oledConfig.main.bottomRowY;

  // Process top row
  char tempMap[34];
  strncpy(tempMap, oledConfig.main.topRowMap, 33);
  tempMap[33] = '\0';
  int itemCount = 0;
  char *p = tempMap;
  while (*p) {
    if (*p == ',')
      itemCount++;
    p++;
  }
  if (strlen(tempMap) > 0)
    itemCount++;

  if (itemCount > 0) {
    int dynColWidth = w / itemCount;

    char *token = strtok(tempMap, ",");
    int i = 0;
    while (token != NULL) {
      // Check if analog input
      if (token[0] == 'A' || token[0] == 'a') {
        int ainIdx = atoi(&token[1]) - 1;
        if (ainIdx >= 0 && ainIdx < MAX_ANALOG_INPUTS &&
            analogInputs[ainIdx].enabled) {
          uint8_t *rgb = analogInputs[ainIdx].rgb;
          uint16_t color =
              ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
          int stripY = topRowY + (labelSize * 8) + 1;
          int maxStripW = dynColWidth - 2;
          float percent = analogInputs[ainIdx].smoothedValue / 4095.0f;
          int stripW = (int)(maxStripW * percent);
          if (stripW < 1 && percent > 0.01f)
            stripW = 1;
          // Clear strip area first (black), then draw filled portion
          int colStart = i * dynColWidth;
          displayPtr->fillRect(colStart, stripY, maxStripW, stripH,
                               ST7735_BLACK);
          displayPtr->fillRect(colStart, stripY, stripW, stripH, color);
        }
      }
      token = strtok(NULL, ",");
      i++;
    }
  }

  // Process bottom row
  strncpy(tempMap, oledConfig.main.bottomRowMap, 33);
  tempMap[33] = '\0';
  itemCount = 0;
  p = tempMap;
  while (*p) {
    if (*p == ',')
      itemCount++;
    p++;
  }
  if (strlen(tempMap) > 0)
    itemCount++;

  if (itemCount > 0) {
    int dynColWidth = w / itemCount;

    char *token = strtok(tempMap, ",");
    int i = 0;
    while (token != NULL) {
      // Check if analog input
      if (token[0] == 'A' || token[0] == 'a') {
        int ainIdx = atoi(&token[1]) - 1;
        if (ainIdx >= 0 && ainIdx < MAX_ANALOG_INPUTS &&
            analogInputs[ainIdx].enabled) {
          uint8_t *rgb = analogInputs[ainIdx].rgb;
          uint16_t color =
              ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
          int stripY = bottomRowY - stripH - 1;
          int maxStripW = dynColWidth - 2;
          float percent = analogInputs[ainIdx].smoothedValue / 4095.0f;
          int stripW = (int)(maxStripW * percent);
          if (stripW < 1 && percent > 0.01f)
            stripW = 1;
          // Clear strip area first (black), then draw filled portion
          int colStart = i * dynColWidth;
          displayPtr->fillRect(colStart, stripY, maxStripW, stripH,
                               ST7735_BLACK);
          displayPtr->fillRect(colStart, stripY, stripW, stripH, color);
        }
      }
      token = strtok(NULL, ",");
      i++;
    }
  }
}

void displayTapTempoMode() {
  if (displayPtr == nullptr || oledConfig.type == OLED_NONE)
    return;
  clearDisplayBuffer();
  displayPtr->setTextColor(DISPLAY_WHITE);

  // Get layout config from oledConfig.tap
  uint8_t labelSize = oledConfig.tap.labelSize;
  uint8_t bpmSize = oledConfig.tap.titleSize;
  uint8_t patternSize = oledConfig.tap.statusSize;
  uint8_t topRowY = oledConfig.tap.topRowY;
  uint8_t bpmY = oledConfig.tap.titleY;
  uint8_t patternY = oledConfig.tap.statusY;
  uint8_t bottomRowY = oledConfig.tap.bottomRowY;

  // For 128x32, adjust Y positions if needed
  if (oledConfig.type == OLED_128X32) {
    if (bpmY > 8)
      bpmY = 8;
    if (patternY > 20)
      patternY = 20;
    if (bottomRowY > 24)
      bottomRowY = 24;
  }

  // For 128x128 TFT, scale up positions for larger display
  if (oledConfig.type == TFT_128X128) {
    topRowY = 10;
    bpmY = 40;
    patternY = 80;
    bottomRowY = 110;
    bpmSize = bpmSize < 4 ? 4 : bpmSize; // Larger BPM text for TFT
  }

  // For 128x160 TFT, scale up positions for taller display
  if (oledConfig.type == TFT_128X160) {
    topRowY = 10;
    bpmY = 50;
    patternY = 100;
    bottomRowY = 140;
    bpmSize = bpmSize < 4 ? 4 : bpmSize; // Larger BPM text for TFT
  }

  // Top row: NEXT (left) and LOCK indicator (right)
  displayPtr->setTextSize(labelSize);
  displayPtr->setCursor(0, topRowY);
  displayPtr->print("NEXT");

  // Lock status indicator in top-right
  displayPtr->setCursor(90, topRowY);
  if (tapModeLocked) {
    displayPtr->print("LOCKED");
  } else {
    displayPtr->print("LOCK");
  }

  // BPM - larger, centered
  displayPtr->setTextSize(bpmSize);
  char bpmStr[8];
  snprintf(bpmStr, sizeof(bpmStr), "%.1f", currentBPM);
  int16_t bx1, by1;
  uint16_t bw, bh;
  displayPtr->getTextBounds(bpmStr, 0, 0, &bx1, &by1, &bw, &bh);
  displayPtr->setCursor((SCREEN_WIDTH - bw) / 2, bpmY);
  displayPtr->print(bpmStr);

  // Middle-bottom: Pattern and delay time
  displayPtr->setTextSize(patternSize);
  displayPtr->setCursor(0, patternY);
  displayPtr->print("Pattern: ");
  displayPtr->print(rhythmNames[rhythmPattern]);

  // Delay ms on right of middle row
  float finalDelayMs =
      (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
  int delayTimeMS = (int)finalDelayMs;
  char delayStr[12];
  snprintf(delayStr, sizeof(delayStr), "%dms", delayTimeMS);
  int16_t x1, y1;
  uint16_t w, h;
  displayPtr->getTextBounds(delayStr, 0, 0, &x1, &y1, &w, &h);
  displayPtr->setCursor(SCREEN_WIDTH - w - 2, patternY);
  displayPtr->print(delayStr);

  // Bottom row: PREV (left) and TAP (right) - skip on 128x32 if no room
  if (oledConfig.type != OLED_128X32 || bottomRowY < 30) {
    displayPtr->setTextSize(labelSize);
    displayPtr->setCursor(0, bottomRowY);
    displayPtr->print("PREV");

    displayPtr->setCursor(100, bottomRowY);
    displayPtr->print("TAP");
  }

  flushDisplay();
}

void displayButtonName() {
  if (displayPtr == nullptr || oledConfig.type == OLED_NONE)
    return;
  clearDisplayBuffer();

  // Get layout config from oledConfig.overlay (unified with menu "Name Font
  // Size")
  uint8_t textSize = oledConfig.overlay.titleSize > 0
                         ? oledConfig.overlay.titleSize
                         : 2; // Default to 2 if not set
  int screenHeight = (oledConfig.type == TFT_128X128)   ? 128
                     : (oledConfig.type == TFT_128X160) ? 160
                     : (oledConfig.type == OLED_128X32) ? 32
                                                        : 64;

  displayPtr->setTextSize(textSize);
  displayPtr->setTextColor(DISPLAY_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  displayPtr->getTextBounds(buttonNameToShow, 0, 0, &x1, &y1, &w, &h);
  displayPtr->setCursor((SCREEN_WIDTH - w) / 2, (screenHeight - h) / 2);
  displayPtr->print(buttonNameToShow);
  flushDisplay();
  displayPtr->setTextSize(1);
}

void displayMenu() {
  clearDisplayBuffer();
  displayPtr->setTextColor(DISPLAY_WHITE);

  // Get layout config from oledConfig.menu
  uint8_t headerSize =
      oledConfig.menu.titleSize > 0 ? oledConfig.menu.titleSize : 1;
  uint8_t itemSize =
      oledConfig.menu.labelSize > 0 ? oledConfig.menu.labelSize : 1;
  uint8_t headerY = oledConfig.menu.topRowY;
  uint8_t itemStartY = oledConfig.menu.titleY > 0 ? oledConfig.menu.titleY : 14;
  int lineHeight = (oledConfig.type == OLED_128X32) ? 8 : 10;
  int maxVisibleItems = (oledConfig.type == OLED_128X32) ? 3 : 5;

  // Adjust for 128x32 to fit items below header without clipping
  if (oledConfig.type == OLED_128X32) {
    itemStartY = 9; // Start just below header (header ends ~8px)
    lineHeight = 8; // 8px per line means items at Y=9, 17, 25 (all fit in 32px)
  }

  // Adjust for 128x128 TFT - more items visible, larger spacing
  if (oledConfig.type == TFT_128X128) {
    itemStartY = 16;     // Start below header with more padding
    lineHeight = 12;     // Taller line height for better readability
    maxVisibleItems = 9; // Show more items on larger screen
  }

  // Adjust for 128x160 TFT - even more items visible on taller screen
  if (oledConfig.type == TFT_128X160) {
    itemStartY = 16;      // Start below header with more padding
    lineHeight = 12;      // Taller line height for better readability
    maxVisibleItems = 11; // Show more items on taller screen
  }

  displayPtr->setTextSize(itemSize);

  // Build menu items dynamically to show status
  char menuItems[15][25]; // Array to hold menu item strings
  strncpy(menuItems[0], "Save and Exit", 25);
  strncpy(menuItems[1], "Exit without Saving", 25);
  snprintf(menuItems[2], 25, "Wi-Fi LoadCfg (%s)", isWifiOn ? "ON" : "OFF");
  snprintf(menuItems[3], 25, "BT SerialEdit (%s)", isBtSerialOn ? "ON" : "OFF");
  strncpy(menuItems[4], "LED Bright (On)", 25);
  strncpy(menuItems[5], "LED Bright (Dim)", 25);
  strncpy(menuItems[6], "Pad Debounce", 25);
  strncpy(menuItems[7], "Clear BLE Bonds", 25);
  strncpy(menuItems[8], "Reboot", 25);
  strncpy(menuItems[9], "Factory Reset", 25);
  strncpy(menuItems[10], "Name Font Size", 25);
  snprintf(menuItems[11], 25, "Wifi %s at Boot",
           systemConfig.wifiOnAtBoot ? "ON" : "OFF");
  // MIDI Mode display - includes USB option for ESP32-S3, EDIT for config mode
  const char *midiModeStr = bleConfigMode                             ? "EDIT"
                            : systemConfig.bleMode == MIDI_USB_ONLY   ? "USB"
                            : systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT"
                            : systemConfig.bleMode == BLE_DUAL_MODE   ? "DUAL"
                                                                    : "SERVER";
  snprintf(menuItems[12], 25, "MIDI Mode: %s", midiModeStr);
  snprintf(menuItems[13], 25, "Analog Debug %s",
           systemConfig.debugAnalogIn ? "ON" : "OFF");
  strncpy(menuItems[14], "Edit Commands", 25);

  int numMenuItems = 15;

  displayPtr->setCursor(0, 0);
  displayPtr->printf("-- Menu CHOCOTONE --");

  if (factoryResetConfirm) {
    // Factory Reset confirmation submenu
    displayPtr->setCursor(0, 0);
    displayPtr->printf("-- WARNING! --");
    displayPtr->setCursor(0, 14);
    displayPtr->print("Clear all settings?");
    displayPtr->setCursor(0, 28);
    displayPtr->print("This cannot be undone!");

    // Show Yes/No options
    displayPtr->setCursor(10, 46);
    if (editingValue == 0) {
      displayPtr->print("> Yes, Reset");
    } else {
      displayPtr->print("  Yes, Reset");
    }
    displayPtr->setCursor(10, 56);
    if (editingValue == 1) {
      displayPtr->print("> No, Go Back");
    } else {
      displayPtr->print("  No, Go Back");
    }
  } else if (inSubMenu) {
    displayPtr->setCursor(10, 20);
    displayPtr->printf("Editing: %s", menuItems[menuSelection]);
    displayPtr->setTextSize(2);
    char valueStr[10];
    snprintf(valueStr, sizeof(valueStr), "%d", editingValue);
    int16_t x1, y1;
    uint16_t w, h;
    displayPtr->getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
    displayPtr->setCursor((SCREEN_WIDTH - w) / 2, 40);
    displayPtr->print(valueStr);
    displayPtr->setTextSize(1);
  } else {
    // Show menu items with scrolling
    int lineOffset = menuSelection - (maxVisibleItems / 2);
    if (lineOffset < 0)
      lineOffset = 0;
    if (lineOffset > numMenuItems - maxVisibleItems)
      lineOffset = numMenuItems - maxVisibleItems;

    for (int i = 0; i < numMenuItems; i++) {
      int displayLine = i - lineOffset;
      if (displayLine >= 0 && displayLine < maxVisibleItems) {
        displayPtr->setCursor(0, itemStartY + (displayLine * lineHeight));
        if (i == menuSelection) {
          displayPtr->print("> ");
        } else {
          displayPtr->print("  ");
        }
        displayPtr->print(menuItems[i]);
      }
    }
  }
  flushDisplay();
}

// ============================================================================
// EDIT COMMANDS SUBMENU (v1.5)
// On-device action editor for buttons and analog inputs
// ============================================================================

// Helper: Short name for MidiCommandType
static const char *getTypeName(MidiCommandType t) {
  switch (t) {
  case MIDI_OFF:
    return "OFF";
  case NOTE_MOMENTARY:
    return "NtMom";
  case NOTE_ON:
    return "NtOn";
  case NOTE_OFF:
    return "NtOff";
  case CC:
    return "CC";
  case PC:
    return "PC";
  case SYSEX:
    return "SysEx";
  case SYSEX_SCROLL:
    return "SxScr";
  case TAP_TEMPO:
    return "Tap";
  case PRESET_UP:
    return "Pre+";
  case PRESET_DOWN:
    return "Pre-";
  case PRESET_1:
    return "Pre1";
  case PRESET_2:
    return "Pre2";
  case PRESET_3:
    return "Pre3";
  case PRESET_4:
    return "Pre4";
  case CLEAR_BLE_BONDS:
    return "ClrBL";
  case WIFI_TOGGLE:
    return "WiFi";
  case MENU_TOGGLE:
    return "Menu";
  case MENU_UP:
    return "MnUp";
  case MENU_DOWN:
    return "MnDn";
  case MENU_ENTER:
    return "MnOk";
  default:
    return "?";
  }
}

// Helper: Short name for ActionType
static const char *getActionName(ActionType a) {
  switch (a) {
  case ACTION_NONE:
    return "NONE";
  case ACTION_PRESS:
    return "PRESS";
  case ACTION_2ND_PRESS:
    return "2PRESS";
  case ACTION_RELEASE:
    return "REL";
  case ACTION_2ND_RELEASE:
    return "2REL";
  case ACTION_LONG_PRESS:
    return "LONG";
  case ACTION_2ND_LONG_PRESS:
    return "2LONG";
  case ACTION_DOUBLE_TAP:
    return "DTAP";
  case ACTION_COMBO:
    return "COMBO";
  case ACTION_NO_ACTION:
    return "NONE";
  default:
    return "?";
  }
}

static const char *getSysexParamName(uint8_t id) {
  switch ((SysexScrollParamId)id) {
  case SYSEX_PARAM_PITCH_HIGH:
    return "PitchHi";
  case SYSEX_PARAM_DRV_GAIN:
    return "DrvGain";
  case SYSEX_PARAM_DLY_FBK:
    return "DlyFbk";
  case SYSEX_PARAM_FX1_RATE:
    return "Fx1Rate";
  case SYSEX_PARAM_RVB_MIX:
    return "RvbMix";
  case SYSEX_PARAM_AMP_GAIN:
    return "AmpGain";
  case SYSEX_PARAM_PITCH_LOW:
    return "PitchLo";
  default:
    return "None";
  }
}

// HSV to RGB conversion (H: 0-360, S: 0-100, V: 0-100)
void hsvToRgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b) {
  float fH = h / 60.0f;
  float fS = s / 100.0f;
  float fV = v / 100.0f;
  int i = (int)fH;
  float f = fH - i;
  float p = fV * (1.0f - fS);
  float q = fV * (1.0f - fS * f);
  float t = fV * (1.0f - fS * (1.0f - f));
  float rr, gg, bb;
  switch (i % 6) {
  case 0:
    rr = fV;
    gg = t;
    bb = p;
    break;
  case 1:
    rr = q;
    gg = fV;
    bb = p;
    break;
  case 2:
    rr = p;
    gg = fV;
    bb = t;
    break;
  case 3:
    rr = p;
    gg = q;
    bb = fV;
    break;
  case 4:
    rr = t;
    gg = p;
    bb = fV;
    break;
  default:
    rr = fV;
    gg = p;
    bb = q;
    break;
  }
  *r = (uint8_t)(rr * 255);
  *g = (uint8_t)(gg * 255);
  *b = (uint8_t)(bb * 255);
}

// RGB to HSV conversion
void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, int *h, int *s, int *v) {
  float rr = r / 255.0f, gg = g / 255.0f, bb = b / 255.0f;
  float maxC = max(rr, max(gg, bb));
  float minC = min(rr, min(gg, bb));
  float delta = maxC - minC;
  *v = (int)(maxC * 100);
  *s = (maxC > 0) ? (int)(delta / maxC * 100) : 0;
  if (delta == 0) {
    *h = 0;
    return;
  }
  float hue;
  if (maxC == rr)
    hue = 60.0f * fmod((gg - bb) / delta, 6.0f);
  else if (maxC == gg)
    hue = 60.0f * ((bb - rr) / delta + 2.0f);
  else
    hue = 60.0f * ((rr - gg) / delta + 4.0f);
  if (hue < 0)
    hue += 360;
  *h = (int)hue;
}

void displayEditMenu() {
  clearDisplayBuffer();
  displayPtr->setTextColor(DISPLAY_WHITE);
  displayPtr->setTextSize(1);

  int lineHeight = (oledConfig.type == OLED_128X32) ? 8 : 10;
  if (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160)
    lineHeight = 12;
  int maxVisible = (oledConfig.type == OLED_128X32)   ? 3
                   : (oledConfig.type == TFT_128X128) ? 9
                   : (oledConfig.type == TFT_128X160) ? 11
                                                      : 5;
  int headerY = 3;
  int itemStartY = (oledConfig.type == OLED_128X32) ? 12 : 17;
  if (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160)
    itemStartY = 19;

  switch (editMenuState) {

  case EDIT_ROOT: {
    // Center the title
    char title[] = "-- Edit Commands --";
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);
    const char *items[] = {"Buttons", "Analog Inputs", "<< Back"};
    for (int i = 0; i < 3; i++) {
      displayPtr->setCursor(0, itemStartY + i * lineHeight);
      displayPtr->print(i == editSubSelection ? "> " : "  ");
      displayPtr->print(items[i]);
    }
    break;
  }

  case EDIT_BTN_LISTEN: {
    char title[] = "-- Select Button --";
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);
    displayPtr->setCursor(0, itemStartY);
    displayPtr->print("Press any button...");
    displayPtr->setCursor(0, itemStartY + lineHeight * 2);
    displayPtr->print("> << Back");
    break;
  }

  case EDIT_BTN_ACTIONS: {
    ButtonConfig &btn = buttonConfigs[currentPreset][editBtnIndex];
    // Centered title
    char title[30];
    snprintf(title, sizeof(title), "BTN %d: %s", editBtnIndex + 1, btn.name);
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);

    // List action slots + Back
    int totalItems = btn.messageCount + 1; // +1 for Back
    int offset = editSubSelection - (maxVisible - 1) / 2;
    if (offset < 0)
      offset = 0;
    if (offset > totalItems - maxVisible)
      offset = totalItems - maxVisible;
    if (offset < 0)
      offset = 0;

    for (int vi = 0; vi < maxVisible && (vi + offset) < totalItems; vi++) {
      int idx = vi + offset;
      displayPtr->setCursor(0, itemStartY + vi * lineHeight);
      displayPtr->print(idx == editSubSelection ? "> " : "  ");
      if (idx < btn.messageCount) {
        ActionMessage &m = btn.messages[idx];
        // Show RGB color swatch (filled rectangle)
        int swatchX = SCREEN_WIDTH - 12;
        int swatchY = itemStartY + vi * lineHeight;
        uint16_t color565 = ((m.rgb[0] & 0xF8) << 8) |
                            ((m.rgb[1] & 0xFC) << 3) | (m.rgb[2] >> 3);
        displayPtr->fillRect(swatchX, swatchY, 10, lineHeight - 1, color565);
        displayPtr->printf("%s:%s Ch%d", getActionName(m.action),
                           getTypeName(m.type), m.channel);
      } else {
        displayPtr->print("<< Back");
      }
    }
    break;
  }

  case EDIT_BTN_FIELD: {
    ButtonConfig &btn = buttonConfigs[currentPreset][editBtnIndex];
    ActionMessage &m = btn.messages[editActionIndex];
    // Centered title
    char title[30];
    snprintf(title, sizeof(title), "%s %s", btn.name, getActionName(m.action));
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);

    // Fields: Type, Channel, Data1, Data2, H, S, V, Back
    int fieldCount = 8;
    for (int i = 0; i < fieldCount; i++) {
      int dispIdx = i;
      // Scrolling if needed
      int lineOff = editFieldIndex - (maxVisible - 1) / 2;
      if (lineOff < 0)
        lineOff = 0;
      if (lineOff > fieldCount - maxVisible)
        lineOff = fieldCount - maxVisible;
      if (lineOff < 0)
        lineOff = 0;
      if (i < lineOff || i >= lineOff + maxVisible)
        continue;
      int row = i - lineOff;

      displayPtr->setCursor(0, itemStartY + row * lineHeight);
      displayPtr->print(i == editFieldIndex ? "> " : "  ");
      if (i == 0) {
        displayPtr->printf("Type: %s", getTypeName(m.type));
      } else if (i == 1) {
        displayPtr->printf("Channel: %d", m.channel);
      } else if (i == 2) {
        displayPtr->printf("Data1: %d", m.data1);
      } else if (i == 3) {
        displayPtr->printf("Data2: %d", m.data2);
      } else if (i == 4) {
        int h, s, v;
        rgbToHsv(m.rgb[0], m.rgb[1], m.rgb[2], &h, &s, &v);
        displayPtr->printf("Hue: %d", h);
      } else if (i == 5) {
        int h, s, v;
        rgbToHsv(m.rgb[0], m.rgb[1], m.rgb[2], &h, &s, &v);
        displayPtr->printf("Sat: %d%%", s);
      } else if (i == 6) {
        int h, s, v;
        rgbToHsv(m.rgb[0], m.rgb[1], m.rgb[2], &h, &s, &v);
        displayPtr->printf("Val: %d%%", v);
      } else {
        displayPtr->print("<< Back");
      }
    }

    // Show color preview swatch
    uint16_t previewColor =
        ((m.rgb[0] & 0xF8) << 8) | ((m.rgb[1] & 0xFC) << 3) | (m.rgb[2] >> 3);
    displayPtr->fillRect(SCREEN_WIDTH - 14, headerY, 12, 10, previewColor);

    // Show editing indicator with large value
    if (inSubMenu && editFieldIndex < 7) {
      displayPtr->setTextSize(2);
      char val[10];
      if (editFieldIndex == 0)
        snprintf(val, sizeof(val), "%s",
                 getTypeName((MidiCommandType)editingValue));
      else
        snprintf(val, sizeof(val), "%d", editingValue);
      int16_t vx1, vy1;
      uint16_t vw, vh;
      displayPtr->getTextBounds(val, 0, 0, &vx1, &vy1, &vw, &vh);
      int editY = itemStartY + min(fieldCount, maxVisible) * lineHeight + 2;
      displayPtr->setCursor((SCREEN_WIDTH - vw) / 2, editY);
      displayPtr->print(val);
      displayPtr->setTextSize(1);
    }
    break;
  }

  case EDIT_AIN_LIST: {
    char title[] = "-- Analog Inputs --";
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);

    int ainCount = systemConfig.analogInputCount;
    int totalItems = ainCount + 1; // +1 for Back
    int offset = editSubSelection - (maxVisible - 1) / 2;
    if (offset < 0)
      offset = 0;
    if (offset > totalItems - maxVisible)
      offset = totalItems - maxVisible;
    if (offset < 0)
      offset = 0;

    for (int vi = 0; vi < maxVisible && (vi + offset) < totalItems; vi++) {
      int idx = vi + offset;
      displayPtr->setCursor(0, itemStartY + vi * lineHeight);
      displayPtr->print(idx == editSubSelection ? "> " : "  ");
      if (idx < ainCount) {
        AnalogInputConfig &ain = analogInputs[idx];
        // Show name + ON/OFF + type info
        if (!ain.enabled) {
          displayPtr->printf("%s OFF", ain.name);
        } else if (ain.messageCount > 0 &&
                   ain.messages[0].type == SYSEX_SCROLL) {
          displayPtr->printf("%s %s", ain.name,
                             getSysexParamName(ain.messages[0].data1));
        } else if (ain.messageCount > 0) {
          displayPtr->printf("%s %s", ain.name,
                             getTypeName(ain.messages[0].type));
        } else {
          displayPtr->printf("%s ON", ain.name);
        }
      } else {
        displayPtr->print("<< Back");
      }
    }
    break;
  }

  case EDIT_AIN_DETAIL: {
    AnalogInputConfig &ain = analogInputs[editAinIndex];
    // Centered title
    char title[30];
    snprintf(title, sizeof(title), "-- %s --", ain.name);
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);

    // Items: Enabled, then each action (with sysex info), then Back
    int totalItems = 1 + ain.messageCount + 1; // enabled + actions + back
    int offset = editSubSelection - (maxVisible - 1) / 2;
    if (offset < 0)
      offset = 0;
    if (offset > totalItems - maxVisible)
      offset = totalItems - maxVisible;
    if (offset < 0)
      offset = 0;

    for (int vi = 0; vi < maxVisible && (vi + offset) < totalItems; vi++) {
      int i = vi + offset;
      displayPtr->setCursor(0, itemStartY + vi * lineHeight);
      displayPtr->print(i == editSubSelection ? "> " : "  ");
      if (i == 0) {
        displayPtr->printf("Enabled: %s", ain.enabled ? "ON" : "OFF");
      } else if (i <= ain.messageCount) {
        ActionMessage &m = ain.messages[i - 1];
        if (m.type == SYSEX_SCROLL) {
          displayPtr->printf("SxScr:%s %d-%d", getSysexParamName(m.data1),
                             m.minOut, m.maxOut);
        } else {
          displayPtr->printf("%s Ch%d #%d", getTypeName(m.type), m.channel,
                             m.data1);
        }
      } else {
        displayPtr->print("<< Back");
      }
    }
    break;
  }

  case EDIT_AIN_FIELD: {
    AnalogInputConfig &ain = analogInputs[editAinIndex];
    ActionMessage &m = ain.messages[editActionIndex];
    // Centered title
    char title[30];
    snprintf(title, sizeof(title), "%s Action %d", ain.name,
             editActionIndex + 1);
    int16_t x1, y1;
    uint16_t tw, th;
    displayPtr->getTextBounds(title, 0, 0, &x1, &y1, &tw, &th);
    displayPtr->setCursor((SCREEN_WIDTH - tw) / 2, headerY);
    displayPtr->print(title);

    // SysEx Scroll: show Param, MinRange, MaxRange, Back
    // Others: Type, Channel, Data1, Data2, Back
    bool isSysexScroll = (m.type == SYSEX_SCROLL);
    int fieldCount = isSysexScroll ? 5 : 5;

    for (int i = 0; i < fieldCount; i++) {
      displayPtr->setCursor(0, itemStartY + i * lineHeight);
      displayPtr->print(i == editFieldIndex ? "> " : "  ");
      if (isSysexScroll) {
        if (i == 0)
          displayPtr->printf("Type: %s", getTypeName(m.type));
        else if (i == 1)
          displayPtr->printf("Param: %s", getSysexParamName(m.data1));
        else if (i == 2)
          displayPtr->printf("Min%%: %d", m.minOut);
        else if (i == 3)
          displayPtr->printf("Max%%: %d", m.maxOut);
        else
          displayPtr->print("<< Back");
      } else {
        if (i == 0)
          displayPtr->printf("Type: %s", getTypeName(m.type));
        else if (i == 1)
          displayPtr->printf("Channel: %d", m.channel);
        else if (i == 2)
          displayPtr->printf("Data1: %d", m.data1);
        else if (i == 3)
          displayPtr->printf("Data2: %d", m.data2);
        else
          displayPtr->print("<< Back");
      }
    }
    if (inSubMenu && editFieldIndex < fieldCount - 1) {
      displayPtr->setTextSize(2);
      char val[10];
      if (editFieldIndex == 0)
        snprintf(val, sizeof(val), "%s",
                 getTypeName((MidiCommandType)editingValue));
      else if (isSysexScroll && editFieldIndex == 1)
        snprintf(val, sizeof(val), "%s", getSysexParamName(editingValue));
      else
        snprintf(val, sizeof(val), "%d", editingValue);
      int16_t vx1, vy1;
      uint16_t vw, vh;
      displayPtr->getTextBounds(val, 0, 0, &vx1, &vy1, &vw, &vh);
      int editY = itemStartY + fieldCount * lineHeight + 2;
      displayPtr->setCursor((SCREEN_WIDTH - vw) / 2, editY);
      displayPtr->print(val);
      displayPtr->setTextSize(1);
    }
    break;
  }

  default:
    break;
  }
  flushDisplay();
}

// ============================================================================
// ANALOG DEBUG SCREEN (v1.5)
// Dedicated screen for viewing all analog input values
// Activated via menu "Analog Debug" or editor checkbox
// Press encoder button to exit back to menu
// ============================================================================
void displayAnalogDebug() {
  clearDisplayBuffer();
  displayPtr->setTextColor(DISPLAY_WHITE);

  bool is32 = (oledConfig.type == OLED_128X32);
  bool is128 = (oledConfig.type == TFT_128X128);
  bool is160 = (oledConfig.type == TFT_128X160);
  int screenHeight = is160 ? 160 : (is128 ? 128 : (is32 ? 32 : 64));

  // Header
  displayPtr->setTextSize(1);
  displayPtr->setCursor(0, 0);
  displayPtr->print("ANALOG DEBUG");

  // Show exit hint on right side
  displayPtr->setCursor(90, 0);
  displayPtr->print("[ENC]");

  // Count ONLY enabled inputs (not all inputs when debug mode is on)
  int enabledCount = 0;
  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    if (analogInputs[i].enabled) {
      enabledCount++;
    }
  }

  if (enabledCount == 0) {
    // No analog inputs enabled - show helpful message
    displayPtr->setCursor(0, is32 ? 12 : 20);
    displayPtr->print("Enable Analog Inputs");
    displayPtr->setCursor(0, is32 ? 22 : 32);
    displayPtr->print("in your Config Profile");
    flushDisplay();
    return;
  }

  // Compact grid layout to fit up to 16 values
  // 128x64: 2 columns, 8 rows (need smaller text or tighter spacing)
  // 128x32: 2 columns, 2 rows (limited)
  int cols = 2;
  int colWidth = SCREEN_WIDTH / cols; // 64 pixels per column
  int startY = 10;                    // Below header
  int lineHeight = is32 ? 10 : 7;     // Tighter for 128x64 to fit 8 rows
  int maxRows = is32 ? 2 : 8;         // 8 rows x 2 cols = 16 values for 128x64

  int col = 0;
  int row = 0;
  int displayedCount = 0;

  for (int i = 0; i < MAX_ANALOG_INPUTS && displayedCount < (maxRows * cols);
       i++) {
    // Only show ENABLED analog inputs
    if (!analogInputs[i].enabled) {
      continue;
    }

    // Get raw ADC value (smoothed)
    int rawVal = (int)analogInputs[i].smoothedValue;

    // Format: "A1:0000" - always use index+1 for label
    char buf[12];
    snprintf(buf, sizeof(buf), "A%d:%04d", i + 1, rawVal);

    int x = col * colWidth;
    int y = startY + (row * lineHeight);

    displayPtr->setCursor(x, y);
    displayPtr->print(buf);

    displayedCount++;
    col++;
    if (col >= cols) {
      col = 0;
      row++;
    }
  }

  flushDisplay();
}

void updateLeds() {
  // USB MIDI MODE: LEDs are disabled on ESP32-S3 (RMT/USB hardware conflict)
  // strip.begin() was never called, so skip all LED processing
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (systemConfig.bleMode == MIDI_USB_ONLY) {
    return; // LEDs disabled in USB MIDI mode
  }
#endif

  // WiFi-safe LED updates: use increased heap threshold and rate limiting
  // ESP32 RMT peripheral handles NeoPixel timing, but WiFi can still
  // interfere

  // Skip LED updates when heap is critically low
  // Threshold lowered for TFT+BLE configs which use more memory (~12KB typical)
  int heapThreshold = isWifiOn ? 8000 : 10000;
  if (ESP.getFreeHeap() < heapThreshold) {
    static unsigned long lastHeapWarn = 0;
    if (millis() - lastHeapWarn > 5000) { // Only warn every 5 seconds
      Serial.printf("LED update skipped: heap %d < %d\n", ESP.getFreeHeap(),
                    heapThreshold);
      lastHeapWarn = millis();
    }
    return; // Safety: prevent crash from memory pressure
  }

  // Rate limit: don't update LEDs more than every 50ms when WiFi is on
  // This gives WiFi stack time to process packets between LED updates
  static unsigned long lastLedUpdate = 0;
  if (isWifiOn && (millis() - lastLedUpdate < 50)) {
    return; // Throttle to prevent WiFi interference
  }
  lastLedUpdate = millis();

  bool needsUpdate = false;

  // Update tap tempo blink timing (non-blocking state machine)
  if (currentMode == 0 && currentBPM > 0) {
    // Calculate the FINAL delay ms (with rhythm pattern applied) - same as
    // sent to SPM
    float finalDelayMs =
        (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
    unsigned long blinkInterval = (unsigned long)finalDelayMs;
    unsigned long now = millis();

    // State machine: OFF -> ON (at beat) -> OFF (after 50ms flash)
    if (tapBlinkState && (now - lastTapBlinkTime >= 50)) {
      tapBlinkState = false;
    } else if (!tapBlinkState && (now - lastTapBlinkTime >= blinkInterval)) {
      tapBlinkState = true;
      lastTapBlinkTime = now;
    }
  } else {
    tapBlinkState = false;
  }

  // LEDs per button - determines single LED mode (with mapping) vs strip mode
  // (sequential)
  uint8_t lpb = systemConfig.ledsPerButton;
  if (lpb < 1)
    lpb = 1; // Safety minimum

  for (int i = 0; i < systemConfig.buttonCount; i++) {
    const ButtonConfig &config = buttonConfigs[currentPreset][i];

    // Safety: bounds check messageCount to prevent garbage data crashes
    uint8_t safeMessageCount = config.messageCount;
    if (safeMessageCount > MAX_ACTIONS_PER_BUTTON)
      safeMessageCount = 0;

    // Find the active message (PRESS or 2ND_PRESS based on toggle state)
    const ActionMessage *msg = nullptr;
    ActionType targetAction =
        config.isAlternate ? ACTION_2ND_PRESS : ACTION_PRESS;
    for (int m = 0; m < safeMessageCount; m++) {
      if (config.messages[m].action == targetAction) {
        msg = &config.messages[m];
        break;
      }
    }
    // Fallback to first message if target action not found
    if (!msg && safeMessageCount > 0) {
      msg = &config.messages[0];
    }

    // TAP_TEMPO buttons use blink state, others use normal brightness
    bool isTapTempo = (msg && msg->type == TAP_TEMPO);
    int brightness;

    if (isTapTempo && tapBlinkState) {
      brightness = ledBrightnessTap; // Use configured brightness during flash
    } else if (isTapTempo) {
      // v1.5.5: TAP_TEMPO buttons always show at dim state when not blinking
      // This ensures they're visible even when not in tap tempo mode
      brightness = ledBrightnessDim > 0 ? ledBrightnessDim : 30;
    } else {
      // Determine LED active state based on preset LED mode
      bool ledActive;
      PresetLedMode presetMode = presetLedModes[currentPreset];
      int8_t selectedBtn = presetSelectionState[currentPreset];

      if (presetMode == PRESET_LED_SELECTION) {
        // All buttons in selection mode - selected button is ON
        ledActive = (i == selectedBtn);
      } else if (presetMode == PRESET_LED_HYBRID && config.inSelectionGroup) {
        // This button is in selection group - check if it's the selected one
        ledActive = (i == selectedBtn);
      } else {
        // Normal mode or Hybrid non-selection button - use existing
        // Toggle/Momentary logic
        if (config.ledMode == LED_TOGGLE) {
          ledActive = ledToggleState[i]; // Use toggle state (persistent)
        } else {
          ledActive =
              buttonPinActive[i]; // Use physical press state (momentary)
        }
      }
      brightness = ledActive ? ledBrightnessOn : ledBrightnessDim;
    }

    // Get RGB from message (default to dim purple if no message)
    int r = msg ? (msg->rgb[0] * brightness) / 255 : 0;
    int g = msg ? (msg->rgb[1] * brightness) / 255 : 0;
    int b = msg ? (msg->rgb[2] * brightness) / 255 : 0;

    uint32_t newColor = strip.Color(r, g, b);

    if (lpb == 1) {
      // SINGLE LED MODE: Use systemConfig.ledMap for backward compatibility
      // ledMap remaps button index to physical LED index
      if (isTapTempo || newColor != lastLedColors[i]) {
        strip.setPixelColor(systemConfig.ledMap[i], newColor);
        lastLedColors[i] = newColor;
        needsUpdate = true;
      }
    } else {
      // STRIP MODE: Each button controls a segment of consecutive LEDs
      // Button i controls LEDs: (i * lpb) to ((i+1) * lpb - 1)
      int startLed = i * lpb;
      int endLed = startLed + lpb;

      for (int led = startLed; led < endLed; led++) {
        if (isTapTempo || newColor != lastLedColors[i]) {
          strip.setPixelColor(led, newColor);
          needsUpdate = true;
        }
      }
      lastLedColors[i] = newColor; // Track by button
    }
  }

  // Only call strip.show() if something actually changed
  if (needsUpdate) {
    // USB MIDI mode: Only update LEDs on preset change to avoid RMT/USB
    // conflict
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    if (systemConfig.bleMode == MIDI_USB_ONLY) {
      if (usbMidiLedUpdatePending) {
        yield();
        strip.show();
        yield();
        usbMidiLedUpdatePending = false; // Reset flag after update
      }
      // Skip show() if not pending - LEDs only change on preset change in USB
      // mode
    } else {
      yield(); // Give WiFi stack time before LED update
      strip.show();
      yield(); // Give WiFi stack time after LED update
    }
#else
    yield(); // Give WiFi stack time before LED update
    strip.show();
    yield(); // Give WiFi stack time after LED update
#endif
  }
}

void updateIndividualLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index >= strip.numPixels())
    return;
  strip.setPixelColor(index, strip.Color(r, g, b));
  strip.show();
}

void blinkAllLeds() {
  // Save current state
  uint32_t savedColors[NUM_LEDS];
  for (int i = 0; i < NUM_LEDS; i++) {
    savedColors[i] = strip.getPixelColor(i);
  }

  // Flash at REDUCED brightness (25% instead of 100%)
  // Prevents power spike: 60mA instead of 480mA
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(64, 64, 64)); // Was 255,255,255
  }
  strip.show();
  delay(100);

  // Restore
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, savedColors[i]);
  }
  strip.show();
}

void blinkTapButton(int buttonIndex) {
  // Blink only the tap tempo button's LED using configurable brightness
  uint8_t lpb = systemConfig.ledsPerButton;
  if (lpb < 1)
    lpb = 1;

  // Calculate white color at configured brightness
  uint8_t tapBright = constrain(ledBrightnessTap, 0, 255);

  if (lpb == 1) {
    // Single LED mode - use ledMap
    int ledIndex = systemConfig.ledMap[buttonIndex];
    uint32_t savedColor = strip.getPixelColor(ledIndex);

    strip.setPixelColor(ledIndex, strip.Color(tapBright, tapBright, tapBright));
    strip.show();
    delay(50);

    strip.setPixelColor(ledIndex, savedColor);
    strip.show();
  } else {
    // Strip mode - button controls multiple LEDs
    int startLed = buttonIndex * lpb;
    int endLed = startLed + lpb;
    uint32_t savedColors[lpb];

    // Save and flash
    for (int i = startLed; i < endLed; i++) {
      savedColors[i - startLed] = strip.getPixelColor(i);
      strip.setPixelColor(i, strip.Color(tapBright, tapBright, tapBright));
    }
    strip.show();
    delay(50);

    // Restore
    for (int i = startLed; i < endLed; i++) {
      strip.setPixelColor(i, savedColors[i - startLed]);
    }
    strip.show();
  }
}

// ============================================================================
// OLED Health Monitoring & Auto-Recovery System
// ============================================================================

bool checkOledHealth() {
  // NOTE: Uses global oledHealthy from Globals.cpp (not static local)
  static unsigned long lastOledCheck = 0;

  // TFT displays use SPI, not I2C - always consider them healthy
  if (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160) {
    return true;
  }

  if (displayPtr == nullptr) {
    return false;
  }

  // Check every 500ms to avoid overhead
  if (millis() - lastOledCheck < 500) {
    return oledHealthy;
  }
  lastOledCheck = millis();

  // Try a simple I2C communication test to 0x3C (OLED address)
  Wire.beginTransmission(0x3C);
  byte error = Wire.endTransmission();

  if (error != 0) {
    // OLED not responding
    if (oledHealthy) {
      Serial.println("OLED not responding, attempting recovery...");
    }
    oledHealthy = false;
    recoverOled();
  } else {
    if (!oledHealthy) {
      Serial.println("OLED communication restored");
    }
    oledHealthy = true;
  }

  return oledHealthy;
}

void recoverOled() {
  Serial.println("Attempting display recovery...");

  // Try to reinitialize display
  bool success = false;
  if (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160) {
    // TFT uses SPI, not I2C - skip I2C recovery
    // For TFT, we could potentially re-init the display, but typically SPI
    // doesn't fail
    success = true;
  } else {
    // Reset I2C bus for OLED displays
    Wire.end();
    delay(10);
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(100000); // 100kHz for stability
    delay(10);

    // Reinitialize OLED
    success = static_cast<Adafruit_SSD1306 *>(displayPtr)
                  ->begin(SSD1306_SWITCHCAPVCC, 0x3C);
  }

  if (success) {
    oledHealthy = true;
    Serial.println("OLED recovered successfully!");
    displayOLED(); // Redraw screen
  } else {
    Serial.println("OLED recovery failed - will retry");
  }
}

void safeDisplayOLED() {
  // v1.5.5: Removed I2C health check as it interferes with BLE scanning
  // and causes display freezes. The OLED library handles errors internally.
  displayOLED();
}

// Hardware Init
void initDisplayHardware() {
  // Skip display initialization if OLED_NONE is set
  if (oledConfig.type == OLED_NONE) {
    Serial.println("Display type is NONE - skipping initialization");
    displayPtr = nullptr;
    return;
  }

  if (displayPtr != nullptr) {
    delete displayPtr;
    displayPtr = nullptr;
  }

  if (oledConfig.type == TFT_128X128 || oledConfig.type == TFT_128X160) {
    // ST7735 TFT Initialization - use configurable pins
    bool is160 = (oledConfig.type == TFT_128X160);
    Serial.printf("Initializing TFT %s...\n", is160 ? "128x160" : "128x128");
    Serial.printf("SPI Pins - CS:%d, DC:%d, RST:%d, MOSI:%d, SCLK:%d, LED:%d\n",
                  systemConfig.tftCsPin, systemConfig.tftDcPin,
                  systemConfig.tftRstPin, systemConfig.tftMosiPin,
                  systemConfig.tftSclkPin, systemConfig.tftLedPin);

    Adafruit_ST7735 *tft = new Adafruit_ST7735(
        systemConfig.tftCsPin, systemConfig.tftDcPin, systemConfig.tftRstPin);

    // Initialize based on display type
    if (is160) {
      Serial.println("Calling initR(INITR_BLACKTAB) for 128x160...");
      tft->initR(INITR_BLACKTAB); // 128x160 1.8" displays
      // Alternative initializations to try if BLACKTAB doesn't work:
      // tft->initR(INITR_GREENTAB);   // Try this if BLACKTAB doesn't work
      // tft->initR(INITR_18GREENTAB); // Some 1.8" displays need this
    } else {
      Serial.println("Calling initR(INITR_144GREENTAB) for 128x128...");
      tft->initR(INITR_144GREENTAB); // 128x128 1.44" displays
      // Alternative initializations to try if GREENTAB doesn't work:
      // tft->initR(INITR_BLACKTAB);   // Try this if GREENTAB doesn't work
      // tft->initR(INITR_REDTAB);     // Or this
    }

    Serial.println("Setting rotation...");
    // Apply rotation (Config stores 0-3 index)
    tft->setRotation(oledConfig.rotation);

    Serial.println("Clearing screen...");
    tft->fillScreen(ST7735_BLACK);

    // Setup Backlight - use configurable pin
    Serial.println("Setting up backlight...");
    pinMode(systemConfig.tftLedPin, OUTPUT);
    digitalWrite(systemConfig.tftLedPin, HIGH); // Turn on backlight

    displayPtr = tft;
    Serial.printf("TFT %s Initialized Successfully!\n",
                  is160 ? "128x160" : "128x128");
  } else {
    // SSD1306 OLED Initialization - use configurable I2C pins
    int h = (oledConfig.type == OLED_128X32) ? 32 : 64;
    Serial.printf("Initializing OLED %dx%d on I2C: SDA=%d, SCL=%d\n",
                  SCREEN_WIDTH, h, systemConfig.oledSdaPin,
                  systemConfig.oledSclPin);

    Adafruit_SSD1306 *oled =
        new Adafruit_SSD1306(SCREEN_WIDTH, h, &Wire, OLED_RESET);

    if (!oled->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed (re-init)"));
      return;
    }

    oled->setRotation(oledConfig.rotation);
    oled->clearDisplay();
    oled->setTextColor(SSD1306_WHITE);
    oled->display();

    displayPtr = oled;
    Serial.printf("OLED Initialized: %dx%d, rotation=%d\n", SCREEN_WIDTH, h,
                  oledConfig.rotation);
  }
}
