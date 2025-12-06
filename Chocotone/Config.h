#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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
#define OLED_RESET -1
#define NUM_BUTTONS 8
#define NUM_LEDS 8
#define LONG_PRESS_DURATION 500
#define ENCODER_BUTTON_DEBOUNCE_DELAY 50

// Default Settings
// These are initial default values used on first boot.
// Users can change these via the web interface (http://192.168.4.1)
// Changed values are stored in NVS and persist across reboots.
//
// SECURITY NOTE: The default AP password is intentionally simple for initial setup.
// Users SHOULD change this password via the web interface after first configuration.
// The password must be at least 8 characters for WPA2 security.
#define DEFAULT_BLE_NAME "ESP32 MIDI Controller"
#define DEFAULT_AP_SSID "ESP32_MIDI_Config"
#define DEFAULT_AP_PASS "12345678"  // Change this via web interface!

#endif
