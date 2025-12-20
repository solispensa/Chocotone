#ifndef BLEMIDI_H
#define BLEMIDI_H

#include "Globals.h"

// BLE Setup and Connection
void setup_ble_midi();
void setup_ble_server();
void setup_ble_config_server();  // Config server for web editor (always runs)
void startBleScan();
void handleBleConnection();

// MIDI Sending (via BLE Client to SPM)
void sendMidiNoteOn(byte ch, byte n, byte v);
void sendMidiNoteOff(byte ch, byte n, byte v);
void sendMidiCC(byte ch, byte n, byte v);
void sendMidiPC(byte ch, byte n);
void sendDelayTime(int delayMs);
void sendSysex(const uint8_t* data, size_t length);

// MIDI via Server (to connected DAW/Apps)
void sendMidiToServer(byte* data, size_t length);

// Utilities
void clearBLEBonds();
void checkForSysex();
void requestPresetState();
void applySpmStateToButtons();  // Apply received SPM state to button toggles

// Mode helpers
inline bool isBleClientEnabled() {
    return systemConfig.bleMode == BLE_CLIENT_ONLY || systemConfig.bleMode == BLE_DUAL_MODE;
}
inline bool isBleServerEnabled() {
    return systemConfig.bleMode == BLE_SERVER_ONLY || systemConfig.bleMode == BLE_DUAL_MODE;
}

#endif
