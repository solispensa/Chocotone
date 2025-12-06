#include "UI_Display.h"
#include <Wire.h>

// MIDI Note Names
const char* MIDI_NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void midiNoteNumberToString(char* b, size_t s, int n){
    if(n<0||n>127){snprintf(b,s,"N/A");return;}
    snprintf(b,s,"%s%d",MIDI_NOTE_NAMES[n%12],(n/12-1));
}

void getButtonSummary(char* b, size_t s, const MidiMessage& c) {
    char n[8]; b[0] = '\0';
    switch(c.type){
        case NOTE_MOMENTARY: midiNoteNumberToString(n,8,c.data1); snprintf(b,s,"%.4s", n); break;
        case NOTE_ON:  midiNoteNumberToString(n,8,c.data1); snprintf(b,s,"^%.3s",n); break;
        case NOTE_OFF: midiNoteNumberToString(n,8,c.data1); snprintf(b,s,"v%.3s",n); break;
        case CC: snprintf(b,s,"CC%d",c.data1); break;
        case PC: snprintf(b,s,"PC%d",c.data1); break;
        case TAP_TEMPO: strncpy(b,"TAP",s-1); b[s-1] = '\0'; break;
        case OFF: strncpy(b,"OFF",s-1); b[s-1] = '\0'; break;
        default: strncpy(b,"---",s-1); b[s-1] = '\0'; break;
    }
}

void displayOLED() {
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

    // Top Row (Buttons 5-8)
    for (int i = 0; i < 4; i++) {
        int buttonIndex = i + 4;
        const ButtonConfig& config = buttonConfigs[currentPreset][buttonIndex];
        char defaultName[21];
        snprintf(defaultName, sizeof(defaultName), "B%d", buttonIndex + 1);
        char toDisplay[10];
        if (strncmp(config.name, defaultName, 20) == 0 || strlen(config.name) == 0) {
            getButtonSummary(summary, sizeof(summary), config.messageA);
            snprintf(toDisplay, sizeof(toDisplay), "%.4s", summary);
        } else {
            snprintf(toDisplay, sizeof(toDisplay), "%.4s", config.name);
        }
        display.setCursor(i * 32, 0);
        display.print(toDisplay);
    }

    // Bottom Row (Buttons 1-4)
    int bottomY = SCREEN_HEIGHT - 8;
    for (int i = 0; i < 4; i++) {
        int buttonIndex = i;
        const ButtonConfig& config = buttonConfigs[currentPreset][buttonIndex];
        char defaultName[21];
        snprintf(defaultName, sizeof(defaultName), "B%d", buttonIndex + 1);
        char toDisplay[10];
        if (strncmp(config.name, defaultName, 20) == 0 || strlen(config.name) == 0) {
            getButtonSummary(summary, sizeof(summary), config.messageA);
            snprintf(toDisplay, sizeof(toDisplay), "%.4s", summary);
        } else {
            snprintf(toDisplay, sizeof(toDisplay), "%.4s", config.name);
        }
        display.setCursor(i * 32, bottomY);
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

    // SPM Connection Status + WiFi Status
    display.setCursor(0, 44);
    display.printf("SPM:%c", clientConnected?'Y':'N');
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
    
    // Title + ms value
    display.setCursor(0, 0);
    display.print("TAP TEMPO");
    int delayMs = (int)((60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern]);
    display.setCursor(90, 0);
    display.printf("%dms", delayMs);
    
    // BPM - larger, centered
    display.setTextSize(3);
    int16_t x1, y1;
    uint16_t w, h;
    char bpmStr[8];
    snprintf(bpmStr, sizeof(bpmStr), "%.1f", currentBPM);
    display.getTextBounds(bpmStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 16);  // Moved up slightly
    display.print(bpmStr);
    
    // Rhythm Pattern - smaller, below BPM
    display.setTextSize(1);
    display.setCursor(0, 46);  // Moved down
    display.print("Pattern: ");
    display.print(rhythmNames[rhythmPattern]);
    
    // Delay Time - bottom right
    float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
    int delayTimeMS = (int)finalDelayMs;
    char delayStr[12];
    snprintf(delayStr, sizeof(delayStr), "%dms", delayTimeMS);
    display.getTextBounds(delayStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 56);
    display.print(delayStr);
    
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
    char menuItems[11][25];  // Array to hold menu item strings
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
    snprintf(menuItems[10], 25, "Wifi %s at Boot", wifiOnAtBoot ? "ON" : "OFF");
    
    int numMenuItems = 11;

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
    bool needsUpdate = false;
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        const ButtonConfig& config = buttonConfigs[currentPreset][i];
        const MidiMessage& msg = config.isAlternate
            ? (config.nextIsB ? config.messageB : config.messageA)
            : config.messageA;

        int brightness = buttonPinActive[i] ? ledBrightnessOn : ledBrightnessDim;
        int r = (msg.rgb[0] * brightness) / 255;
        int g = (msg.rgb[1] * brightness) / 255;
        int b = (msg.rgb[2] * brightness) / 255;

        uint32_t newColor = strip.Color(r, g, b);
        
        // Only update if color changed
        if (newColor != lastLedColors[i]) {
            strip.setPixelColor(ledMap[i], newColor);
            lastLedColors[i] = newColor;
            needsUpdate = true;
        }
    }
    
    // Only call strip.show() if something actually changed
    if (needsUpdate) {
        strip.show();
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

