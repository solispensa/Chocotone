#include "Input.h"
#include "Storage.h"
#include "BleMidi.h"
#include "UI_Display.h"
#include "WebInterface.h"

// Temp variable to track if BLE mode was changed (requires reboot)
static bool bleModeChanged = false;

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
    // Check for tap mode timeout (only if not locked)
    if (inTapTempoMode && !tapModeLocked && millis() > tapModeTimeout) {
        inTapTempoMode = false;
        tapModeLocked = false;  // Reset lock state on exit
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
    
    // Normal preset mode button handling with Hold/Combo support
    for (int i = 0; i < systemConfig.buttonCount; i++) {
        int pin = systemConfig.buttonPins[i];
        bool pressed = (digitalRead(pin) == LOW);

        if (pressed != buttonPinActive[i]) {
            if (pressed) {
                // ===== BUTTON PRESS =====
                if (millis() - lastButtonPressTime_pads[i] > buttonDebounce) {
                    buttonPinActive[i] = true;
                    lastButtonPressTime_pads[i] = millis();
                    
                    // Reset hold/combo tracking for this button
                    buttonHoldStartTime[i] = millis();
                    buttonHoldFired[i] = false;
                    buttonComboChecked[i] = false;
                    
                    ButtonConfig& config = buttonConfigs[currentPreset][i];
                    
                    // DEBUG: Print button press info (from global special actions)
                   Serial.printf("BTN %d pressed | Combo: %s | Partner: %d | Partner active: %s\n",
                        i,
                        globalSpecialActions[i].combo.enabled ? "YES" : "NO",
                        globalSpecialActions[i].combo.partner,
                        (globalSpecialActions[i].combo.partner >= 0 && globalSpecialActions[i].combo.partner < systemConfig.buttonCount) ? 
                            (buttonPinActive[globalSpecialActions[i].combo.partner] ? "YES" : "NO") : "N/A");
                    
                    // Check for combo action FIRST (bidirectional - either button can trigger)
                    bool comboFired = false;
                    
                    // Check if THIS button has a combo configured (use global special actions)
                    if (globalSpecialActions[i].combo.enabled && globalSpecialActions[i].combo.partner >= 0 && globalSpecialActions[i].combo.partner < systemConfig.buttonCount) {
                        int partnerIdx = globalSpecialActions[i].combo.partner;
                        
                        // Wait up to 40ms for partner button to be pressed (active scanning)
                        unsigned long waitStart = millis();
                        while (!buttonPinActive[partnerIdx] && (millis() - waitStart < 40)) {
                            buttonPinActive[partnerIdx] = (digitalRead(systemConfig.buttonPins[partnerIdx]) == LOW);
                            yield();
                            delayMicroseconds(500);  // 0.5ms scan intervals
                        }
                        
                        if (buttonPinActive[partnerIdx] && !buttonComboChecked[partnerIdx]) {
                            // Partner is already pressed - fire THIS button's combo!
                            MidiMessage comboMsg;
                            comboMsg.type = globalSpecialActions[i].combo.type;
                            comboMsg.channel = globalSpecialActions[i].combo.channel;
                            comboMsg.data1 = globalSpecialActions[i].combo.data1;
                            comboMsg.data2 = globalSpecialActions[i].combo.data2;
                            
                            // Use custom label if set, otherwise use default based on type
                            if (globalSpecialActions[i].combo.label[0] != '\0') {
                                // Custom label configured
                                strncpy(buttonNameToShow, globalSpecialActions[i].combo.label, 20);
                            } else {
                                // Default labels based on command type
                                if (comboMsg.type == PRESET_DOWN) {
                                    strncpy(buttonNameToShow, "<", 20);
                                } else if (comboMsg.type == PRESET_UP) {
                                    strncpy(buttonNameToShow, ">", 20);
                                } else if (comboMsg.type == WIFI_TOGGLE) {
                                    strncpy(buttonNameToShow, "WiFi", 20);
                                } else if (comboMsg.type == CLEAR_BLE_BONDS) {
                                    strncpy(buttonNameToShow, "xBLE", 20);
                                } else {
                                    strncpy(buttonNameToShow, config.name, 20);
                                    buttonNameToShow[20] = '\0';
                                    strncat(buttonNameToShow, "+", 20);
                                }
                            }
                            buttonNameToShow[20] = '\0';
                            buttonNameDisplayUntil = millis() + 500;
                            safeDisplayOLED();
                            
                            sendMidiMessage(comboMsg);
                            buttonComboChecked[i] = true;
                            buttonComboChecked[partnerIdx] = true;
                            comboFired = true;
                            
                            yield();
                            delay(5);
                            updateLeds();
                        }
                    }
                    
                    // Also check if PARTNER button has a combo pointing to THIS button (use global special actions)
                    if (!comboFired) {
                        for (int p = 0; p < systemConfig.buttonCount; p++) {
                            if (p != i && buttonPinActive[p] && !buttonComboChecked[p]) {
                                if (globalSpecialActions[p].combo.enabled && globalSpecialActions[p].combo.partner == i) {
                                    // Partner has combo pointing to us - fire partner's combo!
                                    MidiMessage comboMsg;
                                    comboMsg.type = globalSpecialActions[p].combo.type;
                                    comboMsg.channel = globalSpecialActions[p].combo.channel;
                                    comboMsg.data1 = globalSpecialActions[p].combo.data1;
                                    comboMsg.data2 = globalSpecialActions[p].combo.data2;
                                    
                                    // Use custom label if set, otherwise use default based on type
                                    if (globalSpecialActions[p].combo.label[0] != '\0') {
                                        // Custom label configured
                                        strncpy(buttonNameToShow, globalSpecialActions[p].combo.label, 20);
                                    } else {
                                        // Default labels based on command type
                                        if (comboMsg.type == PRESET_DOWN) {
                                            strncpy(buttonNameToShow, "<", 20);
                                        } else if (comboMsg.type == PRESET_UP) {
                                            strncpy(buttonNameToShow, ">", 20);
                                        } else if (comboMsg.type == WIFI_TOGGLE) {
                                            strncpy(buttonNameToShow, "WiFi", 20);
                                        } else if (comboMsg.type == CLEAR_BLE_BONDS) {
                                            strncpy(buttonNameToShow, "xBLE", 20);
                                        } else {
                                            strncpy(buttonNameToShow, buttonConfigs[currentPreset][p].name, 20);
                                            buttonNameToShow[20] = '\0';
                                            strncat(buttonNameToShow, "+", 20);
                                        }
                                    }
                                    buttonNameToShow[20] = '\0';
                                    buttonNameDisplayUntil = millis() + 500;
                                    safeDisplayOLED();
                                    
                                    sendMidiMessage(comboMsg);
                                    buttonComboChecked[i] = true;
                                    buttonComboChecked[p] = true;
                                    comboFired = true;
                                    
                                    yield();
                                    delay(5);
                                    updateLeds();
                                    break;
                                }
                            }
                        }
                    }
                    
                    
                    // If combo didn't fire, check tap tempo mode controls
                    if (!comboFired) {
                        // Check if THIS button is a tap tempo control for any TAP_TEMPO button
                        // In tap tempo mode, these buttons only do their tap tempo function
                        bool isTapControlNow = false;
                        if (inTapTempoMode) {
                            for (int tapBtn = 0; tapBtn < systemConfig.buttonCount; tapBtn++) {
                                MidiMessage& tapMsg = buttonConfigs[currentPreset][tapBtn].messageA;
                                if (tapMsg.type == TAP_TEMPO) {
                                    // Check RHYTHM PREV button
                                    if (tapMsg.rhythmPrevButton == i) {
                                        rhythmPattern = (rhythmPattern - 1 + 4) % 4;  // Go backwards
                                        saveSystemSettings();
                                        
                                        float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                                        int delayTimeMS = constrain((int)finalDelayMs, 0, 1000);
                                        sendDelayTime(delayTimeMS);
                                        
                                        strncpy(buttonNameToShow, rhythmNames[rhythmPattern], 20);
                                        buttonNameToShow[20] = '\0';
                                        buttonNameDisplayUntil = millis() + 500;
                                        
                                        if (!tapModeLocked) tapModeTimeout = millis() + 3000;
                                        safeDisplayOLED();
                                        Serial.printf("Rhythm PREV: %s\n", rhythmNames[rhythmPattern]);
                                        isTapControlNow = true;
                                        break;
                                    }
                                    // Check RHYTHM NEXT button
                                    if (tapMsg.rhythmNextButton == i) {
                                        rhythmPattern = (rhythmPattern + 1) % 4;  // Go forwards
                                        saveSystemSettings();
                                        
                                        float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                                        int delayTimeMS = constrain((int)finalDelayMs, 0, 1000);
                                        sendDelayTime(delayTimeMS);
                                        
                                        strncpy(buttonNameToShow, rhythmNames[rhythmPattern], 20);
                                        buttonNameToShow[20] = '\0';
                                        buttonNameDisplayUntil = millis() + 500;
                                        
                                        if (!tapModeLocked) tapModeTimeout = millis() + 3000;
                                        safeDisplayOLED();
                                        Serial.printf("Rhythm NEXT: %s\n", rhythmNames[rhythmPattern]);
                                        isTapControlNow = true;
                                        break;
                                    }
                                    // Check TAP MODE LOCK button
                                    if (tapMsg.tapModeLockButton == i) {
                                        tapModeLocked = !tapModeLocked;  // Toggle lock
                                        
                                        if (tapModeLocked) {
                                            strncpy(buttonNameToShow, "LOCKED", 20);
                                            buttonNameToShow[20] = '\0';
                                            buttonNameDisplayUntil = millis() + 500;
                                        } else {
                                            // Instant exit when unlocking - no text, just go to main screen
                                            inTapTempoMode = false;
                                            buttonNameDisplayUntil = 0;  // Clear any pending name display
                                        }
                                        safeDisplayOLED();
                                        Serial.printf("Tap Mode Lock: %s\n", tapModeLocked ? "ON" : "OFF");
                                        isTapControlNow = true;
                                        break;
                                    }
                                }
                            }
                        }
                        
                        // If tap control fired, skip all other processing
                        if (isTapControlNow) {
                            yield();
                            delay(5);
                            updateLeds();
                            // Skip everything else
                        } else if (globalSpecialActions[i].hold.enabled) {
                            // Hold is enabled - DON'T send primary message now - wait for release
                            strncpy(buttonNameToShow, config.name, 20);
                            buttonNameToShow[20] = '\0';
                            buttonNameDisplayUntil = millis() + 500;
                            safeDisplayOLED();
                            // Message will be sent on RELEASE (if hold didn't fire)
                        } else {
                            // No hold configured - send immediately on press
                            MidiMessage& msg = config.isAlternate ? (config.nextIsB ? config.messageB : config.messageA) : config.messageA;

                            strncpy(buttonNameToShow, config.name, 20);
                            buttonNameToShow[20] = '\0';
                            buttonNameDisplayUntil = millis() + 500;
                            safeDisplayOLED();

                            if (msg.type == TAP_TEMPO) {
                                handleTapTempo();
                            } else {
                                sendMidiMessage(msg);
                                if (config.isAlternate) {
                                     config.nextIsB = !config.nextIsB;
                                }
                            }
                            
                            yield();
                            delay(5);
                            updateLeds();
                        }
                    }
                }
            } else {
                // ===== BUTTON RELEASE =====
                buttonPinActive[i] = false;
                
                // Only handle release if not combo-checked (combo already handled action)
                if (!buttonComboChecked[i]) {
                    ButtonConfig& config = buttonConfigs[currentPreset][i];
                    MidiMessage& msg = config.isAlternate ? (config.nextIsB ? config.messageB : config.messageA) : config.messageA;
                    
                    // If hold is enabled and hold DIDN'T fire, send primary message now (on release)
                    if (globalSpecialActions[i].hold.enabled && !buttonHoldFired[i]) {
                        // User tapped quickly - send the primary message
                        if (msg.type == TAP_TEMPO) {
                            handleTapTempo();
                        } else if (msg.type == NOTE_MOMENTARY) {
                            // Send note on then note off for momentary
                            sendMidiMessage(msg);
                            delay(10);
                            sendMidiNoteOff(msg.channel, msg.data1, 0);
                        } else {
                            sendMidiMessage(msg);
                        }
                        if (config.isAlternate) {
                            config.nextIsB = !config.nextIsB;
                        }
                        Serial.printf("BTN %d released - primary sent (tap)\n", i);
                    } else if (!globalSpecialActions[i].hold.enabled) {
                        // No hold - send note off for momentary notes
                        if (msg.type == NOTE_MOMENTARY) {
                            sendMidiNoteOff(msg.channel, msg.data1, 0);
                        }
                    } else {
                        // Hold WAS fired - don't send primary
                        Serial.printf("BTN %d released - hold already fired\n", i);
                    }
                }
                
                // Reset combo tracking on release
                buttonComboChecked[i] = false;
                buttonHoldFired[i] = false;
                updateLeds();
            }
        }
        
        // ===== CHECK FOR HOLD ACTION (while button is pressed) =====
        if (buttonPinActive[i] && !buttonHoldFired[i] && !buttonComboChecked[i]) {
            // Use globalSpecialActions for hold settings
            const HoldAction& hold = globalSpecialActions[i].hold;
            if (hold.enabled) {
                unsigned long elapsed = millis() - buttonHoldStartTime[i];
                if (elapsed >= hold.thresholdMs) {
                    // Hold threshold reached - fire hold action
                    ButtonConfig& config = buttonConfigs[currentPreset][i];
                    MidiMessage holdMsg;
                    holdMsg.type = hold.type;
                    holdMsg.channel = hold.channel;
                    holdMsg.data1 = hold.data1;
                    holdMsg.data2 = hold.data2;
                    
                    // Use default label for hold command type (same as button summary)
                    char holdLabel[21];
                    getButtonSummary(holdLabel, sizeof(holdLabel), holdMsg);
                    strncpy(buttonNameToShow, holdLabel, 20);
                    buttonNameToShow[20] = '\0';
                    
                    buttonNameDisplayUntil = millis() + 500;
                    safeDisplayOLED();
                    
                    sendMidiMessage(holdMsg);
                    buttonHoldFired[i] = true;
                    Serial.printf("BTN %d HOLD fired\n", i);
                    
                    yield();
                    delay(5);
                    updateLeds();
                }
            }
        }
    }
}

void loop_menuMode() {
    long newEncoderPosition = encoder.getCount();
    if (newEncoderPosition == oldEncoderPosition) return;

    int change = (newEncoderPosition - oldEncoderPosition);

    int numMenuItems = 12;

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
                if (bleModeChanged) {
                    // BLE mode changed - must reboot
                    Serial.println("BLE Mode changed - rebooting...");
                    delay(500);
                    ESP.restart();
                }
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
                delay(100);  // Let WiFi settle before OLED update
                yield();
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
                systemConfig.wifiOnAtBoot = !systemConfig.wifiOnAtBoot;
                displayMenu();
                break;
            case 11: // BLE Mode - cycle through modes
                if (systemConfig.bleMode == BLE_CLIENT_ONLY) {
                    systemConfig.bleMode = BLE_DUAL_MODE;
                } else if (systemConfig.bleMode == BLE_DUAL_MODE) {
                    systemConfig.bleMode = BLE_SERVER_ONLY;
                } else {
                    systemConfig.bleMode = BLE_CLIENT_ONLY;
                }
                bleModeChanged = true;
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
