#include "Input.h"
#include "AnalogInput.h"
#include "BleMidi.h"
#include "Config.h"
#include "GP5Protocol.h"
#include "Storage.h"
#include "SysexScrollData.h"
#include "UI_Display.h"
#include "WebInterface.h"

// Debug logging helper - only prints when DEBUG_INPUT is defined
#ifdef DEBUG_INPUT
#define DBG_INPUT(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_INPUT(...) ((void)0)
#endif

// Temp variable to track if BLE mode was changed (requires reboot)
static bool bleModeChanged = false;

// Deferred save flag for rhythm pattern (reduces NVS wear)
static bool rhythmPatternDirty = false;

// Forward declaration
void executeActionMessage(const ActionMessage &msg);

// Helper: Fire a global action with OLED feedback
void fireGlobalAction(const ActionMessage &msg, int btnIdx) {
  // Show label
  if (msg.label[0] != '\0') {
    strncpy(buttonNameToShow, msg.label, 20);
  } else if (msg.type == PRESET_DOWN) {
    strncpy(buttonNameToShow, "<", 20);
  } else if (msg.type == PRESET_UP) {
    strncpy(buttonNameToShow, ">", 20);
  } else if (msg.type == WIFI_TOGGLE) {
    strncpy(buttonNameToShow, "WiFi", 20);
  } else {
    // Fallback to button name if no label
    snprintf(buttonNameToShow, 20, "%s+",
             buttonConfigs[currentPreset][btnIdx].name);
  }
  buttonNameToShow[20] = '\0';
  buttonNameDisplayUntil = millis() + 1000;
  safeDisplayOLED();

  executeActionMessage(msg);
}

