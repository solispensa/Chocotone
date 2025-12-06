#ifndef BLEMIDI_H
#define BLEMIDI_H

#include "Globals.h"

void setup_ble_midi();
void startBleScan();
void handleBleConnection();
void sendMidiMessage(const MidiMessage& msg);
void sendMidiNoteOn(byte ch, byte n, byte v);
void sendMidiNoteOff(byte ch, byte n, byte v);
void sendMidiCC(byte ch, byte n, byte v);
void sendMidiPC(byte ch, byte n);
void sendDelayTime(int delayMs);
void sendSysex(const uint8_t* data, size_t length);
void clearBLEBonds();
void checkForSysex();
void requestPresetState();

#endif
