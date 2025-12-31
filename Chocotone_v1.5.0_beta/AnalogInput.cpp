#include "AnalogInput.h"
#include "BleMidi.h"
#include "Globals.h"
#include "Storage.h"
#include <Arduino.h>

// Global array
AnalogInputConfig analogInputs[MAX_ANALOG_INPUTS];

// Initialize analog input pins
void setupAnalogInputs() {
  Serial.println("Setting up analog inputs...");

  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    AnalogInputConfig &cfg = analogInputs[i];
    if (cfg.enabled && cfg.pin >= 32 && cfg.pin <= 39) {
      // Configure ADC
      pinMode(cfg.pin, INPUT);
      analogSetAttenuation(ADC_11db); // Full 0-3.3V range

      // Initialize runtime state
      cfg.smoothedValue = readOversampled(cfg.pin);
      cfg.lastMidiValue = 255;
      cfg.switchState = false;
      cfg.peakValue = 0;
      cfg.isPeakScanning = false;

      Serial.printf("  A%d: GPIO%d, Mode=%d, Msgs=%d\n", i + 1, cfg.pin,
                    cfg.inputMode, cfg.messageCount);
    }
  }
}

uint16_t readOversampled(uint8_t pin) {
  uint32_t sum = 0;
  for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
    sum += analogRead(pin);
  }
  return sum / OVERSAMPLE_COUNT;
}

// Logic to trigger actions based on value/velocity
void triggerAnalogActions(AnalogInputConfig &cfg, int value, int velocity) {
  int valuePct = map(value, 0, 127, 0, 100);

  for (int i = 0; i < cfg.messageCount; i++) {
    ActionMessage &msg = cfg.messages[i];

    // Check Range
    if (valuePct < msg.minInput || valuePct > msg.maxInput)
      continue;

    // Dispatch based on type
    // For Continuous (Pot/FSR): map value within range
    int outVal = value;
    if (cfg.inputMode == AIN_MODE_POT || cfg.inputMode == AIN_MODE_FSR) {
      // Re-map value relative to the zone?
      // Logic: If range is 50-100%, and input is 75%, that's 50% of the zone.
      // For now, simpler global mapping to 0-127 is safer unless specific
      // 'outMin'/'outMax' exist. We'll stick to passing 'value' (0-127)
      // directly.
      outVal = value;
    } else if (cfg.inputMode == AIN_MODE_PIEZO) {
      outVal = velocity; // Velocity is the value
    }

    switch (msg.type) {
    case CC:
      sendMidiCC(msg.data1, outVal, msg.channel);
      break;
    case NOTE_ON: // Piezo triggers Note On
      if (cfg.inputMode == AIN_MODE_PIEZO) {
        sendMidiNoteOn(msg.data1, outVal, msg.channel);
        // Note Off handled? Piezo usually needs short duration or immediate off
        // For now, we rely on receiving device or send Note Off after short
        // delay? Better: Send Note On with Velocity, then Note Off with 0
        // immediately? Drums usually ignore Note Off or use one-shot.
        sendMidiNoteOn(msg.data1, 0, msg.channel); // Immediate Note Off
      }
      break;
    // Add other types as needed
    default:
      break;
    }
  }
}

// Processing for Continuous Inputs (Pot, FSR)
void processContinuous(AnalogInputConfig &cfg, uint16_t raw) {
  // EMA Smoothing
  cfg.smoothedValue =
      (cfg.emaAlpha * raw) + ((1.0f - cfg.emaAlpha) * cfg.smoothedValue);

  // FSR Gate
  if (cfg.inputMode == AIN_MODE_FSR && cfg.smoothedValue < cfg.fsrThreshold) {
    cfg.smoothedValue = 0; // Silence noise
  }

  // Map to MIDI 0-127
  int mapped = map((int)cfg.smoothedValue, cfg.minVal, cfg.maxVal, 0, 127);
  if (cfg.inverted)
    mapped = 127 - mapped;
  mapped = constrain(mapped, 0, 127);

  // Hysteresis
  if (abs(mapped - (int)cfg.lastMidiValue) > cfg.hysteresis ||
      cfg.lastMidiValue == 255) {
    triggerAnalogActions(cfg, mapped, 0);
    cfg.lastMidiValue = mapped;
  }
}

