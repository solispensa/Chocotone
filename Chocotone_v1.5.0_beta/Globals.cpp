#include "Globals.h"
#include "DeviceProfiles.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

Adafruit_GFX *displayPtr = nullptr;
Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
ESP32Encoder encoder;
Preferences systemPrefs;
WebServer server(80);

// ============================================
// BLE GLOBALS
// ============================================

// NOTE: All BLE variables are now defined in BleMidi.cpp:
// Client mode: clientConnected, doConnect, doScan, pClient,
// pRemoteCharacteristic, myDevice Server mode: serverConnected, pServer,
// pServerMidiCharacteristic

// ============================================
// v3.0 SYSTEM CONFIGURATION
// ============================================

SystemConfig systemConfig = {
    DEFAULT_BLE_NAME,     // bleDeviceName[24]
    DEFAULT_AP_SSID,      // apSSID[24]
    DEFAULT_AP_PASS,      // apPassword[16]
    DEFAULT_BUTTON_COUNT, // buttonCount
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    {38, 39, 40, 41, 42, 21, 8, 9, 0, 0}, // buttonPins (S3 Optimized)
#else
    {14, 27, 26, 25, 33, 32, 16, 17, 0, 0}, // buttonPins (Standard Legacy)
#endif
    NEOPIXEL_PIN,                   // ledPin
    DEFAULT_ENCODER_A,              // encoderA (18)
    DEFAULT_ENCODER_B,              // encoderB (19)
    DEFAULT_ENCODER_BTN,            // encoderBtn (23)
    false,                          // wifiOnAtBoot
    BLE_CLIENT_ONLY,                // bleMode
    1,                              // ledsPerButton
    {0, 1, 2, 3, 7, 6, 5, 4, 8, 9}, // ledMap[10]
    DEVICE_SPM,                     // targetDevice
    1,                              // midiChannel
    {},                             // multiplexer (zeroed)
    false,                          // debugAnalogIn
    0,                              // analogInputCount
    0,                              // batteryAdcPin (0=disabled)
    // Display pins (Use Config.h macros)
    OLED_SDA_PIN, // oledSdaPin
    OLED_SCL_PIN, // oledSclPin
    TFT_CS,       // tftCsPin
    TFT_DC,       // tftDcPin
    TFT_RST,      // tftRstPin
    TFT_MOSI,     // tftMosiPin
    TFT_SCLK,     // tftSclkPin
    TFT_LED       // tftLedPin
};

bool isWifiOn = false;
bool isBtSerialOn = false; // Bluetooth Serial (SPP) active

// ============================================
// OLED CONFIGURATION (v1.5 - 128x32 support)
// ============================================

OledConfig oledConfig = {
    OLED_NONE, // type - No display by default (prevents I2C errors)
    0,         // rotation (0°)
    // I2C pins (for OLED displays)
    OLED_SDA_PIN, // sdaPin
    OLED_SCL_PIN, // sclPin
    // SPI pins (for TFT displays)
    TFT_CS,   // csPin
    TFT_DC,   // dcPin
    TFT_RST,  // rstPin
    TFT_MOSI, // mosiPin
    TFT_SCLK, // sclkPin
    TFT_LED,  // ledPin (backlight)
    // Main screen settings
    {
        1,         // labelSize
        2,         // titleSize
        1,         // statusSize
        1,         // bpmSize (v1.5.1)
        0,         // topRowY
        14,        // titleY
        44,        // statusY
        32,        // bpmY (v1.5.1)
        56,        // bottomRowY
        true,      // showBpm
        false,     // showAnalog
        true,      // showTopRow (v1.5.1)
        true,      // showBottomRow (v1.5.1)
        true,      // showStatus (v1.5.1)
        1,         // titleAlign (1=Center)
        0,         // statusAlign (0=Left)
        1,         // bpmAlign (1=Center)
        "5,6,7,8", // topRowMap (v1.5.1)
        "1,2,3,4", // bottomRowMap (v1.5.1)
        // TFT Color Strip settings (v1.5.2)
        false, // showColorStrips
        4,     // colorStripHeight
        0,     // topRowAlign
        0,     // bottomRowAlign
        // Battery Indicator (v1.5)
        false, // showBattery
        116,   // batteryX (right edge - 12px icon)
        0,     // batteryY
        1      // batteryScale
    },
    // Menu screen settings
    {1,     // labelSize (itemSize)
     1,     // titleSize (headerSize)
     1,     // statusSize
     1,     // bpmSize
     0,     // topRowY (headerY)
     14,    // titleY (itemStartY)
     0,     // statusY
     0,     // bpmY
     0,     // bottomRowY
     false, // showBpm
     false, // showAnalog
     true,  // showTopRow
     true,  // showBottomRow
     true,  // showStatus
     0,     0, 0, "", "",
     false, 4, 0, 0, // colorStrips: show, height, topAlign, bottomAlign
     false,          // showBattery
     116,   0, 1},   // batteryX, batteryY, batteryScale
    // Tap tempo screen settings
    {1,     // labelSize
     3,     // titleSize (bpmSize)
     1,     // statusSize (patternSize)
     1,     // bpmSize
     0,     // topRowY (labelTopY)
     16,    // titleY (bpmY)
     46,    // statusY (patternY)
     0,     // bpmY
     56,    // bottomRowY (labelBottomY)
     true,  // showBpm
     false, // showAnalog
     true, true, true, 0, 0, 0, "", "", false, 4, 0,
     0,          // colorStrips: show, height, topAlign,
                 // bottomAlign
     false,      // showBattery
     116, 0, 1}, // batteryX, batteryY, batteryScale
    // Overlay settings
    {2, // labelSize
     2, // titleSize (textSize)
     1,     1, 0, 0, 0,  0,  0,     false, false, true, true,
     true,  0, 0, 0, "", "", false, 4,     0,     0, // colorStrips
     false,                                          // showBattery
     116,   0, 1}}; // batteryX, batteryY, batteryScale

