#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include "Config.h"

// ============================================
// v2.0 MEMORY-OPTIMIZED ENUMS AND STRUCTS
// ============================================

// BLE Mode Configuration
enum BleMode {
    BLE_CLIENT_ONLY,   // Connect to SPM (current behavior)
    BLE_SERVER_ONLY,   // Accept connections from DAW/Apps
    BLE_DUAL_MODE      // Both client and server active
};

// Extended MIDI Command Types
enum MidiCommandType { 
    OFF, 
    NOTE_MOMENTARY, 
    NOTE_ON, 
    NOTE_OFF, 
    CC, 
    PC, 
    TAP_TEMPO,
    PRESET_UP,
    PRESET_DOWN,
    PRESET_1,         // Direct jump to Preset 1
    PRESET_2,         // Direct jump to Preset 2
    PRESET_3,         // Direct jump to Preset 3
    PRESET_4,         // Direct jump to Preset 4
    CLEAR_BLE_BONDS,
    WIFI_TOGGLE
};


// Primary MIDI Message - 23 bytes (same as before, sysex reduced to fit new fields)
struct MidiMessage {
    MidiCommandType type;   // 1 byte
    byte channel;           // 1 byte
    byte data1;             // 1 byte
    byte data2;             // 1 byte
    int8_t rhythmPrevButton;   // 1 byte (-1 = none, 0-7 = button index) - default: 0 (btn 1)
    int8_t rhythmNextButton;   // 1 byte (-1 = none, 0-7 = button index) - default: 4 (btn 5)
    int8_t tapModeLockButton;  // 1 byte (-1 = none, 0-7 = button index) - default: 7 (btn 8)
    char sysex[12];         // 12 bytes (reduced from 16 to keep struct size same)
    byte rgb[3];            // 3 bytes
};

// Hold Action - 21 bytes (no RGB - removed from UI)
struct HoldAction {
    bool enabled;           // 1 byte
    uint16_t thresholdMs;   // 2 bytes (0-65535ms)
    MidiCommandType type;   // 1 byte
    byte channel;           // 1 byte
    byte data1;             // 1 byte
    byte data2;             // 1 byte
    char sysex[14];         // 14 bytes
};

// Combo Action - 21 bytes (no RGB)
struct ComboAction {
    bool enabled;           // 1 byte
    int8_t partner;         // 1 byte (-1 = none, 0-9 = button)
    MidiCommandType type;   // 1 byte
    byte channel;           // 1 byte
    byte data1;             // 1 byte
    byte data2;             // 1 byte
    char label[7];          // 7 bytes (6 chars + null) for custom display text
    char sysex[8];          // 8 bytes (reduced from 14)
};

// Button Config - ~109 bytes total
struct ButtonConfig {
    char name[21];          // 21 bytes
    bool isAlternate;       // 1 byte
    bool nextIsB;           // 1 byte
    MidiMessage messageA;   // 23 bytes
    MidiMessage messageB;   // 23 bytes
    HoldAction hold;        // 21 bytes
    ComboAction combo;      // 21 bytes
};

// System Config - ~93 bytes
struct SystemConfig {
    char bleDeviceName[24]; // 24 bytes
    char apSSID[24];        // 24 bytes
    char apPassword[16];    // 16 bytes (reduced for memory)
    uint8_t buttonCount;    // 1 byte
    uint8_t buttonPins[10]; // 10 bytes
    uint8_t ledPin;         // 1 byte
    uint8_t encoderA;       // 1 byte
    uint8_t encoderB;       // 1 byte
    uint8_t encoderBtn;     // 1 byte
    bool wifiOnAtBoot;      // 1 byte
    BleMode bleMode;        // 1 byte - BLE_CLIENT_ONLY, BLE_SERVER_ONLY, or BLE_DUAL_MODE
    uint8_t ledsPerButton;  // 1 byte - How many consecutive LEDs per button (for LED strips)
    uint8_t ledMap[10];     // 10 bytes - Button to LED index mapping (for single LED mode)
};

