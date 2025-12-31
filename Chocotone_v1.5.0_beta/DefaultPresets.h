#ifndef DEFAULT_PRESETS_H
#define DEFAULT_PRESETS_H

#include "Globals.h"

// ============================================
// v3.0 ACTION-BASED FACTORY PRESETS
// ============================================
//
// Each button uses messages[] array with ActionType:
//   ACTION_PRESS     - Primary press action
//   ACTION_2ND_PRESS - Alternate press (toggle)
//   ACTION_COMBO     - When pressed with partner
//   ACTION_LONG_PRESS, ACTION_RELEASE, ACTION_DOUBLE_TAP
//
// ============================================

// Helper: Set a CC toggle button (PRESS sends 127, 2ND_PRESS sends 0)
void setToggleCC(ButtonConfig& btn, const char* name, uint8_t cc, uint8_t r, uint8_t g, uint8_t b) {
    strncpy(btn.name, name, 20); btn.name[20] = '\0';
    btn.ledMode = LED_TOGGLE;
    btn.inSelectionGroup = false;
    btn.messageCount = 2;
    btn.isAlternate = false;
    
    // PRESS: CC on
    btn.messages[0].action = ACTION_PRESS;
    btn.messages[0].type = CC;
    btn.messages[0].channel = 1;
    btn.messages[0].data1 = cc;
    btn.messages[0].data2 = 127;
    btn.messages[0].rgb[0] = r; btn.messages[0].rgb[1] = g; btn.messages[0].rgb[2] = b;
    
    // 2ND_PRESS: CC off
    btn.messages[1].action = ACTION_2ND_PRESS;
    btn.messages[1].type = CC;
    btn.messages[1].channel = 1;
    btn.messages[1].data1 = cc;
    btn.messages[1].data2 = 0;
    btn.messages[1].rgb[0] = r; btn.messages[1].rgb[1] = g; btn.messages[1].rgb[2] = b;
}

// Helper: Set a program select button (PRESS sends CC with specific value)
void setProgramSelect(ButtonConfig& btn, const char* name, uint8_t value, uint8_t r, uint8_t g, uint8_t b) {
    strncpy(btn.name, name, 20); btn.name[20] = '\0';
    btn.ledMode = LED_MOMENTARY;
    btn.inSelectionGroup = false;
    btn.messageCount = 1;
    btn.isAlternate = false;
    
    btn.messages[0].action = ACTION_PRESS;
    btn.messages[0].type = CC;
    btn.messages[0].channel = 1;
    btn.messages[0].data1 = 1;  // CC#1 for program select
    btn.messages[0].data2 = value;
    btn.messages[0].rgb[0] = r; btn.messages[0].rgb[1] = g; btn.messages[0].rgb[2] = b;
}

// Helper: Add COMBO action to existing button
void addCombo(ButtonConfig& btn, int8_t partner, MidiCommandType type) {
    if (btn.messageCount < MAX_ACTIONS_PER_BUTTON) {
        ActionMessage& msg = btn.messages[btn.messageCount++];
        msg.action = ACTION_COMBO;
        msg.type = type;
        msg.channel = 1;
        msg.data1 = 0;
        msg.data2 = 0;
        msg.combo.partner = partner;
        msg.label[0] = '\0';
    }
}

// Helper: Add LONG_PRESS action to existing button
void addLongPress(ButtonConfig& btn, MidiCommandType type, uint16_t holdMs) {
    if (btn.messageCount < MAX_ACTIONS_PER_BUTTON) {
        ActionMessage& msg = btn.messages[btn.messageCount++];
        msg.action = ACTION_LONG_PRESS;
        msg.type = type;
        msg.channel = 1;
        msg.data1 = 0;
        msg.data2 = 0;
        msg.longPress.holdMs = holdMs;
        msg.label[0] = '\0';
    }
}