// ============================================
// GLOBAL SPECIAL ACTIONS
// ============================================

GlobalSpecialAction globalSpecialActions[MAX_BUTTONS] = {};

// ============================================
// PRESET DATA
// ============================================

int currentPreset = 0;
int presetCount = 4; // Default to 4 presets for backward compatibility
char presetNames[CHOCO_MAX_PRESETS][21] = {"Preset 1", "Preset 2", "Preset 3",
                                           "Preset 4"};
char configProfileName[32] = "My Chocotone Config";         // Editor metadata
char configLastModified[24] = "";                           // Editor metadata
ButtonConfig buttonConfigs[CHOCO_MAX_PRESETS][MAX_BUTTONS]; // Zero-initialized

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
bool factoryResetConfirm = false; // Factory Reset confirmation submenu
int editingValue = 0;

// Button state tracking
bool buttonPinActive[MAX_BUTTONS] = {false};
unsigned long lastButtonPressTime_pads[MAX_BUTTONS] = {0};
unsigned long lastButtonReleaseTime_pads[MAX_BUTTONS] = {0};
int activeNotesOnButtonPins[MAX_BUTTONS] = {0};

// Hold/Combo action tracking
unsigned long buttonHoldStartTime[MAX_BUTTONS] = {0};
bool buttonHoldFired[MAX_BUTTONS] = {false};
bool buttonComboChecked[MAX_BUTTONS] = {false};
bool buttonConsumed[MAX_BUTTONS] = {false}; // Blocks re-trigger until released

// Display state
char buttonNameToShow[21] = "";
unsigned long buttonNameDisplayUntil = 0;
char lastSentMidiString[20] = "";

// ============================================
// LED STATE TRACKING
// ============================================

bool ledToggleState[MAX_BUTTONS] = {false};

// Preset-level LED mode configuration
PresetLedMode presetLedModes[CHOCO_MAX_PRESETS] = {
    PRESET_LED_NORMAL, PRESET_LED_SELECTION, PRESET_LED_SELECTION,
    PRESET_LED_SELECTION};
int8_t presetSelectionState[CHOCO_MAX_PRESETS] = {-1, -1, -1, -1};

uint32_t lastLedColors[NUM_LEDS] = {0};

// ============================================
// EFFECT STATE SYNC (Device-Agnostic + Legacy)
// ============================================

SyncMode presetSyncMode[CHOCO_MAX_PRESETS] = {
    SYNC_NONE, SYNC_NONE, SYNC_NONE, SYNC_NONE}; // Per-preset sync mode
bool spmEffectStates[9] = {
    false}; // Legacy: NR, FX1, DRV, AMP, IR, EQ, FX2, DLY, RVB (SPM only)
bool effectStates[EFFECT_BLOCK_COUNT_MAX] = {
    false}; // Unified effect states for all devices
bool spmStateReceived = false;
unsigned long lastSpmStateRequest = 0;

// ============================================
// OLED HEALTH MONITORING
// ============================================

bool oledHealthy = true;
unsigned long lastOledCheck = 0;

// ============================================
// BATTERY MONITORING (v1.5)
// ============================================

uint8_t batteryPercent = 0;
unsigned long lastBatteryRead = 0;

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
const char *rhythmNames[4] = {"1/4", "1/8", "1/8d", "1/2"};

unsigned long lastTapBlinkTime = 0;
bool tapBlinkState = false;
