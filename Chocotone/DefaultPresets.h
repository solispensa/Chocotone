#ifndef DEFAULT_PRESETS_H
#define DEFAULT_PRESETS_H

#include "Globals.h"

// Hardcoded factory presets based on user's exported configuration
void loadFactoryPresets() {
    // Preset 0: "Note"
    strncpy(presetNames[0], "Note", 20);
    presetNames[0][20] = '\0';
    
    // Button 0: C2
    ButtonConfig& p0b0 = buttonConfigs[0][0];
    strncpy(p0b0.name, "C2", 20); p0b0.name[20] = '\0';
    p0b0.isAlternate = false;
    p0b0.nextIsB = false;
    p0b0.messageA.type = NOTE_MOMENTARY;
    p0b0.messageA.channel = 1;
    p0b0.messageA.data1 = 36;
    p0b0.messageA.data2 = 127;
    p0b0.messageA.rgb[0] = 0xFD; p0b0.messageA.rgb[1] = 0x00; p0b0.messageA.rgb[2] = 0x00;
    p0b0.messageB.type = CC;
    p0b0.messageB.channel = 1;
    p0b0.messageB.data1 = 10;
    p0b0.messageB.data2 = 127;
    p0b0.messageB.rgb[0] = 0xFF; p0b0.messageB.rgb[1] = 0x00; p0b0.messageB.rgb[2] = 0x00;
    
    // Button 1: C#2
    ButtonConfig& p0b1 = buttonConfigs[0][1];
    strncpy(p0b1.name, "C#2", 20); p0b1.name[20] = '\0';
    p0b1.isAlternate = false;
    p0b1.nextIsB = false;
    p0b1.messageA.type = NOTE_MOMENTARY;
    p0b1.messageA.channel = 1;
    p0b1.messageA.data1 = 37;
    p0b1.messageA.data2 = 127;
    p0b1.messageA.rgb[0] = 0xFD; p0b1.messageA.rgb[1] = 0x00; p0b1.messageA.rgb[2] = 0x00;
    p0b1.messageB.type = CC;
    p0b1.messageB.channel = 1;
    p0b1.messageB.data1 = 11;
    p0b1.messageB.data2 = 127;
    p0b1.messageB.rgb[0] = 0xFF; p0b1.messageB.rgb[1] = 0x00; p0b1.messageB.rgb[2] = 0x00;
    
    // Button 2: D2
    ButtonConfig& p0b2 = buttonConfigs[0][2];
    strncpy(p0b2.name, "D2", 20); p0b2.name[20] = '\0';
    p0b2.isAlternate = false;
    p0b2.nextIsB = false;
    p0b2.messageA.type = NOTE_MOMENTARY;
    p0b2.messageA.channel = 1;
    p0b2.messageA.data1 = 38;
    p0b2.messageA.data2 = 127;
    p0b2.messageA.rgb[0] = 0xFD; p0b2.messageA.rgb[1] = 0x00; p0b2.messageA.rgb[2] = 0x00;
    p0b2.messageB.type = CC;
    p0b2.messageB.channel = 1;
    p0b2.messageB.data1 = 12;
    p0b2.messageB.data2 = 127;
    p0b2.messageB.rgb[0] = 0xFF; p0b2.messageB.rgb[1] = 0x00; p0b2.messageB.rgb[2] = 0x00;
    
    // Button 3: D#2
    ButtonConfig& p0b3 = buttonConfigs[0][3];
    strncpy(p0b3.name, "D#2", 20); p0b3.name[20] = '\0';
    p0b3.isAlternate = false;
    p0b3.nextIsB = false;
    p0b3.messageA.type = NOTE_MOMENTARY;
    p0b3.messageA.channel = 1;
    p0b3.messageA.data1 = 39;
    p0b3.messageA.data2 = 127;
    p0b3.messageA.rgb[0] = 0xFD; p0b3.messageA.rgb[1] = 0x00; p0b3.messageA.rgb[2] = 0x00;
    p0b3.messageB.type = CC;
    p0b3.messageB.channel = 1;
    p0b3.messageB.data1 = 13;
    p0b3.messageB.data2 = 127;
    p0b3.messageB.rgb[0] = 0xFF; p0b3.messageB.rgb[1] = 0x00; p0b3.messageB.rgb[2] = 0x00;
    
    // Button 4: E2
    ButtonConfig& p0b4 = buttonConfigs[0][4];
    strncpy(p0b4.name, "E2", 20); p0b4.name[20] = '\0';
    p0b4.isAlternate = false;
    p0b4.nextIsB = false;
    p0b4.messageA.type = NOTE_MOMENTARY;
    p0b4.messageA.channel = 1;
    p0b4.messageA.data1 = 40;
    p0b4.messageA.data2 = 127;
    p0b4.messageA.rgb[0] = 0xFD; p0b4.messageA.rgb[1] = 0x00; p0b4.messageA.rgb[2] = 0x00;
    p0b4.messageB.type = CC;
    p0b4.messageB.channel = 1;
    p0b4.messageB.data1 = 14;
    p0b4.messageB.data2 = 127;
    p0b4.messageB.rgb[0] = 0xFF; p0b4.messageB.rgb[1] = 0x00; p0b4.messageB.rgb[2] = 0x00;
    
    // Button 5: F2
    ButtonConfig& p0b5 = buttonConfigs[0][5];
    strncpy(p0b5.name, "F2", 20); p0b5.name[20] = '\0';
    p0b5.isAlternate = false;
    p0b5.nextIsB = false;
    p0b5.messageA.type = NOTE_MOMENTARY;
    p0b5.messageA.channel = 1;
    p0b5.messageA.data1 = 41;
    p0b5.messageA.data2 = 127;
    p0b5.messageA.rgb[0] = 0xFD; p0b5.messageA.rgb[1] = 0x00; p0b5.messageA.rgb[2] = 0x00;
    p0b5.messageB.type = CC;
    p0b5.messageB.channel = 1;
    p0b5.messageB.data1 = 15;
    p0b5.messageB.data2 = 127;
    p0b5.messageB.rgb[0] = 0xFF; p0b5.messageB.rgb[1] = 0x00; p0b5.messageB.rgb[2] = 0x00;
    
    // Button 6: F#2
    ButtonConfig& p0b6 = buttonConfigs[0][6];
    strncpy(p0b6.name, "F#2", 20); p0b6.name[20] = '\0';
    p0b6.isAlternate = false;
    p0b6.nextIsB = false;
    p0b6.messageA.type = NOTE_MOMENTARY;
    p0b6.messageA.channel = 1;
    p0b6.messageA.data1 = 42;
    p0b6.messageA.data2 = 127;
    p0b6.messageA.rgb[0] = 0xFD; p0b6.messageA.rgb[1] = 0x00; p0b6.messageA.rgb[2] = 0x00;
    p0b6.messageB.type = CC;
    p0b6.messageB.channel = 1;
    p0b6.messageB.data1 = 16;
    p0b6.messageB.data2 = 127;
    p0b6.messageB.rgb[0] = 0xFF; p0b6.messageB.rgb[1] = 0x00; p0b6.messageB.rgb[2] = 0x00;
    
    // Button 7: G2
    ButtonConfig& p0b7 = buttonConfigs[0][7];
    strncpy(p0b7.name, "G2", 20); p0b7.name[20] = '\0';
    p0b7.isAlternate = false;
    p0b7.nextIsB = false;
    p0b7.messageA.type = NOTE_MOMENTARY;
    p0b7.messageA.channel = 1;
    p0b7.messageA.data1 = 43;
    p0b7.messageA.data2 = 127;
    p0b7.messageA.rgb[0] = 0xFD; p0b7.messageA.rgb[1] = 0x00; p0b7.messageA.rgb[2] = 0x00;
    p0b7.messageB.type = CC;
    p0b7.messageB.channel = 1;
    p0b7.messageB.data1 = 17;
    p0b7.messageB.data2 = 127;
    p0b7.messageB.rgb[0] = 0xFF; p0b7.messageB.rgb[1] = 0x00; p0b7.messageB.rgb[2] = 0x00;
    
    // Preset 1: "STOMP"
    strncpy(presetNames[1], "STOMP", 20);
    presetNames[1][20] = '\0';
    
    // Button 0: NR
    ButtonConfig& p1b0 = buttonConfigs[1][0];
    strncpy(p1b0.name, "NR", 20); p1b0.name[20] = '\0';
    p1b0.isAlternate = true;
    p1b0.nextIsB = false;
    p1b0.messageA.type = CC;
    p1b0.messageA.channel = 1;
    p1b0.messageA.data1 = 43;
    p1b0.messageA.data2 = 127;
    p1b0.messageA.rgb[0] = 0xFF; p1b0.messageA.rgb[1] = 0xFF; p1b0.messageA.rgb[2] = 0xFF;
    p1b0.messageB.type = CC;
    p1b0.messageB.channel = 1;
    p1b0.messageB.data1 = 43;
    p1b0.messageB.data2 = 0;
    p1b0.messageB.rgb[0] = 0xFF; p1b0.messageB.rgb[1] = 0xFF; p1b0.messageB.rgb[2] = 0xFF;
    
    // Button 1: FX1
    ButtonConfig& p1b1 = buttonConfigs[1][1];
    strncpy(p1b1.name, "FX1", 20); p1b1.name[20] = '\0';
    p1b1.isAlternate = true;
    p1b1.nextIsB = false;
    p1b1.messageA.type = CC;
    p1b1.messageA.channel = 1;
    p1b1.messageA.data1 = 44;
    p1b1.messageA.data2 = 127;
    p1b1.messageA.rgb[0] = 0x3F; p1b1.messageA.rgb[1] = 0x67; p1b1.messageA.rgb[2] = 0xFF;
    p1b1.messageB.type = CC;
    p1b1.messageB.channel = 1;
    p1b1.messageB.data1 = 44;
    p1b1.messageB.data2 = 0;
    p1b1.messageB.rgb[0] = 0x3F; p1b1.messageB.rgb[1] = 0x67; p1b1.messageB.rgb[2] = 0xFF;
    
    // Button 2: DRV
    ButtonConfig& p1b2 = buttonConfigs[1][2];
    strncpy(p1b2.name, "DRV", 20); p1b2.name[20] = '\0';
    p1b2.isAlternate = true;
    p1b2.nextIsB = false;
    p1b2.messageA.type = CC;
    p1b2.messageA.channel = 1;
    p1b2.messageA.data1 = 45;
    p1b2.messageA.data2 = 127;
    p1b2.messageA.rgb[0] = 0xFC; p1b2.messageA.rgb[1] = 0x2C; p1b2.messageA.rgb[2] = 0x00;
    p1b2.messageB.type = CC;
    p1b2.messageB.channel = 1;
    p1b2.messageB.data1 = 45;
    p1b2.messageB.data2 = 0;
    p1b2.messageB.rgb[0] = 0xFF; p1b2.messageB.rgb[1] = 0x00; p1b2.messageB.rgb[2] = 0x00;
    
    // Button 3: TAP
    ButtonConfig& p1b3 = buttonConfigs[1][3];
    strncpy(p1b3.name, "TAP", 20); p1b3.name[20] = '\0';
    p1b3.isAlternate = false;
    p1b3.nextIsB = false;
    p1b3.messageA.type = TAP_TEMPO;
    p1b3.messageA.channel = 1;
    p1b3.messageA.data1 = 13;
    p1b3.messageA.data2 = 127;
    p1b3.messageA.rgb[0] = 0xFF; p1b3.messageA.rgb[1] = 0xFF; p1b3.messageA.rgb[2] = 0xFF;
    p1b3.messageB.type = CC;
    p1b3.messageB.channel = 1;
    p1b3.messageB.data1 = 13;
    p1b3.messageB.data2 = 127;
    p1b3.messageB.rgb[0] = 0xFF; p1b3.messageB.rgb[1] = 0x00; p1b3.messageB.rgb[2] = 0x00;
    
    // Button 4: EQ
    ButtonConfig& p1b4 = buttonConfigs[1][4];
    strncpy(p1b4.name, "EQ", 20); p1b4.name[20] = '\0';
    p1b4.isAlternate = true;
    p1b4.nextIsB = false;
    p1b4.messageA.type = CC;
    p1b4.messageA.channel = 1;
    p1b4.messageA.data1 = 48;
    p1b4.messageA.data2 = 127;
    p1b4.messageA.rgb[0] = 0x0A; p1b4.messageA.rgb[1] = 0xF5; p1b4.messageA.rgb[2] = 0x00;
    p1b4.messageB.type = CC;
    p1b4.messageB.channel = 1;
    p1b4.messageB.data1 = 48;
    p1b4.messageB.data2 = 0;
    p1b4.messageB.rgb[0] = 0x0A; p1b4.messageB.rgb[1] = 0xF5; p1b4.messageB.rgb[2] = 0x00;
    
    // Button 5: FX2
    ButtonConfig& p1b5 = buttonConfigs[1][5];
    strncpy(p1b5.name, "FX2", 20); p1b5.name[20] = '\0';
    p1b5.isAlternate = true;
    p1b5.nextIsB = false;
    p1b5.messageA.type = CC;
    p1b5.messageA.channel = 1;
    p1b5.messageA.data1 = 49;
    p1b5.messageA.data2 = 127;
    p1b5.messageA.rgb[0] = 0x11; p1b5.messageA.rgb[1] = 0xF3; p1b5.messageA.rgb[2] = 0xFF;
    p1b5.messageB.type = CC;
    p1b5.messageB.channel = 1;
    p1b5.messageB.data1 = 49;
    p1b5.messageB.data2 = 0;
    p1b5.messageB.rgb[0] = 0x11; p1b5.messageB.rgb[1] = 0xF3; p1b5.messageB.rgb[2] = 0xFF;
    
    // Button 6: DLY
    ButtonConfig& p1b6 = buttonConfigs[1][6];
    strncpy(p1b6.name, "DLY", 20); p1b6.name[20] = '\0';
    p1b6.isAlternate = true;
    p1b6.nextIsB = false;
    p1b6.messageA.type = CC;
    p1b6.messageA.channel = 1;
    p1b6.messageA.data1 = 50;
    p1b6.messageA.data2 = 127;
    p1b6.messageA.rgb[0] = 0x33; p1b6.messageA.rgb[1] = 0x2A; p1b6.messageA.rgb[2] = 0xFF;
    p1b6.messageB.type = CC;
    p1b6.messageB.channel = 1;
    p1b6.messageB.data1 = 50;
    p1b6.messageB.data2 = 0;
    p1b6.messageB.rgb[0] = 0x33; p1b6.messageB.rgb[1] = 0x2A; p1b6.messageB.rgb[2] = 0xFF;
    
    // Button 7: RVB
    ButtonConfig& p1b7 = buttonConfigs[1][7];
    strncpy(p1b7.name, "RVB", 20); p1b7.name[20] = '\0';
    p1b7.isAlternate = true;
    p1b7.nextIsB = false;
    p1b7.messageA.type = CC;
    p1b7.messageA.channel = 1;
    p1b7.messageA.data1 = 51;
    p1b7.messageA.data2 = 127;
    p1b7.messageA.rgb[0] = 0x84; p1b7.messageA.rgb[1] = 0x00; p1b7.messageA.rgb[2] = 0xF7;
    p1b7.messageB.type = CC;
    p1b7.messageB.channel = 1;
    p1b7.messageB.data1 = 51;
    p1b7.messageB.data2 = 0;
    p1b7.messageB.rgb[0] = 0x84; p1b7.messageB.rgb[1] = 0x00; p1b7.messageB.rgb[2] = 0xF7;
    
    // Preset 2: "BANKS 1-8"
    strncpy(presetNames[2], "BANKS 1-8", 20);
    presetNames[2][20] = '\0';
    
    // Button 0: B1
    ButtonConfig& p2b0 = buttonConfigs[2][0];
    strncpy(p2b0.name, "B1", 20); p2b0.name[20] = '\0';
    p2b0.isAlternate = false;
    p2b0.nextIsB = false;
    p2b0.messageA.type = CC;
    p2b0.messageA.channel = 1;
    p2b0.messageA.data1 = 1;
    p2b0.messageA.data2 = 1;
    p2b0.messageA.rgb[0] = 0xFF; p2b0.messageA.rgb[1] = 0xFF; p2b0.messageA.rgb[2] = 0xFF;
    p2b0.messageB.type = CC;
    p2b0.messageB.channel = 1;
    p2b0.messageB.data1 = 10;
    p2b0.messageB.data2 = 127;
    p2b0.messageB.rgb[0] = 0xFF; p2b0.messageB.rgb[1] = 0x00; p2b0.messageB.rgb[2] = 0x00;
    
    // Button 1: B2
    ButtonConfig& p2b1 = buttonConfigs[2][1];
    strncpy(p2b1.name, "B2", 20); p2b1.name[20] = '\0';
    p2b1.isAlternate = false;
    p2b1.nextIsB = false;
    p2b1.messageA.type = CC;
    p2b1.messageA.channel = 1;
    p2b1.messageA.data1 = 1;
    p2b1.messageA.data2 = 2;
    p2b1.messageA.rgb[0] = 0xFF; p2b1.messageA.rgb[1] = 0xFF; p2b1.messageA.rgb[2] = 0xFF;
    p2b1.messageB.type = CC;
    p2b1.messageB.channel = 1;
    p2b1.messageB.data1 = 11;
    p2b1.messageB.data2 = 127;
    p2b1.messageB.rgb[0] = 0xFF; p2b1.messageB.rgb[1] = 0x00; p2b1.messageB.rgb[2] = 0x00;
    
    // Button 2: B3
    ButtonConfig& p2b2 = buttonConfigs[2][2];
    strncpy(p2b2.name, "B3", 20); p2b2.name[20] = '\0';
    p2b2.isAlternate = false;
    p2b2.nextIsB = false;
    p2b2.messageA.type = CC;
    p2b2.messageA.channel = 1;
    p2b2.messageA.data1 = 1;
    p2b2.messageA.data2 = 3;
    p2b2.messageA.rgb[0] = 0xFF; p2b2.messageA.rgb[1] = 0xFF; p2b2.messageA.rgb[2] = 0xFF;
    p2b2.messageB.type = CC;
    p2b2.messageB.channel = 1;
    p2b2.messageB.data1 = 12;
    p2b2.messageB.data2 = 127;
    p2b2.messageB.rgb[0] = 0xFF; p2b2.messageB.rgb[1] = 0x00; p2b2.messageB.rgb[2] = 0x00;
    
    // Button 3: B4
    ButtonConfig& p2b3 = buttonConfigs[2][3];
    strncpy(p2b3.name, "B4", 20); p2b3.name[20] = '\0';
    p2b3.isAlternate = false;
    p2b3.nextIsB = false;
    p2b3.messageA.type = CC;
    p2b3.messageA.channel = 1;
    p2b3.messageA.data1 = 1;
    p2b3.messageA.data2 = 4;
    p2b3.messageA.rgb[0] = 0xFF; p2b3.messageA.rgb[1] = 0xFF; p2b3.messageA.rgb[2] = 0xFF;
    p2b3.messageB.type = CC;
    p2b3.messageB.channel = 1;
    p2b3.messageB.data1 = 13;
    p2b3.messageB.data2 = 127;
    p2b3.messageB.rgb[0] = 0xFF; p2b3.messageB.rgb[1] = 0x00; p2b3.messageB.rgb[2] = 0x00;
    
    // Button 4: B5
    ButtonConfig& p2b4 = buttonConfigs[2][4];
    strncpy(p2b4.name, "B5", 20); p2b4.name[20] = '\0';
    p2b4.isAlternate = false;
    p2b4.nextIsB = false;
    p2b4.messageA.type = CC;
    p2b4.messageA.channel = 1;
    p2b4.messageA.data1 = 1;
    p2b4.messageA.data2 = 5;
    p2b4.messageA.rgb[0] = 0x0A; p2b4.messageA.rgb[1] = 0xF5; p2b4.messageA.rgb[2] = 0x00;
    p2b4.messageB.type = CC;
    p2b4.messageB.channel = 1;
    p2b4.messageB.data1 = 14;
    p2b4.messageB.data2 = 127;
    p2b4.messageB.rgb[0] = 0xFF; p2b4.messageB.rgb[1] = 0x00; p2b4.messageB.rgb[2] = 0x00;
    
    // Button 5: B6
    ButtonConfig& p2b5 = buttonConfigs[2][5];
    strncpy(p2b5.name, "B6", 20); p2b5.name[20] = '\0';
    p2b5.isAlternate = false;
    p2b5.nextIsB = false;
    p2b5.messageA.type = CC;
    p2b5.messageA.channel = 1;
    p2b5.messageA.data1 = 1;
    p2b5.messageA.data2 = 6;
    p2b5.messageA.rgb[0] = 0x0A; p2b5.messageA.rgb[1] = 0xF5; p2b5.messageA.rgb[2] = 0x00;
    p2b5.messageB.type = CC;
    p2b5.messageB.channel = 1;
    p2b5.messageB.data1 = 15;
    p2b5.messageB.data2 = 127;
    p2b5.messageB.rgb[0] = 0xFF; p2b5.messageB.rgb[1] = 0x00; p2b5.messageB.rgb[2] = 0x00;
    
    // Button 6: B7
    ButtonConfig& p2b6 = buttonConfigs[2][6];
    strncpy(p2b6.name, "B7", 20); p2b6.name[20] = '\0';
    p2b6.isAlternate = false;
    p2b6.nextIsB = false;
    p2b6.messageA.type = CC;
    p2b6.messageA.channel = 1;
    p2b6.messageA.data1 = 1;
    p2b6.messageA.data2 = 7;
    p2b6.messageA.rgb[0] = 0x0A; p2b6.messageA.rgb[1] = 0xF5; p2b6.messageA.rgb[2] = 0x00;
    p2b6.messageB.type = CC;
    p2b6.messageB.channel = 1;
    p2b6.messageB.data1 = 16;
    p2b6.messageB.data2 = 127;
    p2b6.messageB.rgb[0] = 0xFF; p2b6.messageB.rgb[1] = 0x00; p2b6.messageB.rgb[2] = 0x00;
    
    // Button 7: B8
    ButtonConfig& p2b7 = buttonConfigs[2][7];
    strncpy(p2b7.name, "B8", 20); p2b7.name[20] = '\0';
    p2b7.isAlternate = false;
    p2b7.nextIsB = false;
    p2b7.messageA.type = CC;
    p2b7.messageA.channel = 1;
    p2b7.messageA.data1 = 1;
    p2b7.messageA.data2 = 8;
    p2b7.messageA.rgb[0] = 0x0A; p2b7.messageA.rgb[1] = 0xF5; p2b7.messageA.rgb[2] = 0x00;
    p2b7.messageB.type = CC;
    p2b7.messageB.channel = 1;
    p2b7.messageB.data1 = 17;
    p2b7.messageB.data2 = 127;
    p2b7.messageB.rgb[0] = 0xFF; p2b7.messageB.rgb[1] = 0x00; p2b7.messageB.rgb[2] = 0x00;
    
    // Preset 3: "BANKS 9-16"
    strncpy(presetNames[3], "BANKS 9-16", 20);
    presetNames[3][20] = '\0';
    
    // Button 0: B9
    ButtonConfig& p3b0 = buttonConfigs[3][0];
    strncpy(p3b0.name, "B9", 20); p3b0.name[20] = '\0';
    p3b0.isAlternate = false;
    p3b0.nextIsB = false;
    p3b0.messageA.type = CC;
    p3b0.messageA.channel = 1;
    p3b0.messageA.data1 = 1;
    p3b0.messageA.data2 = 9;
    p3b0.messageA.rgb[0] = 0x11; p3b0.messageA.rgb[1] = 0xF3; p3b0.messageA.rgb[2] = 0xFF;
    p3b0.messageB.type = CC;
    p3b0.messageB.channel = 1;
    p3b0.messageB.data1 = 10;
    p3b0.messageB.data2 = 127;
    p3b0.messageB.rgb[0] = 0xFF; p3b0.messageB.rgb[1] = 0x00; p3b0.messageB.rgb[2] = 0x00;
    
    // Button 1: B10
    ButtonConfig& p3b1 = buttonConfigs[3][1];
    strncpy(p3b1.name, "B10", 20); p3b1.name[20] = '\0';
    p3b1.isAlternate = false;
    p3b1.nextIsB = false;
    p3b1.messageA.type = CC;
    p3b1.messageA.channel = 1;
    p3b1.messageA.data1 = 1;
    p3b1.messageA.data2 = 10;
    p3b1.messageA.rgb[0] = 0x11; p3b1.messageA.rgb[1] = 0xF3; p3b1.messageA.rgb[2] = 0xFF;
    p3b1.messageB.type = CC;
    p3b1.messageB.channel = 1;
    p3b1.messageB.data1 = 11;
    p3b1.messageB.data2 = 127;
    p3b1.messageB.rgb[0] = 0xFF; p3b1.messageB.rgb[1] = 0x00; p3b1.messageB.rgb[2] = 0x00;
    
    // Button 2: B11
    ButtonConfig& p3b2 = buttonConfigs[3][2];
    strncpy(p3b2.name, "B11", 20); p3b2.name[20] = '\0';
    p3b2.isAlternate = false;
    p3b2.nextIsB = false;
    p3b2.messageA.type = CC;
    p3b2.messageA.channel = 1;
    p3b2.messageA.data1 = 1;
    p3b2.messageA.data2 = 11;
    p3b2.messageA.rgb[0] = 0x11; p3b2.messageA.rgb[1] = 0xF3; p3b2.messageA.rgb[2] = 0xFF;
    p3b2.messageB.type = CC;
    p3b2.messageB.channel = 1;
    p3b2.messageB.data1 = 12;
    p3b2.messageB.data2 = 127;
    p3b2.messageB.rgb[0] = 0xFF; p3b2.messageB.rgb[1] = 0x00; p3b2.messageB.rgb[2] = 0x00;
    
    // Button 3: B12
    ButtonConfig& p3b3 = buttonConfigs[3][3];
    strncpy(p3b3.name, "B12", 20); p3b3.name[20] = '\0';
    p3b3.isAlternate = false;
    p3b3.nextIsB = false;
    p3b3.messageA.type = CC;
    p3b3.messageA.channel = 1;
    p3b3.messageA.data1 = 1;
    p3b3.messageA.data2 = 12;
    p3b3.messageA.rgb[0] = 0x11; p3b3.messageA.rgb[1] = 0xF3; p3b3.messageA.rgb[2] = 0xFF;
    p3b3.messageB = p3b3.messageA;  // Copy of A since not alternate
    
    // Button 4: B13
    ButtonConfig& p3b4 = buttonConfigs[3][4];
    strncpy(p3b4.name, "B13", 20); p3b4.name[20] = '\0';
    p3b4.isAlternate = false;
    p3b4.nextIsB = false;
    p3b4.messageA.type = CC;
    p3b4.messageA.channel = 1;
    p3b4.messageA.data1 = 1;
    p3b4.messageA.data2 = 13;
    p3b4.messageA.rgb[0] = 0xFE; p3b4.messageA.rgb[1] = 0x63; p3b4.messageA.rgb[2] = 0xFF;
    p3b4.messageB.type = CC;
    p3b4.messageB.channel = 1;
    p3b4.messageB.data1 = 14;
    p3b4.messageB.data2 = 127;
    p3b4.messageB.rgb[0] = 0xFF; p3b4.messageB.rgb[1] = 0x00; p3b4.messageB.rgb[2] = 0x00;
    
    // Button 5: B14
    ButtonConfig& p3b5 = buttonConfigs[3][5];
    strncpy(p3b5.name, "B14", 20); p3b5.name[20] = '\0';
    p3b5.isAlternate = false;
    p3b5.nextIsB = false;
    p3b5.messageA.type = CC;
    p3b5.messageA.channel = 1;
    p3b5.messageA.data1 = 1;
    p3b5.messageA.data2 = 14;
    p3b5.messageA.rgb[0] = 0xFE; p3b5.messageA.rgb[1] = 0x63; p3b5.messageA.rgb[2] = 0xFF;
    p3b5.messageB.type = CC;
    p3b5.messageB.channel = 1;
    p3b5.messageB.data1 = 15;
    p3b5.messageB.data2 = 127;
    p3b5.messageB.rgb[0] = 0xFF; p3b5.messageB.rgb[1] = 0x00; p3b5.messageB.rgb[2] = 0x00;
    
    // Button 6: B15
    ButtonConfig& p3b6 = buttonConfigs[3][6];
    strncpy(p3b6.name, "B15", 20); p3b6.name[20] = '\0';
    p3b6.isAlternate = false;
    p3b6.nextIsB = false;
    p3b6.messageA.type = CC;
    p3b6.messageA.channel = 1;
    p3b6.messageA.data1 = 1;
    p3b6.messageA.data2 = 15;
    p3b6.messageA.rgb[0] = 0xFE; p3b6.messageA.rgb[1] = 0x63; p3b6.messageA.rgb[2] = 0xFF;
    p3b6.messageB.type = CC;
    p3b6.messageB.channel = 1;
    p3b6.messageB.data1 = 16;
    p3b6.messageB.data2 = 127;
    p3b6.messageB.rgb[0] = 0xFF; p3b6.messageB.rgb[1] = 0x00; p3b6.messageB.rgb[2] = 0x00;
    
    // Button 7: B16
    ButtonConfig& p3b7 = buttonConfigs[3][7];
    strncpy(p3b7.name, "B16", 20); p3b7.name[20] = '\0';
    p3b7.isAlternate = false;
    p3b7.nextIsB = false;
    p3b7.messageA.type = CC;
    p3b7.messageA.channel = 1;
    p3b7.messageA.data1 = 1;
    p3b7.messageA.data2 = 16;
    p3b7.messageA.rgb[0] = 0xFE; p3b7.messageA.rgb[1] = 0x63; p3b7.messageA.rgb[2] = 0xFF;
    p3b7.messageB.type = CC;
    p3b7.messageB.channel = 1;
    p3b7.messageB.data1 = 17;
    p3b7.messageB.data2 = 127;
    p3b7.messageB.rgb[0] = 0xFF; p3b7.messageB.rgb[1] = 0x00; p3b7.messageB.rgb[2] = 0x00;
}

#endif
