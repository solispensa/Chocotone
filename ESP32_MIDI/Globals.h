#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h> // Added WiFi.h
#include <BLEDevice.h>
#include "Config.h"

// Enums and Structs
enum MidiCommandType { OFF, NOTE_MOMENTARY, NOTE_ON, NOTE_OFF, CC, PC, TAP_TEMPO };

struct MidiMessage {
    MidiCommandType type;
    byte channel;
    byte data1;
    byte data2;
    byte rgb[3];
};

struct ButtonConfig {
    char name[21];
    bool isAlternate;
    bool nextIsB; // State for toggle
    MidiMessage messageA;
    MidiMessage messageB;
};

// Global Objects
extern Adafruit_SSD1306 display;
extern Adafruit_NeoPixel strip;
extern ESP32Encoder encoder;
extern Preferences systemPrefs;
extern WebServer server;

// BLE Globals
// BLE Globals
extern bool serverConnected;
extern bool clientConnected;
extern bool doConnect;
extern bool doScan;
extern BLEClient* pClient;
extern BLERemoteCharacteristic* pRemoteCharacteristic;
extern BLECharacteristic* pMidiCharacteristic;
extern BLEAdvertisedDevice* myDevice;

// System Settings
extern char bleDeviceName[32];
extern char apSSID[32];
extern char apPassword[64];
extern bool isWifiOn;
extern bool wifiOnAtBoot;

// Preset Data
extern int currentPreset;
extern char presetNames[4][21];
extern ButtonConfig buttonConfigs[4][NUM_BUTTONS];

// UI Settings
extern int ledBrightnessOn;
extern int ledBrightnessDim;
extern int buttonDebounce;
extern int buttonNameFontSize;

// State Variables
extern long oldEncoderPosition;
extern bool encoderButtonPressed;
extern unsigned long encoderButtonPressStartTime;
extern int currentMode; // 0: Preset, 1: Menu
extern int menuSelection;
extern bool inSubMenu;
extern int editingValue;
extern int buttonPins[NUM_BUTTONS];
extern bool buttonPinActive[NUM_BUTTONS];
extern unsigned long lastButtonPressTime_pads[NUM_BUTTONS];
extern int activeNotesOnButtonPins[NUM_BUTTONS];
extern char buttonNameToShow[21];
extern unsigned long buttonNameDisplayUntil;
extern char lastSentMidiString[20];

// LED State Tracking (for optimized updates)
extern uint32_t lastLedColors[NUM_LEDS];

// OLED Health Monitoring
extern bool oledHealthy;
extern unsigned long lastOledCheck;

// Deferred Updates (for web interface stability)
extern bool pendingDisplayUpdate;

// Tap Tempo Globals
extern unsigned long lastTapTime;
extern unsigned long tapIntervals[4];
extern int tapIndex;
extern int tapCount;
extern float currentBPM;
extern int currentDelayType;
extern const int ledMap[NUM_LEDS];

// Tap Tempo Mode
extern int rhythmPattern; // 0=1/8, 1=1/8d, 2=1/4, 3=1/2
extern bool inTapTempoMode;
extern unsigned long tapModeTimeout;
extern const float rhythmMultipliers[4];
extern const char* rhythmNames[4];

#endif
