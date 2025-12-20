#include "Input.h"
#include "Storage.h"
#include "BleMidi.h"
#include "UI_Display.h"
#include "WebInterface.h"

// Temp variable to track if BLE mode was changed (requires reboot)
static bool bleModeChanged = false;

// ============================================
// ACTION MESSAGE DISPATCH
// ============================================

// Execute a single action message (MIDI or internal command)
void executeActionMessage(const ActionMessage& msg) {
    switch (msg.type) {
        case PRESET_UP:
            currentPreset = (currentPreset + 1) % 4;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected) requestPresetState();
            Serial.printf("PRESET UP → Preset %d\n", currentPreset);
            return;
            
        case PRESET_DOWN:
            currentPreset = (currentPreset - 1 + 4) % 4;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected) requestPresetState();
            Serial.printf("PRESET DOWN → Preset %d\n", currentPreset);
            return;
            
        case PRESET_1: case PRESET_2: case PRESET_3: case PRESET_4:
            currentPreset = msg.type - PRESET_1;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected) requestPresetState();
            return;
            
        case WIFI_TOGGLE:
            if (isWifiOn) {
                turnWifiOff();
                delay(100);
                yield();
                displayOLED();  // Safe to update display after WiFi off
            } else {
                turnWifiOn();
                delay(200);  // Give WiFi more time to stabilize
                yield();
                Serial.println("WiFi toggle complete, skipping display update");
                // Skip displayOLED when WiFi just started - heap is low
            }
            return;
            
        case CLEAR_BLE_BONDS:
            clearBLEBonds();
            return;
            
        case NOTE_ON:
        case NOTE_MOMENTARY:
            sendMidiNoteOn(msg.channel, msg.data1, msg.data2);
            break;
            
        case NOTE_OFF:
            sendMidiNoteOff(msg.channel, msg.data1, msg.data2);
            break;
            
        case CC:
            sendMidiCC(msg.channel, msg.data1, msg.data2);
            break;
            
        case PC:
            sendMidiPC(msg.channel, msg.data1);
            break;
            
        case SYSEX:
            // Send raw SysEx bytes
            if (msg.sysex.length > 0 && msg.sysex.length <= 16) {
                sendSysex(msg.sysex.data, msg.sysex.length);
                Serial.printf("SYSEX sent (%d bytes)\n", msg.sysex.length);
            }
            break;
            
        case MIDI_OFF:
        default:
            break;
    }
}

// ============================================
// TAP TEMPO HANDLING
// ============================================

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
        displayOLED();
        return;
    }
    
    // Calculate BPM from last tap interval
    unsigned long interval = now - lastTapTime;
    lastTapTime = now;
    currentBPM = 60000.0 / interval;
    currentBPM = constrain(currentBPM, 40.0, 300.0);
    
    // Apply rhythm pattern and send
    float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
    int delayTimeMS = constrain((int)finalDelayMs, 20, 1000);
    sendDelayTime(delayTimeMS);
    
    tapModeTimeout = now + 3000;
    displayOLED();
    Serial.printf("Tap Tempo: BPM=%.1f, Pattern=%s, DelayMs=%d\n", 
                  currentBPM, rhythmNames[rhythmPattern], delayTimeMS);
}

// ============================================
// PRESET MODE - MAIN BUTTON LOOP
// ============================================

