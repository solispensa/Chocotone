#ifndef GLOBALS_H
#define GLOBALS_H

#include "Config.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <ESP32Encoder.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

// Forward declaration for DeviceProfiles
enum DeviceType : uint8_t;

// ============================================
// v3.0 ACTION-BASED DATA STRUCTURES
// ============================================

// BLE Mode Configuration
enum BleMode {
  BLE_CLIENT_ONLY, // Connect to SPM (current behavior)
  BLE_SERVER_ONLY, // Accept connections from DAW/Apps
  BLE_DUAL_MODE    // Both client and server active
};

// Extended MIDI Command Types
enum MidiCommandType : uint8_t {
  MIDI_OFF = 0,
  NOTE_MOMENTARY,
  NOTE_ON,
  NOTE_OFF,
  CC,
  PC,
  SYSEX, // Raw SysEx message (hex string stored in sysexData)
  TAP_TEMPO,
  PRESET_UP,
  PRESET_DOWN,
  PRESET_1, // Direct jump to Preset 1
  PRESET_2, // Direct jump to Preset 2
  PRESET_3, // Direct jump to Preset 3
  PRESET_4, // Direct jump to Preset 4
  CLEAR_BLE_BONDS,
  WIFI_TOGGLE,
  // Menu Navigation Commands (v1.5.1)
  MENU_TOGGLE, // Enter/Exit menu mode (long press equivalent)
  MENU_UP,     // Navigate menu up / decrease value
  MENU_DOWN,   // Navigate menu down / increase value
  MENU_ENTER   // Select menu item / confirm value
};

// Action Type - when this message triggers
enum ActionType : uint8_t {
  ACTION_NONE = 0,
  ACTION_PRESS,          // On button press (primary action)
  ACTION_2ND_PRESS,      // Alternate press (toggle behavior)
  ACTION_RELEASE,        // On button release
  ACTION_2ND_RELEASE,    // Alternate release (after 2ND_PRESS)
  ACTION_LONG_PRESS,     // After holding for threshold
  ACTION_2ND_LONG_PRESS, // Alternate long-press (after 2ND_PRESS)
  ACTION_DOUBLE_TAP,     // Quick double press
  ACTION_COMBO,          // When pressed with partner button
  ACTION_NO_ACTION       // Disabled/empty slot
};

// LED Mode - how LED responds to button press (per-button setting)
enum LedMode : uint8_t {
  LED_MOMENTARY = 0, // LED on while pressed, off when released
  LED_TOGGLE = 1     // LED toggles on/off with each press
};

// Preset LED Mode - how LEDs behave at the preset level
enum PresetLedMode : uint8_t {
  PRESET_LED_NORMAL = 0, // Each button uses its own LED mode (Momentary/Toggle)
  PRESET_LED_SELECTION = 1, // Radio-button style - one selected, others dim
  PRESET_LED_HYBRID = 2     // Mix of selection buttons and normal buttons
};

// Sync Mode - which device to sync effect states with
enum SyncMode : uint8_t {
  SYNC_NONE = 0, // No effect state sync
  SYNC_SPM = 1,  // Sync with Sonicake Pocket Master
  SYNC_GP5 = 2   // Sync with Valeton GP-5
};

// ============================================
// ACTION MESSAGE STRUCTURE (~16 bytes each)
// ============================================
struct ActionMessage {
  ActionType action;    // 1 byte - When this action triggers
  MidiCommandType type; // 1 byte - What MIDI to send
  uint8_t channel;      // 1 byte - MIDI channel (1-16)
  uint8_t data1;        // 1 byte - Note/CC number
  uint8_t data2;        // 1 byte - Velocity/Value
  uint8_t rgb[3];       // 3 bytes - LED color
  char label[6]; // 6 bytes - Custom OLED label (5 chars + null) for ALL action
                 // types

  // Analog Range limits (0-100%) - ignored for digital buttons, used for Analog
  // Actions
  uint8_t minInput;
  uint8_t maxInput;
  uint8_t minOut; // Min MIDI output value
  uint8_t maxOut; // Max MIDI output value

