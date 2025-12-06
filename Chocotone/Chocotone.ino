#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Globals.h"
#include "Storage.h"
#include "BleMidi.h"
#include "UI_Display.h"
#include "Input.h"
#include "WebInterface.h"

void setup() {
    Serial.begin(115200);
    delay(100);  // Power stabilization delay
    Serial.println("\n\n=== ESP32 MIDI Controller Starting ===");

    // Initialize Display with slower I2C for stability
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(100000);  // 100kHz instead of default 400kHz - more stable
    delay(50);  // Allow I2C to stabilize
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    // Calculate centering for "Chocotone MIDI"
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds("Chocotone MIDI", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 20);
    display.println(F("Chocotone MIDI"));
    
    // Center "v1.0"
    display.getTextBounds("v1.0", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 32);
    display.println(F("v1.0"));
    
    // Center "Loading...."
    display.getTextBounds("Loading....", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 44);
    display.println(F("Loading...."));
    
    display.display();
    delay(200);

    // Load Settings
    Serial.println("Loading settings...");
    loadSystemSettings();
    loadPresets();
    loadCurrentPresetIndex();  // Restore last used preset

    // Initialize Encoder
    encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    encoder.setCount(0);
    oldEncoderPosition = 0;
    pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Initialize Buttons
    Serial.println("Initializing buttons...");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int pin = buttonPins[i];
        if (pin == 34 || pin == 35) pinMode(pin, INPUT);
        else pinMode(pin, INPUT_PULLUP);
        activeNotesOnButtonPins[i] = -1;
        buttonPinActive[i] = false;
        lastButtonPressTime_pads[i] = 0;
    }

    // Initialize LEDs
    Serial.println("Initializing LEDs...");
    strip.begin();
    strip.show();
    strip.setBrightness(ledBrightnessOn);

    // Initialize BLE Client only (for SPM connection)
    Serial.println("Initializing BLE Client...");
    setup_ble_midi();
    
    // Start BLE Scan for SPM
    Serial.println("Scanning for SPM...");
    startBleScan();

    // Setup web server routes
    Serial.println("Setting up web server...");
    setup_web_server();

    // Start WiFi if enabled at boot
    if (wifiOnAtBoot) {
        Serial.println("WiFi On at Boot enabled - starting WiFi AP...");
        turnWifiOn();
    }

    // Display and LEDs
    displayOLED();
    updateLeds();

    Serial.println("=== Setup Complete ===");
    Serial.printf("BLE Name: %s\n", bleDeviceName);
    Serial.printf("WiFi: %s\n", isWifiOn ? "ON" : "OFF");
    Serial.printf("WiFi On at Boot: %s\n", wifiOnAtBoot ? "ENABLED" : "DISABLED");
    Serial.println("Controller is CLIENT-ONLY - connects TO SPM via BLE");
}

void loop() {
    // Handle WiFi
    if(isWifiOn) {
        server.handleClient();
    }

    handleEncoderButtonPress();

    // Handle Menu or Preset Mode
    if (currentMode == 1) {
        loop_menuMode();
    } else {
        loop_presetMode();
        
        // Update display to handle button name timeout
        if (buttonNameDisplayUntil > 0 && millis() >= buttonNameDisplayUntil) {
            buttonNameDisplayUntil = 0;
            safeDisplayOLED();  // Use safe wrapper with auto-recovery
        }
    }
    
    // Handle deferred display/LED updates from web interface
    if (pendingDisplayUpdate) {
        pendingDisplayUpdate = false;
        safeDisplayOLED();
        updateLeds();
    }
    
    handleBleConnection();
    checkForSysex();
}