void loadFactoryPresets() {
    // ========================================
    // INITIALIZE ALL BUTTONS
    // ========================================
    for (int p = 0; p < 4; p++) {
        for (int b = 0; b < MAX_BUTTONS; b++) {
            ButtonConfig& btn = buttonConfigs[p][b];
            memset(&btn, 0, sizeof(ButtonConfig));
            btn.name[0] = '\0';
            btn.ledMode = LED_MOMENTARY;
            btn.inSelectionGroup = false;
            btn.messageCount = 0;
            btn.isAlternate = false;
        }
    }
    
    // ========================================
    // PRESET 0: "STOMP" - Effect Toggles
    // ========================================
    strncpy(presetNames[0], "STOMP", 20);
    presetNames[0][20] = '\0';
    presetLedModes[0] = PRESET_LED_NORMAL;
    presetSyncMode[0] = SYNC_SPM;  // Enable SPM effect state sync for STOMP preset
    
    // NR (CC#43) - White
    setToggleCC(buttonConfigs[0][0], "NR", 43, 0xFF, 0xFF, 0xFF);
    
    // FX1 (CC#44) - Blue
    setToggleCC(buttonConfigs[0][1], "FX1", 44, 0x3F, 0x67, 0xFF);
    
    // DRV (CC#45) - Orange/Red
    setToggleCC(buttonConfigs[0][2], "DRV", 45, 0xFC, 0x2C, 0x00);
    
    // TAP TEMPO (special)
    {
        ButtonConfig& btn = buttonConfigs[0][3];
        strncpy(btn.name, "TAP", 20); btn.name[20] = '\0';
        btn.ledMode = LED_MOMENTARY;
        btn.messageCount = 1;
        
        btn.messages[0].action = ACTION_PRESS;
        btn.messages[0].type = TAP_TEMPO;
        btn.messages[0].channel = 1;
        btn.messages[0].data1 = 13;
        btn.messages[0].data2 = 127;
        btn.messages[0].rgb[0] = 0xFF; btn.messages[0].rgb[1] = 0xFF; btn.messages[0].rgb[2] = 0xFF;
        btn.messages[0].tapTempo.rhythmPrev = 0;
        btn.messages[0].tapTempo.rhythmNext = 4;
        btn.messages[0].tapTempo.tapLock = 7;
    }
    
    // EQ (CC#48) - Green + LONG_PRESS: PRESET_DOWN
    setToggleCC(buttonConfigs[0][4], "EQ", 48, 0x0A, 0xF5, 0x00);
    addLongPress(buttonConfigs[0][4], PRESET_DOWN, 700);
    
    // FX2 (CC#49) - Cyan
    setToggleCC(buttonConfigs[0][5], "FX2", 49, 0x11, 0xF3, 0xFF);
    
    // DLY (CC#50) - Blue
    setToggleCC(buttonConfigs[0][6], "DLY", 50, 0x33, 0x2A, 0xFF);
    
    // RVB (CC#51) - Purple + LONG_PRESS: PRESET_UP
    setToggleCC(buttonConfigs[0][7], "RVB", 51, 0x84, 0x00, 0xF7);
    addLongPress(buttonConfigs[0][7], PRESET_UP, 700);
    
    // ========================================
    // PRESET 1: "BANKS 1-8" - Program Selection
    // ========================================
    strncpy(presetNames[1], "BANKS 1-8", 20);
    presetNames[1][20] = '\0';
    presetLedModes[1] = PRESET_LED_SELECTION;
    
    setProgramSelect(buttonConfigs[1][0], "B1", 1, 0xFF, 0xFF, 0xFF);
    setProgramSelect(buttonConfigs[1][1], "B2", 2, 0xFF, 0xFF, 0xFF);
    setProgramSelect(buttonConfigs[1][2], "B3", 3, 0xFF, 0xFF, 0xFF);
    setProgramSelect(buttonConfigs[1][3], "B4", 4, 0xFF, 0xFF, 0xFF);
    setProgramSelect(buttonConfigs[1][4], "B5", 5, 0x0A, 0xF5, 0x00);
    addLongPress(buttonConfigs[1][4], PRESET_DOWN, 700);
    setProgramSelect(buttonConfigs[1][5], "B6", 6, 0x0A, 0xF5, 0x00);
    setProgramSelect(buttonConfigs[1][6], "B7", 7, 0x0A, 0xF5, 0x00);
    setProgramSelect(buttonConfigs[1][7], "B8", 8, 0x0A, 0xF5, 0x00);
    addLongPress(buttonConfigs[1][7], PRESET_UP, 700);
    
    // ========================================
    // PRESET 2: "BANKS 9-16" - Program Selection
    // ========================================
    strncpy(presetNames[2], "BANKS 9-16", 20);
    presetNames[2][20] = '\0';
    presetLedModes[2] = PRESET_LED_SELECTION;
    
    setProgramSelect(buttonConfigs[2][0], "B9", 9, 0x11, 0xF3, 0xFF);
    setProgramSelect(buttonConfigs[2][1], "B10", 10, 0x11, 0xF3, 0xFF);
    setProgramSelect(buttonConfigs[2][2], "B11", 11, 0x11, 0xF3, 0xFF);
    setProgramSelect(buttonConfigs[2][3], "B12", 12, 0x11, 0xF3, 0xFF);
    setProgramSelect(buttonConfigs[2][4], "B13", 13, 0xAA, 0x00, 0xFF);
    addLongPress(buttonConfigs[2][4], PRESET_DOWN, 700);
    setProgramSelect(buttonConfigs[2][5], "B14", 14, 0xAA, 0x00, 0xFF);
    setProgramSelect(buttonConfigs[2][6], "B15", 15, 0xAA, 0x00, 0xFF);
    setProgramSelect(buttonConfigs[2][7], "B16", 16, 0xAA, 0x00, 0xFF);
    addLongPress(buttonConfigs[2][7], PRESET_UP, 700);
    
    // ========================================
    // PRESET 3: "Note" - CC values for song selection
    // ========================================
    strncpy(presetNames[3], "Note", 20);
    presetNames[3][20] = '\0';
    presetLedModes[3] = PRESET_LED_SELECTION;
    
    // Note preset uses CC#1 with values 40-47 for song selection
    const uint8_t noteValues[] = {40, 41, 42, 43, 44, 45, 46, 47};
    const char* noteNames[] = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8up"};
    
    for (int i = 0; i < 8; i++) {
        setProgramSelect(buttonConfigs[3][i], noteNames[i], noteValues[i], 0xFD, 0x00, 0x00);
    }
    
    // Add LONG_PRESS for preset navigation
    addLongPress(buttonConfigs[3][4], PRESET_DOWN, 700);
    addLongPress(buttonConfigs[3][7], PRESET_UP, 700);
    
    // ========================================
    // GLOBAL SPECIAL ACTIONS
    // ========================================
    // These are checked in all presets
    
    // Button 4 + Button 0 = PRESET_DOWN
    globalSpecialActions[4].hasCombo = true;
    globalSpecialActions[4].comboAction.action = ACTION_COMBO;
    globalSpecialActions[4].comboAction.type = PRESET_DOWN;
    globalSpecialActions[4].comboAction.combo.partner = 0;
    
    // Button 5 + Button 6 = WIFI_TOGGLE
    globalSpecialActions[5].hasCombo = true;
    globalSpecialActions[5].comboAction.action = ACTION_COMBO;
    globalSpecialActions[5].comboAction.type = WIFI_TOGGLE;
    globalSpecialActions[5].comboAction.combo.partner = 6;
    
    // Button 7 + Button 3 = PRESET_UP
    globalSpecialActions[7].hasCombo = true;
    globalSpecialActions[7].comboAction.action = ACTION_COMBO;
    globalSpecialActions[7].comboAction.type = PRESET_UP;
    globalSpecialActions[7].comboAction.combo.partner = 3;
}

#endif