  // Action-specific data (union to save memory)
  union {
    // For ACTION_COMBO
    struct {
      int8_t partner; // Partner button index (-1 = none)
    } combo;

    // For ACTION_LONG_PRESS
    struct {
      uint16_t holdMs; // Hold threshold in ms
    } longPress;

    // For TAP_TEMPO type
    struct {
      int8_t rhythmPrev; // Button for rhythm--
      int8_t rhythmNext; // Button for rhythm++
      int8_t tapLock;    // Button for tap lock toggle
    } tapTempo;

    // For SYSEX type - 48-byte buffer for longer SPM commands (delay_time,
    // reverb etc.)
    struct {
      uint8_t data[48]; // Raw SysEx bytes (including F0 and F7)
      uint8_t length;   // Number of valid bytes
    } sysex;

    uint8_t _padding[49]; // Ensure union is 49 bytes (sysex is largest now)
  };
};

#define MAX_ACTIONS_PER_BUTTON 6

// ============================================
// BUTTON CONFIG (~85 bytes per button)
// ============================================
struct ButtonConfig {
  char name[21];         // 21 bytes - Button display name
  LedMode ledMode;       // 1 byte - LED behavior (momentary/toggle)
  bool inSelectionGroup; // 1 byte - Part of selection group in Hybrid mode
  uint8_t messageCount;  // 1 byte - Number of active messages
  ActionMessage messages[MAX_ACTIONS_PER_BUTTON]; // ~96 bytes (6 x 16)

  // Runtime state (not saved)
  bool isAlternate; // 1 byte - Current toggle state
};

// Multiplexer Configuration (v1.5)
struct MultiplexerConfig {
  bool enabled;
  char type[16]; // e.g. "cd74hc4067"
  uint8_t signalPin;
  uint8_t selectPins[4];
  char useFor[8];                     // "analog" or "buttons" or "both"
  int8_t buttonChannels[MAX_BUTTONS]; // Mux channel for each physical button
                                      // (-1 = not mux)
};

// ============================================
// SYSTEM CONFIG (~130 bytes)
// ============================================
struct SystemConfig {
  char bleDeviceName[24];          // 24 bytes
  char apSSID[24];                 // 24 bytes
  char apPassword[16];             // 16 bytes
  uint8_t buttonCount;             // 1 byte
  uint8_t buttonPins[MAX_BUTTONS]; // MAX_BUTTONS bytes
  uint8_t ledPin;                  // 1 byte
  uint8_t encoderA;                // 1 byte
  uint8_t encoderB;                // 1 byte
  uint8_t encoderBtn;              // 1 byte
  bool wifiOnAtBoot;               // 1 byte
  BleMode bleMode;                 // 1 byte
  uint8_t ledsPerButton;           // 1 byte
  uint8_t ledMap[MAX_BUTTONS];     // MAX_BUTTONS bytes

  // Device abstraction (v3.1)
  DeviceType
      targetDevice;    // 1 byte - Which device to control (SPM, GP5, Generic)
  uint8_t midiChannel; // 1 byte - MIDI channel for device (1-16)

  MultiplexerConfig multiplexer; // ~25 bytes
  bool debugAnalogIn;            // v1.5: Show raw ADC values on OLED
  uint8_t analogInputCount;      // v1.5: Number of enabled analog inputs (0-16)

  // Display pins (v1.5 - configurable I2C/SPI pins for auto-conflict
  // resolution)
  uint8_t oledSdaPin; // I2C SDA (default: 21)
  uint8_t oledSclPin; // I2C SCL (default: 22)
  uint8_t tftCsPin;   // SPI CS (default: 15)
  uint8_t tftDcPin;   // SPI DC (default: 2)
  uint8_t tftRstPin;  // SPI RST (default: 4)
  uint8_t tftMosiPin; // SPI MOSI (default: 23)
  uint8_t tftSclkPin; // SPI SCLK (default: 18)
  uint8_t tftLedPin;  // SPI LED/Backlight (default: 32)
};

