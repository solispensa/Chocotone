#include "Input.h"
#include "Storage.h"
#include "BleMidi.h"
#include "UI_Display.h"
#include "WebInterface.h"

void handleTapTempo() {
    unsigned long now = millis();
    
    // LED feedback on every tap
    blinkAllLeds();
    
    // Reset if more than 3 seconds since last tap
    if (!inTapTempoMode || (now - lastTapTime > 3000)) {
        Serial.println("Tap Tempo: First tap");
        lastTapTime = now;
        inTapTempoMode = true;
        tapModeTimeout = now + 3000;
        displayOLED();  // Will timeout and return to normal
        return;
    }
    
    // Calculate BPM from last tap interval (rolling 2-tap window)
    unsigned long interval = now - lastTapTime;
    lastTapTime = now;
    currentBPM = 60000.0 / interval;
    currentBPM = constrain(currentBPM, 40.0, 300.0);
    
    // Apply rhythm pattern and send
    float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
    int delayTimeMS = constrain((int)finalDelayMs, 20, 1000);
    sendDelayTime(delayTimeMS);
    
    tapModeTimeout = now + 3000;
    displayOLED();  // Will timeout and return to normal
    Serial.printf("Tap Tempo: BPM=%.1f, Pattern=%s, DelayMs=%d\n", 
                  currentBPM, rhythmNames[rhythmPattern], delayTimeMS);
}

void loop_presetMode() {
    // Check for tap mode timeout
    if (inTapTempoMode && millis() > tapModeTimeout) {
        inTapTempoMode = false;
        displayOLED();  // Refresh when timeout expires
        updateLeds();
    }
    
    // Handle encoder rotation in tap tempo mode (for BPM adjustment)
    if (inTapTempoMode) {
        long newEncoderPosition = encoder.getCount();
        if (newEncoderPosition != oldEncoderPosition) {
            int change = newEncoderPosition - oldEncoderPosition;
            oldEncoderPosition = newEncoderPosition;
            
            // Adjust BPM by 0.5 per detent
            currentBPM += (change * 0.5);
            currentBPM = constrain(currentBPM, 40.0, 300.0);
            currentBPM = round(currentBPM * 2.0) / 2.0;
            
            // Recalculate and send
            float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
            int delayTimeMS = constrain((int)finalDelayMs, 0, 1000);
            
            sendDelayTime(delayTimeMS);
            
            tapModeTimeout = millis() + 3000; // Reset timeout
            displayOLED();  // Update BPM display
        }
        // DON'T return - allow tap tempo button to be processed below!
    }
    
    // Normal preset mode button handling (including tap tempo button in tap mode!)
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int pin = buttonPins[i];
        bool pressed = (digitalRead(pin) == LOW);

        if (pressed != buttonPinActive[i]) {
            if (pressed) {
                if (millis() - lastButtonPressTime_pads[i] > buttonDebounce) {
                    buttonPinActive[i] = true;
                    lastButtonPressTime_pads[i] = millis();
                    
                    ButtonConfig& config = buttonConfigs[currentPreset][i];
                    MidiMessage& msg = config.isAlternate ? (config.nextIsB ? config.messageB : config.messageA) : config.messageA;

                    strncpy(buttonNameToShow, config.name, 20);
                    buttonNameToShow[20] = '\0';
                    buttonNameDisplayUntil = millis() + 1000;
                    safeDisplayOLED();  // Use safe wrapper with auto-recovery

                    if (msg.type == TAP_TEMPO) {
                        handleTapTempo();
                    } else {
                        sendMidiMessage(msg);
                        if (config.isAlternate) {
                             config.nextIsB = !config.nextIsB;
                        }
                    }
                    
                    // Stagger display and LED updates to reduce peak current
                    yield();  // Allow ESP32 to handle background tasks
                    delay(5);  // 5ms delay spreads power consumption
                    updateLeds();
                }
            } else {
                buttonPinActive[i] = false;
                ButtonConfig& config = buttonConfigs[currentPreset][i];
                MidiMessage& msg = config.isAlternate ? (config.nextIsB ? config.messageB : config.messageA) : config.messageA;
                
                if (msg.type == NOTE_MOMENTARY) {
                     sendMidiNoteOff(msg.channel, msg.data1, 0);
                }
                updateLeds();
            }
        }
    }
}

