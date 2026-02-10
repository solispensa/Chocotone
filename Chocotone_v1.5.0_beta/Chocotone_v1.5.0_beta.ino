#include "AnalogInput.h"
#include "BleMidi.h"
#include "Config.h"
#include "Globals.h"
#include "Input.h"
#include "Storage.h"
#include "UI_Display.h"
#include "WebInterface.h"
#include <Arduino.h>
#include <SPI.h> // For TFT displays
#include <Wire.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <USB.h>
#include <USBMIDI.h>
USBMIDI usbMidi;
#endif

// ============================================================================
// PIN CONFLICT RESOLUTION FOR TFT/OLED AUTO-SWITCHING
// ============================================================================

// Check if a pin is in an array
bool isPinInArray(uint8_t pin, uint8_t *arr, int len) {
  for (int i = 0; i < len; i++) {
    if (arr[i] == pin)
      return true;
  }
  return false;
}

// Find an available GPIO pin not in the avoid list
uint8_t findAvailablePin(uint8_t *avoid, int avoidLen) {
  // Safe GPIO pins for reassignment (excluding input-only for outputs)
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // S3 Safe Pins (Avoids 0, 45, 46 strapping & 26-32 Flash/PSRAM)
  uint8_t safePins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
  int safeCount = 14;
#else
  // Original ESP32 Safe Pins
  uint8_t safePins[] = {12, 13, 14, 25, 26, 27, 33};
  int safeCount = 7;
#endif

  for (int i = 0; i < safeCount; i++) {
    if (!isPinInArray(safePins[i], avoid, avoidLen)) {
      return safePins[i];
    }
  }

  // If all safe pins taken, try input-only pins (for buttons only)
  // S3 doesn't have "input only" pins in the same way, but keeping structure
  // for legacy
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
  uint8_t inputOnlyPins[] = {34, 35, 36, 39};
  for (int i = 0; i < 4; i++) {
    if (!isPinInArray(inputOnlyPins[i], avoid, avoidLen)) {
      return inputOnlyPins[i];
    }
  }
#endif

  return 0; // No available pin (should never happen)
}

// Find multiple available pins
void findAvailablePins(uint8_t *out, int count, uint8_t *avoid, int avoidLen) {
  uint8_t tempAvoid[30];
  memcpy(tempAvoid, avoid, avoidLen * sizeof(uint8_t));
  int tempLen = avoidLen;

  for (int i = 0; i < count; i++) {
    out[i] = findAvailablePin(tempAvoid, tempLen);
    tempAvoid[tempLen++] = out[i]; // Add to avoid list for next search
  }
}

// Resolve pin conflicts when TFT display is enabled
// Resolve pin conflicts when TFT display is enabled
void resolveDisplayPinConflicts() {
  if (oledConfig.type != TFT_128X128) {
    return; // No conflicts with I2C OLED
  }

  Serial.println("=== TFT Display Mode - Checking Pin Conflicts ===");

  // Get TFT pins being used
  const int tftPinCount = 6;
  uint8_t tftPins[tftPinCount] = {
      systemConfig.tftCsPin,   systemConfig.tftDcPin,   systemConfig.tftRstPin,
      systemConfig.tftMosiPin, systemConfig.tftSclkPin, systemConfig.tftLedPin};

  bool conflictsFound = false;

  // Initialize avoid list with TFT pins
  // Max size: 6 (TFT) + 3 (Enc) + 10 (Btns) + 1 (LED) = 20 is tight, let's use
  // 32 safe buffer
  uint8_t avoid[32];
  int avoidLen = 0;

  // Add TFT pins to avoid list (ignoring 255/unused)
  for (int i = 0; i < tftPinCount; i++) {
    if (tftPins[i] != 255) {
      avoid[avoidLen++] = tftPins[i];
    }
  }

  // Helper to resolve a single pin
  auto resolvePin = [&](uint8_t &pin, const char *name) {
    if (pin == 255)
      return; // Ignore unused pins

    // Check if pin conflicts with ANY used pin so far (TFT + previously checked
    // items)
    if (isPinInArray(pin, avoid, avoidLen)) {
      uint8_t oldPin = pin;
      pin = findAvailablePin(
          avoid, avoidLen); // Find new pin avoiding all currently used
      Serial.printf("⚠️  %s conflict: GPIO %d → %d\n", name, oldPin, pin);
      conflictsFound = true;
    }
    // Add valid pin (whether old or newly assigned) to avoid list for
    // subsequent checks
    if (avoidLen < 32) {
      avoid[avoidLen++] = pin;
    }
  };

  // Resolve Encoder Pins Individually
  // This preserves user swaps/assignments unless they specifically conflict
  resolvePin(systemConfig.encoderA, "Encoder A");
  resolvePin(systemConfig.encoderB, "Encoder B");
  resolvePin(systemConfig.encoderBtn, "Encoder BTN");

  // Resolve Button Pins
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    char btnName[16];
    snprintf(btnName, sizeof(btnName), "Button #%d", i + 1);
    resolvePin(systemConfig.buttonPins[i], btnName);
  }

  // Resolve LED Pin
  resolvePin(systemConfig.ledPin, "LED Pin");

  // Resolve Analog Inputs (GPIO only)
  bool analogConflicts = false;
  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    AnalogInputConfig &cfg = analogInputs[i];
    if (cfg.enabled && cfg.source == AIN_SOURCE_GPIO) {
      char name[20];
      snprintf(name, sizeof(name), "Analog %d", i + 1);
      // Backup pin to check for change
      uint8_t oldPin = cfg.pin;
      resolvePin(cfg.pin, name);
      if (cfg.pin != oldPin) {
        analogConflicts = true;
        conflictsFound = true;
      }
    }
  }

  if (conflictsFound) {
    Serial.println("💾 Saving auto-resolved pin configuration...");
    saveSystemSettings(); // Save the resolved configuration
    if (analogConflicts) {
      saveAnalogInputs(); // Save resolved analog inputs
    }
  } else {
    Serial.println("✅ No pin conflicts detected");
  }
}

