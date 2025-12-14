#ifndef DEFAULT_PRESETS_H
#define DEFAULT_PRESETS_H

#include "Globals.h"

// Hardcoded factory presets based on user's exported configuration
void loadFactoryPresets() {
    // Initialize tap tempo control buttons with defaults
    // PREV=button 1 (index 0), NEXT=button 5 (index 4), LOCK=button 8 (index 7)
    for (int p = 0; p < 4; p++) {
        for (int b = 0; b < MAX_BUTTONS; b++) {
            buttonConfigs[p][b].messageA.rhythmPrevButton = 0;   // Button 1
            buttonConfigs[p][b].messageA.rhythmNextButton = 4;   // Button 5
            buttonConfigs[p][b].messageA.tapModeLockButton = 7;  // Button 8
            buttonConfigs[p][b].messageB.rhythmPrevButton = 0;
            buttonConfigs[p][b].messageB.rhythmNextButton = 4;
            buttonConfigs[p][b].messageB.tapModeLockButton = 7;
        }
    }
    
    // Preset 0: "STOMP"
    strncpy(presetNames[0], "STOMP", 20);
    presetNames[0][20] = '\0';
    
    // Button 0: NR
    ButtonConfig& p0b0 = buttonConfigs[0][0];
    strncpy(p0b0.name, "NR", 20); p0b0.name[20] = '\0';
    p0b0.isAlternate = true;
    p0b0.nextIsB = false;
    p0b0.messageA.type = CC;
    p0b0.messageA.channel = 1;
    p0b0.messageA.data1 = 43;
    p0b0.messageA.data2 = 127;
    p0b0.messageA.rgb[0] = 0xFF; p0b0.messageA.rgb[1] = 0xFF; p0b0.messageA.rgb[2] = 0xFF;
    p0b0.messageB.type = CC;
    p0b0.messageB.channel = 1;
    p0b0.messageB.data1 = 43;
    p0b0.messageB.data2 = 0;
    p0b0.messageB.rgb[0] = 0xFF; p0b0.messageB.rgb[1] = 0xFF; p0b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: FX1
    ButtonConfig& p0b1 = buttonConfigs[0][1];
    strncpy(p0b1.name, "FX1", 20); p0b1.name[20] = '\0';
    p0b1.isAlternate = true;
    p0b1.nextIsB = false;
    p0b1.messageA.type = CC;
    p0b1.messageA.channel = 1;
    p0b1.messageA.data1 = 44;
    p0b1.messageA.data2 = 127;
    p0b1.messageA.rgb[0] = 0x3F; p0b1.messageA.rgb[1] = 0x67; p0b1.messageA.rgb[2] = 0xFF;
    p0b1.messageB.type = CC;
    p0b1.messageB.channel = 1;
    p0b1.messageB.data1 = 44;
    p0b1.messageB.data2 = 0;
    p0b1.messageB.rgb[0] = 0x3F; p0b1.messageB.rgb[1] = 0x67; p0b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: DRV
    ButtonConfig& p0b2 = buttonConfigs[0][2];
    strncpy(p0b2.name, "DRV", 20); p0b2.name[20] = '\0';
    p0b2.isAlternate = true;
    p0b2.nextIsB = false;
    p0b2.messageA.type = CC;
    p0b2.messageA.channel = 1;
    p0b2.messageA.data1 = 45;
    p0b2.messageA.data2 = 127;
    p0b2.messageA.rgb[0] = 0xFC; p0b2.messageA.rgb[1] = 0x2C; p0b2.messageA.rgb[2] = 0x00;
    p0b2.messageB.type = CC;
    p0b2.messageB.channel = 1;
    p0b2.messageB.data1 = 45;
    p0b2.messageB.data2 = 0;
    p0b2.messageB.rgb[0] = 0xFF; p0b2.messageB.rgb[1] = 0x00; p0b2.messageB.rgb[2] = 0x00;
    
    // Button 3: TAP
    ButtonConfig& p0b3 = buttonConfigs[0][3];
    strncpy(p0b3.name, "TAP", 20); p0b3.name[20] = '\0';
    p0b3.isAlternate = false;
    p0b3.nextIsB = false;
    p0b3.messageA.type = TAP_TEMPO;
    p0b3.messageA.channel = 1;
    p0b3.messageA.data1 = 13;
    p0b3.messageA.data2 = 127;
    p0b3.messageA.rgb[0] = 0xFF; p0b3.messageA.rgb[1] = 0xFF; p0b3.messageA.rgb[2] = 0xFF;
    p0b3.messageB.type = CC;
    p0b3.messageB.channel = 1;
    p0b3.messageB.data1 = 13;
    p0b3.messageB.data2 = 127;
    p0b3.messageB.rgb[0] = 0xFF; p0b3.messageB.rgb[1] = 0x00; p0b3.messageB.rgb[2] = 0x00;
    
    // Button 4: EQ + COMBO (PRESET_DOWN)
    ButtonConfig& p0b4 = buttonConfigs[0][4];
    strncpy(p0b4.name, "EQ", 20); p0b4.name[20] = '\0';
    p0b4.isAlternate = true;
    p0b4.nextIsB = false;
    p0b4.messageA.type = CC;
    p0b4.messageA.channel = 1;
    p0b4.messageA.data1 = 48;
    p0b4.messageA.data2 = 127;
    p0b4.messageA.rgb[0] = 0x0A; p0b4.messageA.rgb[1] = 0xF5; p0b4.messageA.rgb[2] = 0x00;
    p0b4.messageB.type = CC;
    p0b4.messageB.channel = 1;
    p0b4.messageB.data1 = 48;
    p0b4.messageB.data2 = 0;
    p0b4.messageB.rgb[0] = 0x0A; p0b4.messageB.rgb[1] = 0xF5; p0b4.messageB.rgb[2] = 0x00;
    // Combo: Button 4 + Button 0 = PRESET_DOWN
    p0b4.combo.enabled = true;
    p0b4.combo.partner = 0;
    p0b4.combo.type = PRESET_DOWN;
    p0b4.combo.channel = 1;
    p0b4.combo.data1 = 0;
    p0b4.combo.data2 = 0;
    
    // Button 5: FX2
    ButtonConfig& p0b5 = buttonConfigs[0][5];
    strncpy(p0b5.name, "FX2", 20); p0b5.name[20] = '\0';
    p0b5.isAlternate = true;
    p0b5.nextIsB = false;
    p0b5.messageA.type = CC;
    p0b5.messageA.channel = 1;
    p0b5.messageA.data1 = 49;
    p0b5.messageA.data2 = 127;
    p0b5.messageA.rgb[0] = 0x11; p0b5.messageA.rgb[1] = 0xF3; p0b5.messageA.rgb[2] = 0xFF;
    p0b5.messageB.type = CC;
    p0b5.messageB.channel = 1;
    p0b5.messageB.data1 = 49;
    p0b5.messageB.data2 = 0;
    p0b5.messageB.rgb[0] = 0x11; p0b5.messageB.rgb[1] = 0xF3; p0b5.messageB.rgb[2] = 0xFF;

    // Button 6: DLY + COMBO with Button 7 (WIFI_TOGGLE)
    ButtonConfig& p0b6 = buttonConfigs[0][6];
    strncpy(p0b6.name, "DLY", 20); p0b6.name[20] = '\0';
    p0b6.isAlternate = true;
    p0b6.nextIsB = false;
    p0b6.messageA.type = CC;
    p0b6.messageA.channel = 1;
    p0b6.messageA.data1 = 50;
    p0b6.messageA.data2 = 127;
    p0b6.messageA.rgb[0] = 0x33; p0b6.messageA.rgb[1] = 0x2A; p0b6.messageA.rgb[2] = 0xFF;
    p0b6.messageB.type = CC;
    p0b6.messageB.channel = 1;
    p0b6.messageB.data1 = 50;
    p0b6.messageB.data2 = 0;
    p0b6.messageB.rgb[0] = 0x33; p0b6.messageB.rgb[1] = 0x2A; p0b6.messageB.rgb[2] = 0xFF;
    // Combo: Button 6 + Button 7 = WIFI_TOGGLE
    p0b6.combo.enabled = true;
    p0b6.combo.partner = 7;
    p0b6.combo.type = WIFI_TOGGLE;
    p0b6.combo.channel = 1;
    p0b6.combo.data1 = 0;
    p0b6.combo.data2 = 0;
    
    // Button 7: RVB + COMBO with Button 3 (PRESET_UP)
    ButtonConfig& p0b7 = buttonConfigs[0][7];
    strncpy(p0b7.name, "RVB", 20); p0b7.name[20] = '\0';
    p0b7.isAlternate = true;
    p0b7.nextIsB = false;
    p0b7.messageA.type = CC;
    p0b7.messageA.channel = 1;
    p0b7.messageA.data1 = 51;
    p0b7.messageA.data2 = 127;
    p0b7.messageA.rgb[0] = 0x84; p0b7.messageA.rgb[1] = 0x00; p0b7.messageA.rgb[2] = 0xF7;
    p0b7.messageB.type = CC;
    p0b7.messageB.channel = 1;
    p0b7.messageB.data1 = 51;
    p0b7.messageB.data2 = 0;
    p0b7.messageB.rgb[0] = 0x84; p0b7.messageB.rgb[1] = 0x00; p0b7.messageB.rgb[2] = 0xF7;
    // Combo: Button 7 + Button 3 = PRESET_UP
    p0b7.combo.enabled = true;
    p0b7.combo.partner = 3;
    p0b7.combo.type = PRESET_UP;
    p0b7.combo.channel = 1;
    p0b7.combo.data1 = 0;
    p0b7.combo.data2 = 0;
    // Preset 1: "BANKS 1-8"
    strncpy(presetNames[1], "BANKS 1-8", 20);
    presetNames[1][20] = '\0';
    
    // Button 0: B1
    ButtonConfig& p1b0 = buttonConfigs[1][0];
    strncpy(p1b0.name, "B1", 20); p1b0.name[20] = '\0';
    p1b0.isAlternate = false;
    p1b0.nextIsB = false;
    p1b0.messageA.type = CC;
    p1b0.messageA.channel = 1;
    p1b0.messageA.data1 = 1;
    p1b0.messageA.data2 = 1;
    p1b0.messageA.rgb[0] = 0xFF; p1b0.messageA.rgb[1] = 0xFF; p1b0.messageA.rgb[2] = 0xFF;
    p1b0.messageB.type = OFF;
    p1b0.messageB.channel = 1;
    p1b0.messageB.data1 = 0;
    p1b0.messageB.data2 = 0;
    p1b0.messageB.rgb[0] = 0xFF; p1b0.messageB.rgb[1] = 0xFF; p1b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: B2
    ButtonConfig& p1b1 = buttonConfigs[1][1];
    strncpy(p1b1.name, "B2", 20); p1b1.name[20] = '\0';
    p1b1.isAlternate = false;
    p1b1.nextIsB = false;
    p1b1.messageA.type = CC;
    p1b1.messageA.channel = 1;
    p1b1.messageA.data1 = 1;
    p1b1.messageA.data2 = 2;
    p1b1.messageA.rgb[0] = 0xFF; p1b1.messageA.rgb[1] = 0xFF; p1b1.messageA.rgb[2] = 0xFF;
    p1b1.messageB.type = OFF;
    p1b1.messageB.channel = 1;
    p1b1.messageB.data1 = 0;
    p1b1.messageB.data2 = 0;
    p1b1.messageB.rgb[0] = 0xFF; p1b1.messageB.rgb[1] = 0xFF; p1b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: B3
    ButtonConfig& p1b2 = buttonConfigs[1][2];
    strncpy(p1b2.name, "B3", 20); p1b2.name[20] = '\0';
    p1b2.isAlternate = false;
    p1b2.nextIsB = false;
    p1b2.messageA.type = CC;
    p1b2.messageA.channel = 1;
    p1b2.messageA.data1 = 1;
    p1b2.messageA.data2 = 3;
    p1b2.messageA.rgb[0] = 0xFF; p1b2.messageA.rgb[1] = 0xFF; p1b2.messageA.rgb[2] = 0xFF;
    p1b2.messageB.type = OFF;
    p1b2.messageB.channel = 1;
    p1b2.messageB.data1 = 0;
    p1b2.messageB.data2 = 0;
    p1b2.messageB.rgb[0] = 0xFF; p1b2.messageB.rgb[1] = 0xFF; p1b2.messageB.rgb[2] = 0xFF;
    
    // Button 3: B4
    ButtonConfig& p1b3 = buttonConfigs[1][3];
    strncpy(p1b3.name, "B4", 20); p1b3.name[20] = '\0';
    p1b3.isAlternate = false;
    p1b3.nextIsB = false;
    p1b3.messageA.type = CC;
    p1b3.messageA.channel = 1;
    p1b3.messageA.data1 = 1;
    p1b3.messageA.data2 = 4;
    p1b3.messageA.rgb[0] = 0xFF; p1b3.messageA.rgb[1] = 0xFF; p1b3.messageA.rgb[2] = 0xFF;
    p1b3.messageB.type = OFF;
    p1b3.messageB.channel = 1;
    p1b3.messageB.data1 = 0;
    p1b3.messageB.data2 = 0;
    p1b3.messageB.rgb[0] = 0xFF; p1b3.messageB.rgb[1] = 0xFF; p1b3.messageB.rgb[2] = 0xFF;
    
    // Button 4: B5 + COMBO (PRESET_DOWN)
    ButtonConfig& p1b4 = buttonConfigs[1][4];
    strncpy(p1b4.name, "B5", 20); p1b4.name[20] = '\0';
    p1b4.isAlternate = false;
    p1b4.nextIsB = false;
    p1b4.messageA.type = CC;
    p1b4.messageA.channel = 1;
    p1b4.messageA.data1 = 1;
    p1b4.messageA.data2 = 5;
    p1b4.messageA.rgb[0] = 0x0A; p1b4.messageA.rgb[1] = 0xF5; p1b4.messageA.rgb[2] = 0x00;
    p1b4.messageB.type = OFF;
    p1b4.messageB.channel = 1;
    p1b4.messageB.data1 = 0;
    p1b4.messageB.data2 = 0;
    p1b4.messageB.rgb[0] = 0x0A; p1b4.messageB.rgb[1] = 0xF5; p1b4.messageB.rgb[2] = 0x00;
    p1b4.combo.enabled = true;
    p1b4.combo.partner = 0;
    p1b4.combo.type = PRESET_DOWN;
    p1b4.combo.channel = 1;
    p1b4.combo.data1 = 0;
    p1b4.combo.data2 = 0;
    
    // Button 5: B6
    ButtonConfig& p1b5 = buttonConfigs[1][5];
    strncpy(p1b5.name, "B6", 20); p1b5.name[20] = '\0';
    p1b5.isAlternate = false;
    p1b5.nextIsB = false;
    p1b5.messageA.type = CC;
    p1b5.messageA.channel = 1;
    p1b5.messageA.data1 = 1;
    p1b5.messageA.data2 = 6;
    p1b5.messageA.rgb[0] = 0x0A; p1b5.messageA.rgb[1] = 0xF5; p1b5.messageA.rgb[2] = 0x00;
    p1b5.messageB.type = OFF;
    p1b5.messageB.channel = 1;
    p1b5.messageB.data1 = 0;
    p1b5.messageB.data2 = 0;
    p1b5.messageB.rgb[0] = 0x0A; p1b5.messageB.rgb[1] = 0xF5; p1b5.messageB.rgb[2] = 0x00;
    
    // Button 6: B7 + COMBO (WIFI_TOGGLE)
    ButtonConfig& p1b6 = buttonConfigs[1][6];
    strncpy(p1b6.name, "B7", 20); p1b6.name[20] = '\0';
    p1b6.isAlternate = false;
    p1b6.nextIsB = false;
    p1b6.messageA.type = CC;
    p1b6.messageA.channel = 1;
    p1b6.messageA.data1 = 1;
    p1b6.messageA.data2 = 7;
    p1b6.messageA.rgb[0] = 0x0A; p1b6.messageA.rgb[1] = 0xF5; p1b6.messageA.rgb[2] = 0x00;
    p1b6.messageB.type = OFF;
    p1b6.messageB.channel = 1;
    p1b6.messageB.data1 = 0;
    p1b6.messageB.data2 = 0;
    p1b6.messageB.rgb[0] = 0x0A; p1b6.messageB.rgb[1] = 0xF5; p1b6.messageB.rgb[2] = 0x00;
    p1b6.combo.enabled = true;
    p1b6.combo.partner = 7;
    p1b6.combo.type = WIFI_TOGGLE;
    p1b6.combo.channel = 1;
    p1b6.combo.data1 = 0;
    p1b6.combo.data2 = 0;
    
    // Button 7: B8 + COMBO (PRESET_UP)
    ButtonConfig& p1b7 = buttonConfigs[1][7];
    strncpy(p1b7.name, "B8", 20); p1b7.name[20] = '\0';
    p1b7.isAlternate = false;
    p1b7.nextIsB = false;
    p1b7.messageA.type = CC;
    p1b7.messageA.channel = 1;
    p1b7.messageA.data1 = 1;
    p1b7.messageA.data2 = 8;
    p1b7.messageA.rgb[0] = 0x0A; p1b7.messageA.rgb[1] = 0xF5; p1b7.messageA.rgb[2] = 0x00;
    p1b7.messageB.type = OFF;
    p1b7.messageB.channel = 1;
    p1b7.messageB.data1 = 0;
    p1b7.messageB.data2 = 0;
    p1b7.messageB.rgb[0] = 0x0A; p1b7.messageB.rgb[1] = 0xF5; p1b7.messageB.rgb[2] = 0x00;
    p1b7.combo.enabled = true;
    p1b7.combo.partner = 3;
    p1b7.combo.type = PRESET_UP;
    p1b7.combo.channel = 1;
    p1b7.combo.data1 = 0;
    p1b7.combo.data2 = 0;
    
    // Preset 2: "BANKS 9-16"
    strncpy(presetNames[2], "BANKS 9-16", 20);
    presetNames[2][20] = '\0';
    
    // Button 0: B9
    ButtonConfig& p2b0 = buttonConfigs[2][0];
    strncpy(p2b0.name, "B9", 20); p2b0.name[20] = '\0';
    p2b0.isAlternate = false;
    p2b0.nextIsB = false;
    p2b0.messageA.type = CC;
    p2b0.messageA.channel = 1;
    p2b0.messageA.data1 = 1;
    p2b0.messageA.data2 = 9;
    p2b0.messageA.rgb[0] = 0x11; p2b0.messageA.rgb[1] = 0xF3; p2b0.messageA.rgb[2] = 0xFF;
    p2b0.messageB.type = OFF;
    p2b0.messageB.channel = 1;
    p2b0.messageB.data1 = 0;
    p2b0.messageB.data2 = 0;
    p2b0.messageB.rgb[0] = 0x11; p2b0.messageB.rgb[1] = 0xF3; p2b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: B10
    ButtonConfig& p2b1 = buttonConfigs[2][1];
    strncpy(p2b1.name, "B10", 20); p2b1.name[20] = '\0';
    p2b1.isAlternate = false;
    p2b1.nextIsB = false;
    p2b1.messageA.type = CC;
    p2b1.messageA.channel = 1;
    p2b1.messageA.data1 = 1;
    p2b1.messageA.data2 = 10;
    p2b1.messageA.rgb[0] = 0x11; p2b1.messageA.rgb[1] = 0xF3; p2b1.messageA.rgb[2] = 0xFF;
    p2b1.messageB.type = OFF;
    p2b1.messageB.channel = 1;
    p2b1.messageB.data1 = 0;
    p2b1.messageB.data2 = 0;
    p2b1.messageB.rgb[0] = 0x11; p2b1.messageB.rgb[1] = 0xF3; p2b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: B11
    ButtonConfig& p2b2 = buttonConfigs[2][2];
    strncpy(p2b2.name, "B11", 20); p2b2.name[20] = '\0';
    p2b2.isAlternate = false;
    p2b2.nextIsB = false;
    p2b2.messageA.type = CC;
    p2b2.messageA.channel = 1;
    p2b2.messageA.data1 = 1;
    p2b2.messageA.data2 = 11;
    p2b2.messageA.rgb[0] = 0x11; p2b2.messageA.rgb[1] = 0xF3; p2b2.messageA.rgb[2] = 0xFF;
    p2b2.messageB.type = OFF;
    p2b2.messageB.channel = 1;
    p2b2.messageB.data1 = 0;
    p2b2.messageB.data2 = 0;
    p2b2.messageB.rgb[0] = 0x11; p2b2.messageB.rgb[1] = 0xF3; p2b2.messageB.rgb[2] = 0xFF;
    
    // Button 3: B12
    ButtonConfig& p2b3 = buttonConfigs[2][3];
    strncpy(p2b3.name, "B12", 20); p2b3.name[20] = '\0';
    p2b3.isAlternate = false;
    p2b3.nextIsB = false;
    p2b3.messageA.type = CC;
    p2b3.messageA.channel = 1;
    p2b3.messageA.data1 = 1;
    p2b3.messageA.data2 = 12;
    p2b3.messageA.rgb[0] = 0x11; p2b3.messageA.rgb[1] = 0xF3; p2b3.messageA.rgb[2] = 0xFF;
    p2b3.messageB.type = OFF;
    p2b3.messageB.channel = 1;
    p2b3.messageB.data1 = 0;
    p2b3.messageB.data2 = 0;
    p2b3.messageB.rgb[0] = 0x11; p2b3.messageB.rgb[1] = 0xF3; p2b3.messageB.rgb[2] = 0xFF;
    
    // Button 4: B13 + COMBO (PRESET_DOWN)
    ButtonConfig& p2b4 = buttonConfigs[2][4];
    strncpy(p2b4.name, "B13", 20); p2b4.name[20] = '\0';
    p2b4.isAlternate = false;
    p2b4.nextIsB = false;
    p2b4.messageA.type = CC;
    p2b4.messageA.channel = 1;
    p2b4.messageA.data1 = 1;
    p2b4.messageA.data2 = 13;
    p2b4.messageA.rgb[0] = 0xAA; p2b4.messageA.rgb[1] = 0x00; p2b4.messageA.rgb[2] = 0xFF;
    p2b4.messageB.type = OFF;
    p2b4.messageB.channel = 1;
    p2b4.messageB.data1 = 0;
    p2b4.messageB.data2 = 0;
    p2b4.messageB.rgb[0] = 0xAA; p2b4.messageB.rgb[1] = 0x00; p2b4.messageB.rgb[2] = 0xFF;
    p2b4.combo.enabled = true;
    p2b4.combo.partner = 0;
    p2b4.combo.type = PRESET_DOWN;
    p2b4.combo.channel = 1;
    p2b4.combo.data1 = 0;
    p2b4.combo.data2 = 0;
    
    // Button 5: B14
    ButtonConfig& p2b5 = buttonConfigs[2][5];
    strncpy(p2b5.name, "B14", 20); p2b5.name[20] = '\0';
    p2b5.isAlternate = false;
    p2b5.nextIsB = false;
    p2b5.messageA.type = CC;
    p2b5.messageA.channel = 1;
    p2b5.messageA.data1 = 1;
    p2b5.messageA.data2 = 14;
    p2b5.messageA.rgb[0] = 0xAA; p2b5.messageA.rgb[1] = 0x00; p2b5.messageA.rgb[2] = 0xFF;
    p2b5.messageB.type = OFF;
    p2b5.messageB.channel = 1;
    p2b5.messageB.data1 = 0;
    p2b5.messageB.data2 = 0;
    p2b5.messageB.rgb[0] = 0xAA; p2b5.messageB.rgb[1] = 0x00; p2b5.messageB.rgb[2] = 0xFF;
    
    // Button 6: B15 + COMBO (WIFI_TOGGLE)
    ButtonConfig& p2b6 = buttonConfigs[2][6];
    strncpy(p2b6.name, "B15", 20); p2b6.name[20] = '\0';
    p2b6.isAlternate = false;
    p2b6.nextIsB = false;
    p2b6.messageA.type = CC;
    p2b6.messageA.channel = 1;
    p2b6.messageA.data1 = 1;
    p2b6.messageA.data2 = 15;
    p2b6.messageA.rgb[0] = 0xAA; p2b6.messageA.rgb[1] = 0x00; p2b6.messageA.rgb[2] = 0xFF;
    p2b6.messageB.type = OFF;
    p2b6.messageB.channel = 1;
    p2b6.messageB.data1 = 0;
    p2b6.messageB.data2 = 0;
    p2b6.messageB.rgb[0] = 0xAA; p2b6.messageB.rgb[1] = 0x00; p2b6.messageB.rgb[2] = 0xFF;
    p2b6.combo.enabled = true;
    p2b6.combo.partner = 7;
    p2b6.combo.type = WIFI_TOGGLE;
    p2b6.combo.channel = 1;
    p2b6.combo.data1 = 0;
    p2b6.combo.data2 = 0;
    
    // Button 7: B16 + COMBO (PRESET_UP)
    ButtonConfig& p2b7 = buttonConfigs[2][7];
    strncpy(p2b7.name, "B16", 20); p2b7.name[20] = '\0';
    p2b7.isAlternate = false;
    p2b7.nextIsB = false;
    p2b7.messageA.type = CC;
    p2b7.messageA.channel = 1;
    p2b7.messageA.data1 = 1;
    p2b7.messageA.data2 = 16;
    p2b7.messageA.rgb[0] = 0xAA; p2b7.messageA.rgb[1] = 0x00; p2b7.messageA.rgb[2] = 0xFF;
    p2b7.messageB.type = OFF;
    p2b7.messageB.channel = 1;
    p2b7.messageB.data1 = 0;
    p2b7.messageB.data2 = 0;
    p2b7.messageB.rgb[0] = 0xAA; p2b7.messageB.rgb[1] = 0x00; p2b7.messageB.rgb[2] = 0xFF;
    p2b7.combo.enabled = true;
    p2b7.combo.partner = 3;
    p2b7.combo.type = PRESET_UP;
    p2b7.combo.channel = 1;
    p2b7.combo.data1 = 0;
    p2b7.combo.data2 = 0;
    
    // Preset 3: "Note"
    strncpy(presetNames[3], "Note", 20);
    presetNames[3][20] = '\0';
    
    // Button 0: 1st (CC ch1, d1=1, d2=40)
    ButtonConfig& p3b0 = buttonConfigs[3][0];
    strncpy(p3b0.name, "1st", 20); p3b0.name[20] = '\0';
    p3b0.isAlternate = false;
    p3b0.nextIsB = false;
    p3b0.messageA.type = CC;
    p3b0.messageA.channel = 1;
    p3b0.messageA.data1 = 1;
    p3b0.messageA.data2 = 40;
    p3b0.messageA.rgb[0] = 0xFD; p3b0.messageA.rgb[1] = 0x00; p3b0.messageA.rgb[2] = 0x00;
    p3b0.messageB.type = OFF;
    p3b0.messageB.channel = 1;
    p3b0.messageB.data1 = 0;
    p3b0.messageB.data2 = 0;
    p3b0.messageB.rgb[0] = 0xFD; p3b0.messageB.rgb[1] = 0x00; p3b0.messageB.rgb[2] = 0x00;
    
    // Button 1: 2nd (CC ch1, d1=1, d2=41)
    ButtonConfig& p3b1 = buttonConfigs[3][1];
    strncpy(p3b1.name, "2nd", 20); p3b1.name[20] = '\0';
    p3b1.isAlternate = false;
    p3b1.nextIsB = false;
    p3b1.messageA.type = CC;
    p3b1.messageA.channel = 1;
    p3b1.messageA.data1 = 1;
    p3b1.messageA.data2 = 41;
    p3b1.messageA.rgb[0] = 0xFD; p3b1.messageA.rgb[1] = 0x00; p3b1.messageA.rgb[2] = 0x00;
    p3b1.messageB.type = OFF;
    p3b1.messageB.channel = 1;
    p3b1.messageB.data1 = 0;
    p3b1.messageB.data2 = 0;
    p3b1.messageB.rgb[0] = 0xFD; p3b1.messageB.rgb[1] = 0x00; p3b1.messageB.rgb[2] = 0x00;
    
    // Button 2: 3rd (CC ch1, d1=1, d2=42)
    ButtonConfig& p3b2 = buttonConfigs[3][2];
    strncpy(p3b2.name, "3rd", 20); p3b2.name[20] = '\0';
    p3b2.isAlternate = false;
    p3b2.nextIsB = false;
    p3b2.messageA.type = CC;
    p3b2.messageA.channel = 1;
    p3b2.messageA.data1 = 1;
    p3b2.messageA.data2 = 42;
    p3b2.messageA.rgb[0] = 0xFD; p3b2.messageA.rgb[1] = 0x00; p3b2.messageA.rgb[2] = 0x00;
    p3b2.messageB.type = OFF;
    p3b2.messageB.channel = 1;
    p3b2.messageB.data1 = 0;
    p3b2.messageB.data2 = 0;
    p3b2.messageB.rgb[0] = 0xFD; p3b2.messageB.rgb[1] = 0x00; p3b2.messageB.rgb[2] = 0x00;
    
    // Button 3: 4th (CC ch1, d1=1, d2=43)
    ButtonConfig& p3b3 = buttonConfigs[3][3];
    strncpy(p3b3.name, "4th", 20); p3b3.name[20] = '\0';
    p3b3.isAlternate = false;
    p3b3.nextIsB = false;
    p3b3.messageA.type = CC;
    p3b3.messageA.channel = 1;
    p3b3.messageA.data1 = 1;
    p3b3.messageA.data2 = 43;
    p3b3.messageA.rgb[0] = 0xFD; p3b3.messageA.rgb[1] = 0x00; p3b3.messageA.rgb[2] = 0x00;
    p3b3.messageB.type = OFF;
    p3b3.messageB.channel = 1;
    p3b3.messageB.data1 = 0;
    p3b3.messageB.data2 = 0;
    p3b3.messageB.rgb[0] = 0xFD; p3b3.messageB.rgb[1] = 0x00; p3b3.messageB.rgb[2] = 0x00;
    
    // Button 4: 5th + COMBO (PRESET_DOWN)
    ButtonConfig& p3b4 = buttonConfigs[3][4];
    strncpy(p3b4.name, "5th", 20); p3b4.name[20] = '\0';
    p3b4.isAlternate = false;
    p3b4.nextIsB = false;
    p3b4.messageA.type = CC;
    p3b4.messageA.channel = 1;
    p3b4.messageA.data1 = 1;
    p3b4.messageA.data2 = 44;
    p3b4.messageA.rgb[0] = 0xFD; p3b4.messageA.rgb[1] = 0x00; p3b4.messageA.rgb[2] = 0x00;
    p3b4.messageB.type = OFF;
    p3b4.messageB.channel = 1;
    p3b4.messageB.data1 = 0;
    p3b4.messageB.data2 = 0;
    p3b4.messageB.rgb[0] = 0xFD; p3b4.messageB.rgb[1] = 0x00; p3b4.messageB.rgb[2] = 0x00;
    p3b4.combo.enabled = true;
    p3b4.combo.partner = 0;
    p3b4.combo.type = PRESET_DOWN;
    p3b4.combo.channel = 1;
    p3b4.combo.data1 = 0;
    p3b4.combo.data2 = 0;
    
    // Button 5: 6th
    ButtonConfig& p3b5 = buttonConfigs[3][5];
    strncpy(p3b5.name, "6th", 20); p3b5.name[20] = '\0';
    p3b5.isAlternate = false;
    p3b5.nextIsB = false;
    p3b5.messageA.type = CC;
    p3b5.messageA.channel = 1;
    p3b5.messageA.data1 = 1;
    p3b5.messageA.data2 = 45;
    p3b5.messageA.rgb[0] = 0xFD; p3b5.messageA.rgb[1] = 0x00; p3b5.messageA.rgb[2] = 0x00;
    p3b5.messageB.type = OFF;
    p3b5.messageB.channel = 1;
    p3b5.messageB.data1 = 0;
    p3b5.messageB.data2 = 0;
    p3b5.messageB.rgb[0] = 0xFD; p3b5.messageB.rgb[1] = 0x00; p3b5.messageB.rgb[2] = 0x00;
    
    // Button 6: 7th + COMBO (WIFI_TOGGLE)
    ButtonConfig& p3b6 = buttonConfigs[3][6];
    strncpy(p3b6.name, "7th", 20); p3b6.name[20] = '\0';
    p3b6.isAlternate = false;
    p3b6.nextIsB = false;
    p3b6.messageA.type = CC;
    p3b6.messageA.channel = 1;
    p3b6.messageA.data1 = 1;
    p3b6.messageA.data2 = 46;
    p3b6.messageA.rgb[0] = 0xFD; p3b6.messageA.rgb[1] = 0x00; p3b6.messageA.rgb[2] = 0x00;
    p3b6.messageB.type = OFF;
    p3b6.messageB.channel = 1;
    p3b6.messageB.data1 = 0;
    p3b6.messageB.data2 = 0;
    p3b6.messageB.rgb[0] = 0xFD; p3b6.messageB.rgb[1] = 0x00; p3b6.messageB.rgb[2] = 0x00;
    p3b6.combo.enabled = true;
    p3b6.combo.partner = 7;
    p3b6.combo.type = WIFI_TOGGLE;
    p3b6.combo.channel = 1;
    p3b6.combo.data1 = 0;
    p3b6.combo.data2 = 0;
    
    // Button 7: 8up + COMBO (PRESET_UP)
    ButtonConfig& p3b7 = buttonConfigs[3][7];
    strncpy(p3b7.name, "8up", 20); p3b7.name[20] = '\0';
    p3b7.isAlternate = false;
    p3b7.nextIsB = false;
    p3b7.messageA.type = CC;
    p3b7.messageA.channel = 1;
    p3b7.messageA.data1 = 1;
    p3b7.messageA.data2 = 47;
    p3b7.messageA.rgb[0] = 0xFD; p3b7.messageA.rgb[1] = 0x00; p3b7.messageA.rgb[2] = 0x00;
    p3b7.messageB.type = OFF;
    p3b7.messageB.channel = 1;
    p3b7.messageB.data1 = 0;
    p3b7.messageB.data2 = 0;
    p3b7.messageB.rgb[0] = 0xFD; p3b7.messageB.rgb[1] = 0x00; p3b7.messageB.rgb[2] = 0x00;
    p3b7.combo.enabled = true;
    p3b7.combo.partner = 3;
    p3b7.combo.type = PRESET_UP;
    p3b7.combo.channel = 1;
    p3b7.combo.data1 = 0;
    p3b7.combo.data2 = 0;
    
    // ========================================
    // GLOBAL SPECIAL ACTIONS (Independent of preset)
    // ========================================
    
    // Button 4 (User Button 5) + Button 0 (User Button 1) = PRESET_DOWN
    globalSpecialActions[4].combo.enabled = true;
    globalSpecialActions[4].combo.partner = 0;
    globalSpecialActions[4].combo.type = PRESET_DOWN;
    globalSpecialActions[4].combo.channel = 1;
    globalSpecialActions[4].combo.data1 = 0;
    globalSpecialActions[4].combo.data2 = 0;
    
    // Button 5 (User Button 6) + Button 6 (User Button 7) = WIFI_TOGGLE
    globalSpecialActions[5].combo.enabled = true;
    globalSpecialActions[5].combo.partner = 6;
    globalSpecialActions[5].combo.type = WIFI_TOGGLE;
    globalSpecialActions[5].combo.channel = 1;
    globalSpecialActions[5].combo.data1 = 0;
    globalSpecialActions[5].combo.data2 = 0;
    
    // Button 7 (User Button 8) + Button 3 (User Button 4) = PRESET_UP
    globalSpecialActions[7].combo.enabled = true;
    globalSpecialActions[7].combo.partner = 3;
    globalSpecialActions[7].combo.type = PRESET_UP;
    globalSpecialActions[7].combo.channel = 1;
    globalSpecialActions[7].combo.data1 = 0;
    globalSpecialActions[7].combo.data2 = 0;
    
    // Note: TAP_TEMPO control buttons (rhythmPrevButton, rhythmNextButton, tapModeLockButton)
    // are already initialized with defaults at the start of loadFactoryPresets()
}

#endif
