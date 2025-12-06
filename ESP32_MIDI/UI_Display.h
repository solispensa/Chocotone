#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include "Globals.h"

void displayOLED();
void displayMenu();
void updateLeds();
void blinkAllLeds();
void displayTapTempoMode();
void displayButtonName();
void midiNoteNumberToString(char* buffer, size_t bufferSize, int note);
void getButtonSummary(char* b, size_t s, const MidiMessage& c);

// OLED Health Monitoring & Recovery
bool checkOledHealth();
void recoverOled();
void safeDisplayOLED();

#endif