void setup() {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // Enable Native USB for CDC (Serial) and MIDI
  USB.begin();
  usbMidi.begin();
#endif

  Serial.begin(115200);
  delay(100); // Power stabilization delay
  Serial.println("\n\n=== CHOCOTONE MIDI Controller Firmware Starting ===");

  // Load Settings FIRST to determine OLED type
  Serial.println("Loading settings to determine hardware config...");
  loadSystemSettings();
  yield(); // Feed watchdog after NVS operations

  // Auto-resolve any pin conflicts between display and other peripherals
  resolveDisplayPinConflicts();

  // Initialize Display hardware communication
  if (oledConfig.type == TFT_128X128) {
    // TFT uses SPI - use configurable pins from systemConfig
    SPI.begin(systemConfig.tftSclkPin, -1, systemConfig.tftMosiPin,
              systemConfig.tftCsPin);
    Serial.printf("SPI initialized for TFT: SCLK=%d, MOSI=%d, CS=%d\n",
                  systemConfig.tftSclkPin, systemConfig.tftMosiPin,
                  systemConfig.tftCsPin);
  } else {
    // OLED uses I2C - use configurable pins from systemConfig
    Wire.begin(systemConfig.oledSdaPin, systemConfig.oledSclPin);
    Wire.setClock(100000);
    delay(50);
    Serial.printf("I2C initialized for OLED: SDA=%d, SCL=%d\n",
                  systemConfig.oledSdaPin, systemConfig.oledSclPin);
  }
  initDisplayHardware();

  // === Chocotone v1.5 BETA Loading Screen ===
  // Skip if no display configured
  if (displayPtr != nullptr && oledConfig.type != OLED_NONE) {
    int16_t x1, y1;
    uint16_t w, h;

    // Properly detect display type using oledConfig instead of height()
    bool is32 = (oledConfig.type == OLED_128X32);
    bool is128 = (oledConfig.type == TFT_128X128);

    if (is32) {
      // 128x32 Compact Layout
      displayPtr->setTextSize(1);
      displayPtr->setCursor(0, 10);
      displayPtr->print(F("CHOCOTONE"));
      displayPtr->setCursor(94, 10);
      displayPtr->print(F("v1.5b"));
    } else if (is128) {
      // 128x128 TFT Layout - matching editor OLED preview
      displayPtr->setTextSize(2);
      displayPtr->getTextBounds("CHOCOTONE", 0, 0, &x1, &y1, &w, &h);
      displayPtr->setCursor((128 - w) / 2, 35);
      displayPtr->print(F("CHOCOTONE"));

      // Subtitle - uppercase "BY" to match preview
      displayPtr->setTextSize(1);
      displayPtr->getTextBounds("MIDI BY ANDRE SOLIS", 0, 0, &x1, &y1, &w, &h);
      displayPtr->setCursor((128 - w) / 2, 60);
      displayPtr->print(F("MIDI BY ANDRE SOLIS"));

      // Version text at bottom (dots drawn by loading section later)
      displayPtr->setTextSize(1);
      displayPtr->setCursor(72, 106);
      displayPtr->print(F("V1.5B"));
    } else {
      // 128x64 Standard Layout
      // Title: "CHOCOTONE" - Size 2, Centered
      displayPtr->setTextSize(2);
      displayPtr->getTextBounds("CHOCOTONE", 0, 0, &x1, &y1, &w, &h);
      displayPtr->setCursor((128 - w) / 2, 10);
      displayPtr->print(F("CHOCOTONE"));

      // Subtitle: "MIDI by ANDRE SOLIS" - Size 1, Centered
      displayPtr->setTextSize(1);
      displayPtr->getTextBounds("MIDI by ANDRE SOLIS", 0, 0, &x1, &y1, &w, &h);
      displayPtr->setCursor((128 - w) / 2, 30);
      displayPtr->print(F("MIDI by ANDRE SOLIS"));

      // Version Badge - White rounded rect with black text
      displayPtr->fillRoundRect(94, 47, 28, 10, 2, DISPLAY_WHITE);
      displayPtr->setTextColor(DISPLAY_BLACK);
      displayPtr->setTextSize(1);
      displayPtr->setCursor(96, 48);
      displayPtr->print(F("v1.5b"));
      displayPtr->setTextColor(DISPLAY_WHITE); // Reset
    }

    // 8 Dots Grid - adapted for each display type
    if (is32) {
      // Single row for 32px
      for (int col = 0; col < 8; col++) {
        int x = 4 + col * 12 + 2;
        int y = 22;
        displayPtr->drawCircle(x, y, 2, DISPLAY_WHITE);
      }
    } else if (is128) {
      // 4 loading dots for 128x128, matching the static dots before V1.5B
      for (int col = 0; col < 4; col++) {
        int x = 35 + col * 8; // Same as static dots
        int y = 110;
        displayPtr->drawCircle(x, y, 2, DISPLAY_WHITE);
      }
    } else {
      // 2 rows x 4 cols for 64px
      for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
          int x = 4 + col * 8 + 2;
          int y = 45 + row * 8 + 2;
          displayPtr->drawCircle(x, y, 2, DISPLAY_WHITE);
        }
      }
    }
    flushDisplay();
  } // End of display-dependent loading screen code

  // Helper lambda to fill a loading dot (0-7 for OLED 64, 0-3 for TFT)
  // Safe to call even when display is NONE - just skips drawing
  bool is32 = (oledConfig.type == OLED_128X32);
  bool is128 = (oledConfig.type == TFT_128X128);
  auto fillLoadingDot = [is32, is128](int dotIndex) {
    if (displayPtr == nullptr || oledConfig.type == OLED_NONE)
      return;
    if (is32) {
      int x = 4 + dotIndex * 12 + 2;
      int y = 22;
      displayPtr->fillCircle(x, y, 2, DISPLAY_WHITE);
    } else if (is128) {
      // Only 4 dots for TFT, match static dot positions
      if (dotIndex < 4) {
        int x = 35 + dotIndex * 8;
        int y = 110;
        displayPtr->fillCircle(x, y, 2, DISPLAY_WHITE);
      }
    } else {
      int row = dotIndex / 4;
      int col = dotIndex % 4;
      int x = 4 + col * 8 + 2;
      int y = 45 + row * 8 + 2;
      displayPtr->fillCircle(x, y, 2, DISPLAY_WHITE);
    }
    flushDisplay();
  };

  // Dot 0: Display initialized
  fillLoadingDot(0);

  // Load Settings
  // Settings already loaded at start
  fillLoadingDot(1); // Dot 1: System settings loaded

  loadPresets();
  yield();           // Feed watchdog after SPIFFS operations
  fillLoadingDot(2); // Dot 2: Presets loaded

  loadCurrentPresetIndex(); // Restore last used preset

  // Load Analog Inputs (formerly Expression Pedals)
  loadAnalogInputs();
  setupAnalogInputs();

  // Initialize Encoder with resolved pins (may differ from ENCODER_*_PIN
  // macros)
  Serial.printf("Encoder Pins: A=%d, B=%d, BTN=%d\n", systemConfig.encoderA,
                systemConfig.encoderB, systemConfig.encoderBtn);
  encoder.attachHalfQuad(systemConfig.encoderA, systemConfig.encoderB);
  encoder.setCount(0);
  oldEncoderPosition = 0;
  pinMode(systemConfig.encoderBtn, INPUT_PULLUP);
  fillLoadingDot(3); // Dot 3: Encoder initialized

  // Initialize Buttons
  Serial.println("Initializing buttons...");
  Serial.printf("Button count: %d\n", systemConfig.buttonCount);
  Serial.print("Button pins: ");
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    Serial.printf("%d ", systemConfig.buttonPins[i]);
  }
  Serial.println();
  Serial.printf("TFT Pins - CS:%d DC:%d RST:%d MOSI:%d SCLK:%d LED:%d\n",
                TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_LED);
  Serial.printf("Encoder Pins - A:%d B:%d BTN:%d\n", systemConfig.encoderA,
                systemConfig.encoderB, systemConfig.encoderBtn);

  Serial.println("Setting pinMode for each button...");

  for (int i = 0; i < systemConfig.buttonCount; i++) {
    int pin = systemConfig.buttonPins[i];
    if (pin == 34 || pin == 35)
      pinMode(pin, INPUT);
    else
      pinMode(pin, INPUT_PULLUP);
    activeNotesOnButtonPins[i] = -1;
    buttonPinActive[i] = false;
    lastButtonPressTime_pads[i] = 0;
  }
  fillLoadingDot(4); // Dot 4: Buttons initialized

  // Initialize LEDs with resolved pin (may differ from compile-time
  // NEOPIXEL_PIN)
  // USB MIDI MODE: Skip LED init entirely - RMT peripheral conflicts with USB
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (systemConfig.bleMode == MIDI_USB_ONLY) {
    Serial.println("USB MIDI Mode: LEDs DISABLED (RMT/USB hardware conflict)");
  } else {
#endif
    Serial.println("Initializing LEDs...");
    Serial.printf("LED Pin: %d (resolved from potential conflicts)\n",
                  systemConfig.ledPin);

    // Reinitialize strip with the resolved pin
    strip.updateType(NEO_GRB + NEO_KHZ800);
    strip.updateLength(NUM_LEDS);
    strip.setPin(systemConfig.ledPin);
    strip.begin();
    strip.show();
    strip.setBrightness(ledBrightnessOn);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  }