// Processing for Piezo (Peak Detect)
void processPiezo(AnalogInputConfig &cfg, uint16_t raw) {
  unsigned long now = millis();

  // 1. Masking Window (Debounce)
  if (cfg.isInMask) {
    if (now > cfg.maskEndTime) {
      cfg.isInMask = false;
    } else {
      return; // Ignore signal during mask
    }
  }

  // 2. Threshold Check to start scan
  if (!cfg.isPeakScanning) {
    if (raw > cfg.piezoThreshold) {
      cfg.isPeakScanning = true;
      cfg.peakStartTime = now;
      cfg.peakValue = raw;
    }
  } else {
    // 3. Peak Scanning
    if (raw > cfg.peakValue) {
      cfg.peakValue = raw;
    }

    // Check if scan time expired
    if (now - cfg.peakStartTime >= cfg.piezoScanTime) {
      // Scan done - we have the peak!
      // Map peak to velocity (0-127) based on Threshold-MaxVal
      int velocity = map(cfg.peakValue, cfg.piezoThreshold, cfg.maxVal, 1, 127);
      velocity = constrain(velocity, 1, 127);

      // Trigger Actions
      triggerAnalogActions(cfg, velocity,
                           velocity); // Value=Velocity, Vel=Velocity

      // Enter Mask Phase
      cfg.isPeakScanning = false;
      cfg.isInMask = true;
      cfg.maskEndTime = now + cfg.piezoMaskTime;
    }
  }
}

// Processing for Switch
void processSwitch(AnalogInputConfig &cfg, uint16_t raw) {
  // Simple threshold at 50%
  bool newState = (raw > 2048);

  if (newState != cfg.switchState) {
    cfg.switchState = newState;
    // Trigger? Could send CC 127 on press, 0 on release
    // Re-use triggerAnalogActions?
    // If Switch is ON (Press), trigger with val 127.
    // If Switch is OFF (Release), trigger with val 0.
    triggerAnalogActions(cfg, newState ? 127 : 0, 0);
  }
}

void readAnalogInputs() {
  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    AnalogInputConfig &cfg = analogInputs[i];
    if (!cfg.enabled)
      continue;

    unsigned long now = millis();
    if (now - cfg.lastReadTime < ANALOG_READ_INTERVAL_MS)
      continue;
    cfg.lastReadTime = now;

    uint16_t raw = readOversampled(cfg.pin);

    // Debug: Print readings every 500ms
    static unsigned long lastDebugPrint = 0;
    if (now - lastDebugPrint > 500) {
      Serial.printf(
          "Analog Input %d (%s on GPIO%d): RAW=%d, SMOOTH=%d, MIDI=%d\n", i + 1,
          cfg.name, cfg.pin, raw, (int)cfg.smoothedValue, cfg.lastMidiValue);
      if (i == MAX_ANALOG_INPUTS - 1)
        lastDebugPrint = now;
    }

    switch (cfg.inputMode) {
    case AIN_MODE_PIEZO:
      processPiezo(cfg, raw);
      break;
    case AIN_MODE_SWITCH:
      processSwitch(cfg, raw);
      break;
    case AIN_MODE_POT:
    case AIN_MODE_FSR:
    default:
      processContinuous(cfg, raw);
      break;
    }
  }
}

void startCalibration(uint8_t index) {
  if (index < MAX_ANALOG_INPUTS) {
    analogInputs[index].calibrating = true;
    analogInputs[index].calMinSeen = 4095;
    analogInputs[index].calMaxSeen = 0;
  }
}

void stopCalibration(uint8_t index) {
  if (index < MAX_ANALOG_INPUTS && analogInputs[index].calibrating) {
    analogInputs[index].calibrating = false;
    if (analogInputs[index].calMaxSeen - analogInputs[index].calMinSeen > 100) {
      analogInputs[index].minVal = analogInputs[index].calMinSeen;
      analogInputs[index].maxVal = analogInputs[index].calMaxSeen;
      saveAnalogInputs(); // Save immediately
    }
  }
}