void loop_presetMode() {
    // Check for tap mode timeout (only if not locked)
    if (inTapTempoMode && !tapModeLocked && millis() > tapModeTimeout) {
        inTapTempoMode = false;
        tapModeLocked = false;
        displayOLED();
        updateLeds();
    }
    
    // Handle encoder rotation in tap tempo mode
    if (inTapTempoMode) {
        long newEncoderPosition = encoder.getCount();
        if (newEncoderPosition != oldEncoderPosition) {
            int change = newEncoderPosition - oldEncoderPosition;
            oldEncoderPosition = newEncoderPosition;
            
            currentBPM += (change * 0.5);
            currentBPM = constrain(currentBPM, 40.0, 300.0);
            currentBPM = round(currentBPM * 2.0) / 2.0;
            
            float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
            int delayTimeMS = constrain((int)finalDelayMs, 0, 1000);
            sendDelayTime(delayTimeMS);
            
            tapModeTimeout = millis() + 3000;
            displayOLED();
        }
    }
    
    // ===== BUTTON LOOP =====
    for (int i = 0; i < systemConfig.buttonCount; i++) {
        int pin = systemConfig.buttonPins[i];
        bool pressed = (digitalRead(pin) == LOW);

        if (pressed != buttonPinActive[i]) {
            if (pressed) {
                // ===== BUTTON PRESS =====
                if (millis() - lastButtonPressTime_pads[i] > buttonDebounce) {
                    buttonPinActive[i] = true;
                    lastButtonPressTime_pads[i] = millis();
                    buttonHoldStartTime[i] = millis();
                    buttonHoldFired[i] = false;
                    buttonComboChecked[i] = false;
                    
                    ButtonConfig& config = buttonConfigs[currentPreset][i];
                    
                    // Handle LED state
                    PresetLedMode presetMode = presetLedModes[currentPreset];
                    bool isSelectionButton = (presetMode == PRESET_LED_SELECTION) || 
                                              (presetMode == PRESET_LED_HYBRID && config.inSelectionGroup);
                    
                    if (isSelectionButton) {
                        presetSelectionState[currentPreset] = i;
                    } else if (config.ledMode == LED_TOGGLE) {
                        // For SPM sync presets, don't toggle here - wait for SPM response
                        // For non-SPM presets, toggle immediately as before
                        if (presetSyncMode[currentPreset] == SYNC_NONE) {
                            ledToggleState[i] = !ledToggleState[i];
                        }
                    }
                    updateLeds();
                    
                    // ===== CHECK COMBO FIRST (using global special actions) =====
                    bool comboFired = false;
                    
                    if (globalSpecialActions[i].hasCombo) {
                        int8_t partner = globalSpecialActions[i].comboAction.combo.partner;
                        
                        if (partner >= 0 && partner < systemConfig.buttonCount) {
                            // Wait up to 40ms for partner
                            unsigned long waitStart = millis();
                            while (!buttonPinActive[partner] && (millis() - waitStart < 40)) {
                                if (digitalRead(systemConfig.buttonPins[partner]) == LOW) {
                                    buttonPinActive[partner] = true;
                                    lastButtonPressTime_pads[partner] = millis();
                                    
                                    ButtonConfig& partnerConfig = buttonConfigs[currentPreset][partner];
                                    if (presetMode == PRESET_LED_SELECTION || 
                                        (presetMode == PRESET_LED_HYBRID && partnerConfig.inSelectionGroup)) {
                                        presetSelectionState[currentPreset] = partner;
                                    } else if (partnerConfig.ledMode == LED_TOGGLE) {
                                        // For SPM sync presets, don't toggle here
                                        if (presetSyncMode[currentPreset] == SYNC_NONE) {
                                            ledToggleState[partner] = !ledToggleState[partner];
                                        }
                                    }
                                    updateLeds();
                                }
                                yield();
                                delayMicroseconds(500);
                            }
                            
                            if (buttonPinActive[partner] && !buttonComboChecked[partner]) {
                                // Fire combo!
                                const ActionMessage& comboMsg = globalSpecialActions[i].comboAction;
                                
                                // Show label
                                if (comboMsg.combo.label[0] != '\0') {
                                    strncpy(buttonNameToShow, comboMsg.combo.label, 20);
                                } else if (comboMsg.type == PRESET_DOWN) {
                                    strncpy(buttonNameToShow, "<", 20);
                                } else if (comboMsg.type == PRESET_UP) {
                                    strncpy(buttonNameToShow, ">", 20);
                                } else if (comboMsg.type == WIFI_TOGGLE) {
                                    strncpy(buttonNameToShow, "WiFi", 20);
                                } else {
                                    snprintf(buttonNameToShow, 20, "%s+", config.name);
                                }
                                buttonNameToShow[20] = '\0';
                                buttonNameDisplayUntil = millis() + 500;
                                safeDisplayOLED();
                                
                                executeActionMessage(comboMsg);
                                buttonComboChecked[i] = true;
                                buttonComboChecked[partner] = true;
                                comboFired = true;
                            }
                        }
                    }
                    
                    // Check reverse combo (partner has combo pointing to us)
                    if (!comboFired) {
                        for (int p = 0; p < systemConfig.buttonCount; p++) {
                            if (p != i && buttonPinActive[p] && !buttonComboChecked[p] && 
                                globalSpecialActions[p].hasCombo &&
                                globalSpecialActions[p].comboAction.combo.partner == i) {
                                
                                const ActionMessage& comboMsg = globalSpecialActions[p].comboAction;
                                
                                if (comboMsg.combo.label[0] != '\0') {
                                    strncpy(buttonNameToShow, comboMsg.combo.label, 20);
                                } else if (comboMsg.type == PRESET_DOWN) {
                                    strncpy(buttonNameToShow, "<", 20);
                                } else if (comboMsg.type == PRESET_UP) {
                                    strncpy(buttonNameToShow, ">", 20);
                                } else if (comboMsg.type == WIFI_TOGGLE) {
                                    strncpy(buttonNameToShow, "WiFi", 20);
                                } else {
                                    snprintf(buttonNameToShow, 20, "%s+", buttonConfigs[currentPreset][p].name);
                                }
                                buttonNameToShow[20] = '\0';
                                buttonNameDisplayUntil = millis() + 500;
                                safeDisplayOLED();
                                
                                executeActionMessage(comboMsg);
                                buttonComboChecked[p] = true;
                                comboFired = true;
                                updateLeds();
                                break;
                            }
                        }
                    }
                    
                    // ===== TAP TEMPO CONTROLS (when in tap mode) =====
                    if (!comboFired && inTapTempoMode) {
                        bool isTapControl = false;
                        
                        for (int tapBtn = 0; tapBtn < systemConfig.buttonCount; tapBtn++) {
                            ActionMessage* tapAction = findAction(buttonConfigs[currentPreset][tapBtn], ACTION_PRESS);
                            if (tapAction && tapAction->type == TAP_TEMPO) {
                                // RHYTHM PREV
                                if (tapAction->tapTempo.rhythmPrev == i) {
                                    rhythmPattern = (rhythmPattern - 1 + 4) % 4;
                                    saveSystemSettings();
                                    float delayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                                    sendDelayTime(constrain((int)delayMs, 0, 1000));
                                    strncpy(buttonNameToShow, rhythmNames[rhythmPattern], 20);
                                    buttonNameDisplayUntil = millis() + 500;
                                    if (!tapModeLocked) tapModeTimeout = millis() + 3000;
                                    safeDisplayOLED();
                                    isTapControl = true;
                                    break;
                                }
                                // RHYTHM NEXT
                                if (tapAction->tapTempo.rhythmNext == i) {
                                    rhythmPattern = (rhythmPattern + 1) % 4;
                                    saveSystemSettings();
                                    float delayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                                    sendDelayTime(constrain((int)delayMs, 0, 1000));
                                    strncpy(buttonNameToShow, rhythmNames[rhythmPattern], 20);
                                    buttonNameDisplayUntil = millis() + 500;
                                    if (!tapModeLocked) tapModeTimeout = millis() + 3000;
                                    safeDisplayOLED();
                                    isTapControl = true;
                                    break;
                                }
                                // TAP LOCK
                                if (tapAction->tapTempo.tapLock == i) {
                                    tapModeLocked = !tapModeLocked;
                                    if (tapModeLocked) {
                                        strncpy(buttonNameToShow, "LOCKED", 20);
                                        buttonNameDisplayUntil = millis() + 500;
                                    } else {
                                        inTapTempoMode = false;
                                        buttonNameDisplayUntil = 0;
                                    }
                                    safeDisplayOLED();
                                    isTapControl = true;
                                    break;
                                }
                            }
                        }
                        
                        if (isTapControl) continue;  // Skip normal button handling
                    }
                    
                    // ===== NORMAL BUTTON PRESS (no combo, no tap control) =====
                    if (!comboFired) {
                        // Find the appropriate action based on toggle state
                        ActionMessage* action = nullptr;
                        
                        Serial.printf("BTN %d: isAlternate=%d, has2nd=%d\n", 
                            i, config.isAlternate, hasAction(config, ACTION_2ND_PRESS));
                        
                        if (config.isAlternate) {
                            // Toggle mode - alternate between PRESS and 2ND_PRESS
                            action = findAction(config, ACTION_2ND_PRESS);
                        } else {
                            action = findAction(config, ACTION_PRESS);
                        }
                        
                        // Fallback to PRESS if no 2ND_PRESS found
                        if (!action) {
                            action = findAction(config, ACTION_PRESS);
                        }
                        
                        if (action) {
                            Serial.printf("BTN %d: action type=%d, data1=%d, data2=%d\n",
                                i, action->type, action->data1, action->data2);
                            
                            strncpy(buttonNameToShow, config.name, 20);
                            buttonNameToShow[20] = '\0';
                            buttonNameDisplayUntil = millis() + 500;
                            safeDisplayOLED();
                            
                            if (action->type == TAP_TEMPO) {
                                handleTapTempo();
                            } else {
                                executeActionMessage(*action);
                                
                                // Toggle alternate state if button has 2ND_PRESS
                                if (hasAction(config, ACTION_2ND_PRESS)) {
                                    config.isAlternate = !config.isAlternate;
                                    // Also update LED state to match (for SPM sync presets that skip early toggle)
                                    ledToggleState[i] = config.isAlternate;
                                    Serial.printf("BTN %d: toggled isAlternate to %d, LED=%d\n", 
                                        i, config.isAlternate, ledToggleState[i]);
                                    updateLeds();
                                }
                            }
                        } else {
                            Serial.printf("BTN %d: NO ACTION FOUND!\n", i);
                        }
                    }
                }
            } else {
                // ===== BUTTON RELEASE =====
                buttonPinActive[i] = false;
                
                if (!buttonComboChecked[i]) {
                    ButtonConfig& config = buttonConfigs[currentPreset][i];
                    
                    // Check for RELEASE or 2ND_RELEASE action based on isAlternate state
                    ActionType releaseType = config.isAlternate ? ACTION_2ND_RELEASE : ACTION_RELEASE;
                    ActionMessage* releaseAction = findAction(config, releaseType);
                    if (!releaseAction) {
                        // Fallback: try the regular RELEASE action if 2ND_RELEASE not found
                        releaseAction = findAction(config, ACTION_RELEASE);
                    }
                    if (releaseAction) {
                        executeActionMessage(*releaseAction);
                    }
                    
                    // Handle NOTE_MOMENTARY note off
                    ActionMessage* pressAction = findAction(config, ACTION_PRESS);
                    if (pressAction && pressAction->type == NOTE_MOMENTARY) {
                        sendMidiNoteOff(pressAction->channel, pressAction->data1, 0);
                    }
                }
                
                buttonComboChecked[i] = false;
                buttonHoldFired[i] = false;
                updateLeds();
            }
        }
        
        // ===== CHECK FOR LONG_PRESS OR 2ND_LONG_PRESS ACTION =====
        if (buttonPinActive[i] && !buttonHoldFired[i] && !buttonComboChecked[i]) {
            ButtonConfig& config = buttonConfigs[currentPreset][i];
            // Check for 2ND_LONG_PRESS if in alternate state, otherwise LONG_PRESS
            ActionType longPressType = config.isAlternate ? ACTION_2ND_LONG_PRESS : ACTION_LONG_PRESS;
            ActionMessage* longPress = findAction(config, longPressType);
            if (!longPress) {
                // Fallback: try regular LONG_PRESS if 2ND_LONG_PRESS not found
                longPress = findAction(config, ACTION_LONG_PRESS);
            }
            
            if (longPress) {
                unsigned long elapsed = millis() - buttonHoldStartTime[i];
                uint16_t threshold = longPress->longPress.holdMs > 0 ? longPress->longPress.holdMs : 500;
                
                if (elapsed >= threshold) {
                    // Fire long press action
                    strncpy(buttonNameToShow, "HOLD", 20);
                    buttonNameDisplayUntil = millis() + 500;
                    safeDisplayOLED();
                    
                    executeActionMessage(*longPress);
                    buttonHoldFired[i] = true;
                    Serial.printf("BTN %d LONG_PRESS fired\n", i);
                    updateLeds();
                }
            }
        }
    }
}

