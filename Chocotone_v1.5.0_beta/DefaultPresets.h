#ifndef DEFAULT_PRESETS_H
#define DEFAULT_PRESETS_H

#include "Globals.h"

// ============================================
// v3.0 ACTION-BASED FACTORY PRESETS
// ============================================
//
// Each button uses messages[] array with ActionType:
//   ACTION_PRESS     - Primary press action
//   ACTION_2ND_PRESS - Alternate press (toggle)
//   ACTION_COMBO     - When pressed with partner
//   ACTION_LONG_PRESS, ACTION_RELEASE, ACTION_DOUBLE_TAP
//
// ============================================

// Helper: Set a CC toggle button (PRESS sends 127, 2ND_PRESS sends 0)
void setToggleCC(ButtonConfig &btn, const char *name, uint8_t cc, uint8_t r,
                 uint8_t g, uint8_t b) {
  strncpy(btn.name, name, 20);
  btn.name[20] = '\0';
  btn.ledMode = LED_TOGGLE;
  btn.inSelectionGroup = false;
  btn.messageCount = 2;
  btn.isAlternate = false;

  // PRESS: CC on
  btn.messages[0].action = ACTION_PRESS;
  btn.messages[0].type = CC;
  btn.messages[0].channel = 1;
  btn.messages[0].data1 = cc;
  btn.messages[0].data2 = 127;
  btn.messages[0].rgb[0] = r;
  btn.messages[0].rgb[1] = g;
  btn.messages[0].rgb[2] = b;

  // 2ND_PRESS: CC off
  btn.messages[1].action = ACTION_2ND_PRESS;
  btn.messages[1].type = CC;
  btn.messages[1].channel = 1;
  btn.messages[1].data1 = cc;
  btn.messages[1].data2 = 0;
  btn.messages[1].rgb[0] = r;
  btn.messages[1].rgb[1] = g;
  btn.messages[1].rgb[2] = b;
}

// Helper: Set a program select button (PRESS sends CC with specific value)
void setProgramSelect(ButtonConfig &btn, const char *name, uint8_t value,
                      uint8_t r, uint8_t g, uint8_t b) {
  strncpy(btn.name, name, 20);
  btn.name[20] = '\0';
  btn.ledMode = LED_MOMENTARY;
  btn.inSelectionGroup = false;
  btn.messageCount = 1;
  btn.isAlternate = false;

  btn.messages[0].action = ACTION_PRESS;
  btn.messages[0].type = CC;
  btn.messages[0].channel = 1;
  btn.messages[0].data1 = 1; // CC#1 for program select
  btn.messages[0].data2 = value;
  btn.messages[0].rgb[0] = r;
  btn.messages[0].rgb[1] = g;
  btn.messages[0].rgb[2] = b;
}

// Helper: Add COMBO action to existing button
void addCombo(ButtonConfig &btn, int8_t partner, MidiCommandType type) {
  if (btn.messageCount < MAX_ACTIONS_PER_BUTTON) {
    ActionMessage &msg = btn.messages[btn.messageCount++];
    msg.action = ACTION_COMBO;
    msg.type = type;
    msg.channel = 1;
    msg.data1 = 0;
    msg.data2 = 0;
    msg.combo.partner = partner;
    msg.label[0] = '\0';
  }
}

// Helper: Add LONG_PRESS action to existing button
void addLongPress(ButtonConfig &btn, MidiCommandType type, uint16_t holdMs) {
  if (btn.messageCount < MAX_ACTIONS_PER_BUTTON) {
    ActionMessage &msg = btn.messages[btn.messageCount++];
    msg.action = ACTION_LONG_PRESS;
    msg.type = type;
    msg.channel = 1;
    msg.data1 = 0;
    msg.data2 = 0;
    msg.longPress.holdMs = holdMs;
    msg.label[0] = '\0';
  }
}

void loadFactoryPresets() {
  // ========================================
  // MINIMAL EMPTY CONFIG
  // Load your own config via the editor
  // ========================================

  // Default to 4 presets (can be changed via editor)
  presetCount = 4;

  // Initialize all buttons as empty (no actions)
  for (int p = 0; p < CHOCO_MAX_PRESETS; p++) {
    for (int b = 0; b < MAX_BUTTONS; b++) {
      ButtonConfig &btn = buttonConfigs[p][b];
      memset(&btn, 0, sizeof(ButtonConfig));
      snprintf(btn.name, 20, "BTN%d", b + 1);
      btn.ledMode = LED_MOMENTARY;
      btn.inSelectionGroup = false;
      btn.messageCount = 0;
      btn.isAlternate = false;
    }
  }

  // Empty preset names
  for (int p = 0; p < CHOCO_MAX_PRESETS; p++) {
    snprintf(presetNames[p], 20, "Preset %d", p + 1);
    presetLedModes[p] = PRESET_LED_NORMAL;
    presetSyncMode[p] = SYNC_NONE;
  }

  // Clear global special actions
  for (int i = 0; i < MAX_BUTTONS; i++) {
    globalSpecialActions[i].hasCombo = false;
    globalSpecialActions[i].partner = -1;
    memset(&globalSpecialActions[i].comboAction, 0, sizeof(ActionMessage));
  }

  // Set minimal system config (no OLED by default to avoid I2C errors)
  // OLED type OLED_NONE would prevent initialization, but we'll use a flag
  // For now, keep the OLED config but user should upload their own config
  // via editor to set proper display type based on their hardware
}

#endif
