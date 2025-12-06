#include "Globals.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
ESP32Encoder encoder;
Preferences systemPrefs;
WebServer server(80);

// BLE Globals (only non-BLE-client related ones)
bool serverConnected = false;
BLECharacteristic* pMidiCharacteristic = nullptr;

// System Settings
char bleDeviceName[32] = DEFAULT_BLE_NAME;
char apSSID[32] = DEFAULT_AP_SSID;
char apPassword[64] = DEFAULT_AP_PASS;
bool isWifiOn = false;
bool wifiOnAtBoot = false;

// Preset Data
int currentPreset = 0;
char presetNames[4][21] = {"Preset 1", "Preset 2", "Preset 3", "Preset 4"};
ButtonConfig buttonConfigs[4][NUM_BUTTONS];

// UI Settings
int ledBrightnessOn = 20;  // Reduced from 50 for power stability
int ledBrightnessDim = 45; // Reduced from 90 for power stability
int buttonDebounce = 120;
int buttonNameFontSize = 4;

// State Variables
long oldEncoderPosition = 0;
bool encoderButtonPressed = false;
unsigned long encoderButtonPressStartTime = 0;
int currentMode = 0;
int menuSelection = 0;
bool inSubMenu = false;
int editingValue = 0;
int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25, 33, 32, 16, 17};
bool buttonPinActive[NUM_BUTTONS] = {false};
unsigned long lastButtonPressTime_pads[NUM_BUTTONS] = {0};
int activeNotesOnButtonPins[NUM_BUTTONS];
char buttonNameToShow[21] = "";
unsigned long buttonNameDisplayUntil = 0;
char lastSentMidiString[20] = "";

// LED State Tracking (for optimized updates)
uint32_t lastLedColors[NUM_LEDS] = {0};

// OLED Health Monitoring
bool oledHealthy = true;
unsigned long lastOledCheck = 0;

// Deferred Updates (for web interface stability)
bool pendingDisplayUpdate = false;

// Tap Tempo Globals
unsigned long lastTapTime = 0;
unsigned long tapIntervals[4] = {0, 0, 0, 0};
int tapIndex = 0;
int tapCount = 0;
float currentBPM = 120.0;
int currentDelayType = 0;
const int ledMap[NUM_LEDS] = {0, 1, 2, 3, 7, 6, 5, 4};

// Tap Tempo Mode
int rhythmPattern = 0; // Default to 1/8
bool inTapTempoMode = false;
unsigned long tapModeTimeout = 0;
const float rhythmMultipliers[4] = {1.0, 0.5, 0.75, 2.0};  // 1/4, 1/8, 1/8d, 1/2
const char* rhythmNames[4] = {"1/4", "1/8", "1/8d", "1/2"};
