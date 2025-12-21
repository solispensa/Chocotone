#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "Globals.h"
#include "UI_Display.h"
#include <WiFi.h>

void handleRoot();
void handleSave();
void handleSaveSystem();
void handleExport();
void handleImport();
void handleImportUpload();
void rebootESP(String message);
void setup_web_server();
void turnWifiOn();
void turnWifiOff();
void rgbToHex(char* buffer, size_t size, const byte rgb[3]);
void hexToRgb(const String& hex, byte rgb[3]);
void sendOptions(String& out, MidiCommandType currentType);
void sendActionFields(String& out, const char* id, const ActionMessage* msg);
void handleSerialConfig();  // Handle serial commands (GET_CONFIG, SET_CONFIG)

// Bluetooth Serial (SPP) for wireless editor connection
void turnBtSerialOn();
void turnBtSerialOff();
void handleBtSerialConfig();

#endif