// ============================================
// OLED CONFIGURATION (v1.5 - 128x32 support)
// ============================================
enum OledType : uint8_t { OLED_128X64 = 0, OLED_128X32 = 1, TFT_128X128 = 2 };

struct OledScreenConfig {
  uint8_t labelSize;   // 1 byte - Text size for button labels (1-3)
  uint8_t titleSize;   // 1 byte - Text size for preset name (1-4)
  uint8_t statusSize;  // 1 byte - Text size for BLE status (1-3)
  uint8_t bpmSize;     // 1 byte - Text size for BPM (1-3) - v1.5.1
  uint8_t topRowY;     // 1 byte - Y position for top labels
  uint8_t titleY;      // 1 byte - Y position for preset name
  uint8_t statusY;     // 1 byte - Y position for BLE status
  uint8_t bpmY;        // 1 byte - Y position for BPM - v1.5.1
  uint8_t bottomRowY;  // 1 byte - Y position for bottom labels
  bool showBpm;        // 1 byte - Show BPM on main screen
  bool showAnalog;     // 1 byte - Show analog input values
  bool showTopRow;     // 1 byte - Show top row inputs - v1.5.1
  bool showBottomRow;  // 1 byte - Show bottom row inputs - v1.5.1
  bool showStatus;     // 1 byte - Show BLE status line - v1.5.1
  uint8_t titleAlign;  // 1 byte - 0=Left, 1=Center, 2=Right - v1.5.1
  uint8_t statusAlign; // 1 byte - 0=Left, 1=Center, 2=Right - v1.5.1
  uint8_t bpmAlign;    // 1 byte - 0=Left, 1=Center, 2=Right - v1.5.1
  char topRowMap[32]; // 32 bytes - Input mapping for top row (e.g. "5,6,7,8") -
                      // v1.5.1
  char bottomRowMap[32]; // 32 bytes - Input mapping for bottom row (e.g.
                         // "1,2,3,4") - v1.5.1

  // TFT Color Strip Feature (v1.5.2)
  bool showColorStrips; // 1 byte - Show colored bars matching button RGB (TFT
                        // only)
  uint8_t colorStripHeight; // 1 byte - Color strip thickness in pixels (1-10,
                            // default 4)
  uint8_t topRowAlign;      // 1 byte - 0=Left, 1=Center, 2=Right for top row
  uint8_t bottomRowAlign;   // 1 byte - 0=Left, 1=Center, 2=Right for bottom row
};

struct OledConfig {
  OledType type;            // 1 byte - OLED_128X64 or OLED_128X32
  uint8_t rotation;         // 1 byte - 0, 90, 180, 270
  OledScreenConfig main;    // Main screen settings
  OledScreenConfig menu;    // Menu screen settings
  OledScreenConfig tap;     // Tap tempo screen settings
  OledScreenConfig overlay; // Button overlay settings
};

extern OledConfig oledConfig;

// Color abstraction for different display types
#define DISPLAY_WHITE                                                          \
  (oledConfig.type == TFT_128X128 ? ST7735_WHITE : SSD1306_WHITE)
#define DISPLAY_BLACK                                                          \
  (oledConfig.type == TFT_128X128 ? ST7735_BLACK : SSD1306_BLACK)

// ============================================
// GLOBAL SPECIAL ACTIONS (per-button, across presets)
// ============================================
struct GlobalSpecialAction {
  ActionMessage comboAction; // Global combo override
  bool hasCombo;             // Whether global combo is active
};

// ============================================
// GLOBAL OBJECTS
// ============================================

// Modified to pointer to allow dynamic height configuration (128x64 vs 128x32
// vs 128x128)
extern Adafruit_GFX *displayPtr;
// PREVIOUS MACRO REMOVED due to collision with display() method
// All code must now use displayPtr-> instead of display.
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
extern bool
    bleConfigMode; // True when BLE Config Mode is active (pauses scanning)
extern BLEClient *pClient;
extern BLERemoteCharacteristic *pRemoteCharacteristic;
extern BLEAdvertisedDevice *myDevice;

// Server mode (for DAW/App connections TO Chocotone)
extern bool serverConnected;
extern BLEServer *pServer;
extern BLECharacteristic *pServerMidiCharacteristic;

