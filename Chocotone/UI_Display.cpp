#include "UI_Display.h"
#include <Wire.h>

// MIDI Note Names
const char* MIDI_NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void midiNoteNumberToString(char* b, size_t s, int n){
    if(n<0||n>127){snprintf(b,s,"N/A");return;}
    snprintf(b,s,"%s%d",MIDI_NOTE_NAMES[n%12],(n/12-1));
}

// Get button summary from command type and data
void getButtonSummary(char* b, size_t s, MidiCommandType type, int data1) {
    char n[8]; b[0] = '\0';
    switch(type){
        case NOTE_MOMENTARY: midiNoteNumberToString(n,8,data1); snprintf(b,s,"%.4s", n); break;
        case NOTE_ON:  midiNoteNumberToString(n,8,data1); snprintf(b,s,"^%.3s",n); break;
        case NOTE_OFF: midiNoteNumberToString(n,8,data1); snprintf(b,s,"v%.3s",n); break;
        case CC: snprintf(b,s,"CC%d",data1); break;
        case PC: snprintf(b,s,"PC%d",data1); break;
        case TAP_TEMPO: strncpy(b,"TAP",s-1); b[s-1] = '\0'; break;
        case PRESET_UP: strncpy(b,">",s-1); b[s-1] = '\0'; break;
        case PRESET_DOWN: strncpy(b,"<",s-1); b[s-1] = '\0'; break;
        case PRESET_1: snprintf(b,s,"%.10s",presetNames[0]); break;
        case PRESET_2: snprintf(b,s,"%.10s",presetNames[1]); break;
        case PRESET_3: snprintf(b,s,"%.10s",presetNames[2]); break;
        case PRESET_4: snprintf(b,s,"%.10s",presetNames[3]); break;
        case WIFI_TOGGLE: strncpy(b,"WiFi",s-1); b[s-1] = '\0'; break;
        case CLEAR_BLE_BONDS: strncpy(b,"xBLE",s-1); b[s-1] = '\0'; break;
        case SYSEX: strncpy(b,"SYS",s-1); b[s-1] = '\0'; break;
        case MIDI_OFF: strncpy(b,"OFF",s-1); b[s-1] = '\0'; break;
        default: strncpy(b,"---",s-1); b[s-1] = '\0'; break;
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

    display.clearDisplay();
    display.setTextSize(1);
    char summary[10];

    // Dynamic layout based on button count
    int btnCount = systemConfig.buttonCount;
    int buttonsPerRow = (btnCount > 8) ? 5 : 4;  // 5 per row for 9-10 buttons
    int colWidth = SCREEN_WIDTH / buttonsPerRow;  // 25 or 32 pixels
    int maxChars = (btnCount > 8) ? 3 : 4;  // Fewer chars when cramped

    // Top Row (upper half of buttons)
    int topRowStart = btnCount / 2;  // 5 for 10 buttons, 4 for 8 buttons
    for (int i = 0; i < buttonsPerRow && (topRowStart + i) < btnCount; i++) {
        int buttonIndex = topRowStart + i;
        const ButtonConfig& config = buttonConfigs[currentPreset][buttonIndex];
        char defaultName[21];
        snprintf(defaultName, sizeof(defaultName), "B%d", buttonIndex + 1);
        char toDisplay[10];
        if (strncmp(config.name, defaultName, 20) == 0 || strlen(config.name) == 0) {
            // Get first PRESS action for summary
            ActionMessage* firstAction = (config.messageCount > 0) ? (ActionMessage*)&config.messages[0] : nullptr;
            if (firstAction) {
                getButtonSummary(summary, sizeof(summary), firstAction->type, firstAction->data1);
            } else {
                strncpy(summary, "---", sizeof(summary));
            }
            snprintf(toDisplay, sizeof(toDisplay), "%.*s", maxChars, summary);
        } else {
            snprintf(toDisplay, sizeof(toDisplay), "%.*s", maxChars, config.name);
        }
        display.setCursor(i * colWidth, 0);
        display.print(toDisplay);
    }

    // Bottom Row (lower half of buttons)
    int bottomY = SCREEN_HEIGHT - 8;
    for (int i = 0; i < buttonsPerRow && i < topRowStart; i++) {
        int buttonIndex = i;
        const ButtonConfig& config = buttonConfigs[currentPreset][buttonIndex];
        char defaultName[21];
        snprintf(defaultName, sizeof(defaultName), "B%d", buttonIndex + 1);
        char toDisplay[10];
        if (strncmp(config.name, defaultName, 20) == 0 || strlen(config.name) == 0) {
            // Get first PRESS action for summary
            ActionMessage* firstAction = (config.messageCount > 0) ? (ActionMessage*)&config.messages[0] : nullptr;
            if (firstAction) {
                getButtonSummary(summary, sizeof(summary), firstAction->type, firstAction->data1);
            } else {
                strncpy(summary, "---", sizeof(summary));
            }
            snprintf(toDisplay, sizeof(toDisplay), "%.*s", maxChars, summary);
        } else {
            snprintf(toDisplay, sizeof(toDisplay), "%.*s", maxChars, config.name);
        }
        display.setCursor(i * colWidth, bottomY);
        display.print(toDisplay);
    }

    // Middle Area - Preset Name
    int16_t x1, y1;
    uint16_t w, h;
    char truncatedName[11];
    snprintf(truncatedName, sizeof(truncatedName), "%.10s", presetNames[currentPreset]);
    display.setTextSize(2);
    display.getTextBounds(truncatedName, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 14);
    display.print(truncatedName);
    display.setTextSize(1);

    // BPM Display
    char bpmStr[16];
    snprintf(bpmStr, sizeof(bpmStr), "BPM:%.1f", currentBPM);
    display.getTextBounds(bpmStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 32);
    display.print(bpmStr);

    // BLE Connection Status + WiFi Status
    display.setCursor(0, 44);
    display.printf("BLE:%c", clientConnected?'Y':'N');
    display.setCursor(90, 44);
    display.printf("WiFi:%c", isWifiOn?'Y':'N');

    // Last Sent
    lastSentMidiString[19] = '\0';
    display.getTextBounds(lastSentMidiString, 0, 0, &x1, &y1, &w, &h);
    int16_t lastNoteX = SCREEN_WIDTH - w - 2;
    if (lastNoteX < SCREEN_WIDTH / 2) lastNoteX = SCREEN_WIDTH / 2;
    display.setCursor(lastNoteX, 44);
    display.print(lastSentMidiString);

    display.display();
}

void displayTapTempoMode() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Top row: NEXT (left) and LOCK indicator (right)
    display.setCursor(0, 0);
    display.print("NEXT");
    
    // Lock status indicator in top-right
    display.setCursor(90, 0);
    if (tapModeLocked) {
        display.print("LOCKED");
    } else {
        display.print("LOCK");
    }
    
    // BPM - larger, centered
    display.setTextSize(3);
    char bpmStr[8];
    snprintf(bpmStr, sizeof(bpmStr), "%.1f", currentBPM);
    int16_t bx1, by1;
    uint16_t bw, bh;
    display.getTextBounds(bpmStr, 0, 0, &bx1, &by1, &bw, &bh);
    display.setCursor((SCREEN_WIDTH - bw) / 2, 16);
    display.print(bpmStr);
    
    // Middle-bottom: Pattern and delay time
    display.setTextSize(1);
    display.setCursor(0, 46);
    display.print("Pattern: ");
    display.print(rhythmNames[rhythmPattern]);
    
    // Delay ms on right of middle row
    float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
    int delayTimeMS = (int)finalDelayMs;
    char delayStr[12];
    snprintf(delayStr, sizeof(delayStr), "%dms", delayTimeMS);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(delayStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w - 2, 46);
    display.print(delayStr);
    
    // Bottom row: PREV (left) and TAP (right)
    display.setCursor(0, 56);
    display.print("PREV");
    
    display.setCursor(100, 56);
    display.print("TAP");
    
    display.display();
}

