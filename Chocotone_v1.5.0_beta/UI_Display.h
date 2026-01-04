#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include "Globals.h"

void displayOLED();
void displayMenu();
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

// Hardware Init
void initDisplayHardware();

// Display abstraction helpers
void flushDisplay();
void clearDisplayBuffer();

#endif
