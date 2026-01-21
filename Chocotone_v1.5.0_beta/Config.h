#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================
// DEBUG FLAGS - Comment out to disable verbose logging
// ============================================
// #define DEBUG_INPUT  // Button press/release logging
// #define DEBUG_BLE    // BLE connection logging
// #define DEBUG_MIDI   // MIDI message logging

// Pin Definitions
#define ENCODER_A_PIN 18
#define ENCODER_B_PIN 19
#define ENCODER_BUTTON_PIN 23 // Restored from 5
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define NEOPIXEL_PIN 5 // Restored from 4

// Constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_HEIGHT_160 160 // For ST7735 1.8" TFT displays
#define OLED_RESET -1

// TFT SPI Pins (ST7735)
#define TFT_CS 15
#define TFT_RST 4
#define TFT_DC 2    // A0
#define TFT_MOSI 23 // SDA (Data)
#define TFT_SCLK 18 // SCL (Clock)
#define TFT_LED 32  // Backlight

// v2.0 Dynamic Button Configuration
#define MAX_BUTTONS 16                   // Maximum supported buttons
#define DEFAULT_BUTTON_COUNT 8           // Default active buttons
#define NUM_BUTTONS DEFAULT_BUTTON_COUNT // Backward compatibility
#define NUM_LEDS MAX_BUTTONS             // LEDs match max buttons
#define LONG_PRESS_DURATION 500
#define ENCODER_BUTTON_DEBOUNCE_DELAY                                          \
  100 // Increased from 50 for noise immunity

// Default Button Pins (verified correct: 14,27,26,25,33,32,16,17)
const uint8_t DEFAULT_BUTTON_PINS[MAX_BUTTONS] = {
    14, 27, 26, 25, 33, 32, 16, 17, 0, 0, 0, 0, 0, 0, 0, 0};

// Default Encoder Pins (original design - will auto-resolve conflicts with TFT)
#define DEFAULT_ENCODER_A 18
#define DEFAULT_ENCODER_B 19
#define DEFAULT_ENCODER_BTN 23

// Default Settings
// These are initial default values used on first boot.
// Users can change these via the web interface (http://192.168.4.1)
// Changed values are stored in NVS and persist across reboots.
//
// SECURITY NOTE: The default AP password is intentionally simple for initial
// setup. Users SHOULD change this password via the web interface after first
// configuration. The password must be at least 8 characters for WPA2 security.
#define DEFAULT_BLE_NAME "CHOCOTONE"
#define DEFAULT_AP_SSID "CHOCOTONE"
#define DEFAULT_AP_PASS "12345678" // Change this via web interface!

#endif