#endif
  fillLoadingDot(5); // Dot 5: LEDs initialized (or skipped)

  // Initialize BLE (Client and/or Server based on bleMode)
  // USB MIDI MODE: Skip BLE entirely - we only need USB MIDI
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (systemConfig.bleMode == MIDI_USB_ONLY) {
    Serial.println("USB MIDI Mode: BLE DISABLED");
  } else {
#endif
    Serial.println("Initializing BLE...");
    setup_ble_midi();
    fillLoadingDot(6); // Dot 6: BLE initialized

    // Start BLE Scan for SPM (only if Client or Dual mode)
    if (systemConfig.bleMode == BLE_CLIENT_ONLY ||
        systemConfig.bleMode == BLE_DUAL_MODE) {
      Serial.println("Scanning for SPM...");
      startBleScan();
    }
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  }
#endif

  // Setup web server routes
  Serial.println("Setting up web server...");
  setup_web_server();
  fillLoadingDot(7); // Dot 7: Web server ready

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
  Serial.printf("BLE Mode: %s\n", systemConfig.bleMode == BLE_CLIENT_ONLY
                                      ? "CLIENT_ONLY (->SPM)"
                                  : systemConfig.bleMode == BLE_SERVER_ONLY
                                      ? "SERVER_ONLY (DAW->)"
                                      : "DUAL_MODE (DAW->ESP->SPM)");
  Serial.printf("WiFi: %s\n", isWifiOn ? "ON" : "OFF");
  Serial.printf("WiFi On at Boot: %s\n",
                systemConfig.wifiOnAtBoot ? "ENABLED" : "DISABLED");

  // Debug: Print button 4 action settings
  Serial.println("\n=== Button 4 (User Button 5) Action Settings ===");
  const ButtonConfig &debugBtn = buttonConfigs[currentPreset][4];
  Serial.printf("Button Name: %s, Message Count: %d\n", debugBtn.name,
                debugBtn.messageCount);
  for (int i = 0; i < debugBtn.messageCount; i++) {
    const ActionMessage &m = debugBtn.messages[i];
    Serial.printf("  Action %d: type=%d, channel=%d, data1=%d\n", m.action,
                  m.type, m.channel, m.data1);
  }
}