// Special Action - 42 bytes per button (hold + combo, independent of presets)
struct SpecialAction {
    HoldAction hold;        // 21 bytes
    ComboAction combo;      // 21 bytes
};

// Global Override - 43 bytes per button
struct GlobalOverride {
    HoldAction hold;        // 21 bytes
    ComboAction combo;      // 21 bytes
    bool overridesPreset;   // 1 byte
};

// ============================================
// GLOBAL OBJECTS
// ============================================

extern Adafruit_SSD1306 display;
extern Adafruit_NeoPixel strip;
extern ESP32Encoder encoder;
extern Preferences systemPrefs;
extern WebServer server;

// ============================================
// BLE GLOBALS
// ============================================

// Client mode (for connecting TO SPM)
extern bool clientConnected;
extern bool doConnect;
extern bool doScan;
extern BLEClient* pClient;
extern BLERemoteCharacteristic* pRemoteCharacteristic;
extern BLEAdvertisedDevice* myDevice;

// Server mode (for DAW/App connections TO Chocotone)
extern bool serverConnected;
extern BLEServer* pServer;
extern BLECharacteristic* pServerMidiCharacteristic;

// ============================================
// v2.0 SYSTEM CONFIGURATION
// ============================================

extern SystemConfig systemConfig;
extern bool isWifiOn;  // Runtime state (separate from wifiOnAtBoot)

// ============================================
// PRESET DATA
// ============================================

extern int currentPreset;
extern char presetNames[4][21];
extern ButtonConfig buttonConfigs[4][MAX_BUTTONS];  // Expanded to MAX_BUTTONS
extern GlobalOverride globalOverrides[MAX_BUTTONS]; // Keep for memory layout
extern SpecialAction globalSpecialActions[MAX_BUTTONS]; // Global special actions (hold/combo)

// ============================================
// UI SETTINGS
// ============================================

extern int ledBrightnessOn;
extern int ledBrightnessDim;
extern int buttonDebounce;
extern int buttonNameFontSize;

// ============================================
// STATE VARIABLES
// ============================================

extern long oldEncoderPosition;
extern bool encoderButtonPressed;
extern unsigned long encoderButtonPressStartTime;
extern int currentMode; // 0: Preset, 1: Menu
extern int menuSelection;
extern bool inSubMenu;
extern int editingValue;

// Button state tracking (dynamic size)
extern bool buttonPinActive[MAX_BUTTONS];
extern unsigned long lastButtonPressTime_pads[MAX_BUTTONS];
extern int activeNotesOnButtonPins[MAX_BUTTONS];

// Hold/Combo action tracking
extern unsigned long buttonHoldStartTime[MAX_BUTTONS];  // When button was first pressed
extern bool buttonHoldFired[MAX_BUTTONS];                // If hold action already fired
extern bool buttonComboChecked[MAX_BUTTONS];             // If combo already checked this press

// Display state
extern char buttonNameToShow[21];
extern unsigned long buttonNameDisplayUntil;
extern char lastSentMidiString[20];

// ============================================
// LED STATE TRACKING
// ============================================

extern uint32_t lastLedColors[NUM_LEDS];

// ============================================
// OLED HEALTH MONITORING
// ============================================

extern bool oledHealthy;
extern unsigned long lastOledCheck;

// ============================================
// DEFERRED UPDATES
// ============================================

extern bool pendingDisplayUpdate;

// ============================================
// TAP TEMPO
// ============================================

extern unsigned long lastTapTime;
extern unsigned long tapIntervals[4];
extern int tapIndex;
extern int tapCount;
extern float currentBPM;
extern int currentDelayType;
// ledMap is now in systemConfig.ledMap

extern int rhythmPattern; // 0=1/8, 1=1/8d, 2=1/4, 3=1/2
extern bool inTapTempoMode;
extern bool tapModeLocked;  // When true, tap tempo mode stays active until unlocked
extern unsigned long tapModeTimeout;
extern const float rhythmMultipliers[4];
extern const char* rhythmNames[4];

// Tap tempo LED blinking
extern unsigned long lastTapBlinkTime;
extern bool tapBlinkState;

#endif
