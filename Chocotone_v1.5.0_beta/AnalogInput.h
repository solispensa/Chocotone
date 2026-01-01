#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

#include "Globals.h" // For ActionMessage struct
#include <Arduino.h>

// Signal conditioning constants
#define MAX_ANALOG_INPUTS 4
#define OVERSAMPLE_COUNT 64
#define DEFAULT_EMA_ALPHA 0.05f
#define DEFAULT_HYSTERESIS 3
#define ANALOG_READ_INTERVAL_MS 2 // Faster read (500Hz) for Piezo

// Input Modes
enum AnalogInputMode : uint8_t {
  AIN_MODE_POT = 0,
  AIN_MODE_PIEZO = 1,
  AIN_MODE_FSR = 2,
  AIN_MODE_SWITCH = 3
};

// Input Sources
enum AnalogInputSource : uint8_t { AIN_SOURCE_GPIO = 0, AIN_SOURCE_MUX = 1 };

// Action Types (Signal Processing Curve)
enum AnalogActionType : uint8_t {
  AIN_ACTION_LINEAR = 0,
  AIN_ACTION_LOG = 1,
  AIN_ACTION_EXP = 2,
  AIN_ACTION_JOYSTICK = 3
};

// Analog Input Configuration
struct AnalogInputConfig {
  // Configuration (saved to SPIFFS)
  bool enabled = false;
  AnalogInputSource source = AIN_SOURCE_GPIO;
  uint8_t pin = 34;     // GPIO or Mux channel
  char name[11] = "A1"; // Short name

  // Mode & Parameters
  AnalogInputMode inputMode = AIN_MODE_POT;

  // Signal Processing
  AnalogActionType actionType = AIN_ACTION_LINEAR;
  float curve = 2.0f;     // For LOG/EXP curves
  uint16_t center = 2048; // For Joystick mode
  uint8_t deadzone = 5;   // For Joystick mode %

  // LED Feedback
  uint8_t rgb[3] = {245, 158, 11}; // RGB color
  int16_t ledIndex = -1;           // Optional LED index (-1 = none)

  // Piezo / Peak Detection
  uint16_t piezoThreshold = 400;
  uint8_t piezoScanTime = 20;
  uint16_t piezoMaskTime = 30;

  // FSR / Threshold
  uint16_t fsrThreshold = 100;

  // Calibration (ADC Raw Values)
  uint16_t minVal = 0;    // Calibration minimum
  uint16_t maxVal = 4095; // Calibration maximum
  bool inverted = false;

  // Smoothing
  float emaAlpha = DEFAULT_EMA_ALPHA;
  uint8_t hysteresis = DEFAULT_HYSTERESIS;

  // Messages (List)
  uint8_t messageCount = 0;
  ActionMessage messages[4]; // Fixed max 4 messages per input

  // Runtime state (not saved)
  float smoothedValue = 0;
  uint16_t peakValue = 0;
  unsigned long peakStartTime = 0;
  unsigned long maskEndTime = 0;
  bool isInMask = false;
  bool isPeakScanning = false;

  uint8_t lastMidiValue = 255;
  unsigned long lastReadTime = 0;
  bool switchState = false; // For switch mode

  // Calibration state
  bool calibrating = false;
  uint16_t calMinSeen = 4095;
  uint16_t calMaxSeen = 0;
};

// Function declarations
void setupAnalogInputs();
void readAnalogInputs(); // Called from main loop()
void startCalibration(uint8_t index);
void stopCalibration(uint8_t index);
uint16_t readOversampled(uint8_t pin);
bool readMuxDigital(uint8_t channel);

// External array declaration
extern AnalogInputConfig analogInputs[MAX_ANALOG_INPUTS];

#endif