// ============================================
// MENU MODE
// ============================================

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
            case 0:
                saveSystemSettings();
                if (bleModeChanged) {
                    Serial.println("BLE Mode changed - rebooting...");
                    delay(500);
                    ESP.restart();
                }
                currentMode = 0;
                safeDisplayOLED();
                updateLeds();
                break;
            case 1:
                currentMode = 0;
                safeDisplayOLED();
                updateLeds();
                break;
            case 2:
                if(isWifiOn) {
                    turnWifiOff();
                    delay(100);
                    yield();
                    displayMenu();  // Safe after WiFi off
                } else {
                    turnWifiOn();
                    delay(200);
                    yield();
                    // Skip displayMenu when WiFi just started - heap is low
                }
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
            case 10:
                systemConfig.wifiOnAtBoot = !systemConfig.wifiOnAtBoot;
                displayMenu();
                break;
            case 11:
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
        inSubMenu = false;
        switch (menuSelection) {
            case 3: ledBrightnessOn = editingValue; updateLeds(); break;
            case 4: ledBrightnessDim = editingValue; updateLeds(); break;
            case 5: buttonDebounce = editingValue; break;
            case 9: buttonNameFontSize = editingValue; break;
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
            if (inTapTempoMode) {
                rhythmPattern = (rhythmPattern + 1) % 4;
                saveSystemSettings();
                float finalDelayMs = (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                sendDelayTime(constrain((int)finalDelayMs, 0, 1000));
                tapModeTimeout = millis() + 3000;
                safeDisplayOLED();
            } else if (currentMode == 1) {
                handleMenuSelection();
            } else if (currentMode == 0) {
                currentPreset = (currentPreset + 1) % 4;
                for (int j = 0; j < MAX_BUTTONS; j++) {
                    ledToggleState[j] = false;
                }
                saveCurrentPresetIndex();
                displayOLED();
                updateLeds();
                if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected) requestPresetState();
            }
        } else if (pressDuration >= LONG_PRESS_DURATION) {
            // Long press only enters menu from preset mode
            if (currentMode == 0) {
                currentMode = 1;
                menuSelection = 0;
                inSubMenu = false;
                inTapTempoMode = false;
                displayMenu();
            }
            // In menu mode: long press does nothing (use Save & Exit or Cancel)
        }
    } else if (currentState && !encoderButtonPressed) {
        encoderButtonPressed = true;
        encoderButtonPressStartTime = millis();
    }
}
