#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include "Globals.h"

void displayOLED();
void displayMenu();
void displayEditMenu(); // v1.5: On-device action editor
void hsvToRgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b);
void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, int *h, int *s, int *v);

void displayAnalogDebug(); // v1.5: Dedicated analog debug screen
void updateLeds();
void updateIndividualLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void blinkAllLeds();
void blinkTapButton(int buttonIndex);
void displayTapTempoMode();
void displayButtonName();
void midiNoteNumberToString(char *buffer, size_t bufferSize, int note);
void getButtonSummary(char *b, size_t s, MidiCommandType type, int data1);

// OLED Health Monitoring & Recovery
bool checkOledHealth();
void recoverOled();
void safeDisplayOLED();
void updateAnalogColorStrips(); // Partial update for analog loading bars (no
                                // flicker)

// Hardware Init
void initDisplayHardware();

// Display abstraction helpers
void flushDisplay();
void clearDisplayBuffer();

// Battery Monitoring (v1.5)
void updateBatteryLevel();
void drawBatteryIcon(int x, int y, int scale = 1);

#endif
