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
    
    // === Chocotone v1.2 Loading Screen ===
    int16_t x1, y1;
    uint16_t w, h;
    
    // Title: "CHOCOTONE" - Size 2, Centered
    display.setTextSize(2);
    display.getTextBounds("CHOCOTONE", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 10);
    display.print(F("CHOCOTONE"));
    
    // Subtitle: "MIDI by ANDRE SOLIS" - Size 1, Centered
    display.setTextSize(1);
    display.getTextBounds("MIDI by ANDRE SOLIS", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 30);
    display.print(F("MIDI by ANDRE SOLIS"));
    
    // 8 Dots Grid (2 rows × 4 cols) - Initially hollow
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
            int x = 4 + col * 8 + 2;
            int y = 45 + row * 8 + 2;
            display.drawCircle(x, y, 2, SSD1306_WHITE);
        }
    }
    
    // Version Badge - White rounded rect with black text
    display.fillRoundRect(94, 47, 28, 10, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(96, 48);
    display.print(F("v1.2"));
    display.setTextColor(SSD1306_WHITE);  // Reset
    
    display.display();
    
    // Helper lambda to fill a loading dot (0-7)
    auto fillLoadingDot = [](int dotIndex) {
        int row = dotIndex / 4;
        int col = dotIndex % 4;
        int x = 4 + col * 8 + 2;
        int y = 45 + row * 8 + 2;
        display.fillCircle(x, y, 2, SSD1306_WHITE);
        display.display();
    };
    
    // Dot 0: Display initialized
    fillLoadingDot(0);

    // Load Settings
    Serial.println("Loading settings...");
    loadSystemSettings();
    fillLoadingDot(1);  // Dot 1: System settings loaded
    
    loadPresets();
    fillLoadingDot(2);  // Dot 2: Presets loaded
    
    loadCurrentPresetIndex();  // Restore last used preset

    // Initialize Encoder
    encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    encoder.setCount(0);
    oldEncoderPosition = 0;
    pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);
    fillLoadingDot(3);  // Dot 3: Encoder initialized

    // Initialize Buttons
    Serial.println("Initializing buttons...");
    Serial.print("Button pins: ");
    for (int i = 0; i < systemConfig.buttonCount; i++) {
        Serial.printf("%d ", systemConfig.buttonPins[i]);
    }
    Serial.println();
    
    for (int i = 0; i < systemConfig.buttonCount; i++) {
        int pin = systemConfig.buttonPins[i];
        if (pin == 34 || pin == 35) pinMode(pin, INPUT);
        else pinMode(pin, INPUT_PULLUP);
        activeNotesOnButtonPins[i] = -1;
        buttonPinActive[i] = false;
        lastButtonPressTime_pads[i] = 0;
    }
    fillLoadingDot(4);  // Dot 4: Buttons initialized

    // Initialize LEDs
    Serial.println("Initializing LEDs...");
    strip.begin();
    strip.show();
    strip.setBrightness(ledBrightnessOn);
    fillLoadingDot(5);  // Dot 5: LEDs initialized

    // Initialize BLE (Client and/or Server based on bleMode)
    Serial.println("Initializing BLE...");
    setup_ble_midi();
    fillLoadingDot(6);  // Dot 6: BLE initialized
    
    // Start BLE Scan for SPM (only if Client or Dual mode)
    if (systemConfig.bleMode == BLE_CLIENT_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
        Serial.println("Scanning for SPM...");
        startBleScan();
    }

    // Setup web server routes
    Serial.println("Setting up web server...");
    setup_web_server();
    fillLoadingDot(7);  // Dot 7: Web server ready

    // Start WiFi if enabled at boot
    if (systemConfig.wifiOnAtBoot) {
        Serial.println("WiFi On at Boot enabled - starting WiFi AP...");
        turnWifiOn();
    }

    // Display and LEDs
    if (!isWifiOn) {
        displayOLED();
        updateLeds();
    } else {
        Serial.println("Skipping OLED/LED/BleScan update due to WiFi On");
        // Optional: Draw simple text on OLED if safe? 
        // For now, safety first.
    }

    Serial.println("=== Setup Complete ===");
    Serial.printf("BLE Name: %s\n", systemConfig.bleDeviceName);
    Serial.printf("BLE Mode: %s\n", 
        systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT_ONLY (→SPM)" :
        systemConfig.bleMode == BLE_SERVER_ONLY ? "SERVER_ONLY (DAW→)" : "DUAL_MODE (DAW→ESP→SPM)");
    Serial.printf("WiFi: %s\n", isWifiOn ? "ON" : "OFF");
    Serial.printf("WiFi On at Boot: %s\n", systemConfig.wifiOnAtBoot ? "ENABLED" : "DISABLED");
    
    // Debug: Print button 4 action settings
    Serial.println("\n=== Button 4 (User Button 5) Action Settings ===");
    const ButtonConfig& debugBtn = buttonConfigs[currentPreset][4];
    Serial.printf("Button Name: %s, Message Count: %d\n", debugBtn.name, debugBtn.messageCount);
    for (int i = 0; i < debugBtn.messageCount; i++) {
        const ActionMessage& m = debugBtn.messages[i];
        Serial.printf("  Action %d: type=%d, channel=%d, data1=%d\n", m.action, m.type, m.channel, m.data1);
    }
}

void loop() {
    // Handle WiFi
    if(isWifiOn) {
        server.handleClient();
        yield();  // Extra yield after handling client
    }

    handleEncoderButtonPress();

    // Handle Menu or Preset Mode
    if (currentMode == 1) {
        loop_menuMode();
    } else {
        loop_presetMode();
        
        // Update display to handle button name timeout
        // SKIP when WiFi is on to prevent crash
        if (!isWifiOn && buttonNameDisplayUntil > 0 && millis() >= buttonNameDisplayUntil) {
            buttonNameDisplayUntil = 0;
            safeDisplayOLED();
        }
    }
    
    // Update LEDs continuously (needed for tap tempo blink)
    // Skip when WiFi is on (heap too low)
    if (!isWifiOn) {
        updateLeds();
    }
    
    // Handle deferred display updates from web interface
    if (pendingDisplayUpdate) {
        pendingDisplayUpdate = false;
        safeDisplayOLED();
        // Skip LED update if WiFi is on (NeoPixel disables interrupts, crashes WiFi)
        if (!isWifiOn) {
            updateLeds();
        }
    }
    
    // Skip BLE operations when WiFi is on (already paused)
    if (!isWifiOn) {
        handleBleConnection();
        checkForSysex();
    }
    
    // Handle serial commands for offline editor config transfer
    handleSerialConfig();
    
    yield();  // Feed watchdog
}