void displayButtonName() {
    display.clearDisplay();
    display.setTextSize(buttonNameFontSize);
    display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buttonNameToShow, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
    display.print(buttonNameToShow);
    display.display();
    display.setTextSize(1);
}

void displayMenu() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Build menu items dynamically to show status
    char menuItems[12][25];  // Array to hold menu item strings
    strncpy(menuItems[0], "Save and Exit", 25);
    strncpy(menuItems[1], "Exit without Saving", 25);
    snprintf(menuItems[2], 25, "Wi-Fi Editor (%s)", isWifiOn ? "ON" : "OFF");
    strncpy(menuItems[3], "LED Bright (On)", 25);
    strncpy(menuItems[4], "LED Bright (Dim)", 25);
    strncpy(menuItems[5], "Pad Debounce", 25);
    strncpy(menuItems[6], "Clear BLE Bonds", 25);
    strncpy(menuItems[7], "Reboot", 25);
    strncpy(menuItems[8], "Factory Reset", 25);
    strncpy(menuItems[9], "Name Font Size", 25);
    snprintf(menuItems[10], 25, "Wifi %s at Boot", systemConfig.wifiOnAtBoot ? "ON" : "OFF");
    // BLE Mode display
    const char* bleModeStr = systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT" :
                             systemConfig.bleMode == BLE_DUAL_MODE ? "DUAL" : "SERVER";
    snprintf(menuItems[11], 25, "BLE: %s", bleModeStr);
    
    int numMenuItems = 12;

    display.setCursor(0, 0);
    display.printf("-- Menu CHOCOTONE --");

    if (inSubMenu) {
        display.setCursor(10, 20);
        display.printf("Editing: %s", menuItems[menuSelection]);
        display.setTextSize(2);
        char valueStr[10];
        snprintf(valueStr, sizeof(valueStr), "%d", editingValue);
        int16_t x1, y1; uint16_t w, h;
        display.getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((SCREEN_WIDTH - w) / 2, 40);
        display.print(valueStr);
        display.setTextSize(1);
    } else {
        // Show 5 lines at a time, scrolling
        int lineOffset = menuSelection - 2;
        if (lineOffset < 0) lineOffset = 0;
        if (lineOffset > numMenuItems - 5) lineOffset = numMenuItems - 5;
        
        for (int i = 0; i < numMenuItems; i++) {
            int displayLine = i - lineOffset;
            if (displayLine >= 0 && displayLine < 5) {
                display.setCursor(0, 14 + (displayLine * 10));
                if (i == menuSelection) {
                    display.print("> ");
                } else {
                    display.print("  ");
                }
                display.print(menuItems[i]);
            }
        }
    }
    display.display();
}

