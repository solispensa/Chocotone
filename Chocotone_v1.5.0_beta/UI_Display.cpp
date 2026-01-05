#include "UI_Display.h"
#include "AnalogInput.h"
#include <SPI.h> // For TFT displays
#include <Wire.h>

// Color abstraction
#define DISPLAY_WHITE                                                          \
  (oledConfig.type == TFT_128X128 ? ST7735_WHITE : SSD1306_WHITE)
#define DISPLAY_BLACK                                                          \
  (oledConfig.type == TFT_128X128 ? ST7735_BLACK : SSD1306_BLACK)

// Helper to flush display (OLED needs this, TFT doesn't or at least not the
// same way)
void flushDisplay() {
  if (displayPtr == nullptr)
    return;
  if (oledConfig.type != TFT_128X128) {
    static_cast<Adafruit_SSD1306 *>(displayPtr)->display();
  }
}

// Helper to clear display buffer
void clearDisplayBuffer() {
  if (displayPtr == nullptr)
    return;
  if (oledConfig.type == TFT_128X128) {
    displayPtr->fillScreen(ST7735_BLACK);
  } else {
    static_cast<Adafruit_SSD1306 *>(displayPtr)->clearDisplay();
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
  int screenHeight = (oledConfig.type == OLED_128X32) ? 32 : 64;
  if (oledConfig.type == OLED_128X32 && bottomRowY > 24) {
    bottomRowY = 24; // Clamp for 128x32
  }

  // Debug: Print layout values (comment out after debugging)
  static unsigned long lastLayoutLog = 0;
  if (millis() - lastLayoutLog > 10000) { // Print every 10 seconds
    lastLayoutLog = millis();
    Serial.printf("[OLED Layout] type=%d, topY=%d, titleY=%d, statusY=%d, "
                  "bottomY=%d, sizes=%d/%d/%d\n",
                  oledConfig.type, topRowY, titleY, statusY, bottomRowY,
                  labelSize, titleSize, statusSize);
  }

  displayPtr->setTextSize(labelSize);
  char summary[10];

  // Dynamic layout based on button count and orientation
  bool isVertical = (oledConfig.rotation == 90 || oledConfig.rotation == 270);
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
          displayPtr->setCursor(i * dynColWidth, topRowY);
          displayPtr->print(label);

          // Draw color strip below label for TFT when enabled
          if (oledConfig.type == TFT_128X128 &&
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
                displayPtr->fillRect(i * dynColWidth, stripY, stripW, stripH,
                                     color);
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
                  displayPtr->fillRect(i * dynColWidth, stripY, maxStripW,
                                       stripH, color);
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
          displayPtr->setCursor(i * dynColWidth, bottomRowY);
          displayPtr->print(label);

          // Draw color strip above label for TFT when enabled
          if (oledConfig.type == TFT_128X128 &&
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
                displayPtr->fillRect(i * dynColWidth, stripY, stripW, stripH,
                                     color);
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
                  displayPtr->fillRect(i * dynColWidth, stripY, maxStripW,
                                       stripH, color);
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
      // Normal mode - show BLE state
      const char *bleMode = bleConfigMode                             ? "EDIT"
                            : systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT"
                            : systemConfig.bleMode == BLE_SERVER_ONLY ? "SERVER"
                                                                      : "DUAL";
      const char *bleState = clientConnected ? "Synced" : "Scanning";

      // For SERVER mode or EDIT mode, don't show scanning
      if (systemConfig.bleMode == BLE_SERVER_ONLY || bleConfigMode) {
        bleState = clientConnected ? "Synced" : "";
      }

      char statusLine[32];
      if (strlen(bleState) > 0) {
        snprintf(statusLine, sizeof(statusLine), "BLE:%s %s", bleMode,
                 bleState);
      } else {
        snprintf(statusLine, sizeof(statusLine), "BLE:%s", bleMode);
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

  flushDisplay();
}

void displayTapTempoMode() {
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
  clearDisplayBuffer();

  // Get layout config from oledConfig.overlay (unified with menu "Name Font
  // Size")
  uint8_t textSize = oledConfig.overlay.titleSize > 0
                         ? oledConfig.overlay.titleSize
                         : 2; // Default to 2 if not set
  int screenHeight = (oledConfig.type == TFT_128X128)   ? 128
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

  displayPtr->setTextSize(itemSize);

  // Build menu items dynamically to show status
  char menuItems[14][25]; // Array to hold menu item strings
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
  // BLE Mode display - includes EDIT option for config mode
  const char *bleModeStr = bleConfigMode                             ? "EDIT"
                           : systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT"
                           : systemConfig.bleMode == BLE_DUAL_MODE   ? "DUAL"
                                                                     : "SERVER";
  snprintf(menuItems[12], 25, "BLE Mode: %s", bleModeStr);
  snprintf(menuItems[13], 25, "Analog Debug %s",
           systemConfig.debugAnalogIn ? "ON" : "OFF");

  int numMenuItems = 14;

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
  int screenHeight = is128 ? 128 : (is32 ? 32 : 64);

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
  // WiFi-safe LED updates: use increased heap threshold and rate limiting
  // ESP32 RMT peripheral handles NeoPixel timing, but WiFi can still
  // interfere

  // Skip LED updates when heap is critically low
  // Threshold lowered to allow updates even with WiFi on (heap ~13KB)
  int heapThreshold = isWifiOn ? 10000 : 15000;
  if (ESP.getFreeHeap() < heapThreshold) {
    Serial.printf("LED update skipped: heap %d < %d\n", ESP.getFreeHeap(),
                  heapThreshold);
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
  // Now uses rhythm-adjusted delay value and works in tap tempo mode too
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
      brightness = 255; // Full bright during flash
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
    yield(); // Give WiFi stack time before LED update
    strip.show();
    yield(); // Give WiFi stack time after LED update
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
  static bool oledHealthy = true;
  static unsigned long lastOledCheck = 0;

  // TFT displays use SPI, not I2C - always consider them healthy
  if (oledConfig.type == TFT_128X128) {
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
  if (oledConfig.type == TFT_128X128) {
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
  // Check OLED health before updating
  if (checkOledHealth()) {
    displayOLED();
  }
  // If unhealthy, recovery is already attempted by checkOledHealth()
}

// Hardware Init
void initDisplayHardware() {
  if (displayPtr != nullptr) {
    delete displayPtr;
    displayPtr = nullptr;
  }

  if (oledConfig.type == TFT_128X128) {
    // ST7735 TFT 128x128 Initialization - use configurable pins
    Serial.println("Initializing TFT 128x128...");
    Serial.printf("SPI Pins - CS:%d, DC:%d, RST:%d, MOSI:%d, SCLK:%d, LED:%d\n",
                  systemConfig.tftCsPin, systemConfig.tftDcPin,
                  systemConfig.tftRstPin, systemConfig.tftMosiPin,
                  systemConfig.tftSclkPin, systemConfig.tftLedPin);

    Adafruit_ST7735 *tft = new Adafruit_ST7735(
        systemConfig.tftCsPin, systemConfig.tftDcPin, systemConfig.tftRstPin);

    // Try different init methods - comment/uncomment as needed
    Serial.println("Calling initR(INITR_144GREENTAB)...");
    tft->initR(INITR_144GREENTAB); // 128x128 1.44" displays
    // Alternative initializations to try if above doesn't work:
    // tft->initR(INITR_BLACKTAB);   // Try this if GREENTAB doesn't work
    // tft->initR(INITR_REDTAB);     // Or this

    Serial.println("Setting rotation...");
    // Apply rotation (Config uses 0, 90, 180, 270)
    tft->setRotation(oledConfig.rotation / 90);

    Serial.println("Clearing screen...");
    tft->fillScreen(ST7735_BLACK);

    // Setup Backlight - use configurable pin
    Serial.println("Setting up backlight...");
    pinMode(systemConfig.tftLedPin, OUTPUT);
    digitalWrite(systemConfig.tftLedPin, HIGH); // Turn on backlight

    displayPtr = tft;
    Serial.println("TFT 128x128 Initialized Successfully!");
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

    oled->setRotation(oledConfig.rotation / 90);
    oled->clearDisplay();
    oled->setTextColor(SSD1306_WHITE);
    oled->display();

    displayPtr = oled;
    Serial.printf("OLED Initialized: %dx%d, rotation=%d\n", SCREEN_WIDTH, h,
                  oledConfig.rotation);
  }
}
