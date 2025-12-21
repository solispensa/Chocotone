#include "Globals.h"
#include "DeviceProfiles.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
ESP32Encoder encoder;
Preferences systemPrefs;
WebServer server(80);

// ============================================
// BLE GLOBALS
// ============================================

// NOTE: All BLE variables are now defined in BleMidi.cpp:
// Client mode: clientConnected, doConnect, doScan, pClient, pRemoteCharacteristic, myDevice
// Server mode: serverConnected, pServer, pServerMidiCharacteristic


// ============================================
// v3.0 SYSTEM CONFIGURATION
// ============================================

SystemConfig systemConfig = {
    DEFAULT_BLE_NAME,           // bleDeviceName[24]
    DEFAULT_AP_SSID,            // apSSID[24]
    DEFAULT_AP_PASS,            // apPassword[16]
    DEFAULT_BUTTON_COUNT,       // buttonCount
    {14, 27, 26, 25, 33, 32, 16, 17, 0, 0},  // buttonPins[10]
    NEOPIXEL_PIN,               // ledPin
    DEFAULT_ENCODER_A,          // encoderA
    DEFAULT_ENCODER_B,          // encoderB
    DEFAULT_ENCODER_BTN,        // encoderBtn
    false,                      // wifiOnAtBoot
    BLE_CLIENT_ONLY,            // bleMode
    1,                          // ledsPerButton
    {0, 1, 2, 3, 7, 6, 5, 4, 8, 9},  // ledMap[10]
    DEVICE_SPM,                 // targetDevice (default: SPM for backward compat)
    1                           // midiChannel (default: 1)
};

bool isWifiOn = false;

// ============================================
// GLOBAL SPECIAL ACTIONS
// ============================================

GlobalSpecialAction globalSpecialActions[MAX_BUTTONS] = {};

// ============================================
// PRESET DATA
// ============================================

int currentPreset = 0;
char presetNames[4][21] = {"Preset 1", "Preset 2", "Preset 3", "Preset 4"};
char configProfileName[32] = "My Chocotone Config";  // Editor metadata
char configLastModified[24] = "";                     // Editor metadata
ButtonConfig buttonConfigs[4][MAX_BUTTONS];  // Zero-initialized

// ============================================
// UI SETTINGS
// ============================================

int ledBrightnessOn = 220;
int ledBrightnessDim = 20;
int ledBrightnessTap = 240;
int buttonDebounce = 120;
int buttonNameFontSize = 5;

// ============================================
// STATE VARIABLES
// ============================================

long oldEncoderPosition = 0;
bool encoderButtonPressed = false;
unsigned long encoderButtonPressStartTime = 0;
int currentMode = 0;
int menuSelection = 0;
bool inSubMenu = false;
int editingValue = 0;

// Button state tracking
bool buttonPinActive[MAX_BUTTONS] = {false};
unsigned long lastButtonPressTime_pads[MAX_BUTTONS] = {0};
int activeNotesOnButtonPins[MAX_BUTTONS] = {0};

// Hold/Combo action tracking
unsigned long buttonHoldStartTime[MAX_BUTTONS] = {0};
bool buttonHoldFired[MAX_BUTTONS] = {false};
bool buttonComboChecked[MAX_BUTTONS] = {false};

// Display state
char buttonNameToShow[21] = "";
unsigned long buttonNameDisplayUntil = 0;
char lastSentMidiString[20] = "";

// ============================================
// LED STATE TRACKING
// ============================================

bool ledToggleState[MAX_BUTTONS] = {false};

// Preset-level LED mode configuration
PresetLedMode presetLedModes[4] = {PRESET_LED_NORMAL, PRESET_LED_SELECTION, PRESET_LED_SELECTION, PRESET_LED_SELECTION};
int8_t presetSelectionState[4] = {-1, -1, -1, -1};

uint32_t lastLedColors[NUM_LEDS] = {0};

// ============================================
// EFFECT STATE SYNC (Device-Agnostic + Legacy)
// ============================================

SyncMode presetSyncMode[4] = {SYNC_NONE, SYNC_NONE, SYNC_NONE, SYNC_NONE};  // Per-preset sync mode
bool spmEffectStates[9] = {false};  // Legacy: NR, FX1, DRV, AMP, IR, EQ, FX2, DLY, RVB (SPM only)
bool effectStates[EFFECT_BLOCK_COUNT_MAX] = {false};  // Unified effect states for all devices
bool spmStateReceived = false;
unsigned long lastSpmStateRequest = 0;

// ============================================
// OLED HEALTH MONITORING
// ============================================

bool oledHealthy = true;
unsigned long lastOledCheck = 0;

// ============================================
// DEFERRED UPDATES
// ============================================

bool pendingDisplayUpdate = false;

// ============================================
// TAP TEMPO
// ============================================

unsigned long lastTapTime = 0;
unsigned long tapIntervals[4] = {0, 0, 0, 0};
int tapIndex = 0;
int tapCount = 0;
float currentBPM = 120.0;
int currentDelayType = 0;

int rhythmPattern = 0;
bool inTapTempoMode = false;
bool tapModeLocked = false;
unsigned long tapModeTimeout = 0;
const float rhythmMultipliers[4] = {1.0, 0.5, 0.75, 2.0};
const char* rhythmNames[4] = {"1/4", "1/8", "1/8d", "1/2"};

unsigned long lastTapBlinkTime = 0;
bool tapBlinkState = false;