void loop_menuMode() {
    long newEncoderPosition = encoder.getCount();
    if (newEncoderPosition == oldEncoderPosition) return;

    int change = (newEncoderPosition - oldEncoderPosition);

    int numMenuItems = 11;

    if (inSubMenu) {
        editingValue += change;
        oldEncoderPosition = newEncoderPosition;
        if (menuSelection == 3) editingValue = constrain(editingValue, 0, 255);
        if (menuSelection == 4) editingValue = constrain(editingValue, 0, 255);
        if (menuSelection == 5) editingValue = constrain(editingValue, 1, 500);
        if (menuSelection == 9) editingValue = constrain(editingValue, 1, 5);
    } else {
        long menuStep = newEncoderPosition / 2;
        long oldMenuStep = oldEncoderPosition / 2;
        if (menuStep != oldMenuStep) {
             menuSelection += (menuStep - oldMenuStep);
             menuSelection = (menuSelection % numMenuItems + numMenuItems) % numMenuItems;
        }
        oldEncoderPosition = newEncoderPosition;
    }
    displayMenu();
}

void handleMenuSelection() {
    if (!inSubMenu) {
        switch (menuSelection) {
            case 0: // Save and Exit
                saveSystemSettings();
                currentMode = 0;
                safeDisplayOLED();
                updateLeds();
                break;
            case 1: // Exit without Saving
                currentMode = 0;
                safeDisplayOLED();
                updateLeds();
                break;
            case 2: // Wi-Fi Config
                if(isWifiOn) turnWifiOff(); else turnWifiOn();
                displayMenu();
                break;
            case 3: inSubMenu = true; editingValue = ledBrightnessOn; break;
            case 4: inSubMenu = true; editingValue = ledBrightnessDim; break;
            case 5: inSubMenu = true; editingValue = buttonDebounce; break;
            case 6: clearBLEBonds(); currentMode = 0; break;
            case 7: ESP.restart(); break;
            case 8: 
               systemPrefs.begin("midi_presets", false); systemPrefs.clear(); systemPrefs.end();
                systemPrefs.begin("midi_system", false); systemPrefs.clear(); systemPrefs.end();
                ESP.restart(); 
                break;
            case 9: inSubMenu = true; editingValue = buttonNameFontSize; break;
            case 10: // WiFi On at Boot - toggle directly
                wifiOnAtBoot = !wifiOnAtBoot;
                displayMenu();
                break;
        }
    } else {
        // Exiting submenu - apply changes to globals
        inSubMenu = false;
        switch (menuSelection) {
            case 3: ledBrightnessOn = editingValue; updateLeds(); break;
            case 4: ledBrightnessDim = editingValue; updateLeds(); break;
            case 5: buttonDebounce = editingValue; break;
            case 9: 
                buttonNameFontSize = editingValue; 
                Serial.printf("Font changed to: %d\n", buttonNameFontSize);
                break;
        }
    }
    displayMenu();
}

void handleEncoderButtonPress() {
    bool currentState = (digitalRead(ENCODER_BUTTON_PIN) == LOW);

    if (!currentState && encoderButtonPressed) {
        encoderButtonPressed = false;
        unsigned long pressDuration = millis() - encoderButtonPressStartTime;

        if (pressDuration > ENCODER_BUTTON_DEBOUNCE_DELAY && pressDuration < LONG_PRESS_DURATION) {
            // Short press
            if (inTapTempoMode) {
                // Cycle rhythm pattern
                rhythmPattern = (rhythmPattern + 1) % 4;
                saveSystemSettings();
                
                // Recalculate and send
                float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                int delayTimeMS = constrain((int)finalDelayMs, 0, 1000);
                
                sendDelayTime(delayTimeMS);
                
                tapModeTimeout = millis() + 3000; // Reset timeout
                safeDisplayOLED();
                Serial.printf("Rhythm Pattern: %s\n", rhythmNames[rhythmPattern]);
            } else if (currentMode == 1) {
                // Menu mode
                handleMenuSelection();
            } else if (currentMode == 0) {
                // Preset mode - cycle presets
                currentPreset++;
                if (currentPreset > 3) currentPreset = 0;
                saveCurrentPresetIndex();
                safeDisplayOLED();
                updateLeds();
            }
        } else if (pressDuration >= LONG_PRESS_DURATION) {
            // Long press - toggle menu
            if (currentMode == 0) {
                currentMode = 1;
                menuSelection = 0;
                inSubMenu = false;
                inTapTempoMode = false; // Exit tap mode
                displayMenu();
            } else {
                // Exiting menu - save any unsaved changes
                saveSystemSettings();
                currentMode = 0;
                safeDisplayOLED();
                updateLeds();
            }
        }
    } else if (currentState && !encoderButtonPressed) {
        encoderButtonPressed = true;
        encoderButtonPressStartTime = millis();
    }
}