// Execute a single action message (MIDI or internal command)
void executeActionMessage(const ActionMessage &msg) {
  switch (msg.type) {
  case PRESET_UP:
    // Clear all button states to prevent triggers in new preset
    for (int b = 0; b < MAX_BUTTONS; b++) {
      buttonPinActive[b] = false;
      buttonHoldFired[b] = true;    // Mark as fired to prevent deferred PRESS
      buttonComboChecked[b] = true; // Block combo/release actions
      buttonConsumed[b] = true;     // BLOCK re-trigger until released
    }
    currentPreset = (currentPreset + 1) % 4;
    saveCurrentPresetIndex();
    displayOLED();
    usbMidiLedUpdatePending = true; // Allow LED update in USB MIDI mode
    updateLeds();
    if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected)
      requestPresetState();
    Serial.printf("PRESET UP → Preset %d\n", currentPreset);
    return;

  case PRESET_DOWN:
    // Clear all button states to prevent triggers in new preset
    for (int b = 0; b < MAX_BUTTONS; b++) {
      buttonPinActive[b] = false;
      buttonHoldFired[b] = true;
      buttonComboChecked[b] = true;
      buttonConsumed[b] = true; // BLOCK re-trigger until released
    }
    currentPreset = (currentPreset - 1 + 4) % 4;
    saveCurrentPresetIndex();
    displayOLED();
    usbMidiLedUpdatePending = true; // Allow LED update in USB MIDI mode
    updateLeds();
    if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected)
      requestPresetState();
    Serial.printf("PRESET DOWN → Preset %d\n", currentPreset);
    return;

  case PRESET_1:
  case PRESET_2:
  case PRESET_3:
  case PRESET_4:
    // Clear all button states to prevent triggers in new preset
    for (int b = 0; b < MAX_BUTTONS; b++) {
      buttonPinActive[b] = false;
      buttonHoldFired[b] = true;
      buttonComboChecked[b] = true;
      buttonConsumed[b] = true; // BLOCK re-trigger until released
    }
    currentPreset = msg.type - PRESET_1;
    saveCurrentPresetIndex();
    displayOLED();
    updateLeds();
    if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected)
      requestPresetState();
    return;

  case WIFI_TOGGLE:
    if (isWifiOn) {
      turnWifiOff();
      delay(100);
      yield();
      displayOLED(); // Safe to update display after WiFi off
    } else {
      turnWifiOn();
      delay(200); // Give WiFi more time to stabilize
      yield();
      Serial.println("WiFi toggle complete, skipping display update");
      // Skip displayOLED when WiFi just started - heap is low
    }
    return;

  case CLEAR_BLE_BONDS:
    clearBLEBonds();
    return;

  case MENU_TOGGLE:
    // Toggle between preset mode (0) and menu mode (1)
    if (currentMode == 0) {
      currentMode = 1;
      menuSelection = 0;
      inSubMenu = false;
      inTapTempoMode = false;
      displayMenu();
    } else {
      // Exit menu without saving
      currentMode = 0;
      safeDisplayOLED();
      updateLeds();
    }
    return;

  case MENU_UP:
    if (currentMode == 1) {
      if (inSubMenu || factoryResetConfirm) {
        editingValue--;
      } else {
        menuSelection = (menuSelection - 1 + 13) % 13;
      }
      displayMenu();
    }
    return;

  case MENU_DOWN:
    if (currentMode == 1) {
      if (inSubMenu || factoryResetConfirm) {
        editingValue++;
      } else {
        menuSelection = (menuSelection + 1) % 13;
      }
      displayMenu();
    }
    return;

  case MENU_ENTER:
    if (currentMode == 1) {
      handleMenuSelection();
    }
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
    if (msg.sysex.length > 0 && msg.sysex.length <= 48) {
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

void handleTapTempo(int buttonIndex) {
  unsigned long now = millis();

  // LED feedback on tap - blink only the tap button
  blinkTapButton(buttonIndex);

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
  float finalDelayMs =
      (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
  int delayTimeMS = constrain((int)finalDelayMs, 20, 1000);
  sendDelayTime(delayTimeMS);

  tapModeTimeout = now + 3000;
  displayOLED();
  Serial.printf("Tap Tempo: BPM=%.1f, Pattern=%s, DelayMs=%d\n", currentBPM,
                rhythmNames[rhythmPattern], delayTimeMS);
}

// ============================================
// PRESET MODE - MAIN BUTTON LOOP
// ============================================

void loop_presetMode() {
  unsigned long now = millis();
  // Check for tap mode timeout (only if not locked)
  if (inTapTempoMode && !tapModeLocked && millis() > tapModeTimeout) {
    inTapTempoMode = false;
    tapModeLocked = false;
    // Save rhythm pattern if it was changed during tap mode
    if (rhythmPatternDirty) {
      saveSystemSettings();
      rhythmPatternDirty = false;
    }
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

      float finalDelayMs =
          (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
      int delayTimeMS = constrain((int)finalDelayMs, 0, 1000);
      sendDelayTime(delayTimeMS);

      tapModeTimeout = millis() + 3000;
      displayOLED();
    }
  }

  // ===== BUTTON LOOP =====
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    int pin = systemConfig.buttonPins[i];
    bool pressed = false;

    // Check if multiplexed button
    if (systemConfig.multiplexer.enabled &&
        (strstr(systemConfig.multiplexer.useFor, "buttons") != NULL ||
         strstr(systemConfig.multiplexer.useFor, "both") != NULL) &&
        systemConfig.multiplexer.buttonChannels[i] >= 0) {
      pressed =
          (readMuxDigital(systemConfig.multiplexer.buttonChannels[i]) == LOW);
    } else {
      pressed = (digitalRead(pin) == LOW);
    }

    if (pressed != buttonPinActive[i]) {
      if (pressed) {
        // ===== BUTTON PRESS =====
        // Skip if button was consumed by preset change (requires release first)
        // Track state for proper release detection, but keep holdFired true
        // to prevent any action on release
        if (buttonConsumed[i]) {
          buttonPinActive[i] = true; // Track state for release detection
          buttonHoldFired[i] = true; // Ensure deferred PRESS doesn't fire
          continue;
        }

        if (millis() - lastButtonPressTime_pads[i] > buttonDebounce) {
          buttonPinActive[i] = true;
          lastButtonPressTime_pads[i] = millis();
          buttonHoldStartTime[i] = millis();
          buttonHoldFired[i] = false;
          buttonComboChecked[i] = false;

          ButtonConfig &config = buttonConfigs[currentPreset][i];

          // Check if button has a global LONG_PRESS override configured
          // If so, defer LED toggle and overlay until we know if it's a short
          // press
          bool hasGlobalLongPressAction = false;
          if (globalSpecialActions[i].hasCombo &&
              globalSpecialActions[i].partner == -1) {
            const ActionMessage &globalMsg =
                globalSpecialActions[i].comboAction;
            if (globalMsg.action == ACTION_LONG_PRESS ||
                globalMsg.action == ACTION_2ND_LONG_PRESS) {
              hasGlobalLongPressAction = true;
            }
          }

          // Handle LED state - defer if global LONG_PRESS configured
          if (!hasGlobalLongPressAction) {
            PresetLedMode presetMode = presetLedModes[currentPreset];
            bool isSelectionButton =
                (presetMode == PRESET_LED_SELECTION) ||
                (presetMode == PRESET_LED_HYBRID && config.inSelectionGroup);

            if (isSelectionButton) {
              presetSelectionState[currentPreset] = i;
            } else if (config.ledMode == LED_TOGGLE) {
              // For sync presets (SPM/GP5), don't toggle here - wait for device
              // response For non-sync presets, toggle immediately as before
              if (presetSyncMode[currentPreset] == SYNC_NONE) {
                ledToggleState[i] = !ledToggleState[i];
              }
            }
            // Only update LEDs immediately if NOT in sync mode
            // In sync mode, LEDs are updated when device response arrives
            if (presetSyncMode[currentPreset] == SYNC_NONE) {
              updateLeds();
            }
          }

          // ===== CHECK GLOBAL ACTION FIRST =====
          bool comboFired = false;

          if (globalSpecialActions[i].hasCombo && !inTapTempoMode) {
            const ActionMessage &comboMsg = globalSpecialActions[i].comboAction;
            int8_t partner = globalSpecialActions[i].partner;

            if (partner >= 0 && partner < systemConfig.buttonCount) {
              // COMBO MODE: Wait for partner
              unsigned long waitStart = millis();
              while (!buttonPinActive[partner] && (millis() - waitStart < 40)) {
                // ... (partner checking logic)
                if (digitalRead(systemConfig.buttonPins[partner]) == LOW) {
                  buttonPinActive[partner] = true;
                  lastButtonPressTime_pads[partner] = millis();
                  // Update partner LED state if needed
                  ButtonConfig &partnerConfig =
                      buttonConfigs[currentPreset][partner];
                  PresetLedMode presetMode = presetLedModes[currentPreset];
                  if (presetMode == PRESET_LED_SELECTION ||
                      (presetMode == PRESET_LED_HYBRID &&
                       partnerConfig.inSelectionGroup)) {
                    presetSelectionState[currentPreset] = partner;
                  } else if (partnerConfig.ledMode == LED_TOGGLE) {
                    if (presetSyncMode[currentPreset] == SYNC_NONE)
                      ledToggleState[partner] = !ledToggleState[partner];
                  }
                  // Only update LEDs immediately if NOT in sync mode
                  if (presetSyncMode[currentPreset] == SYNC_NONE) {
                    updateLeds();
                  }
                }
                yield();
                delayMicroseconds(500);
              }

              if (buttonPinActive[partner] && !buttonComboChecked[partner]) {
                // Fire combo!
                fireGlobalAction(comboMsg, i);
                buttonComboChecked[i] = true;
                buttonComboChecked[partner] = true;
                comboFired = true;
              }
            } else if (globalSpecialActions[i].partner == -1) {
              // OVERRIDE MODE: Check for DOUBLE_TAP or toggle types
              ActionType triggerType = ACTION_PRESS;
              bool isDoubleTap = (now - lastButtonReleaseTime_pads[i] < 300);
              if (isDoubleTap && (comboMsg.action == ACTION_DOUBLE_TAP)) {
                triggerType = ACTION_DOUBLE_TAP;
                DBG_INPUT("BTN %d: Global Double Tap detected\n", i);
              } else if (buttonConfigs[currentPreset][i].isAlternate) {
                triggerType = ACTION_2ND_PRESS;
              }

              // Fire if it matches the current event OR its alternate/fallback
              if (comboMsg.action == triggerType ||
                  ((triggerType == ACTION_PRESS ||
                    triggerType == ACTION_2ND_PRESS) &&
                   comboMsg.action == ACTION_COMBO) ||
                  (triggerType == ACTION_2ND_PRESS &&
                   comboMsg.action == ACTION_PRESS)) {
                fireGlobalAction(comboMsg, i);

                // Toggle alternate state if global action has 2ND_PRESS
                if (comboMsg.action == ACTION_2ND_PRESS ||
                    (triggerType == ACTION_PRESS &&
                     hasAction(buttonConfigs[currentPreset][i],
                               ACTION_2ND_PRESS))) {
                  // Note: we use the regular button's toggle state for
                  // convenience
                  buttonConfigs[currentPreset][i].isAlternate =
                      !buttonConfigs[currentPreset][i].isAlternate;
                  ledToggleState[i] =
                      buttonConfigs[currentPreset][i].isAlternate;
                }

                // Only block local button logic if the override actually fired
                buttonComboChecked[i] = true;
                comboFired = true;
              }
            }
          }
          // Check reverse combo (partner has combo pointing to us)
          if (!comboFired && !inTapTempoMode) {
            for (int p = 0; p < systemConfig.buttonCount; p++) {
              if (p != i && buttonPinActive[p] && !buttonComboChecked[p] &&
                  globalSpecialActions[p].hasCombo &&
                  globalSpecialActions[p].partner == i) {

                fireGlobalAction(globalSpecialActions[p].comboAction, p);
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
              ActionMessage *tapAction = findAction(
                  buttonConfigs[currentPreset][tapBtn], ACTION_PRESS);
              if (tapAction && tapAction->type == TAP_TEMPO) {
                // RHYTHM PREV
                if (tapAction->tapTempo.rhythmPrev == i) {
                  rhythmPattern = (rhythmPattern - 1 + 4) % 4;
                  rhythmPatternDirty = true; // Defer save to mode exit
                  float delayMs =
                      (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                  sendDelayTime(constrain((int)delayMs, 0, 1000));
                  strncpy(buttonNameToShow, rhythmNames[rhythmPattern], 20);
                  buttonNameDisplayUntil = millis() + 1000;
                  if (!tapModeLocked)
                    tapModeTimeout = millis() + 3000;
                  safeDisplayOLED();
                  isTapControl = true;
                  break;
                }
                // RHYTHM NEXT
                if (tapAction->tapTempo.rhythmNext == i) {
                  rhythmPattern = (rhythmPattern + 1) % 4;
                  rhythmPatternDirty = true; // Defer save to mode exit
                  float delayMs =
                      (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
                  sendDelayTime(constrain((int)delayMs, 0, 1000));
                  strncpy(buttonNameToShow, rhythmNames[rhythmPattern], 20);
                  buttonNameDisplayUntil = millis() + 1000;
                  if (!tapModeLocked)
                    tapModeTimeout = millis() + 3000;
                  safeDisplayOLED();
                  isTapControl = true;
                  break;
                }
                // TAP LOCK
                if (tapAction->tapTempo.tapLock == i) {
                  tapModeLocked = !tapModeLocked;
                  if (tapModeLocked) {
                    strncpy(buttonNameToShow, "LOCKED", 20);
                    buttonNameDisplayUntil = millis() + 1000;
                  } else {
                    inTapTempoMode = false;
                    // Save rhythm pattern if changed during tap mode
                    if (rhythmPatternDirty) {
                      saveSystemSettings();
                      rhythmPatternDirty = false;
                    }
                    buttonNameDisplayUntil = 0;
                  }
                  safeDisplayOLED();
                  isTapControl = true;
                  break;
                }
              }
            }

            if (isTapControl) {
              buttonConsumed[i] = true;
              continue; // Skip normal button handling
            }
          }

          // ===== NORMAL BUTTON PRESS (no combo, no tap control) =====
          if (!comboFired) {
            // Check for Double Tap for regular buttons
            ActionMessage *doubleTapAction =
                findAction(config, ACTION_DOUBLE_TAP);
            if (doubleTapAction &&
                (now - lastButtonReleaseTime_pads[i] < 300) &&
                (!inTapTempoMode || doubleTapAction->type == TAP_TEMPO)) {
              DBG_INPUT("BTN %d: Double Tap detected\n", i);
              executeActionMessage(*doubleTapAction);
              // Block regular press
              buttonComboChecked[i] = true;
              updateLeds();
              continue;
            }

            // Determine the target action type based on toggle state
            ActionType targetActionType = ACTION_PRESS;
            if (config.isAlternate) {
              // Try 2ND_PRESS first, fallback to PRESS if no 2ND_PRESS
              // configured
              if (hasAction(config, ACTION_2ND_PRESS)) {
                targetActionType = ACTION_2ND_PRESS;
              }
            }

            DBG_INPUT("BTN %d: Target Action Type = %d\n", i, targetActionType);

            // CHECK FOR LONG PRESS FIRST (Deferral Logic)
            // If ANY long press action exists, we must defer the primary press
            ActionType longPressType =
                config.isAlternate ? ACTION_2ND_LONG_PRESS : ACTION_LONG_PRESS;
            ActionMessage *longPress = findAction(config, longPressType);
            if (!longPress) {
              longPress = findAction(config, ACTION_LONG_PRESS);
            }

            // Global Long Press Override Check
            bool hasGlobalLongPress = false;
            if (globalSpecialActions[i].hasCombo &&
                globalSpecialActions[i].partner == -1) {
              const ActionMessage &globalMsg =
                  globalSpecialActions[i].comboAction;
              if (globalMsg.action == ACTION_LONG_PRESS ||
                  globalMsg.action == ACTION_2ND_LONG_PRESS) {
                hasGlobalLongPress = true;
              }
            }

            if (longPress || hasGlobalLongPress) {
              DBG_INPUT("BTN %d: Deferring PRESS (has LONG_PRESS: local=%d, "
                        "global=%d)\n",
                        i, longPress != nullptr, hasGlobalLongPress);
            } else {
              // EXECUTE ALL MATCHING ACTIONS
              bool actionExecuted = false;
              bool tapTempoHandled = false;

              for (int m = 0; m < config.messageCount; m++) {
                ActionMessage &msg = config.messages[m];
                if (msg.action == targetActionType) {

                  if (inTapTempoMode && msg.type != TAP_TEMPO)
                    continue;

                  if (msg.type == TAP_TEMPO) {
                    if (!tapTempoHandled) { // Only handle tap once per press
                      handleTapTempo(i);
                      tapTempoHandled = true;
                    }
                  } else {
                    // Display logic (use label from first executed action)
                    if (!actionExecuted) {
                      if (msg.label[0] != '\0') {
                        strncpy(buttonNameToShow, msg.label, 20);
                      } else {
                        strncpy(buttonNameToShow, config.name, 20);
                      }
                      buttonNameToShow[20] = '\0';
                      buttonNameDisplayUntil = millis() + 1000;
                      safeDisplayOLED();
                    }

                    executeActionMessage(msg);
                    actionExecuted = true;
                  }
                }
              }

              if (actionExecuted) {
                // GP5 Sync Request
                if (presetSyncMode[currentPreset] == SYNC_GP5 &&
                    clientConnected) {
                  delay(100);
                  requestPresetState();
                }

                // Toggle Alternate State (only once per press)
                if (hasAction(config, ACTION_2ND_PRESS)) {
                  config.isAlternate = !config.isAlternate;
                  ledToggleState[i] = config.isAlternate;
                  DBG_INPUT("BTN %d: toggled isAlternate to %d, LED=%d\n", i,
                            config.isAlternate, ledToggleState[i]);
                  updateLeds();
                }
              } else {
                DBG_INPUT("BTN %d: NO ACTION FOUND for type %d\n", i,
                          targetActionType);
              }
            }
          }
        }
      } else {
        // ===== BUTTON RELEASE =====
        buttonPinActive[i] = false;
        lastButtonReleaseTime_pads[i] = millis();

        // Skip ALL release handling if button was consumed (e.g., by preset
        // change) This ensures no action fires when releasing after a global
        // LONG_PRESS
        if (buttonConsumed[i] || buttonHoldFired[i]) {
          DBG_INPUT("BTN %d: Release skipped (consumed=%d, holdFired=%d)\n", i,
                    buttonConsumed[i], buttonHoldFired[i]);
          buttonComboChecked[i] = false;
          buttonHoldFired[i] = false;
          buttonConsumed[i] = false;
          updateLeds();
          continue;
        }

        if (!buttonComboChecked[i]) {
          ButtonConfig &config = buttonConfigs[currentPreset][i];

          // ===== DEFERRED PRESS (for buttons with LONG_PRESS) =====
          // If button has LONG_PRESS (local or global) but it didn't fire, fire
          // PRESS now
          if (!buttonHoldFired[i]) {
            ActionType longPressType =
                config.isAlternate ? ACTION_2ND_LONG_PRESS : ACTION_LONG_PRESS;
            ActionMessage *longPress = findAction(config, longPressType);
            if (!longPress) {
              longPress = findAction(config, ACTION_LONG_PRESS);
            }

            // Also check for GLOBAL LONG_PRESS override
            bool hasGlobalLongPress = false;
            if (globalSpecialActions[i].hasCombo &&
                globalSpecialActions[i].partner == -1) {
              const ActionMessage &globalMsg =
                  globalSpecialActions[i].comboAction;
              if (globalMsg.action == ACTION_LONG_PRESS ||
                  globalMsg.action == ACTION_2ND_LONG_PRESS) {
                hasGlobalLongPress = true;
              }
            }

            if (longPress || hasGlobalLongPress) {
              // Button has LONG_PRESS configured (local or global) but it
              // didn't fire This means we need to fire the deferred PRESS now
              ActionMessage *pressAction =
                  config.isAlternate ? findAction(config, ACTION_2ND_PRESS)
                                     : findAction(config, ACTION_PRESS);
              if (!pressAction) {
                pressAction = findAction(config, ACTION_PRESS);
              }

              if (pressAction && pressAction->type != TAP_TEMPO &&
                  !inTapTempoMode) {
                DBG_INPUT(
                    "BTN %d: Firing deferred PRESS on release (global=%d)\n", i,
                    hasGlobalLongPress);

                // Handle LED toggle that was deferred
                PresetLedMode presetMode = presetLedModes[currentPreset];
                bool isSelectionButton = (presetMode == PRESET_LED_SELECTION) ||
                                         (presetMode == PRESET_LED_HYBRID &&
                                          config.inSelectionGroup);

                if (isSelectionButton) {
                  presetSelectionState[currentPreset] = i;
                } else if (config.ledMode == LED_TOGGLE) {
                  if (presetSyncMode[currentPreset] == SYNC_NONE) {
                    ledToggleState[i] = !ledToggleState[i];
                  }
                }

                // Display action label/button name (was deferred)
                if (pressAction->label[0] != '\0') {
                  strncpy(buttonNameToShow, pressAction->label, 20);
                } else {
                  strncpy(buttonNameToShow, config.name, 20);
                }
                buttonNameToShow[20] = '\0';
                buttonNameDisplayUntil = millis() + 1000;
                safeDisplayOLED();

                executeActionMessage(*pressAction);

                // GP5 Sync: Request state after any button action
                if (presetSyncMode[currentPreset] == SYNC_GP5 &&
                    clientConnected) {
                  delay(100);
                  requestPresetState();
                }
                // Update LEDs only if NOT in sync mode
                // In sync mode, LEDs are updated when device response arrives
                if (presetSyncMode[currentPreset] == SYNC_NONE) {
                  updateLeds();
                }

                // Toggle alternate state if button has 2ND_PRESS
                // Toggle locally as we sent a toggle command to device
                if (hasAction(config, ACTION_2ND_PRESS)) {
                  config.isAlternate = !config.isAlternate;
                  ledToggleState[i] = config.isAlternate;
                  DBG_INPUT("BTN %d: toggled isAlternate to %d\n", i,
                            config.isAlternate);
                  updateLeds();
                }
              } else if (pressAction && pressAction->type == TAP_TEMPO) {
                // Handle deferred TAP_TEMPO
                handleTapTempo(i);
              }
            }
          }

          // Check for RELEASE or 2ND_RELEASE action based on isAlternate state
          ActionType releaseType =
              config.isAlternate ? ACTION_2ND_RELEASE : ACTION_RELEASE;

          // Check if we need to fallback: if target is 2ND_RELEASE but none
          // exists, try RELEASE
          if (releaseType == ACTION_2ND_RELEASE &&
              !hasAction(config, ACTION_2ND_RELEASE)) {
            releaseType = ACTION_RELEASE;
          }

          // Execute ALL matching release actions
          for (int m = 0; m < config.messageCount; m++) {
            ActionMessage &msg = config.messages[m];
            if (msg.action == releaseType) {
              if (inTapTempoMode && msg.type != TAP_TEMPO)
                continue;

              executeActionMessage(msg);
            }
          }

          // Handle NOTE_MOMENTARY note off for ALL matching messages
          ActionType momentaryType =
              config.isAlternate ? ACTION_2ND_PRESS : ACTION_PRESS;
          for (int m = 0; m < config.messageCount; m++) {
            ActionMessage &msg = config.messages[m];
            if ((msg.action == momentaryType || msg.action == ACTION_PRESS) &&
                msg.type == NOTE_MOMENTARY) {
              sendMidiNoteOff(msg.channel, msg.data1, 0);
            }
          }
        } else {
          // Check for Global Override RELEASE or 2ND_RELEASE
          if (globalSpecialActions[i].hasCombo &&
              globalSpecialActions[i].partner == -1 && !inTapTempoMode) {
            const ActionMessage &comboMsg = globalSpecialActions[i].comboAction;
            ActionType releaseType = buttonConfigs[currentPreset][i].isAlternate
                                         ? ACTION_2ND_RELEASE
                                         : ACTION_RELEASE;

            if (comboMsg.action == releaseType ||
                (releaseType == ACTION_2ND_RELEASE &&
                 comboMsg.action == ACTION_RELEASE)) {
              fireGlobalAction(comboMsg, i);
            }
          }
        }

        buttonComboChecked[i] = false;
        buttonHoldFired[i] = false;
        buttonConsumed[i] =
            false; // Allow button to trigger again after release
        updateLeds();
      }
    }

    // ===== CHECK FOR LONG_PRESS OR 2ND_LONG_PRESS ACTION =====
    if (buttonPinActive[i] && !buttonHoldFired[i]) {
      // Check for Global Override ACTION_LONG_PRESS or 2ND_LONG_PRESS
      if (globalSpecialActions[i].hasCombo &&
          globalSpecialActions[i].partner == -1 && !inTapTempoMode) {
        const ActionMessage &comboMsg = globalSpecialActions[i].comboAction;
        ActionType holdType = buttonConfigs[currentPreset][i].isAlternate
                                  ? ACTION_2ND_LONG_PRESS
                                  : ACTION_LONG_PRESS;

        if (comboMsg.action == holdType ||
            (holdType == ACTION_2ND_LONG_PRESS &&
             comboMsg.action == ACTION_LONG_PRESS)) {
          unsigned long elapsed = millis() - buttonHoldStartTime[i];
          uint16_t threshold =
              comboMsg.longPress.holdMs > 0 ? comboMsg.longPress.holdMs : 700;
          if (elapsed >= threshold) {
            fireGlobalAction(comboMsg, i);
            buttonHoldFired[i] = true;
            buttonComboChecked[i] = true; // Block all normal button handling
            DBG_INPUT("BTN %d Global LONG_PRESS fired (type=%d)\n", i,
                      comboMsg.action);
            updateLeds();
          }
        }
      } else if (!buttonComboChecked[i]) {
        // Normal button hold check
        ButtonConfig &config = buttonConfigs[currentPreset][i];
        // Check for 2ND_LONG_PRESS if in alternate state, otherwise LONG_PRESS
        ActionType longPressType =
            config.isAlternate ? ACTION_2ND_LONG_PRESS : ACTION_LONG_PRESS;
        ActionMessage *longPress = findAction(config, longPressType);
        if (!longPress) {
          // Fallback: try regular LONG_PRESS if 2ND_LONG_PRESS not found
          longPress = findAction(config, ACTION_LONG_PRESS);
        }

        if (longPress && (!inTapTempoMode || longPress->type == TAP_TEMPO)) {
          unsigned long elapsed = millis() - buttonHoldStartTime[i];
          uint16_t threshold = longPress->longPress.holdMs > 0
                                   ? longPress->longPress.holdMs
                                   : 700;

          if (elapsed >= threshold) {
            // Fire long press action - show label if available
            if (longPress->label[0] != '\0') {
              strncpy(buttonNameToShow, longPress->label, 20);
            } else {
              // Try to get a meaningful name from the command type
              switch (longPress->type) {
              case PRESET_UP:
                strncpy(buttonNameToShow, "PRESET+", 20);
                break;
              case PRESET_DOWN:
                strncpy(buttonNameToShow, "PRESET-", 20);
                break;
              case WIFI_TOGGLE:
                strncpy(buttonNameToShow, "WiFi", 20);
                break;
              case CLEAR_BLE_BONDS:
                strncpy(buttonNameToShow, "BLE CLR", 20);
                break;
              default:
                strncpy(buttonNameToShow, "HOLD", 20);
                break;
              }
            }
            buttonNameToShow[20] = '\0';
            buttonNameDisplayUntil = millis() + 1000;
            safeDisplayOLED();

            executeActionMessage(*longPress);

            // GP5 Sync: Request state after any button action
            if (presetSyncMode[currentPreset] == SYNC_GP5 && clientConnected) {
              delay(100);
              requestPresetState();
            }

            buttonHoldFired[i] = true;
            DBG_INPUT("BTN %d LONG_PRESS fired\n", i);
            updateLeds();
          }
        }
      }
    }
  }
}

// ============================================
// MENU MODE
// ============================================

// Forward declaration
void handleEditMenuInput(int direction, bool enter);

void loop_menuMode() {
  // === Edit Commands mode: intercept button presses for button selection ===
  if (editMenuState == EDIT_BTN_LISTEN) {
    for (int i = 0; i < systemConfig.buttonCount; i++) {
      int pin = systemConfig.buttonPins[i];
      bool isPressed = (digitalRead(pin) == LOW);
      if (isPressed && !buttonPinActive[i]) {
        unsigned long now = millis();
        if (now - lastButtonPressTime_pads[i] > buttonDebounce) {
          lastButtonPressTime_pads[i] = now;
          buttonPinActive[i] = true;
          // Skip MENU buttons - they should still navigate
          ButtonConfig &btn = buttonConfigs[currentPreset][i];
          if (btn.messageCount > 0 && (btn.messages[0].type == MENU_UP ||
                                       btn.messages[0].type == MENU_DOWN ||
                                       btn.messages[0].type == MENU_ENTER ||
                                       btn.messages[0].type == MENU_TOGGLE)) {
            // Let menu buttons fall through to normal handling below
          } else {
            // Select this button for editing
            editBtnIndex = i;
            editSubSelection = 0;
            editMenuState = EDIT_BTN_ACTIONS;
            displayEditMenu();
            return;
          }
        }
      } else if (!isPressed && buttonPinActive[i]) {
        buttonPinActive[i] = false;
      }
    }
  }

  // === Scan buttons for menu commands ===
  // This allows buttons configured with MENU_UP/DOWN/ENTER to work in menu mode
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    int pin = systemConfig.buttonPins[i];
    bool isPressed = (digitalRead(pin) == LOW);

    if (isPressed && !buttonPinActive[i]) {
      // Button just pressed - check for menu commands
      unsigned long now = millis();
      if (now - lastButtonPressTime_pads[i] > buttonDebounce) {
        lastButtonPressTime_pads[i] = now;
        buttonPinActive[i] = true;

        // Check this button's first message for menu commands
        ButtonConfig &btn = buttonConfigs[currentPreset][i];
        if (btn.messageCount > 0) {
          ActionMessage &msg = btn.messages[0];
          if (msg.action == ACTION_PRESS) {
            switch (msg.type) {
            case MENU_TOGGLE:
              // Exit edit mode if active, then exit menu
              if (editMenuState != EDIT_NONE) {
                editMenuState = EDIT_NONE;
              }
              currentMode = 0;
              safeDisplayOLED();
              updateLeds();
              return;
            case MENU_UP:
              if (editMenuState != EDIT_NONE) {
                handleEditMenuInput(-1, false);
              } else if (inSubMenu || factoryResetConfirm) {
                editingValue--;
              } else {
                menuSelection = (menuSelection - 1 + 15) % 15;
              }
              if (editMenuState != EDIT_NONE)
                displayEditMenu();
              else
                displayMenu();
              return;
            case MENU_DOWN:
              if (editMenuState != EDIT_NONE) {
                handleEditMenuInput(1, false);
              } else if (inSubMenu || factoryResetConfirm) {
                editingValue++;
              } else {
                menuSelection = (menuSelection + 1) % 15;
              }
              if (editMenuState != EDIT_NONE)
                displayEditMenu();
              else
                displayMenu();
              return;
            case MENU_ENTER:
              if (editMenuState != EDIT_NONE) {
                handleEditMenuInput(0, true);
              } else {
                handleMenuSelection();
              }
              return;
            default:
              break;
            }
          }
        }
      }
    } else if (!isPressed && buttonPinActive[i]) {
      buttonPinActive[i] = false;
    }
  }

  // === Encoder handling ===
  long newEncoderPosition = encoder.getCount();
  if (newEncoderPosition == oldEncoderPosition)
    return;

  int change = (newEncoderPosition - oldEncoderPosition);
  int numMenuItems = 15; // Menu items count

  // Route to edit menu handler if in edit mode
  if (editMenuState != EDIT_NONE) {
    long menuStep = newEncoderPosition / 2;
    long oldMenuStep = oldEncoderPosition / 2;
    if (menuStep != oldMenuStep) {
      int dir = (menuStep > oldMenuStep) ? 1 : -1;
      handleEditMenuInput(dir, false);
    }
    oldEncoderPosition = newEncoderPosition;
    displayEditMenu();
    return;
  }

  if (factoryResetConfirm) {
    // In Factory Reset confirmation - just toggle between Yes (0) and No (1)
    if (change != 0) {
      editingValue = (editingValue == 0) ? 1 : 0;
    }
    oldEncoderPosition = newEncoderPosition;
  } else if (inSubMenu) {
    editingValue += change;
    oldEncoderPosition = newEncoderPosition;
    if (menuSelection == 4)
      editingValue = constrain(editingValue, 0, 255); // LED Bright On
    if (menuSelection == 5)
      editingValue = constrain(editingValue, 0, 255); // LED Bright Dim
    if (menuSelection == 6)
      editingValue = constrain(editingValue, 1, 500); // Pad Debounce
    if (menuSelection == 10)
      editingValue = constrain(editingValue, 1, 7); // Name Font Size (Overlay)
  } else {
    long menuStep = newEncoderPosition / 2;
    long oldMenuStep = oldEncoderPosition / 2;
    if (menuStep != oldMenuStep) {
      menuSelection += (menuStep - oldMenuStep);
      menuSelection =
          (menuSelection % numMenuItems + numMenuItems) % numMenuItems;
    }
    oldEncoderPosition = newEncoderPosition;
  }
  displayMenu();
}

void handleMenuSelection() {
  // Handle Factory Reset confirmation first
  if (factoryResetConfirm) {
    if (editingValue == 0) {
      // User selected "Yes, Reset"
      systemPrefs.begin("midi_presets", false);
      systemPrefs.clear();
      systemPrefs.end();
      systemPrefs.begin("midi_system", false);
      systemPrefs.clear();
      systemPrefs.end();
      ESP.restart();
    } else {
      // User selected "No, Go Back"
      factoryResetConfirm = false;
      displayMenu();
    }
    return;
  }

  if (!inSubMenu) {
    switch (menuSelection) {
    case 0: // Save and Exit
      saveSystemSettings();
      savePresets(); // Also save button/analog edits
      if (bleModeChanged) {
        Serial.println("BLE Mode changed - rebooting...");
        delay(500);
        ESP.restart();
      }
      currentMode = 0;
      safeDisplayOLED();
      updateLeds();
      return; // Exit function to prevent displayMenu() at end
    case 1:   // Exit without Saving
      currentMode = 0;
      safeDisplayOLED();
      updateLeds();
      return; // Exit function to prevent displayMenu() at end
    case 2:   // WiFi Toggle
      if (isWifiOn) {
        turnWifiOff();
        delay(100);
        yield();
        displayMenu(); // Safe after WiFi off
      } else {
        if (isBtSerialOn) {
          Serial.println("Cannot enable WiFi while BT Serial is on");
          // Turn off BT Serial first
          turnBtSerialOff();
          delay(200);
        }
        turnWifiOn();
        delay(200);
        yield();
        // Skip displayMenu when WiFi just started - heap is low
      }
      break;
    case 3: // BT Serial Toggle (NEW)
      if (isBtSerialOn) {
        turnBtSerialOff();
        delay(100);
        yield();
        displayMenu();
      } else {
        if (isWifiOn) {
          Serial.println("Cannot enable BT Serial while WiFi is on");
          // Turn off WiFi first
          turnWifiOff();
          delay(200);
        }
        turnBtSerialOn();
        delay(200);
        yield();
        displayMenu();
      }
      break;
    case 4:
      inSubMenu = true;
      editingValue = ledBrightnessOn;
      break;
    case 5:
      inSubMenu = true;
      editingValue = ledBrightnessDim;
      break;
    case 6:
      inSubMenu = true;
      editingValue = buttonDebounce;
      break;
    case 7:
      clearBLEBonds();
      currentMode = 0;
      safeDisplayOLED();
      updateLeds();
      return; // Exit function to prevent displayMenu() at end
    case 8:
      ESP.restart();
      break;
    case 9: // Factory Reset - show confirmation
      if (!factoryResetConfirm) {
        factoryResetConfirm = true; // Enter confirmation mode
        editingValue = 1;           // Default to "No, go back" (1 = No)
      } else {
        // Already in confirmation, this shouldn't be reached normally
        factoryResetConfirm = false;
      }
      displayMenu();
      break;
    case 10:
      inSubMenu = true;
      editingValue =
          oledConfig.overlay.titleSize > 0 ? oledConfig.overlay.titleSize : 2;
      break;
    case 11: // WiFi at Boot
      systemConfig.wifiOnAtBoot = !systemConfig.wifiOnAtBoot;
      displayMenu();
      break;
    case 12: // MIDI Mode - cycles: CLIENT → SERVER → DUAL → USB → EDIT → CLIENT
      if (bleConfigMode) {
        // Currently in EDIT mode, go to CLIENT
        toggleBleConfigMode(); // Turn off config mode
        systemConfig.bleMode = BLE_CLIENT_ONLY;
        bleModeChanged = true;
      } else if (systemConfig.bleMode == BLE_CLIENT_ONLY) {
        systemConfig.bleMode = BLE_SERVER_ONLY;
        bleModeChanged = true;
      } else if (systemConfig.bleMode == BLE_SERVER_ONLY) {
        systemConfig.bleMode = BLE_DUAL_MODE;
        bleModeChanged = true;
      } else if (systemConfig.bleMode == BLE_DUAL_MODE) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
        // ESP32-S3: Add USB MIDI option
        systemConfig.bleMode = MIDI_USB_ONLY;
        bleModeChanged = true;
#else
        // Non-S3: Skip USB mode, go to EDIT
        toggleBleConfigMode(); // Turn on config mode
#endif
      } else if (systemConfig.bleMode == MIDI_USB_ONLY) {
        // From USB mode, enter EDIT mode
        toggleBleConfigMode(); // Turn on config mode
      } else {
        // Any other mode, go to CLIENT
        systemConfig.bleMode = BLE_CLIENT_ONLY;
        bleModeChanged = true;
      }
      displayMenu();
      break;
    case 13: // Analog Debug
      systemConfig.debugAnalogIn = !systemConfig.debugAnalogIn;
      displayMenu();
      break;
    case 14: // Edit Commands
      editMenuState = EDIT_ROOT;
      editSubSelection = 0;
      displayEditMenu();
      return; // Skip displayMenu() at end
    }
  } else {
    inSubMenu = false;
    switch (menuSelection) {
    case 4:
      ledBrightnessOn = editingValue;
      updateLeds();
      break;
    case 5:
      ledBrightnessDim = editingValue;
      updateLeds();
      break;
    case 6:
      buttonDebounce = editingValue;
      break;
    case 10:
      oledConfig.overlay.titleSize = editingValue;
      break;
    }
  }
  displayMenu();
}

// ============================================
// EDIT COMMANDS INPUT HANDLER
// direction: -1 (up/prev), +1 (down/next), 0 (no scroll)
// enter: true when ENTER/confirm is pressed
// ============================================
void handleEditMenuInput(int direction, bool enter) {
  switch (editMenuState) {

  case EDIT_ROOT: {
    if (!enter) {
      editSubSelection = (editSubSelection + direction + 3) % 3;
    } else {
      switch (editSubSelection) {
      case 0: // Buttons
        editMenuState = EDIT_BTN_LISTEN;
        break;
      case 1: // Analog Inputs
        editMenuState = EDIT_AIN_LIST;
        editSubSelection = 0;
        break;
      case 2: // Back
        editMenuState = EDIT_NONE;
        displayMenu();
        return;
      }
    }
    displayEditMenu();
    break;
  }

  case EDIT_BTN_LISTEN: {
    // Only ENTER goes back (button presses handled separately in loop_menuMode)
    if (enter) {
      editMenuState = EDIT_ROOT;
      editSubSelection = 0;
      displayEditMenu();
    }
    break;
  }

  case EDIT_BTN_ACTIONS: {
    ButtonConfig &btn = buttonConfigs[currentPreset][editBtnIndex];
    int totalItems = btn.messageCount + 1; // actions + Back
    if (!enter) {
      editSubSelection += direction;
      if (editSubSelection < 0)
        editSubSelection = totalItems - 1;
      if (editSubSelection >= totalItems)
        editSubSelection = 0;
    } else {
      if (editSubSelection < btn.messageCount) {
        // Enter action field editor
        editActionIndex = editSubSelection;
        editFieldIndex = 0;
        inSubMenu = false;
        editMenuState = EDIT_BTN_FIELD;
      } else {
        // Back
        editMenuState = EDIT_BTN_LISTEN;
        editSubSelection = 0;
      }
    }
    displayEditMenu();
    break;
  }

  case EDIT_BTN_FIELD: {
    ButtonConfig &btn = buttonConfigs[currentPreset][editBtnIndex];
    ActionMessage &m = btn.messages[editActionIndex];
    int fieldCount = 8; // Type, Channel, Data1, Data2, Hue, Sat, Val, Back

    if (inSubMenu) {
      // Currently editing a value with encoder
      if (!enter) {
        editingValue += direction;
        // Constrain based on field
        if (editFieldIndex == 0) { // Type
          if (editingValue < 0)
            editingValue = MIDI_TYPE_COUNT - 1;
          if (editingValue >= MIDI_TYPE_COUNT)
            editingValue = 0;
        } else if (editFieldIndex == 1) { // Channel
          editingValue = constrain(editingValue, 1, 16);
        } else if (editFieldIndex <= 3) { // Data1/Data2
          editingValue = constrain(editingValue, 0, 127);
        } else if (editFieldIndex == 4) { // Hue
          if (editingValue < 0)
            editingValue = 359;
          if (editingValue >= 360)
            editingValue = 0;
        } else { // Sat/Val
          editingValue = constrain(editingValue, 0, 100);
        }
      } else {
        // Confirm value
        if (editFieldIndex == 0)
          m.type = (MidiCommandType)editingValue;
        else if (editFieldIndex == 1)
          m.channel = editingValue;
        else if (editFieldIndex == 2)
          m.data1 = editingValue;
        else if (editFieldIndex == 3)
          m.data2 = editingValue;
        else if (editFieldIndex >= 4 && editFieldIndex <= 6) {
          // HSV: get current HSV, update the changed component, convert back to
          // RGB
          int h, s, v;
          rgbToHsv(m.rgb[0], m.rgb[1], m.rgb[2], &h, &s, &v);
          if (editFieldIndex == 4)
            h = editingValue;
          else if (editFieldIndex == 5)
            s = editingValue;
          else
            v = editingValue;
          hsvToRgb(h, s, v, &m.rgb[0], &m.rgb[1], &m.rgb[2]);
        }
        inSubMenu = false;
      }
    } else {
      // Navigating fields
      if (!enter) {
        editFieldIndex += direction;
        if (editFieldIndex < 0)
          editFieldIndex = fieldCount - 1;
        if (editFieldIndex >= fieldCount)
          editFieldIndex = 0;
      } else {
        if (editFieldIndex == 7) {
          // Back to action list
          editMenuState = EDIT_BTN_ACTIONS;
          editSubSelection = editActionIndex;
        } else {
          // Start editing this field
          inSubMenu = true;
          if (editFieldIndex == 0)
            editingValue = (int)m.type;
          else if (editFieldIndex == 1)
            editingValue = m.channel;
          else if (editFieldIndex == 2)
            editingValue = m.data1;
          else if (editFieldIndex == 3)
            editingValue = m.data2;
          else {
            // HSV fields
            int h, s, v;
            rgbToHsv(m.rgb[0], m.rgb[1], m.rgb[2], &h, &s, &v);
            if (editFieldIndex == 4)
              editingValue = h;
            else if (editFieldIndex == 5)
              editingValue = s;
            else
              editingValue = v;
          }
        }
      }
    }
    displayEditMenu();
    break;
  }

  case EDIT_AIN_LIST: {
    int ainCount = systemConfig.analogInputCount;
    int totalItems = ainCount + 1; // inputs + Back
    if (!enter) {
      editSubSelection += direction;
      if (editSubSelection < 0)
        editSubSelection = totalItems - 1;
      if (editSubSelection >= totalItems)
        editSubSelection = 0;
    } else {
      if (editSubSelection < ainCount) {
        editAinIndex = editSubSelection;
        editSubSelection = 0;
        editMenuState = EDIT_AIN_DETAIL;
      } else {
        // Back
        editMenuState = EDIT_ROOT;
        editSubSelection = 1; // Return cursor to "Analog Inputs"
      }
    }
    displayEditMenu();
    break;
  }

  case EDIT_AIN_DETAIL: {
    AnalogInputConfig &ain = analogInputs[editAinIndex];
    int totalItems = 1 + ain.messageCount + 1; // enabled + actions + back
    if (!enter) {
      editSubSelection += direction;
      if (editSubSelection < 0)
        editSubSelection = totalItems - 1;
      if (editSubSelection >= totalItems)
        editSubSelection = 0;
    } else {
      if (editSubSelection == 0) {
        // Toggle enabled
        ain.enabled = !ain.enabled;
      } else if (editSubSelection <= ain.messageCount) {
        // Enter action field editor
        editActionIndex = editSubSelection - 1;
        editFieldIndex = 0;
        inSubMenu = false;
        editMenuState = EDIT_AIN_FIELD;
      } else {
        // Back
        editMenuState = EDIT_AIN_LIST;
        editSubSelection = editAinIndex;
      }
    }
    displayEditMenu();
    break;
  }

  case EDIT_AIN_FIELD: {
    AnalogInputConfig &ain = analogInputs[editAinIndex];
    ActionMessage &m = ain.messages[editActionIndex];
    bool isSysexScroll = (m.type == SYSEX_SCROLL);
    int fieldCount =
        isSysexScroll
            ? 5
            : 5; // SysEx: Type,Param,Min,Max,Back | Others: Type,Ch,D1,D2,Back
    int backIdx = fieldCount - 1;

    if (inSubMenu) {
      if (!enter) {
        editingValue += direction;
        if (editFieldIndex == 0) { // Type
          if (editingValue < 0)
            editingValue = MIDI_TYPE_COUNT - 1;
          if (editingValue >= MIDI_TYPE_COUNT)
            editingValue = 0;
        } else if (isSysexScroll) {
          if (editFieldIndex == 1) { // Param ID
            if (editingValue < 0)
              editingValue = 7;
            if (editingValue > 7)
              editingValue = 0;
          } else if (editFieldIndex == 2) { // Min range %
            editingValue = constrain(editingValue, 0, 100);
          } else if (editFieldIndex == 3) { // Max range %
            editingValue = constrain(editingValue, 0, 100);
          }
        } else {
          if (editFieldIndex == 1)
            editingValue = constrain(editingValue, 1, 16);
          else
            editingValue = constrain(editingValue, 0, 127);
        }
      } else {
        // Confirm
        if (editFieldIndex == 0) {
          m.type = (MidiCommandType)editingValue;
          // If type changed to/from SYSEX_SCROLL, reset field index
          editFieldIndex = 0;
        } else if (isSysexScroll) {
          if (editFieldIndex == 1)
            m.data1 = editingValue;
          else if (editFieldIndex == 2)
            m.minOut = editingValue;
          else if (editFieldIndex == 3)
            m.maxOut = editingValue;
        } else {
          if (editFieldIndex == 1)
            m.channel = editingValue;
          else if (editFieldIndex == 2)
            m.data1 = editingValue;
          else if (editFieldIndex == 3)
            m.data2 = editingValue;
        }
        inSubMenu = false;
      }
    } else {
      if (!enter) {
        editFieldIndex += direction;
        if (editFieldIndex < 0)
          editFieldIndex = backIdx;
        if (editFieldIndex > backIdx)
          editFieldIndex = 0;
      } else {
        if (editFieldIndex == backIdx) {
          editMenuState = EDIT_AIN_DETAIL;
          editSubSelection = editActionIndex + 1;
        } else {
          inSubMenu = true;
          if (editFieldIndex == 0)
            editingValue = (int)m.type;
          else if (isSysexScroll) {
            if (editFieldIndex == 1)
              editingValue = m.data1;
            else if (editFieldIndex == 2)
              editingValue = m.minOut;
            else if (editFieldIndex == 3)
              editingValue = m.maxOut;
          } else {
            if (editFieldIndex == 1)
              editingValue = m.channel;
            else if (editFieldIndex == 2)
              editingValue = m.data1;
            else if (editFieldIndex == 3)
              editingValue = m.data2;
          }
        }
      }
    }
    displayEditMenu();
    break;
  }

  default:
    break;
  }
}

void handleEncoderButtonPress() {
  bool currentState = (digitalRead(ENCODER_BUTTON_PIN) == LOW);

  if (!currentState && encoderButtonPressed) {
    encoderButtonPressed = false;
    unsigned long pressDuration = millis() - encoderButtonPressStartTime;

    if (pressDuration > ENCODER_BUTTON_DEBOUNCE_DELAY &&
        pressDuration < LONG_PRESS_DURATION) {
      if (inTapTempoMode) {
        rhythmPattern = (rhythmPattern + 1) % 4;
        saveSystemSettings();
        float finalDelayMs =
            (60000.0 / currentBPM) * rhythmMultipliers[rhythmPattern];
        sendDelayTime(constrain((int)finalDelayMs, 0, 1000));

        // Send BPM to GP5 via CC 118 (if in GP5 sync mode and connected)
        if (presetSyncMode[currentPreset] == SYNC_GP5 && clientConnected) {
          // CC 118 value: BPM mapped to 0-127
          uint8_t bpmValue =
              constrain((int)currentBPM - 40, 0, 127); // GP5 range: 40-167 BPM
          sendMidiCC(systemConfig.midiChannel, GP5MidiCC::BPM, bpmValue);
          Serial.printf("GP5: Sent BPM %d (CC 118 = %d)\n", (int)currentBPM,
                        bpmValue);
        }

        tapModeTimeout = millis() + 3000;
        safeDisplayOLED();
      } else if (currentMode == 1) {
        if (editMenuState != EDIT_NONE) {
          handleEditMenuInput(0, true); // Enter/confirm in edit menu
        } else {
          handleMenuSelection();
        }
      } else if (currentMode == 0) {
        // If in analog debug mode, pressing encoder exits to menu
        if (systemConfig.debugAnalogIn) {
          systemConfig.debugAnalogIn = false; // Exit analog debug
          currentMode = 1;                    // Go to menu
          menuSelection = 13;                 // Highlight "Analog Debug" option
          displayMenu();
        } else {
          currentPreset = (currentPreset + 1) % 4;
          // Only reset LED states if NOT in GP5 sync mode
          // GP5 sync will provide correct states via requestPresetState()
          if (presetSyncMode[currentPreset] != SYNC_GP5) {
            for (int j = 0; j < MAX_BUTTONS; j++) {
              ledToggleState[j] = false;
            }
          }
          saveCurrentPresetIndex();
          displayOLED();
          usbMidiLedUpdatePending = true; // Allow LED update in USB MIDI mode
          updateLeds();
          if (presetSyncMode[currentPreset] != SYNC_NONE && clientConnected)
            requestPresetState();
        }
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