void updateLeds() {
    // WiFi-safe LED updates: use increased heap threshold and rate limiting
    // ESP32 RMT peripheral handles NeoPixel timing, but WiFi can still interfere
    
    // Skip LED updates when heap is critically low
    // Threshold lowered to allow updates even with WiFi on (heap ~13KB)
    int heapThreshold = isWifiOn ? 10000 : 15000;
    if (ESP.getFreeHeap() < heapThreshold) {
        Serial.printf("LED update skipped: heap %d < %d\n", ESP.getFreeHeap(), heapThreshold);
        return;  // Safety: prevent crash from memory pressure
    }
    
    // Rate limit: don't update LEDs more than every 50ms when WiFi is on
    // This gives WiFi stack time to process packets between LED updates
    static unsigned long lastLedUpdate = 0;
    if (isWifiOn && (millis() - lastLedUpdate < 50)) {
        return;  // Throttle to prevent WiFi interference
    }
    lastLedUpdate = millis();
    
    bool needsUpdate = false;
    
    // Update tap tempo blink timing (non-blocking state machine)
    // Now uses rhythm-adjusted delay value and works in tap tempo mode too
    if (currentMode == 0 && currentBPM > 0) {
        // Calculate the FINAL delay ms (with rhythm pattern applied) - same as sent to SPM
        float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
        unsigned long blinkInterval = (unsigned long)finalDelayMs;
        unsigned long now = millis();
        
        // State machine: OFF → ON (at beat) → OFF (after 50ms flash)
        if (tapBlinkState && (now - lastTapBlinkTime >= 50)) {
            tapBlinkState = false;
        } else if (!tapBlinkState && (now - lastTapBlinkTime >= blinkInterval)) {
            tapBlinkState = true;
            lastTapBlinkTime = now;
        }
    } else {
        tapBlinkState = false;
    }
    
    // LEDs per button - determines single LED mode (with mapping) vs strip mode (sequential)
    uint8_t lpb = systemConfig.ledsPerButton;
    if (lpb < 1) lpb = 1;  // Safety minimum
    
    for (int i = 0; i < systemConfig.buttonCount; i++) {
        const ButtonConfig& config = buttonConfigs[currentPreset][i];
        
        // Safety: bounds check messageCount to prevent garbage data crashes
        uint8_t safeMessageCount = config.messageCount;
        if (safeMessageCount > MAX_ACTIONS_PER_BUTTON) safeMessageCount = 0;
        
        // Find the active message (PRESS or 2ND_PRESS based on toggle state)
        const ActionMessage* msg = nullptr;
        ActionType targetAction = config.isAlternate ? ACTION_2ND_PRESS : ACTION_PRESS;
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
            brightness = 255;  // Full bright during flash
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
                // Normal mode or Hybrid non-selection button - use existing Toggle/Momentary logic
                if (config.ledMode == LED_TOGGLE) {
                    ledActive = ledToggleState[i];  // Use toggle state (persistent)
                } else {
                    ledActive = buttonPinActive[i];  // Use physical press state (momentary)
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
            lastLedColors[i] = newColor;  // Track by button
        }
    }
    
    // Only call strip.show() if something actually changed
    if (needsUpdate) {
        yield();  // Give WiFi stack time before LED update
        strip.show();
        yield();  // Give WiFi stack time after LED update
    }
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
        strip.setPixelColor(i, strip.Color(64, 64, 64));  // Was 255,255,255
    }
    strip.show();
    delay(100);
    
    // Restore
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, savedColors[i]);
    }
    strip.show();
}

// ============================================================================
// OLED Health Monitoring & Auto-Recovery System
// ============================================================================

bool checkOledHealth() {
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
            Serial.println("⚠️  OLED not responding, attempting recovery...");
        }
        oledHealthy = false;
        recoverOled();
    } else {
        if (!oledHealthy) {
            Serial.println("✅ OLED communication restored");
        }
        oledHealthy = true;
    }
    
    return oledHealthy;
}

void recoverOled() {
    Serial.println("🔧 Attempting OLED recovery...");
    
    // Reset I2C bus
    Wire.end();
    delay(10);
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(100000);  // 100kHz for stability
    delay(10);
    
    // Try to reinitialize display
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        oledHealthy = true;
        Serial.println("✅ OLED recovered successfully!");
        displayOLED();  // Redraw screen
    } else {
        Serial.println("❌ OLED recovery failed - will retry");
    }
}

void safeDisplayOLED() {
    // Check OLED health before updating
    if (checkOledHealth()) {
        displayOLED();
    }
    // If unhealthy, recovery is already attempted by checkOledHealth()
}

