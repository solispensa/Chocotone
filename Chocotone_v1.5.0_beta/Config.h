#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================
// DEBUG FLAGS - Comment out to disable verbose logging
// ============================================
// #define DEBUG_INPUT  // Button press/release logging
// #define DEBUG_BLE    // BLE connection logging
// #define DEBUG_MIDI   // MIDI message logging

// ============================================
// COMMON CONSTANTS
// ============================================

// Constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_HEIGHT_160 160 // For ST7735 1.8" TFT displays
#define OLED_RESET -1

// v2.0 Dynamic Button Configuration
#define MAX_BUTTONS 16                   // Maximum supported buttons
#define DEFAULT_BUTTON_COUNT 8           // Default active buttons
#define NUM_BUTTONS DEFAULT_BUTTON_COUNT // Backward compatibility
#define NUM_LEDS MAX_BUTTONS             // LEDs match max buttons
#define LONG_PRESS_DURATION 500
#define ENCODER_BUTTON_DEBOUNCE_DELAY                                          \
  100 // Increased from 50 for noise immunity

// ============================================
// HARDWARE PROFILES
// ============================================

#if defined(CONFIG_IDF_TARGET_ESP32S3)
// ============================================
// ESP32-S3 DEFAULT PINOUT (Safe & WiFi-Compatible)
// Avoids: Strapping (0,45,46), Flash/PSRAM (26-32 + 33-37 for Octal)
// Uses: ADC1 for Analog/Battery to work with WiFi
// ============================================

// Encoder (Safe GPIOs)
#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 17
#define ENCODER_BUTTON_PIN 18

// I2C Display (OLED)
#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

// LED
#define NEOPIXEL_PIN 48

// TFT SPI Pins (FSPI)
#define TFT_CS 10
#define TFT_RST 14
#define TFT_DC 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_LED 15

// Default Button Pins (Optimized: 38-42, 21, 8, 9)
// Replaces 1/2 (UART) with 8/9 (formerly OLED)
const uint8_t DEFAULT_BUTTON_PINS[MAX_BUTTONS] = {38, 39, 40, 41, 42, 21, 8, 9,
                                                  0,  0,  0,  0,  0,  0,  0, 0};

// Default Configuration Constants
#define DEFAULT_ENCODER_A 16
#define DEFAULT_ENCODER_B 17
#define DEFAULT_ENCODER_BTN 18

#else
// ============================================
// STANDARD ESP32 DEFAULT PINOUT (Legacy)
// ============================================

// Pin Definitions
#define ENCODER_A_PIN 18
#define ENCODER_B_PIN 19
#define ENCODER_BUTTON_PIN 23 // Restored from 5
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define NEOPIXEL_PIN 5 // Restored from 4

// TFT SPI Pins (ST7735)
#define TFT_CS 15
#define TFT_RST 4
#define TFT_DC 2    // A0
#define TFT_MOSI 23 // SDA (Data)
#define TFT_SCLK 18 // SCL (Clock)
#define TFT_LED 32  // Backlight

// Default Button Pins (verified correct: 14,27,26,25,33,32,16,17)
const uint8_t DEFAULT_BUTTON_PINS[MAX_BUTTONS] = {
    14, 27, 26, 25, 33, 32, 16, 17, 0, 0, 0, 0, 0, 0, 0, 0};

// Default Encoder Pins (original design - will auto-resolve conflicts with TFT)
#define DEFAULT_ENCODER_A 18
#define DEFAULT_ENCODER_B 19
#define DEFAULT_ENCODER_BTN 23

#endif

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
