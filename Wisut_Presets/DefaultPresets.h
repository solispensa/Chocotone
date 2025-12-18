#ifndef DEFAULT_PRESETS_H
#define DEFAULT_PRESETS_H

#include "Globals.h"

// ============================================
// WISUT'S CUSTOM FACTORY PRESETS
// 10-Button Controller Configuration
// ============================================
// 
// HOW TO USE:
// 1. Replace your existing DefaultPresets.h with this file
// 2. Recompile and upload to your ESP32
// 
// Button Layout (2 rows x 5 columns):
//   [5] [6] [7] [8] [9]     <- Top row
//   [0] [1] [2] [3] [4]     <- Bottom row
//
// Global Combos:
//   Button 5 (index 4) + Button 1 (index 0) = PRESET_DOWN
//   Button 6 (index 5) + Button 7 (index 6) = WIFI_TOGGLE
//   Button 8 (index 7) + Button 4 (index 3) = PRESET_UP
//
// ============================================

void loadFactoryPresets() {
    // ========================================
    // INITIALIZE ALL BUTTONS WITH SAFE DEFAULTS
    // ========================================
    for (int p = 0; p < 4; p++) {
        for (int b = 0; b < MAX_BUTTONS; b++) {
            // Initialize tap tempo control buttons
            buttonConfigs[p][b].messageA.rhythmPrevButton = 0;   // Button 1
            buttonConfigs[p][b].messageA.rhythmNextButton = 4;   // Button 5
            buttonConfigs[p][b].messageA.tapModeLockButton = 7;  // Button 8
            buttonConfigs[p][b].messageB.rhythmPrevButton = 0;
            buttonConfigs[p][b].messageB.rhythmNextButton = 4;
            buttonConfigs[p][b].messageB.tapModeLockButton = 7;
            
            // Initialize combo as disabled (prevents garbage values)
            buttonConfigs[p][b].combo.enabled = false;
            buttonConfigs[p][b].combo.partner = -1;
            
            // Initialize hold as disabled
            buttonConfigs[p][b].hold.enabled = false;
            
            // Default button state
            buttonConfigs[p][b].isAlternate = false;
            buttonConfigs[p][b].nextIsB = false;
        }
    }
    
    // ========================================
    // PRESET 0: "BANK 1" - Program Selection (P1-P5) + Effects
    // ========================================
    strncpy(presetNames[0], "BANK 1", 20);
    presetNames[0][20] = '\0';
    
    // Button 0: P1 (CC#1 val=1)
    ButtonConfig& p0b0 = buttonConfigs[0][0];
    strncpy(p0b0.name, "P1", 20); p0b0.name[20] = '\0';
    p0b0.isAlternate = false;
    p0b0.messageA.type = CC;
    p0b0.messageA.channel = 1;
    p0b0.messageA.data1 = 1;
    p0b0.messageA.data2 = 1;
    p0b0.messageA.rgb[0] = 0xFF; p0b0.messageA.rgb[1] = 0xFF; p0b0.messageA.rgb[2] = 0xFF;
    p0b0.messageB.type = OFF;
    p0b0.messageB.rgb[0] = 0xFF; p0b0.messageB.rgb[1] = 0xFF; p0b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: P2 (CC#1 val=2)
    ButtonConfig& p0b1 = buttonConfigs[0][1];
    strncpy(p0b1.name, "P2", 20); p0b1.name[20] = '\0';
    p0b1.isAlternate = false;
    p0b1.messageA.type = CC;
    p0b1.messageA.channel = 1;
    p0b1.messageA.data1 = 1;
    p0b1.messageA.data2 = 2;
    p0b1.messageA.rgb[0] = 0xFF; p0b1.messageA.rgb[1] = 0xFF; p0b1.messageA.rgb[2] = 0xFF;
    p0b1.messageB.type = OFF;
    p0b1.messageB.rgb[0] = 0xFF; p0b1.messageB.rgb[1] = 0xFF; p0b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: P3 (CC#1 val=3)
    ButtonConfig& p0b2 = buttonConfigs[0][2];
    strncpy(p0b2.name, "P3", 20); p0b2.name[20] = '\0';
    p0b2.isAlternate = false;
    p0b2.messageA.type = CC;
    p0b2.messageA.channel = 1;
    p0b2.messageA.data1 = 1;
    p0b2.messageA.data2 = 3;
    p0b2.messageA.rgb[0] = 0xFF; p0b2.messageA.rgb[1] = 0xFF; p0b2.messageA.rgb[2] = 0xFF;
    p0b2.messageB.type = OFF;
    p0b2.messageB.rgb[0] = 0xFF; p0b2.messageB.rgb[1] = 0xFF; p0b2.messageB.rgb[2] = 0xFF;
    
    // Button 3: P4 (CC#1 val=4)
    ButtonConfig& p0b3 = buttonConfigs[0][3];
    strncpy(p0b3.name, "P4", 20); p0b3.name[20] = '\0';
    p0b3.isAlternate = false;
    p0b3.messageA.type = CC;
    p0b3.messageA.channel = 1;
    p0b3.messageA.data1 = 1;
    p0b3.messageA.data2 = 4;
    p0b3.messageA.rgb[0] = 0xFF; p0b3.messageA.rgb[1] = 0xFF; p0b3.messageA.rgb[2] = 0xFF;
    p0b3.messageB.type = OFF;
    p0b3.messageB.rgb[0] = 0xFF; p0b3.messageB.rgb[1] = 0xFF; p0b3.messageB.rgb[2] = 0xFF;
    
    // Button 4: P5 (CC#1 val=5) + COMBO with Button 0 = PRESET_DOWN
    ButtonConfig& p0b4 = buttonConfigs[0][4];
    strncpy(p0b4.name, "P5", 20); p0b4.name[20] = '\0';
    p0b4.isAlternate = false;
    p0b4.messageA.type = CC;
    p0b4.messageA.channel = 1;
    p0b4.messageA.data1 = 1;
    p0b4.messageA.data2 = 5;
    p0b4.messageA.rgb[0] = 0x0A; p0b4.messageA.rgb[1] = 0xF5; p0b4.messageA.rgb[2] = 0x00; // Green
    p0b4.messageB.type = OFF;
    p0b4.messageB.rgb[0] = 0x0A; p0b4.messageB.rgb[1] = 0xF5; p0b4.messageB.rgb[2] = 0x00;
    p0b4.combo.enabled = true;
    p0b4.combo.partner = 0;
    p0b4.combo.type = PRESET_DOWN;
    p0b4.combo.channel = 1;
    
    // Button 5: FX1 Toggle (CC#44)
    ButtonConfig& p0b5 = buttonConfigs[0][5];
    strncpy(p0b5.name, "FX1", 20); p0b5.name[20] = '\0';
    p0b5.isAlternate = true;  // Toggle mode
    p0b5.messageA.type = CC;
    p0b5.messageA.channel = 1;
    p0b5.messageA.data1 = 44;
    p0b5.messageA.data2 = 127;
    p0b5.messageA.rgb[0] = 0xFF; p0b5.messageA.rgb[1] = 0xFF; p0b5.messageA.rgb[2] = 0xFF; // White=ON
    p0b5.messageB.type = CC;
    p0b5.messageB.channel = 1;
    p0b5.messageB.data1 = 44;
    p0b5.messageB.data2 = 0;
    p0b5.messageB.rgb[0] = 0x3F; p0b5.messageB.rgb[1] = 0x67; p0b5.messageB.rgb[2] = 0xFF; // Blue=OFF
    // COMBO with Button 6 = WIFI_TOGGLE
    p0b5.combo.enabled = true;
    p0b5.combo.partner = 6;
    p0b5.combo.type = WIFI_TOGGLE;
    p0b5.combo.channel = 1;
    
    // Button 6: DRV Toggle (CC#45)
    ButtonConfig& p0b6 = buttonConfigs[0][6];
    strncpy(p0b6.name, "DRV", 20); p0b6.name[20] = '\0';
    p0b6.isAlternate = true;
    p0b6.messageA.type = CC;
    p0b6.messageA.channel = 1;
    p0b6.messageA.data1 = 45;
    p0b6.messageA.data2 = 127;
    p0b6.messageA.rgb[0] = 0xFF; p0b6.messageA.rgb[1] = 0xFF; p0b6.messageA.rgb[2] = 0xFF; // White=ON
    p0b6.messageB.type = CC;
    p0b6.messageB.channel = 1;
    p0b6.messageB.data1 = 45;
    p0b6.messageB.data2 = 0;
    p0b6.messageB.rgb[0] = 0xFF; p0b6.messageB.rgb[1] = 0x00; p0b6.messageB.rgb[2] = 0x00; // Red=OFF
    
    // Button 7: FX2 Toggle (CC#49) + COMBO with Button 3 = PRESET_UP
    ButtonConfig& p0b7 = buttonConfigs[0][7];
    strncpy(p0b7.name, "FX2", 20); p0b7.name[20] = '\0';
    p0b7.isAlternate = true;
    p0b7.messageA.type = CC;
    p0b7.messageA.channel = 1;
    p0b7.messageA.data1 = 49;
    p0b7.messageA.data2 = 127;
    p0b7.messageA.rgb[0] = 0xFF; p0b7.messageA.rgb[1] = 0xFF; p0b7.messageA.rgb[2] = 0xFF;
    p0b7.messageB.type = CC;
    p0b7.messageB.channel = 1;
    p0b7.messageB.data1 = 49;
    p0b7.messageB.data2 = 0;
    p0b7.messageB.rgb[0] = 0x11; p0b7.messageB.rgb[1] = 0xF3; p0b7.messageB.rgb[2] = 0xFF; // Cyan=OFF
    p0b7.combo.enabled = true;
    p0b7.combo.partner = 3;
    p0b7.combo.type = PRESET_UP;
    p0b7.combo.channel = 1;
    
    // Button 8: DLY Toggle (CC#50)
    ButtonConfig& p0b8 = buttonConfigs[0][8];
    strncpy(p0b8.name, "DLY", 20); p0b8.name[20] = '\0';
    p0b8.isAlternate = true;
    p0b8.messageA.type = CC;
    p0b8.messageA.channel = 1;
    p0b8.messageA.data1 = 50;
    p0b8.messageA.data2 = 127;
    p0b8.messageA.rgb[0] = 0xFF; p0b8.messageA.rgb[1] = 0xFF; p0b8.messageA.rgb[2] = 0xFF;
    p0b8.messageB.type = CC;
    p0b8.messageB.channel = 1;
    p0b8.messageB.data1 = 50;
    p0b8.messageB.data2 = 0;
    p0b8.messageB.rgb[0] = 0x33; p0b8.messageB.rgb[1] = 0x2A; p0b8.messageB.rgb[2] = 0xFF; // Blue=OFF
    
    // Button 9: TAP TEMPO (CC#13)
    ButtonConfig& p0b9 = buttonConfigs[0][9];
    strncpy(p0b9.name, "TAP", 20); p0b9.name[20] = '\0';
    p0b9.isAlternate = false;
    p0b9.messageA.type = TAP_TEMPO;
    p0b9.messageA.channel = 1;
    p0b9.messageA.data1 = 13;
    p0b9.messageA.data2 = 127;
    p0b9.messageA.rgb[0] = 0xFF; p0b9.messageA.rgb[1] = 0xFF; p0b9.messageA.rgb[2] = 0xFF;
    p0b9.messageB.type = CC;
    p0b9.messageB.channel = 1;
    p0b9.messageB.data1 = 13;
    p0b9.messageB.data2 = 127;
    p0b9.messageB.rgb[0] = 0xFF; p0b9.messageB.rgb[1] = 0x00; p0b9.messageB.rgb[2] = 0x00;
    
    // ========================================
    // PRESET 1: "BANK 2" - Program Selection (P6-P10) + Effects
    // ========================================
    strncpy(presetNames[1], "BANK 2", 20);
    presetNames[1][20] = '\0';
    
    // Button 0: P6 (CC#1 val=6)
    ButtonConfig& p1b0 = buttonConfigs[1][0];
    strncpy(p1b0.name, "P6", 20); p1b0.name[20] = '\0';
    p1b0.isAlternate = false;
    p1b0.messageA.type = CC;
    p1b0.messageA.channel = 1;
    p1b0.messageA.data1 = 1;
    p1b0.messageA.data2 = 6;
    p1b0.messageA.rgb[0] = 0xFF; p1b0.messageA.rgb[1] = 0xFF; p1b0.messageA.rgb[2] = 0xFF;
    p1b0.messageB.type = OFF;
    p1b0.messageB.rgb[0] = 0xFF; p1b0.messageB.rgb[1] = 0xFF; p1b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: P7 (CC#1 val=7)
    ButtonConfig& p1b1 = buttonConfigs[1][1];
    strncpy(p1b1.name, "P7", 20); p1b1.name[20] = '\0';
    p1b1.isAlternate = false;
    p1b1.messageA.type = CC;
    p1b1.messageA.channel = 1;
    p1b1.messageA.data1 = 1;
    p1b1.messageA.data2 = 7;
    p1b1.messageA.rgb[0] = 0xFF; p1b1.messageA.rgb[1] = 0xFF; p1b1.messageA.rgb[2] = 0xFF;
    p1b1.messageB.type = OFF;
    p1b1.messageB.rgb[0] = 0xFF; p1b1.messageB.rgb[1] = 0xFF; p1b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: P8 (CC#1 val=8)
    ButtonConfig& p1b2 = buttonConfigs[1][2];
    strncpy(p1b2.name, "P8", 20); p1b2.name[20] = '\0';
    p1b2.isAlternate = false;
    p1b2.messageA.type = CC;
    p1b2.messageA.channel = 1;
    p1b2.messageA.data1 = 1;
    p1b2.messageA.data2 = 8;
    p1b2.messageA.rgb[0] = 0xFF; p1b2.messageA.rgb[1] = 0xFF; p1b2.messageA.rgb[2] = 0xFF;
    p1b2.messageB.type = OFF;
    p1b2.messageB.rgb[0] = 0xFF; p1b2.messageB.rgb[1] = 0xFF; p1b2.messageB.rgb[2] = 0xFF;
    
    // Button 3: P9 (CC#1 val=9)
    ButtonConfig& p1b3 = buttonConfigs[1][3];
    strncpy(p1b3.name, "P9", 20); p1b3.name[20] = '\0';
    p1b3.isAlternate = false;
    p1b3.messageA.type = CC;
    p1b3.messageA.channel = 1;
    p1b3.messageA.data1 = 1;
    p1b3.messageA.data2 = 9;
    p1b3.messageA.rgb[0] = 0xFF; p1b3.messageA.rgb[1] = 0xFF; p1b3.messageA.rgb[2] = 0xFF;
    p1b3.messageB.type = OFF;
    p1b3.messageB.rgb[0] = 0xFF; p1b3.messageB.rgb[1] = 0xFF; p1b3.messageB.rgb[2] = 0xFF;
    
    // Button 4: P10 (CC#1 val=10) + COMBO = PRESET_DOWN
    ButtonConfig& p1b4 = buttonConfigs[1][4];
    strncpy(p1b4.name, "P10", 20); p1b4.name[20] = '\0';
    p1b4.isAlternate = false;
    p1b4.messageA.type = CC;
    p1b4.messageA.channel = 1;
    p1b4.messageA.data1 = 1;
    p1b4.messageA.data2 = 10;
    p1b4.messageA.rgb[0] = 0x0A; p1b4.messageA.rgb[1] = 0xF5; p1b4.messageA.rgb[2] = 0x00;
    p1b4.messageB.type = OFF;
    p1b4.messageB.rgb[0] = 0x0A; p1b4.messageB.rgb[1] = 0xF5; p1b4.messageB.rgb[2] = 0x00;
    p1b4.combo.enabled = true;
    p1b4.combo.partner = 0;
    p1b4.combo.type = PRESET_DOWN;
    p1b4.combo.channel = 1;
    
    // Button 5: FX1 Toggle (CC#44)
    ButtonConfig& p1b5 = buttonConfigs[1][5];
    strncpy(p1b5.name, "FX1", 20); p1b5.name[20] = '\0';
    p1b5.isAlternate = true;
    p1b5.messageA.type = CC;
    p1b5.messageA.channel = 1;
    p1b5.messageA.data1 = 44;
    p1b5.messageA.data2 = 127;
    p1b5.messageA.rgb[0] = 0xFF; p1b5.messageA.rgb[1] = 0xFF; p1b5.messageA.rgb[2] = 0xFF;
    p1b5.messageB.type = CC;
    p1b5.messageB.channel = 1;
    p1b5.messageB.data1 = 44;
    p1b5.messageB.data2 = 0;
    p1b5.messageB.rgb[0] = 0x3F; p1b5.messageB.rgb[1] = 0x67; p1b5.messageB.rgb[2] = 0xFF;
    p1b5.combo.enabled = true;
    p1b5.combo.partner = 6;
    p1b5.combo.type = WIFI_TOGGLE;
    p1b5.combo.channel = 1;
    
    // Button 6: DRV Toggle (CC#45)
    ButtonConfig& p1b6 = buttonConfigs[1][6];
    strncpy(p1b6.name, "DRV", 20); p1b6.name[20] = '\0';
    p1b6.isAlternate = true;
    p1b6.messageA.type = CC;
    p1b6.messageA.channel = 1;
    p1b6.messageA.data1 = 45;
    p1b6.messageA.data2 = 127;
    p1b6.messageA.rgb[0] = 0xFF; p1b6.messageA.rgb[1] = 0xFF; p1b6.messageA.rgb[2] = 0xFF;
    p1b6.messageB.type = CC;
    p1b6.messageB.channel = 1;
    p1b6.messageB.data1 = 45;
    p1b6.messageB.data2 = 0;
    p1b6.messageB.rgb[0] = 0xFF; p1b6.messageB.rgb[1] = 0x00; p1b6.messageB.rgb[2] = 0x00;
    
    // Button 7: FX2 Toggle (CC#49) + COMBO = PRESET_UP
    ButtonConfig& p1b7 = buttonConfigs[1][7];
    strncpy(p1b7.name, "FX2", 20); p1b7.name[20] = '\0';
    p1b7.isAlternate = true;
    p1b7.messageA.type = CC;
    p1b7.messageA.channel = 1;
    p1b7.messageA.data1 = 49;
    p1b7.messageA.data2 = 127;
    p1b7.messageA.rgb[0] = 0xFF; p1b7.messageA.rgb[1] = 0xFF; p1b7.messageA.rgb[2] = 0xFF;
    p1b7.messageB.type = CC;
    p1b7.messageB.channel = 1;
    p1b7.messageB.data1 = 49;
    p1b7.messageB.data2 = 0;
    p1b7.messageB.rgb[0] = 0x11; p1b7.messageB.rgb[1] = 0xF3; p1b7.messageB.rgb[2] = 0xFF;
    p1b7.combo.enabled = true;
    p1b7.combo.partner = 3;
    p1b7.combo.type = PRESET_UP;
    p1b7.combo.channel = 1;
    
    // Button 8: DLY Toggle (CC#50)
    ButtonConfig& p1b8 = buttonConfigs[1][8];
    strncpy(p1b8.name, "DLY", 20); p1b8.name[20] = '\0';
    p1b8.isAlternate = true;
    p1b8.messageA.type = CC;
    p1b8.messageA.channel = 1;
    p1b8.messageA.data1 = 50;
    p1b8.messageA.data2 = 127;
    p1b8.messageA.rgb[0] = 0xFF; p1b8.messageA.rgb[1] = 0xFF; p1b8.messageA.rgb[2] = 0xFF;
    p1b8.messageB.type = CC;
    p1b8.messageB.channel = 1;
    p1b8.messageB.data1 = 50;
    p1b8.messageB.data2 = 0;
    p1b8.messageB.rgb[0] = 0x33; p1b8.messageB.rgb[1] = 0x2A; p1b8.messageB.rgb[2] = 0xFF;
    
    // Button 9: TAP TEMPO
    ButtonConfig& p1b9 = buttonConfigs[1][9];
    strncpy(p1b9.name, "TAP", 20); p1b9.name[20] = '\0';
    p1b9.isAlternate = false;
    p1b9.messageA.type = TAP_TEMPO;
    p1b9.messageA.channel = 1;
    p1b9.messageA.data1 = 13;
    p1b9.messageA.data2 = 127;
    p1b9.messageA.rgb[0] = 0xFF; p1b9.messageA.rgb[1] = 0xFF; p1b9.messageA.rgb[2] = 0xFF;
    p1b9.messageB.type = CC;
    p1b9.messageB.channel = 1;
    p1b9.messageB.data1 = 13;
    p1b9.messageB.data2 = 127;
    p1b9.messageB.rgb[0] = 0xFF; p1b9.messageB.rgb[1] = 0x00; p1b9.messageB.rgb[2] = 0x00;
    
    // ========================================
    // PRESET 2: "GP5" - Guitar Processor Mode
    // ========================================
    strncpy(presetNames[2], "GP5", 20);
    presetNames[2][20] = '\0';
    
    // Button 0: T1 (CC#0 val=1)
    ButtonConfig& p2b0 = buttonConfigs[2][0];
    strncpy(p2b0.name, "T1", 20); p2b0.name[20] = '\0';
    p2b0.isAlternate = false;
    p2b0.messageA.type = CC;
    p2b0.messageA.channel = 1;
    p2b0.messageA.data1 = 0;
    p2b0.messageA.data2 = 1;
    p2b0.messageA.rgb[0] = 0xFF; p2b0.messageA.rgb[1] = 0xFF; p2b0.messageA.rgb[2] = 0xFF;
    p2b0.messageB.type = OFF;
    p2b0.messageB.rgb[0] = 0xFF; p2b0.messageB.rgb[1] = 0xFF; p2b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: T2 (CC#0 val=2)
    ButtonConfig& p2b1 = buttonConfigs[2][1];
    strncpy(p2b1.name, "T2", 20); p2b1.name[20] = '\0';
    p2b1.isAlternate = false;
    p2b1.messageA.type = CC;
    p2b1.messageA.channel = 1;
    p2b1.messageA.data1 = 0;
    p2b1.messageA.data2 = 2;
    p2b1.messageA.rgb[0] = 0xFF; p2b1.messageA.rgb[1] = 0xFF; p2b1.messageA.rgb[2] = 0xFF;
    p2b1.messageB.type = OFF;
    p2b1.messageB.rgb[0] = 0xFF; p2b1.messageB.rgb[1] = 0xFF; p2b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: T3 (CC#0 val=3)
    ButtonConfig& p2b2 = buttonConfigs[2][2];
    strncpy(p2b2.name, "T3", 20); p2b2.name[20] = '\0';
    p2b2.isAlternate = false;
    p2b2.messageA.type = CC;
    p2b2.messageA.channel = 1;
    p2b2.messageA.data1 = 0;
    p2b2.messageA.data2 = 3;
    p2b2.messageA.rgb[0] = 0xFF; p2b2.messageA.rgb[1] = 0xFF; p2b2.messageA.rgb[2] = 0xFF;
    p2b2.messageB.type = OFF;
    p2b2.messageB.rgb[0] = 0xFF; p2b2.messageB.rgb[1] = 0xFF; p2b2.messageB.rgb[2] = 0xFF;
    
    // Button 3: T4 (CC#0 val=4)
    ButtonConfig& p2b3 = buttonConfigs[2][3];
    strncpy(p2b3.name, "T4", 20); p2b3.name[20] = '\0';
    p2b3.isAlternate = false;
    p2b3.messageA.type = CC;
    p2b3.messageA.channel = 1;
    p2b3.messageA.data1 = 0;
    p2b3.messageA.data2 = 4;
    p2b3.messageA.rgb[0] = 0xFF; p2b3.messageA.rgb[1] = 0xFF; p2b3.messageA.rgb[2] = 0xFF;
    p2b3.messageB.type = OFF;
    p2b3.messageB.rgb[0] = 0xFF; p2b3.messageB.rgb[1] = 0xFF; p2b3.messageB.rgb[2] = 0xFF;
    
    // Button 4: T5 (CC#0 val=5) + COMBO = PRESET_DOWN
    ButtonConfig& p2b4 = buttonConfigs[2][4];
    strncpy(p2b4.name, "T5", 20); p2b4.name[20] = '\0';
    p2b4.isAlternate = false;
    p2b4.messageA.type = CC;
    p2b4.messageA.channel = 1;
    p2b4.messageA.data1 = 0;
    p2b4.messageA.data2 = 5;
    p2b4.messageA.rgb[0] = 0x0A; p2b4.messageA.rgb[1] = 0xF5; p2b4.messageA.rgb[2] = 0x00;
    p2b4.messageB.type = OFF;
    p2b4.messageB.rgb[0] = 0x0A; p2b4.messageB.rgb[1] = 0xF5; p2b4.messageB.rgb[2] = 0x00;
    p2b4.combo.enabled = true;
    p2b4.combo.partner = 0;
    p2b4.combo.type = PRESET_DOWN;
    p2b4.combo.channel = 1;
    
    // Button 5: PRE Toggle (CC#49)
    ButtonConfig& p2b5 = buttonConfigs[2][5];
    strncpy(p2b5.name, "PRE", 20); p2b5.name[20] = '\0';
    p2b5.isAlternate = true;
    p2b5.messageA.type = CC;
    p2b5.messageA.channel = 1;
    p2b5.messageA.data1 = 49;
    p2b5.messageA.data2 = 127;
    p2b5.messageA.rgb[0] = 0xFF; p2b5.messageA.rgb[1] = 0xFF; p2b5.messageA.rgb[2] = 0xFF;
    p2b5.messageB.type = CC;
    p2b5.messageB.channel = 1;
    p2b5.messageB.data1 = 49;
    p2b5.messageB.data2 = 0;
    p2b5.messageB.rgb[0] = 0x3F; p2b5.messageB.rgb[1] = 0x67; p2b5.messageB.rgb[2] = 0xFF;
    p2b5.combo.enabled = true;
    p2b5.combo.partner = 6;
    p2b5.combo.type = WIFI_TOGGLE;
    p2b5.combo.channel = 1;
    
    // Button 6: DST Toggle (CC#50)
    ButtonConfig& p2b6 = buttonConfigs[2][6];
    strncpy(p2b6.name, "DST", 20); p2b6.name[20] = '\0';
    p2b6.isAlternate = true;
    p2b6.messageA.type = CC;
    p2b6.messageA.channel = 1;
    p2b6.messageA.data1 = 50;
    p2b6.messageA.data2 = 127;
    p2b6.messageA.rgb[0] = 0xFC; p2b6.messageA.rgb[1] = 0x2C; p2b6.messageA.rgb[2] = 0x00; // Orange=ON
    p2b6.messageB.type = CC;
    p2b6.messageB.channel = 1;
    p2b6.messageB.data1 = 50;
    p2b6.messageB.data2 = 0;
    p2b6.messageB.rgb[0] = 0xFF; p2b6.messageB.rgb[1] = 0x00; p2b6.messageB.rgb[2] = 0x00; // Red=OFF
    
    // Button 7: MOD Toggle (CC#55) + COMBO = PRESET_UP
    ButtonConfig& p2b7 = buttonConfigs[2][7];
    strncpy(p2b7.name, "MOD", 20); p2b7.name[20] = '\0';
    p2b7.isAlternate = true;
    p2b7.messageA.type = CC;
    p2b7.messageA.channel = 1;
    p2b7.messageA.data1 = 55;
    p2b7.messageA.data2 = 127;
    p2b7.messageA.rgb[0] = 0xFF; p2b7.messageA.rgb[1] = 0xFF; p2b7.messageA.rgb[2] = 0xFF;
    p2b7.messageB.type = CC;
    p2b7.messageB.channel = 1;
    p2b7.messageB.data1 = 55;
    p2b7.messageB.data2 = 0;
    p2b7.messageB.rgb[0] = 0x11; p2b7.messageB.rgb[1] = 0xF3; p2b7.messageB.rgb[2] = 0xFF;
    p2b7.combo.enabled = true;
    p2b7.combo.partner = 3;
    p2b7.combo.type = PRESET_UP;
    p2b7.combo.channel = 1;
    
    // Button 8: DLY Toggle (CC#56)
    ButtonConfig& p2b8 = buttonConfigs[2][8];
    strncpy(p2b8.name, "DLY", 20); p2b8.name[20] = '\0';
    p2b8.isAlternate = true;
    p2b8.messageA.type = CC;
    p2b8.messageA.channel = 1;
    p2b8.messageA.data1 = 56;
    p2b8.messageA.data2 = 127;
    p2b8.messageA.rgb[0] = 0xFF; p2b8.messageA.rgb[1] = 0xFF; p2b8.messageA.rgb[2] = 0xFF;
    p2b8.messageB.type = CC;
    p2b8.messageB.channel = 1;
    p2b8.messageB.data1 = 56;
    p2b8.messageB.data2 = 0;
    p2b8.messageB.rgb[0] = 0x33; p2b8.messageB.rgb[1] = 0x2A; p2b8.messageB.rgb[2] = 0xFF;
    
    // Button 9: TAP TEMPO
    ButtonConfig& p2b9 = buttonConfigs[2][9];
    strncpy(p2b9.name, "TAP", 20); p2b9.name[20] = '\0';
    p2b9.isAlternate = false;
    p2b9.messageA.type = TAP_TEMPO;
    p2b9.messageA.channel = 1;
    p2b9.messageA.data1 = 13;
    p2b9.messageA.data2 = 127;
    p2b9.messageA.rgb[0] = 0xFF; p2b9.messageA.rgb[1] = 0xFF; p2b9.messageA.rgb[2] = 0xFF;
    p2b9.messageB.type = CC;
    p2b9.messageB.channel = 1;
    p2b9.messageB.data1 = 13;
    p2b9.messageB.data2 = 127;
    p2b9.messageB.rgb[0] = 0xFF; p2b9.messageB.rgb[1] = 0x00; p2b9.messageB.rgb[2] = 0x00;
    
    // ========================================
    // PRESET 3: "Note" - Musical Intervals
    // ========================================
    strncpy(presetNames[3], "Note", 20);
    presetNames[3][20] = '\0';
    
    // Button 0: 1st (CC#1 val=40)
    ButtonConfig& p3b0 = buttonConfigs[3][0];
    strncpy(p3b0.name, "1st", 20); p3b0.name[20] = '\0';
    p3b0.isAlternate = false;
    p3b0.messageA.type = CC;
    p3b0.messageA.channel = 1;
    p3b0.messageA.data1 = 1;
    p3b0.messageA.data2 = 40;
    p3b0.messageA.rgb[0] = 0xFD; p3b0.messageA.rgb[1] = 0x00; p3b0.messageA.rgb[2] = 0x00;
    p3b0.messageB.type = OFF;
    p3b0.messageB.rgb[0] = 0xFD; p3b0.messageB.rgb[1] = 0x00; p3b0.messageB.rgb[2] = 0x00;
    
    // Button 1: 2nd (CC#1 val=41)
    ButtonConfig& p3b1 = buttonConfigs[3][1];
    strncpy(p3b1.name, "2nd", 20); p3b1.name[20] = '\0';
    p3b1.isAlternate = false;
    p3b1.messageA.type = CC;
    p3b1.messageA.channel = 1;
    p3b1.messageA.data1 = 1;
    p3b1.messageA.data2 = 41;
    p3b1.messageA.rgb[0] = 0xFD; p3b1.messageA.rgb[1] = 0x00; p3b1.messageA.rgb[2] = 0x00;
    p3b1.messageB.type = OFF;
    p3b1.messageB.rgb[0] = 0xFD; p3b1.messageB.rgb[1] = 0x00; p3b1.messageB.rgb[2] = 0x00;
    
    // Button 2: 3rd (CC#1 val=42)
    ButtonConfig& p3b2 = buttonConfigs[3][2];
    strncpy(p3b2.name, "3rd", 20); p3b2.name[20] = '\0';
    p3b2.isAlternate = false;
    p3b2.messageA.type = CC;
    p3b2.messageA.channel = 1;
    p3b2.messageA.data1 = 1;
    p3b2.messageA.data2 = 42;
    p3b2.messageA.rgb[0] = 0xFD; p3b2.messageA.rgb[1] = 0x00; p3b2.messageA.rgb[2] = 0x00;
    p3b2.messageB.type = OFF;
    p3b2.messageB.rgb[0] = 0xFD; p3b2.messageB.rgb[1] = 0x00; p3b2.messageB.rgb[2] = 0x00;
    
    // Button 3: 4th (CC#1 val=43)
    ButtonConfig& p3b3 = buttonConfigs[3][3];
    strncpy(p3b3.name, "4th", 20); p3b3.name[20] = '\0';
    p3b3.isAlternate = false;
    p3b3.messageA.type = CC;
    p3b3.messageA.channel = 1;
    p3b3.messageA.data1 = 1;
    p3b3.messageA.data2 = 43;
    p3b3.messageA.rgb[0] = 0xFD; p3b3.messageA.rgb[1] = 0x00; p3b3.messageA.rgb[2] = 0x00;
    p3b3.messageB.type = OFF;
    p3b3.messageB.rgb[0] = 0xFD; p3b3.messageB.rgb[1] = 0x00; p3b3.messageB.rgb[2] = 0x00;
    
    // Button 4: 5th (CC#1 val=44) + COMBO = PRESET_DOWN
    ButtonConfig& p3b4 = buttonConfigs[3][4];
    strncpy(p3b4.name, "5th", 20); p3b4.name[20] = '\0';
    p3b4.isAlternate = false;
    p3b4.messageA.type = CC;
    p3b4.messageA.channel = 1;
    p3b4.messageA.data1 = 1;
    p3b4.messageA.data2 = 44;
    p3b4.messageA.rgb[0] = 0xFD; p3b4.messageA.rgb[1] = 0x00; p3b4.messageA.rgb[2] = 0x00;
    p3b4.messageB.type = OFF;
    p3b4.messageB.rgb[0] = 0xFD; p3b4.messageB.rgb[1] = 0x00; p3b4.messageB.rgb[2] = 0x00;
    p3b4.combo.enabled = true;
    p3b4.combo.partner = 0;
    p3b4.combo.type = PRESET_DOWN;
    p3b4.combo.channel = 1;
    
    // Button 5: 6th (CC#1 val=45) + COMBO = WIFI_TOGGLE
    ButtonConfig& p3b5 = buttonConfigs[3][5];
    strncpy(p3b5.name, "6th", 20); p3b5.name[20] = '\0';
    p3b5.isAlternate = false;
    p3b5.messageA.type = CC;
    p3b5.messageA.channel = 1;
    p3b5.messageA.data1 = 1;
    p3b5.messageA.data2 = 45;
    p3b5.messageA.rgb[0] = 0xFD; p3b5.messageA.rgb[1] = 0x00; p3b5.messageA.rgb[2] = 0x00;
    p3b5.messageB.type = OFF;
    p3b5.messageB.rgb[0] = 0xFD; p3b5.messageB.rgb[1] = 0x00; p3b5.messageB.rgb[2] = 0x00;
    p3b5.combo.enabled = true;
    p3b5.combo.partner = 6;
    p3b5.combo.type = WIFI_TOGGLE;
    p3b5.combo.channel = 1;
    
    // Button 6: 7th (CC#1 val=46)
    ButtonConfig& p3b6 = buttonConfigs[3][6];
    strncpy(p3b6.name, "7th", 20); p3b6.name[20] = '\0';
    p3b6.isAlternate = false;
    p3b6.messageA.type = CC;
    p3b6.messageA.channel = 1;
    p3b6.messageA.data1 = 1;
    p3b6.messageA.data2 = 46;
    p3b6.messageA.rgb[0] = 0xFD; p3b6.messageA.rgb[1] = 0x00; p3b6.messageA.rgb[2] = 0x00;
    p3b6.messageB.type = OFF;
    p3b6.messageB.rgb[0] = 0xFD; p3b6.messageB.rgb[1] = 0x00; p3b6.messageB.rgb[2] = 0x00;
    
    // Button 7: 8up (CC#1 val=47) + COMBO = PRESET_UP
    ButtonConfig& p3b7 = buttonConfigs[3][7];
    strncpy(p3b7.name, "8up", 20); p3b7.name[20] = '\0';
    p3b7.isAlternate = false;
    p3b7.messageA.type = CC;
    p3b7.messageA.channel = 1;
    p3b7.messageA.data1 = 1;
    p3b7.messageA.data2 = 47;
    p3b7.messageA.rgb[0] = 0xFD; p3b7.messageA.rgb[1] = 0x00; p3b7.messageA.rgb[2] = 0x00;
    p3b7.messageB.type = OFF;
    p3b7.messageB.rgb[0] = 0xFD; p3b7.messageB.rgb[1] = 0x00; p3b7.messageB.rgb[2] = 0x00;
    p3b7.combo.enabled = true;
    p3b7.combo.partner = 3;
    p3b7.combo.type = PRESET_UP;
    p3b7.combo.channel = 1;
    
    // Button 8: 9th (CC#1 val=48)
    ButtonConfig& p3b8 = buttonConfigs[3][8];
    strncpy(p3b8.name, "9th", 20); p3b8.name[20] = '\0';
    p3b8.isAlternate = false;
    p3b8.messageA.type = CC;
    p3b8.messageA.channel = 1;
    p3b8.messageA.data1 = 1;
    p3b8.messageA.data2 = 48;
    p3b8.messageA.rgb[0] = 0xFD; p3b8.messageA.rgb[1] = 0x00; p3b8.messageA.rgb[2] = 0x00;
    p3b8.messageB.type = OFF;
    p3b8.messageB.rgb[0] = 0xFD; p3b8.messageB.rgb[1] = 0x00; p3b8.messageB.rgb[2] = 0x00;
    
    // Button 9: OCT (Octave, CC#1 val=52)
    ButtonConfig& p3b9 = buttonConfigs[3][9];
    strncpy(p3b9.name, "OCT", 20); p3b9.name[20] = '\0';
    p3b9.isAlternate = false;
    p3b9.messageA.type = CC;
    p3b9.messageA.channel = 1;
    p3b9.messageA.data1 = 1;
    p3b9.messageA.data2 = 52;
    p3b9.messageA.rgb[0] = 0xFF; p3b9.messageA.rgb[1] = 0xFF; p3b9.messageA.rgb[2] = 0x00; // Yellow
    p3b9.messageB.type = OFF;
    p3b9.messageB.rgb[0] = 0xFF; p3b9.messageB.rgb[1] = 0xFF; p3b9.messageB.rgb[2] = 0x00;
    
    // ========================================
    // GLOBAL SPECIAL ACTIONS (Work in ALL presets)
    // ========================================
    
    // Button 4 + Button 0 = PRESET_DOWN
    globalSpecialActions[4].combo.enabled = true;
    globalSpecialActions[4].combo.partner = 0;
    globalSpecialActions[4].combo.type = PRESET_DOWN;
    globalSpecialActions[4].combo.channel = 1;
    globalSpecialActions[4].combo.data1 = 0;
    globalSpecialActions[4].combo.data2 = 0;
    
    // Button 5 + Button 6 = WIFI_TOGGLE
    globalSpecialActions[5].combo.enabled = true;
    globalSpecialActions[5].combo.partner = 6;
    globalSpecialActions[5].combo.type = WIFI_TOGGLE;
    globalSpecialActions[5].combo.channel = 1;
    globalSpecialActions[5].combo.data1 = 0;
    globalSpecialActions[5].combo.data2 = 0;
    
    // Button 7 + Button 3 = PRESET_UP
    globalSpecialActions[7].combo.enabled = true;
    globalSpecialActions[7].combo.partner = 3;
    globalSpecialActions[7].combo.type = PRESET_UP;
    globalSpecialActions[7].combo.channel = 1;
    globalSpecialActions[7].combo.data1 = 0;
    globalSpecialActions[7].combo.data2 = 0;
}

#endif