void loop() {
  // Handle WiFi
  if (isWifiOn) {
    server.handleClient();
    yield(); // Extra yield after handling client
  }

  handleEncoderButtonPress();

  // Handle Menu or Preset Mode
  if (currentMode == 1) {
    loop_menuMode();
  } else {
    // Show analog debug screen if enabled (dedicated screen mode)
    if (systemConfig.debugAnalogIn) {
      static unsigned long lastAnalogDebugRefresh = 0;
      if (millis() - lastAnalogDebugRefresh > 100) { // 10Hz refresh
        lastAnalogDebugRefresh = millis();
        displayAnalogDebug();
      }
      // Skip normal preset mode when in analog debug
    } else {
      loop_presetMode();

      // Update display to handle button name timeout
      // SKIP when WiFi is on to prevent crash
      if (!isWifiOn && buttonNameDisplayUntil > 0 &&
          millis() >= buttonNameDisplayUntil) {
        buttonNameDisplayUntil = 0;
        safeDisplayOLED();
      }
    }
  }

  // Update LEDs continuously (needed for tap tempo blink)
  // Skip when WiFi is on (heap too low) or when pending update will do it
  if (!isWifiOn && !pendingDisplayUpdate) {
    updateLeds();
  }

  // Handle deferred display updates from web interface
  if (pendingDisplayUpdate) {
    pendingDisplayUpdate = false;
    safeDisplayOLED();
    // Skip LED update if WiFi is on (NeoPixel disables interrupts, crashes
    // WiFi)
    if (!isWifiOn) {
      updateLeds();
    }
  }

  // Skip BLE operations when WiFi is on (already paused)
  if (!isWifiOn) {
    handleBleConnection();
    checkForSysex();

    // Apply SPM effect state to buttons if received and sync enabled
    if (spmStateReceived && presetSyncMode[currentPreset] != SYNC_NONE) {
      spmStateReceived = false;
      applySpmStateToButtons();
    }

    // Retry deferred state requests (from debounce in requestPresetState)
    extern volatile bool deferredStateRequest;
    if (deferredStateRequest && clientConnected) {
      deferredStateRequest = false;
      requestPresetState();
    }

    // Read expression pedals and send MIDI CC
    readAnalogInputs();

    // Throttled display refresh for analog loading bars (TFT only)
    // Check every 50ms if analog values changed significantly
    static unsigned long lastAnalogDisplayUpdate = 0;
    static float lastAnalogDisplayValues[MAX_ANALOG_INPUTS] = {0};
    if (oledConfig.type == TFT_128X128 && oledConfig.main.showColorStrips) {
      unsigned long now = millis();
      if (now - lastAnalogDisplayUpdate >= 50) {
        lastAnalogDisplayUpdate = now;
        bool needsRedraw = false;
        for (int i = 0;
             i < MAX_ANALOG_INPUTS && i < systemConfig.analogInputCount; i++) {
          if (analogInputs[i].enabled) {
            float diff =
                abs(analogInputs[i].smoothedValue - lastAnalogDisplayValues[i]);
            if (diff > 122) { // ~3% change threshold (4095 * 0.03)
              lastAnalogDisplayValues[i] = analogInputs[i].smoothedValue;
              needsRedraw = true;
            }
          }
        }
        if (needsRedraw) {
          updateAnalogColorStrips(); // Partial update - no flicker
        }
      }
    }

    // Periodic display refresh for battery icon and BLE status (every 5 sec)
    // This ensures battery % and connection state stay updated during idle
    static unsigned long lastPeriodicDisplayUpdate = 0;
    if (systemConfig.batteryAdcPin > 0 ||
        systemConfig.bleMode == BLE_CLIENT_ONLY ||
        systemConfig.bleMode == BLE_DUAL_MODE) {
      unsigned long now = millis();
      if (now - lastPeriodicDisplayUpdate >= 5000) {
        lastPeriodicDisplayUpdate = now;
        if (currentMode == 0 && buttonNameDisplayUntil == 0) {
          displayOLED(); // Refresh main screen
        }
      }
    }
  }

  // Handle serial commands for offline editor config transfer
  handleSerialConfig();

  // Handle Bluetooth Serial commands (wireless editor)
  handleBtSerialConfig();

  yield(); // Feed watchdog
}
