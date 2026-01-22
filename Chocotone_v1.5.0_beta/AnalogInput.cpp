#include "AnalogInput.h"
#include "BleMidi.h"
#include "Globals.h"
#include "Storage.h"
#include "SysexScrollData.h"
#include "UI_Display.h"
#include <Arduino.h>

// Global array
AnalogInputConfig analogInputs[MAX_ANALOG_INPUTS];

uint16_t readOversampled(uint8_t pin) {
  uint32_t sum = 0;
  for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
    sum += analogRead(pin);
  }
  return sum / OVERSAMPLE_COUNT;
}

uint16_t readMux(uint8_t channel) {
  if (!systemConfig.multiplexer.enabled)
    return 0;
  for (int i = 0; i < 4; i++) {
    digitalWrite(systemConfig.multiplexer.selectPins[i], (channel >> i) & 0x01);
  }
  delayMicroseconds(10); // Settle time
  return readOversampled(systemConfig.multiplexer.signalPin);
}

// Initialize analog input pins
void setupAnalogInputs() {
  Serial.println("Setting up analog inputs...");

  if (systemConfig.multiplexer.enabled) {
    for (int i = 0; i < 4; i++) {
      pinMode(systemConfig.multiplexer.selectPins[i], OUTPUT);
    }
    // If used for buttons, common pin needs pullup
    if (strstr(systemConfig.multiplexer.useFor, "buttons") != NULL ||
        strstr(systemConfig.multiplexer.useFor, "both") != NULL) {
      pinMode(systemConfig.multiplexer.signalPin, INPUT_PULLUP);
    }
  }

  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    AnalogInputConfig &cfg = analogInputs[i];
    if (cfg.enabled || systemConfig.debugAnalogIn) {
      if (cfg.source == AIN_SOURCE_GPIO) {
        if (cfg.pin >= 0 && cfg.pin <= 39) {
          pinMode(cfg.pin, INPUT);
          analogSetAttenuation(ADC_11db);
        }
      }

      // Initialize runtime state
      if (cfg.source == AIN_SOURCE_MUX) {
        cfg.smoothedValue = readMux(cfg.pin);
      } else {
        cfg.smoothedValue = readOversampled(cfg.pin);
      }
      cfg.lastMidiValue = 255;
      cfg.switchState = false;
      cfg.peakValue = 0;
      cfg.isPeakScanning = false;

      if (cfg.enabled) {
        Serial.printf("  A%d: GPIO%d, Mode=%d, Msgs=%d\n", i + 1, cfg.pin,
                      cfg.inputMode, cfg.messageCount);
      } else if (systemConfig.debugAnalogIn) {
        Serial.printf("  A%d: Debug Setup (Source=%d, Pin=%d)\n", i + 1,
                      cfg.source, cfg.pin);
      }
    }
  }
}

bool readMuxDigital(uint8_t channel) {
  if (!systemConfig.multiplexer.enabled)
    return HIGH;
  for (int i = 0; i < 4; i++) {
    digitalWrite(systemConfig.multiplexer.selectPins[i], (channel >> i) & 0x01);
  }
  delayMicroseconds(5); // Settle time (faster for digital)
  return digitalRead(systemConfig.multiplexer.signalPin);
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
    int outVal = value;
    if (cfg.inputMode == AIN_MODE_POT || cfg.inputMode == AIN_MODE_FSR) {
      // Map based on minOut/maxOut (v1.5)
      outVal = map(value, 0, 127, msg.minOut, msg.maxOut);
    } else if (cfg.inputMode == AIN_MODE_PIEZO) {
      outVal = map(velocity, 0, 127, msg.minOut, msg.maxOut);
    }
    outVal = constrain(outVal, 0, 127);

    switch (msg.type) {
    case CC:
      sendMidiCC(msg.channel, msg.data1, outVal);
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
    case SYSEX_SCROLL: {
      // Map analog value to list index
      SysexScrollParamId paramId =
          (SysexScrollParamId)msg.data1; // data1 = param ID
      const SysexScrollList *list = getSysexScrollList(paramId);
      if (list && list->msgCount > 0) {
        // Map outVal (0-127) to list index (0 to msgCount-1)
        int listIndex = map(outVal, 0, 127, 0, list->msgCount - 1);
        listIndex = constrain(listIndex, 0, list->msgCount - 1);

        // Get message from PROGMEM
        uint8_t msgLen = 0;
        const uint8_t *msgData =
            getSysexScrollMessage(paramId, listIndex, &msgLen);
        if (msgData && msgLen > 0) {
          // Copy from PROGMEM to RAM buffer
          uint8_t buffer[SYSEX_SCROLL_MSG_MAX_LEN];
          for (int j = 0; j < msgLen && j < SYSEX_SCROLL_MSG_MAX_LEN; j++) {
            buffer[j] = pgm_read_byte(msgData + j);
          }
          // Send SysEx (skip first 2 envelope bytes 0x80 0x80)
          if (msgLen > 2) {
            sendSysex(buffer + 2, msgLen - 2);
          }
        }
      }
      break;
    }
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

  // Apply Curves (v1.5)
  if (cfg.actionType == AIN_ACTION_LOG || cfg.actionType == AIN_ACTION_EXP) {
    float x = (float)mapped / 127.0f;
    float y = x;
    float k = cfg.curve;
    if (k > 0) {
      if (cfg.actionType == AIN_ACTION_LOG) {
        y = log(1.0f + k * x) / log(1.0f + k);
      } else {
        y = (exp(k * x) - 1.0f) / (exp(k) - 1.0f);
      }
    }
    mapped = (int)(y * 127.0f);
  } else if (cfg.actionType == AIN_ACTION_JOYSTICK) {
    int adc = (int)cfg.smoothedValue;
    int dz = (cfg.maxVal - cfg.minVal) *
             (cfg.deadzone / 200.0f); // Half for each side
    if (adc > cfg.center + dz) {
      mapped = map(adc, cfg.center + dz, cfg.maxVal, 64, 127);
    } else if (adc < cfg.center - dz) {
      mapped = map(adc, cfg.minVal, cfg.center - dz, 0, 64);
    } else {
      mapped = 64;
    }
  }

  // Hysteresis
  if (abs(mapped - (int)cfg.lastMidiValue) > cfg.hysteresis ||
      cfg.lastMidiValue == 255) {
    triggerAnalogActions(cfg, mapped, 0);
    cfg.lastMidiValue = mapped;

    // LED Feedback (v1.5)
    if (cfg.ledIndex < 10) { // Check if LED is assigned
      // Scale brightness based on value (0-127)
      uint8_t r = (uint16_t)cfg.rgb[0] * mapped / 127;
      uint8_t g = (uint16_t)cfg.rgb[1] * mapped / 127;
      uint8_t b = (uint16_t)cfg.rgb[2] * mapped / 127;
      updateIndividualLed(cfg.ledIndex, r, g, b);
    }
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
    if (!cfg.enabled && !systemConfig.debugAnalogIn)
      continue;

    unsigned long now = millis();
    if (now - cfg.lastReadTime < ANALOG_READ_INTERVAL_MS)
      continue;
    cfg.lastReadTime = now;

    uint16_t raw = (cfg.source == AIN_SOURCE_MUX) ? readMux(cfg.pin)
                                                  : readOversampled(cfg.pin);

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