// ============================================
// SYSTEM CONFIGURATION
// ============================================

extern SystemConfig systemConfig;
extern bool isWifiOn;
extern bool isBtSerialOn; // Bluetooth Serial active (disables BLE like WiFi)

// ============================================
// PRESET DATA
// ============================================

extern int currentPreset;
extern char presetNames[4][21];
extern char configProfileName[32];  // Config profile name (editor metadata)
extern char configLastModified[24]; // Last modified timestamp (editor metadata)
extern ButtonConfig buttonConfigs[4][MAX_BUTTONS];
extern GlobalSpecialAction globalSpecialActions[MAX_BUTTONS];

// ============================================
// UI SETTINGS
// ============================================

extern int ledBrightnessOn;
extern int ledBrightnessDim;
extern int ledBrightnessTap;
extern int buttonDebounce;
extern int buttonNameFontSize;

// ============================================
// STATE VARIABLES
// ============================================

extern long oldEncoderPosition;
extern bool encoderButtonPressed;
extern unsigned long encoderButtonPressStartTime;
extern int currentMode;
extern int menuSelection;
extern bool inSubMenu;
extern bool factoryResetConfirm;
extern int editingValue;

// Button state tracking
extern bool buttonPinActive[MAX_BUTTONS];
extern unsigned long lastButtonPressTime_pads[MAX_BUTTONS];
extern unsigned long lastButtonReleaseTime_pads[MAX_BUTTONS];
extern int activeNotesOnButtonPins[MAX_BUTTONS];

// Hold/Combo action tracking
extern unsigned long buttonHoldStartTime[MAX_BUTTONS];
extern bool buttonHoldFired[MAX_BUTTONS];
extern bool buttonComboChecked[MAX_BUTTONS];
extern bool buttonConsumed[MAX_BUTTONS];

// Display state
extern char buttonNameToShow[21];
extern unsigned long buttonNameDisplayUntil;
extern char lastSentMidiString[20];

// ============================================
// LED STATE TRACKING
// ============================================

extern bool ledToggleState[MAX_BUTTONS];
extern PresetLedMode presetLedModes[4];
extern int8_t presetSelectionState[4];
extern uint32_t lastLedColors[NUM_LEDS];

// ============================================
// EFFECT STATE SYNC (Device-Agnostic + Legacy)
// ============================================

// Number of abstract effect blocks (from DeviceProfiles.h)
#define EFFECT_BLOCK_COUNT_MAX 10

extern SyncMode presetSyncMode[4]; // Per-preset sync mode (NONE, SPM, GP5)
extern bool spmEffectStates[9]; // Legacy: NR, FX1, DRV, AMP, IR, EQ, FX2, DLY,
                                // RVB (SPM only)
extern bool effectStates[EFFECT_BLOCK_COUNT_MAX]; // Unified effect states for
                                                  // all devices
extern bool spmStateReceived; // Flag: state was received from device
extern unsigned long lastSpmStateRequest; // Debounce state requests

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

extern int rhythmPattern;
extern bool inTapTempoMode;
extern bool tapModeLocked;
extern unsigned long tapModeTimeout;
extern const float rhythmMultipliers[4];
extern const char *rhythmNames[4];

extern unsigned long lastTapBlinkTime;
extern bool tapBlinkState;

// ============================================
// HELPER FUNCTIONS
// ============================================

// Find action by type in a button's message array
inline ActionMessage *findAction(ButtonConfig &btn, ActionType actionType) {
  for (int i = 0; i < btn.messageCount; i++) {
    if (btn.messages[i].action == actionType) {
      return &btn.messages[i];
    }
  }
  return nullptr;
}

// Check if button has a specific action type
inline bool hasAction(ButtonConfig &btn, ActionType actionType) {
  return findAction(btn, actionType) != nullptr;
}

// Get partner button index for combo action
inline int8_t getComboPartner(ButtonConfig &btn) {
  ActionMessage *combo = findAction(btn, ACTION_COMBO);
  return combo ? combo->combo.partner : -1;
}

#endif
