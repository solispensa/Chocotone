#include "WebInterface.h"
#include "AnalogInput.h"
#include "BleMidi.h"
#include "BluetoothSerial.h"
#include "Storage.h"
#include "UI_Display.h"
#include <ArduinoJson.h>

// Bluetooth Serial (SPP) for wireless editor connection
BluetoothSerial SerialBT;

// Minimal WiFi page - just import/export (full editing via
// offline_editor_v2.html + Serial)
const char MINIMAL_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Chocotone Config</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui;background:#121212;color:#e0e0e0;padding:20px;max-width:600px;margin:0 auto}
h1{color:#bb86fc;text-align:center;margin-bottom:20px;font-size:1.5em}
.info{background:#1e1e1e;padding:15px;border-radius:8px;margin-bottom:15px;font-size:0.9em;color:#888}
.btns{display:flex;gap:10px;flex-wrap:wrap;margin-bottom:15px}
button{flex:1;padding:12px;border:none;border-radius:8px;font-size:1em;cursor:pointer;min-width:120px}
.export{background:#4CAF50;color:#fff}
.import{background:#2196F3;color:#fff}
.save{background:#bb86fc;color:#000;font-weight:bold}
textarea{width:100%;height:200px;background:#1e1e1e;color:#e0e0e0;border:1px solid #333;border-radius:8px;padding:10px;font-family:monospace;font-size:12px;resize:vertical}
input[type=file]{display:none}
.status{text-align:center;padding:10px;margin-top:10px;border-radius:8px}
.ok{background:#1b5e20;color:#fff}
.err{background:#b71c1c;color:#fff}
</style></head><body>
<h1>Chocotone Config</h1>
<div class='info'>
  <b>Quick Import/Export</b><br>
  For full editing, use <b>chocotone_midi_editor.html</b> with USB Serial connection.<br>
  <span style='color:#ffab40'>⚠ BLE is disabled while WiFi is active.</span>
</div>
<div class='btns'>
  <button class='export' onclick='doExport()'>Export Config</button>
  <button class='import' onclick='document.getElementById("fileIn").click()'>Import File</button>
  <input type='file' id='fileIn' accept='.json' onchange='loadFile(this)'>
</div>
<textarea id='json' placeholder='Config JSON will appear here...'></textarea>
<div class='btns'>
  <button class='save' onclick='doSave()'>Save to Device</button>
</div>
<div id='status'></div>
<script>
function status(msg,ok){var s=document.getElementById('status');s.textContent=msg;s.className='status '+(ok?'ok':'err');setTimeout(()=>s.textContent='',5000);}
function doExport(){fetch('/export').then(r=>r.text()).then(d=>{document.getElementById('json').value=d;status('Exported!',true);}).catch(e=>status('Export failed: '+e,false));}
function loadFile(inp){var f=inp.files[0];if(!f)return;var r=new FileReader();r.onload=e=>{document.getElementById('json').value=e.target.result;status('File loaded. Click Save to apply.',true);};r.readAsText(f);}
function doSave(){var json=document.getElementById('json').value;if(!json){status('No config to save',false);return;}
try{JSON.parse(json);}catch(e){status('Invalid JSON: '+e.message,false);return;}
var blob=new Blob([json],{type:'application/json'});var fd=new FormData();fd.append('config',blob,'config.json');
var xhr=new XMLHttpRequest();xhr.open('POST','/import',true);xhr.timeout=30000;
xhr.onreadystatechange=function(){if(xhr.readyState===4){if(xhr.status===200){status('Saved! Device rebooting...',true);}else if(xhr.status===400){status('Error: '+xhr.responseText,false);}else{status('Device rebooting...',true);}}};
xhr.onerror=function(){status('Device rebooting...',true);};xhr.send(fd);}
window.onload=doExport;
</script></body></html>
)rawliteral";

// Request throttling to prevent crashes from concurrent or rapid requests
static volatile bool requestInProgress = false;
static unsigned long lastRequestTime = 0;
const unsigned long MIN_REQUEST_INTERVAL =
    500; // Minimum 500ms between requests

const char *getCommandTypeName(MidiCommandType t) {
  switch (t) {
  case NOTE_MOMENTARY:
    return "Note (Momentary)";
  case NOTE_ON:
    return "Note On";
  case NOTE_OFF:
    return "Note Off";
  case CC:
    return "CC";
  case PC:
    return "PC";
  case SYSEX:
    return "SysEx";
  case TAP_TEMPO:
    return "Tap Tempo";
  case PRESET_UP:
    return "Preset Up";
  case PRESET_DOWN:
    return "Preset Down";
  case PRESET_1:
    return "Preset 1";
  case PRESET_2:
    return "Preset 2";
  case PRESET_3:
    return "Preset 3";
  case PRESET_4:
    return "Preset 4";
  case CLEAR_BLE_BONDS:
    return "Clear BLE Bonds";
  case WIFI_TOGGLE:
    return "WiFi Toggle";
  default:
    return "Off";
  }
}
const char *getCommandTypeString(MidiCommandType t) {
  switch (t) {
  case NOTE_MOMENTARY:
    return "NOTE_MOMENTARY";
  case NOTE_ON:
    return "NOTE_ON";
  case NOTE_OFF:
    return "NOTE_OFF";
  case CC:
    return "CC";
  case PC:
    return "PC";
  case SYSEX:
    return "SYSEX";
  case TAP_TEMPO:
    return "TAP_TEMPO";
  case PRESET_UP:
    return "PRESET_UP";
  case PRESET_DOWN:
    return "PRESET_DOWN";
  case PRESET_1:
    return "PRESET_1";
  case PRESET_2:
    return "PRESET_2";
  case PRESET_3:
    return "PRESET_3";
  case PRESET_4:
    return "PRESET_4";
  case CLEAR_BLE_BONDS:
    return "CLEAR_BLE_BONDS";
  case WIFI_TOGGLE:
    return "WIFI_TOGGLE";
  default:
    return "OFF";
  }
}
MidiCommandType parseCommandType(String s) {
  if (s == "NOTE_MOMENTARY")
    return NOTE_MOMENTARY;
  if (s == "NOTE_ON")
    return NOTE_ON;
  if (s == "NOTE_OFF")
    return NOTE_OFF;
  if (s == "CC")
    return CC;
  if (s == "PC")
    return PC;
  if (s == "SYSEX")
    return SYSEX;
  if (s == "TAP_TEMPO")
    return TAP_TEMPO;
  if (s == "PRESET_UP")
    return PRESET_UP;
  if (s == "PRESET_DOWN")
    return PRESET_DOWN;
  if (s == "PRESET_1")
    return PRESET_1;
  if (s == "PRESET_2")
    return PRESET_2;
  if (s == "PRESET_3")
    return PRESET_3;
  if (s == "PRESET_4")
    return PRESET_4;
  if (s == "CLEAR_BLE_BONDS")
    return CLEAR_BLE_BONDS;
  if (s == "WIFI_TOGGLE")
    return WIFI_TOGGLE;
  return MIDI_OFF;
}

void rgbToHex(char *buffer, size_t size, const byte rgb[3]) {
  snprintf(buffer, size, "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
}
void hexToRgb(const String &hex, byte rgb[3]) {
  long c = strtol(hex.substring(1).c_str(), NULL, 16);
  rgb[0] = (c >> 16) & 0xFF;
  rgb[1] = (c >> 8) & 0xFF;
  rgb[2] = c & 0xFF;
}

String escapeJson(const String &s) {
  String res = "";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"')
      res += "\\\"";
    else if (c == '\\')
      res += "\\\\";
    else if (c == '\b')
      res += "\\b";
    else if (c == '\f')
      res += "\\f";
    else if (c == '\n')
      res += "\\n";
    else if (c == '\r')
      res += "\\r";
    else if (c == '\t')
      res += "\\t";
    else if (c < 32) {
      char buf[16];
      snprintf(buf, sizeof(buf), "\\u%04x", c);
      res += buf;
    } else
      res += c;
  }
  return res;
}

// Find an action by type in ButtonConfig.messages[] (returns nullptr if not
// found)
ActionMessage *findAction(const ButtonConfig &cfg, ActionType actionType) {
  for (int i = 0; i < cfg.messageCount && i < MAX_ACTIONS_PER_BUTTON; i++) {
    if (cfg.messages[i].action == actionType) {
      return const_cast<ActionMessage *>(&cfg.messages[i]);
    }
  }
  return nullptr;
}

// Find or create an action by type (adds new action if not found and space
// available)
ActionMessage *findOrCreateAction(ButtonConfig &cfg, ActionType actionType) {
  // First try to find existing
  for (int i = 0; i < cfg.messageCount && i < MAX_ACTIONS_PER_BUTTON; i++) {
    if (cfg.messages[i].action == actionType) {
      return &cfg.messages[i];
    }
  }
  // Not found - add new if space available
  if (cfg.messageCount < MAX_ACTIONS_PER_BUTTON) {
    ActionMessage *msg = &cfg.messages[cfg.messageCount];
    memset(msg, 0, sizeof(ActionMessage));
    msg->action = actionType;
    msg->type = MIDI_OFF;
    msg->channel = 1;
    cfg.messageCount++;
    return msg;
  }
  return nullptr; // No space
}

// Remove an action by type (shifts remaining actions down)
void removeAction(ButtonConfig &cfg, ActionType actionType) {
  for (int i = 0; i < cfg.messageCount; i++) {
    if (cfg.messages[i].action == actionType) {
      // Shift remaining actions down
      for (int j = i; j < cfg.messageCount - 1; j++) {
        cfg.messages[j] = cfg.messages[j + 1];
      }
      cfg.messageCount--;
      return;
    }
  }
}

void sendOptions(String &out, MidiCommandType currentType) {
  const char *types[] = {"OFF",
                         "NOTE_MOMENTARY",
                         "NOTE_ON",
                         "NOTE_OFF",
                         "CC",
                         "PC",
                         "SYSEX",
                         "TAP_TEMPO",
                         "PRESET_UP",
                         "PRESET_DOWN",
                         "PRESET_1",
                         "PRESET_2",
                         "PRESET_3",
                         "PRESET_4",
                         "CLEAR_BLE_BONDS",
                         "WIFI_TOGGLE"};
  const char *names[] = {
      "Off",      "Note (Mom)", "Note On",         "Note Off",
      "CC",       "PC",         "SysEx",           "Tap Tempo",
      "Preset +", "Preset -",   "Preset 1",        "Preset 2",
      "Preset 3", "Preset 4",   "Clear BLE Bonds", "WiFi Toggle"};
  const MidiCommandType typeEnums[] = {
      MIDI_OFF,   NOTE_MOMENTARY, NOTE_ON,   NOTE_OFF,  CC,
      PC,         SYSEX,          TAP_TEMPO, PRESET_UP, PRESET_DOWN,
      PRESET_1,   PRESET_2,       PRESET_3,  PRESET_4,  CLEAR_BLE_BONDS,
      WIFI_TOGGLE};

  for (int i = 0; i < 16; i++) {
    char buf[128];
    snprintf(buf, sizeof(buf), "<option value='%s'%s>%s</option>", types[i],
             (typeEnums[i] == currentType ? " selected" : ""), names[i]);
    out += buf;
  }
}

// Send HTML fields for an ActionMessage (simplified for online editor)
void sendActionFields(String &out, const char *id, const ActionMessage *msg) {
  char buf[256];

  // Get RGB color hex
  char hex[8] = "#bb86fc";
  if (msg) {
    snprintf(hex, sizeof(hex), "#%02x%02x%02x", msg->rgb[0], msg->rgb[1],
             msg->rgb[2]);
  }

  // Type Select
  out += F("<div class='f'><label>Type:</label><select name='");
  out += id;
  out += F("_type' onchange='toggleFields(this)'>");
  sendOptions(out, msg ? msg->type : MIDI_OFF);
  out += F("</select></div>");

  // Channel
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>Ch:</label><input type='number' name='%s_ch' "
           "min='1' max='16' value='%d'></div>",
           id, msg ? msg->channel : 1);
  out += buf;

  // Data 1
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>D1:</label><input type='number' name='%s_d1' "
           "min='0' max='127' value='%d'></div>",
           id, msg ? msg->data1 : 0);
  out += buf;

  // Data 2
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>D2:</label><input type='number' name='%s_d2' "
           "min='0' max='127' value='%d'></div>",
           id, msg ? msg->data2 : 0);
  out += buf;

  // Tap Tempo fields (shown via JS when TAP_TEMPO selected)
  int8_t safePrev = msg ? msg->tapTempo.rhythmPrev : 0;
  int8_t safeNext = msg ? msg->tapTempo.rhythmNext : 4;
  int8_t safeLock = msg ? msg->tapTempo.tapLock : 7;
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>R.Prev:</label><input type='number' "
           "name='%s_rprev' min='0' max='9' value='%d'></div>",
           id, safePrev);
  out += buf;
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>R.Next:</label><input type='number' "
           "name='%s_rnext' min='0' max='9' value='%d'></div>",
           id, safeNext);
  out += buf;
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>Lock:</label><input type='number' "
           "name='%s_lock' min='0' max='9' value='%d'></div>",
           id, safeLock);
  out += buf;

  // Color
  snprintf(buf, sizeof(buf),
           "<div class='f'><label>RGB:</label><input type='color' "
           "name='%s_rgb' value='%s'></div>",
           id, hex);
  out += buf;
}

void handleRoot() {
  // Request throttling: prevent concurrent or rapid requests from crashing
  unsigned long now = millis();
  if (requestInProgress) {
    server.send(200, "text/html",
                "<html><head><meta http-equiv='refresh' content='1'></head>"
                "<body "
                "style='background:#121212;color:#e0e0e0;text-align:center;"
                "padding:50px'>"
                "<p>Loading... please wait</p></body></html>");
    return;
  }
  if (now - lastRequestTime < MIN_REQUEST_INTERVAL) {
    server.send(200, "text/html",
                "<html><head><meta http-equiv='refresh' content='1'></head>"
                "<body "
                "style='background:#121212;color:#e0e0e0;text-align:center;"
                "padding:50px'>"
                "<p>Please wait...</p></body></html>");
    return;
  }

  requestInProgress = true;
  lastRequestTime = now;

  // Memory safety: yield and check heap before serving
  yield();
  int freeHeap = ESP.getFreeHeap();
  Serial.printf("handleRoot: Free heap = %d bytes\n", freeHeap);

  // If heap is critically low (<15KB), show a simple error page
  if (freeHeap < 15000) {
    char errorPage[256];
    snprintf(errorPage, sizeof(errorPage),
             "<html><body "
             "style='background:#121212;color:#e0e0e0;text-align:center;"
             "padding:50px'>"
             "<h2 style='color:#ff6b6b'>Low Memory</h2>"
             "<p>Free heap: %d bytes</p>"
             "<p>Turn WiFi off and on again via OLED menu.</p>"
             "<a href='/' style='color:#bb86fc'>Retry</a>"
             "</body></html>",
             freeHeap);
    server.send(200, "text/html", errorPage);
    requestInProgress = false;
    return;
  }

  // Serve minimal import/export page (full editing via offline_editor_v2.html +
  // Serial)
  server.send_P(200, "text/html", MINIMAL_PAGE);

  // Release request lock
  requestInProgress = false;
}

void handleSave() {
  bool changed = false;
  int preset = server.hasArg("preset") ? server.arg("preset").toInt() : 0;

  if (server.hasArg("name")) {
    String newName = server.arg("name");
    if (strncmp(presetNames[preset], newName.c_str(), 20) != 0) {
      strncpy(presetNames[preset], newName.c_str(), 20);
      presetNames[preset][20] = '\0';
      changed = true;
    }
  }

  // Preset LED Mode
  if (server.hasArg("presetLedMode")) {
    PresetLedMode newMode = (PresetLedMode)server.arg("presetLedMode").toInt();
    if (presetLedModes[preset] != newMode) {
      presetLedModes[preset] = newMode;
      changed = true;
    }
  }

  yield(); // Allow WDT to reset

  for (int t = 0; t < systemConfig.buttonCount; t++) {
    char id[10];
    snprintf(id, sizeof(id), "b%d", t);
    ButtonConfig &config = buttonConfigs[preset][t];

    if (server.hasArg(String(id) + "_n")) {
      String newName = server.arg(String(id) + "_n");
      if (strncmp(config.name, newName.c_str(), 20) != 0) {
        strncpy(config.name, newName.c_str(), 20);
        config.name[20] = '\0';
        changed = true;
      }
    }

    // Message A - only process if the fields exist in the form (main view)
    char ida[15];
    snprintf(ida, sizeof(ida), "%s_a", id);
    bool newIsAlternate = config.isAlternate; // Keep existing value by default
    if (server.hasArg(
            String(ida) +
            "_type")) { // Only update if Message A fields exist (main view)
      // Alt checkbox is only in main view, so update it here
      newIsAlternate = server.hasArg(String(id) + "_alt");
      if (config.isAlternate != newIsAlternate) {
        config.isAlternate = newIsAlternate;
        changed = true;
      }

      // LED Mode
      if (server.hasArg(String(id) + "_ledMode")) {
        String ledModeStr = server.arg(String(id) + "_ledMode");
        LedMode newLedMode =
            (ledModeStr == "TOGGLE") ? LED_TOGGLE : LED_MOMENTARY;
        if (config.ledMode != newLedMode) {
          config.ledMode = newLedMode;
          changed = true;
        }
      }

      // Selection Group (for Hybrid mode)
      bool newInSelGroup = server.hasArg(String(id) + "_selGroup");
      if (config.inSelectionGroup != newInSelGroup) {
        config.inSelectionGroup = newInSelGroup;
        changed = true;
      }

      // --- PRESS Action (Primary Message A) ---
      MidiCommandType newTypeA =
          parseCommandType(server.arg(String(ida) + "_type"));
      ActionMessage *pressMsg = findOrCreateAction(config, ACTION_PRESS);
      if (pressMsg) {
        byte newChA = server.arg(String(ida) + "_ch").toInt();
        byte newD1A = server.arg(String(ida) + "_d1").toInt();
        byte newD2A = server.arg(String(ida) + "_d2").toInt();
        byte newRgbA[3];
        hexToRgb(server.arg(String(ida) + "_rgb"), newRgbA);

        if (pressMsg->type != newTypeA) {
          pressMsg->type = newTypeA;
          changed = true;
        }
        if (pressMsg->channel != newChA) {
          pressMsg->channel = newChA;
          changed = true;
        }
        if (pressMsg->data1 != newD1A) {
          pressMsg->data1 = newD1A;
          changed = true;
        }
        if (pressMsg->data2 != newD2A) {
          pressMsg->data2 = newD2A;
          changed = true;
        }

        // Tap Tempo control buttons (stored in tapTempo union member)
        int8_t newRPrevA = server.arg(String(ida) + "_rprev").toInt();
        int8_t newRNextA = server.arg(String(ida) + "_rnext").toInt();
        int8_t newLockA = server.arg(String(ida) + "_lock").toInt();
        if (pressMsg->tapTempo.rhythmPrev != newRPrevA) {
          pressMsg->tapTempo.rhythmPrev = newRPrevA;
          changed = true;
        }
        if (pressMsg->tapTempo.rhythmNext != newRNextA) {
          pressMsg->tapTempo.rhythmNext = newRNextA;
          changed = true;
        }
        if (pressMsg->tapTempo.tapLock != newLockA) {
          pressMsg->tapTempo.tapLock = newLockA;
          changed = true;
        }

        if (memcmp(pressMsg->rgb, newRgbA, 3) != 0) {
          memcpy(pressMsg->rgb, newRgbA, 3);
          changed = true;
        }
      }
    }

    // --- 2ND_PRESS Action (Alternate Message B) ---
    char idb[15];
    snprintf(idb, sizeof(idb), "%s_b", id);
    if (newIsAlternate && server.hasArg(String(idb) + "_type")) {
      MidiCommandType newTypeB =
          parseCommandType(server.arg(String(idb) + "_type"));
      ActionMessage *altMsg = findOrCreateAction(config, ACTION_2ND_PRESS);
      if (altMsg) {
        byte newChB = server.arg(String(idb) + "_ch").toInt();
        byte newD1B = server.arg(String(idb) + "_d1").toInt();
        byte newD2B = server.arg(String(idb) + "_d2").toInt();
        byte newRgbB[3];
        hexToRgb(server.arg(String(idb) + "_rgb"), newRgbB);

        if (altMsg->type != newTypeB) {
          altMsg->type = newTypeB;
          changed = true;
        }
        if (altMsg->channel != newChB) {
          altMsg->channel = newChB;
          changed = true;
        }
        if (altMsg->data1 != newD1B) {
          altMsg->data1 = newD1B;
          changed = true;
        }
        if (altMsg->data2 != newD2B) {
          altMsg->data2 = newD2B;
          changed = true;
        }

        // Tap Tempo control buttons
        int8_t newRPrevB = server.arg(String(idb) + "_rprev").toInt();
        int8_t newRNextB = server.arg(String(idb) + "_rnext").toInt();
        int8_t newLockB = server.arg(String(idb) + "_lock").toInt();
        if (altMsg->tapTempo.rhythmPrev != newRPrevB) {
          altMsg->tapTempo.rhythmPrev = newRPrevB;
          changed = true;
        }
        if (altMsg->tapTempo.rhythmNext != newRNextB) {
          altMsg->tapTempo.rhythmNext = newRNextB;
          changed = true;
        }
        if (altMsg->tapTempo.tapLock != newLockB) {
          altMsg->tapTempo.tapLock = newLockB;
          changed = true;
        }

        if (memcmp(altMsg->rgb, newRgbB, 3) != 0) {
          memcpy(altMsg->rgb, newRgbB, 3);
          changed = true;
        }
      }
    } else if (!newIsAlternate) {
      // If alternate is disabled, remove 2ND_PRESS action
      if (findAction(config, ACTION_2ND_PRESS)) {
        removeAction(config, ACTION_2ND_PRESS);
        changed = true;
      }
    }

    // --- LONG_PRESS Action (Special View) ---
    if (server.hasArg(String(id) +
                      "_hold_ms")) { // If hold section exists in form
      bool newHoldEn = server.hasArg(String(id) + "_hold_en");

      if (newHoldEn) {
        ActionMessage *longPressMsg =
            findOrCreateAction(config, ACTION_LONG_PRESS);
        if (longPressMsg) {
          uint16_t newMs = server.arg(String(id) + "_hold_ms").toInt();
          if (longPressMsg->longPress.holdMs != newMs) {
            longPressMsg->longPress.holdMs = newMs;
            changed = true;
          }

          if (server.hasArg(String(id) + "_hold_type")) {
            MidiCommandType newType =
                parseCommandType(server.arg(String(id) + "_hold_type"));
            if (longPressMsg->type != newType) {
              longPressMsg->type = newType;
              changed = true;
            }
          }
          if (server.hasArg(String(id) + "_hold_ch")) {
            byte newCh = server.arg(String(id) + "_hold_ch").toInt();
            if (longPressMsg->channel != newCh) {
              longPressMsg->channel = newCh;
              changed = true;
            }
          }
          if (server.hasArg(String(id) + "_hold_d1")) {
            byte newD1 = server.arg(String(id) + "_hold_d1").toInt();
            if (longPressMsg->data1 != newD1) {
              longPressMsg->data1 = newD1;
              changed = true;
            }
          }
          if (server.hasArg(String(id) + "_hold_d2")) {
            byte newD2 = server.arg(String(id) + "_hold_d2").toInt();
            if (longPressMsg->data2 != newD2) {
              longPressMsg->data2 = newD2;
              changed = true;
            }
          }
        }
      } else {
        // Long press disabled - remove action if it exists
        if (findAction(config, ACTION_LONG_PRESS)) {
          removeAction(config, ACTION_LONG_PRESS);
          changed = true;
        }
      }
    }

    // --- COMBO Action (Special View) ---
    if (server.hasArg(String(id) +
                      "_combo_partner")) { // If combo section exists in form
      bool newComboEn = server.hasArg(String(id) + "_combo_en");

      if (newComboEn) {
        ActionMessage *comboMsg = findOrCreateAction(config, ACTION_COMBO);
        if (comboMsg) {
          if (server.hasArg(String(id) + "_combo_label")) {
            String newLabel = server.arg(String(id) + "_combo_label");
            if (strncmp(comboMsg->label, newLabel.c_str(), 6) != 0) {
              strncpy(comboMsg->label, newLabel.c_str(), 6);
              comboMsg->label[5] = '\0';
              changed = true;
            }
          }

          int8_t newPartner =
              server.arg(String(id) + "_combo_partner").toInt() - 1;
          if (comboMsg->combo.partner != newPartner) {
            comboMsg->combo.partner = newPartner;
            changed = true;
          }

          if (server.hasArg(String(id) + "_combo_type")) {
            MidiCommandType newType =
                parseCommandType(server.arg(String(id) + "_combo_type"));
            if (comboMsg->type != newType) {
              comboMsg->type = newType;
              changed = true;
            }
          }
          if (server.hasArg(String(id) + "_combo_ch")) {
            byte newCh = server.arg(String(id) + "_combo_ch").toInt();
            if (comboMsg->channel != newCh) {
              comboMsg->channel = newCh;
              changed = true;
            }
          }
          if (server.hasArg(String(id) + "_combo_d1")) {
            byte newD1 = server.arg(String(id) + "_combo_d1").toInt();
            if (comboMsg->data1 != newD1) {
              comboMsg->data1 = newD1;
              changed = true;
            }
          }
          if (server.hasArg(String(id) + "_combo_d2")) {
            byte newD2 = server.arg(String(id) + "_combo_d2").toInt();
            if (comboMsg->data2 != newD2) {
              comboMsg->data2 = newD2;
              changed = true;
            }
          }
        }
      } else {
        // Combo disabled - remove action if it exists
        if (findAction(config, ACTION_COMBO)) {
          removeAction(config, ACTION_COMBO);
          changed = true;
        }
      }
    }

    yield(); // Allow WDT to reset after each button
  }

  if (changed) {
    yield(); // Before blocking NVS write
    savePresets();

    Serial.println("Save complete - deferring display update...");
    Serial.printf("Free heap after save: %d bytes\n", ESP.getFreeHeap());

    // DEFER display/LED updates to main loop - calling them here crashes due to
    // low heap with WiFi
    pendingDisplayUpdate = true;

    // Give time for memory to stabilize before redirect
    delay(150);
    yield();

    // Get view parameter to redirect back to same view
    String view = server.hasArg("view") ? server.arg("view") : "main";

    // Simple success page - NO auto-redirect to prevent memory crash
    server.send(
        200, "text/html",
        "<html><head><meta charset='UTF-8'></head>"
        "<body "
        "style='background:#121212;color:#e0e0e0;font-family:sans-serif;text-"
        "align:center;padding:80px'>"
        "<h2 style='color:#bb86fc'>✓ Saved!</h2>"
        "<p>Settings saved to NVS successfully.</p>"
        "<a href='/?preset=" +
            String(preset) + "&view=" + view +
            "' style='display:inline-block;margin-top:30px;padding:12px "
            "30px;background:#3700b3;color:#fff;text-decoration:none;border-"
            "radius:8px'>Back to Editor</a>"
            "</body></html>");
  } else {
    String view = server.hasArg("view") ? server.arg("view") : "main";
    server.send(200, "text/html",
                "<html><body "
                "style='background:#222;color:#fff;text-align:center;padding:"
                "50px'>No changes. <a href='/?preset=" +
                    String(preset) + "&view=" + view +
                    "' style='color:#8af'>Back</a></body></html>");
  }
}

void handleSaveSystem() {
  bool changed = false;
  if (server.hasArg("ble")) {
    String newName = server.arg("ble");
    if (strncmp(systemConfig.bleDeviceName, newName.c_str(), 23) != 0) {
      strncpy(systemConfig.bleDeviceName, newName.c_str(), 23);
      systemConfig.bleDeviceName[23] = '\0';
      changed = true;
    }
  }
  if (server.hasArg("ssid")) {
    String newSSID = server.arg("ssid");
    if (strncmp(systemConfig.apSSID, newSSID.c_str(), 23) != 0) {
      strncpy(systemConfig.apSSID, newSSID.c_str(), 23);
      systemConfig.apSSID[23] = '\0';
      changed = true;
    }
  }
  if (server.hasArg("pass")) {
    String newPass = server.arg("pass");
    if (newPass.length() >= 8 && newPass.length() < 16 &&
        strncmp(systemConfig.apPassword, newPass.c_str(), 15) != 0) {
      strncpy(systemConfig.apPassword, newPass.c_str(), 15);
      systemConfig.apPassword[15] = '\0';
      changed = true;
    }
  }

  // BLE Mode
  if (server.hasArg("bleMode")) {
    int newMode = server.arg("bleMode").toInt();
    if (newMode >= 0 && newMode <= 2 &&
        (BleMode)newMode != systemConfig.bleMode) {
      systemConfig.bleMode = (BleMode)newMode;
      changed = true;
      Serial.printf("BLE Mode changed to: %d\n", newMode);
    }
  }

  // v2 Hardware Configuration
  if (server.hasArg("btnCount")) {
    int newCount = server.arg("btnCount").toInt();
    if (newCount >= 4 && newCount <= 10 &&
        newCount != systemConfig.buttonCount) {
      systemConfig.buttonCount = newCount;
      changed = true;
    }
  }
  if (server.hasArg("btnPins")) {
    String pins = server.arg("btnPins");
    // Parse comma-separated pins
    int idx = 0;
    int start = 0;
    for (int i = 0; i <= pins.length() && idx < 10; i++) {
      if (i == pins.length() || pins[i] == ',') {
        String p = pins.substring(start, i);
        p.trim();
        if (p.length() > 0) {
          int pin = p.toInt();
          if (pin >= 0 && pin <= 39) {
            systemConfig.buttonPins[idx++] = pin;
            changed = true;
          }
        }
        start = i + 1;
      }
    }
  }
  if (server.hasArg("ledPin")) {
    int pin = server.arg("ledPin").toInt();
    if (pin >= 0 && pin <= 39 && pin != systemConfig.ledPin) {
      systemConfig.ledPin = pin;
      changed = true;
    }
  }
  if (server.hasArg("ledsPerBtn")) {
    int lpb = server.arg("ledsPerBtn").toInt();
    if (lpb >= 1 && lpb <= 32 && lpb != systemConfig.ledsPerButton) {
      systemConfig.ledsPerButton = lpb;
      changed = true;
    }
  }
  if (server.hasArg("encA")) {
    int pin = server.arg("encA").toInt();
    if (pin >= 0 && pin <= 39 && pin != systemConfig.encoderA) {
      systemConfig.encoderA = pin;
      changed = true;
    }
  }
  if (server.hasArg("encB")) {
    int pin = server.arg("encB").toInt();
    if (pin >= 0 && pin <= 39 && pin != systemConfig.encoderB) {
      systemConfig.encoderB = pin;
      changed = true;
    }
  }
  if (server.hasArg("encBtn")) {
    int pin = server.arg("encBtn").toInt();
    if (pin >= 0 && pin <= 39 && pin != systemConfig.encoderBtn) {
      systemConfig.encoderBtn = pin;
      changed = true;
    }
  }
  if (server.hasArg("ledMap")) {
    String mapStr = server.arg("ledMap");
    // Parse comma-separated LED map values
    int idx = 0;
    int start = 0;
    for (int i = 0; i <= mapStr.length() && idx < 10; i++) {
      if (i == mapStr.length() || mapStr[i] == ',') {
        String v = mapStr.substring(start, i);
        v.trim();
        if (v.length() > 0) {
          int val = v.toInt();
          if (val >= 0 && val <= 255) {
            systemConfig.ledMap[idx++] = val;
            changed = true;
          }
        }
        start = i + 1;
      }
    }
  }

  if (changed) {
    saveSystemSettings();
    rebootESP("Settings Saved!");
  } else {
    rebootESP("No Changes.");
  }
}

void rebootESP(String message) {
  String html =
      F("<!DOCTYPE "
        "html><html><head><title>Rebooting...</"
        "title><style>body{font-family:Inter,sans-serif;background-color:#"
        "1a1a2e;color:#e0e0e0;display:flex;justify-content:center;align-items:"
        "center;height:100vh;margin:0;text-align:center;}.message-box{"
        "background-color:#2a2a4a;padding:30px;border-radius:12px;}h1{color:#"
        "8d8dff;}</style></head><body><div class='message-box'><h1>");
  html += message;
  html += F("</h1><p>Device is rebooting. Please reconnect to the new Wi-Fi AP "
            "if you changed it.</p></div></body></html>");
  server.send(200, "text/html", html);
  delay(1000);
  ESP.restart();
}

void handleExport() {
  server.sendHeader("Content-Disposition",
                    "attachment; filename=\"midi_presets_v2.json\"");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  // Use a larger buffer to reduce TCP packets
  String chunkBuffer = "";
  chunkBuffer.reserve(2048);

  server.sendContent("{\"version\":3,\"configName\":\"");
  server.sendContent(configProfileName);
  server.sendContent("\",\"lastModified\":\"");
  server.sendContent(configLastModified);
  server.sendContent("\",\"presets\":[");

  for (int p = 0; p < 4; p++) {
    if (p > 0)
      server.sendContent(",");
    yield(); // Yield once per preset is sufficient

    // Start Preset Object
    const char *ledModeStr = (presetLedModes[p] == PRESET_LED_NORMAL) ? "NORMAL"
                             : (presetLedModes[p] == PRESET_LED_SELECTION)
                                 ? "SELECTION"
                                 : "HYBRID";

    chunkBuffer = "{\"name\":\"";
    chunkBuffer += String(presetNames[p]);
    chunkBuffer += "\",\"presetLedMode\":\"";
    chunkBuffer += ledModeStr;
    chunkBuffer += "\",\"syncMode\":\"";
    chunkBuffer += (presetSyncMode[p] == SYNC_SPM)   ? "SPM"
                   : (presetSyncMode[p] == SYNC_GP5) ? "GP5"
                                                     : "NONE";
    chunkBuffer += "\",\"buttons\":[";
    server.sendContent(chunkBuffer);
    chunkBuffer = ""; // Reset buffer

    for (int t = 0; t < systemConfig.buttonCount; t++) {
      yield(); // Restore yield to prevent WDT crashes
      if (t > 0)
        chunkBuffer += ",";

      // Use a small StaticJsonDocument for just ONE button
      StaticJsonDocument<1024> doc;
      JsonObject bObj = doc.to<JsonObject>();

      const ButtonConfig &config = buttonConfigs[p][t];
      bObj["name"] = config.name;
      bObj["ledMode"] = (config.ledMode == LED_TOGGLE) ? "TOGGLE" : "MOMENTARY";
      bObj["inSelectionGroup"] = config.inSelectionGroup;

      // Export messages array
      JsonArray messagesArr = bObj.createNestedArray("messages");
      for (int m = 0; m < config.messageCount && m < MAX_ACTIONS_PER_BUTTON;
           m++) {
        const ActionMessage &msg = config.messages[m];
        JsonObject msgObj = messagesArr.createNestedObject();

        // Action type name
        const char *actionName = "PRESS";
        switch (msg.action) {
        case ACTION_PRESS:
          actionName = "PRESS";
          break;
        case ACTION_2ND_PRESS:
          actionName = "2ND_PRESS";
          break;
        case ACTION_RELEASE:
          actionName = "RELEASE";
          break;
        case ACTION_2ND_RELEASE:
          actionName = "2ND_RELEASE";
          break;
        case ACTION_LONG_PRESS:
          actionName = "LONG_PRESS";
          break;
        case ACTION_2ND_LONG_PRESS:
          actionName = "2ND_LONG_PRESS";
          break;
        case ACTION_DOUBLE_TAP:
          actionName = "DOUBLE_TAP";
          break;
        case ACTION_COMBO:
          actionName = "COMBO";
          break;
        default:
          actionName = "NO_ACTION";
          break;
        }
        msgObj["action"] = actionName;
        msgObj["type"] = getCommandTypeString(msg.type);
        msgObj["channel"] = msg.channel;
        msgObj["data1"] = msg.data1;
        msgObj["data2"] = msg.data2;
        char hexColor[8];
        rgbToHex(hexColor, sizeof(hexColor), msg.rgb);
        msgObj["rgb"] = hexColor;

        // Action-specific fields
        if (msg.action == ACTION_COMBO) {
          msgObj["partner"] = msg.combo.partner;
        }
        // Export label for ALL actions (not just COMBO)
        if (msg.label[0]) {
          char labelCopy[7] = {0};
          strncpy(labelCopy, msg.label, 6);
          msgObj["label"] = labelCopy;
        }
        if (msg.action == ACTION_LONG_PRESS ||
            msg.action == ACTION_2ND_LONG_PRESS) {
          msgObj["holdMs"] = msg.longPress.holdMs;
        }
        if (msg.type == TAP_TEMPO) {
          msgObj["rhythmPrev"] = msg.tapTempo.rhythmPrev;
          msgObj["rhythmNext"] = msg.tapTempo.rhythmNext;
          msgObj["tapLock"] = msg.tapTempo.tapLock;
        }
        if (msg.type == SYSEX && msg.sysex.length > 0) {
          String sysexHex = "";
          for (int s = 0; s < msg.sysex.length; s++) {
            char hx[3];
            snprintf(hx, sizeof(hx), "%02x", msg.sysex.data[s]);
            sysexHex += hx;
          }
          msgObj["sysex"] = sysexHex;
        }
      }

      // Serialize button to buffer manually
      String buttonJson;
      serializeJson(bObj, buttonJson);
      chunkBuffer += buttonJson;

      // Send chunk if it gets too large (>1KB)
      if (chunkBuffer.length() > 1024) {
        server.sendContent(chunkBuffer);
        chunkBuffer = "";
      }
    }

    // Send any remaining buttons
    if (chunkBuffer.length() > 0) {
      server.sendContent(chunkBuffer);
      chunkBuffer = "";
    }

    // End Buttons Array and Preset Object
    server.sendContent("]}");
  }

  // End Presets Array
  server.sendContent("],");

  // Add currentPreset (required by web editor for render())
  server.sendContent("\"currentPreset\":0,");

  // Add System Settings (v2 extended)
  String sys = "\"system\":{";
  sys += "\"ble\":\"" + String(systemConfig.bleDeviceName) + "\",";
  sys += "\"ssid\":\"" + String(systemConfig.apSSID) + "\",";
  sys += "\"pass\":\"" + String(systemConfig.apPassword) + "\",";
  sys += "\"buttonCount\":" + String(systemConfig.buttonCount) + ",";
  sys += "\"buttonPins\":\"";
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    if (i > 0)
      sys += ",";
    sys += String(systemConfig.buttonPins[i]);
  }
  sys += "\",";
  sys += "\"ledPin\":" + String(systemConfig.ledPin) + ",";
  sys += "\"encoderA\":" + String(systemConfig.encoderA) + ",";
  sys += "\"encoderB\":" + String(systemConfig.encoderB) + ",";
  sys += "\"encoderBtn\":" + String(systemConfig.encoderBtn) + "}";
  server.sendContent(sys);

  // End Root Object
  server.sendContent("}");

  // Terminate chunked transfer
  server.sendContent("");
}

void handleImport() {
  String html = F("<!DOCTYPE html><html><head><title>Import Presets</title>");
  html +=
      F("<style>body{font-family:Inter,sans-serif;background-color:#1a1a2e;"
        "color:#e0e0e0;display:flex;justify-content:center;align-items:center;"
        "height:100vh;margin:0;text-align:center;}.container{background-color:#"
        "2a2a4a;padding:30px;border-radius:12px;width:80%;max-width:600px;}h1{"
        "color:#8d8dff;}textarea{width:100%;box-sizing:border-box;background-"
        "color:#3a3a5a;color:#e0e0e0;border:1px solid "
        "#5a5a7a;border-radius:8px;padding:10px;height:200px;margin-top:15px;"
        "font-family:monospace;}button{width:100%;padding:10px "
        "18px;background-color:#8d8dff;color:white;border:none;border-radius:"
        "8px;cursor:pointer;font-size:1em;margin-top: 15px;font-weight:bold;} "
        "button:hover{background-color:#7a7aff;}</style>");
  html += F("</head><body><div class='container'><h1>Import "
            "Presets</h1><p>Paste your JSON content below. This will <strong "
            "style='color:#ffccaa;'>OVERWRITE</strong> all presets.</p>");
  html += F("<form action='/import' method='POST'>");
  html += F("<textarea name='json_data' placeholder='Paste midi_presets.json "
            "content here...'></textarea>");
  html += F("<button type='submit'>Import & Save All Presets</button></form>");
  html += F("<p style='margin-top:15px;font-size:0.9em;'><a href='/' "
            "style='color:#aaaaff;'>Cancel and go back</a></p>");
  html += F("</div></body></html>");
  server.send(200, "text/html", html);
}

// ============================================================================
// MULTIPART UPLOAD HANDLERS FOR IMPORT
// ============================================================================

// Static buffer for upload - stored in BSS, not heap (avoids fragmentation)
static char uploadBuffer[14336]; // 14KB static buffer
static size_t uploadBufferLen = 0;
static bool pendingRestart = false; // Flag to trigger restart after response
static char uploadError[128] = "";  // Error message for browser

// Response handler - called when upload is complete
void handleImportUploadResponse() {
  Serial.println("Upload complete, sending response");

  if (strlen(uploadError) > 0) {
    // Send error message to browser
    Serial.printf("Sending error to browser: %s\n", uploadError);
    server.send(400, "text/plain", uploadError);
    uploadError[0] = '\0'; // Clear for next upload
    return;
  }

  server.send(200, "text/plain", "OK - Saved! Rebooting...");

  // Check if we should restart (set by upload handler after successful save)
  if (pendingRestart) {
    Serial.println("=== Rebooting device now... ===");
    Serial.flush();
    delay(500);
    ESP.restart();
  }
}

// Data handler - called repeatedly with data chunks
void handleImportUploadData() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload Start: %s, Free heap: %d\n", upload.filename.c_str(),
                  ESP.getFreeHeap());
    uploadBufferLen = 0;
    memset(uploadBuffer, 0, sizeof(uploadBuffer));
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadBufferLen + upload.currentSize >= sizeof(uploadBuffer)) {
      Serial.printf("ERROR: Upload too large! Need %d bytes, buffer is %d\n",
                    uploadBufferLen + upload.currentSize, sizeof(uploadBuffer));
      snprintf(uploadError, sizeof(uploadError),
               "Upload too large: %d bytes exceeds %d byte buffer",
               (int)(uploadBufferLen + upload.currentSize),
               (int)sizeof(uploadBuffer));
      return;
    }
    memcpy(uploadBuffer + uploadBufferLen, upload.buf, upload.currentSize);
    uploadBufferLen += upload.currentSize;
    Serial.printf("Upload chunk: %d bytes (Total: %d / %d)\n",
                  upload.currentSize, uploadBufferLen, sizeof(uploadBuffer));
  } else if (upload.status == UPLOAD_FILE_END) {
    uploadBuffer[uploadBufferLen] = '\0'; // Null terminate
    Serial.printf("Upload End. Total: %d bytes, Free heap: %d\n",
                  uploadBufferLen, ESP.getFreeHeap());

    if (uploadBufferLen == 0)
      return;

    yield();

    // Parse JSON directly - server stays running so response can be sent
    Serial.printf("Parsing JSON (%d bytes), Free heap: %d\\n", uploadBufferLen,
                  ESP.getFreeHeap());

    JsonDocument doc;
    DeserializationError error =
        deserializeJson(doc, uploadBuffer, uploadBufferLen);

    if (error) {
      Serial.printf("JSON Error: %s\\n", error.c_str());
      if (strcmp(error.c_str(), "NoMemory") == 0) {
        snprintf(uploadError, sizeof(uploadError),
                 "Out of memory! JSON too large (%d bytes). Turn WiFi off/on "
                 "and retry.",
                 uploadBufferLen);
      } else {
        snprintf(uploadError, sizeof(uploadError), "JSON Error: %s (%d bytes)",
                 error.c_str(), uploadBufferLen);
      }
      return;
    }

    // Parse config metadata (editor fields)
    const char *cfgName = doc["configName"] | "";
    if (strlen(cfgName) > 0) {
      strncpy(configProfileName, cfgName, sizeof(configProfileName) - 1);
      configProfileName[sizeof(configProfileName) - 1] = '\0';
    }
    const char *cfgModified = doc["lastModified"] | "";
    if (strlen(cfgModified) > 0) {
      strncpy(configLastModified, cfgModified, sizeof(configLastModified) - 1);
      configLastModified[sizeof(configLastModified) - 1] = '\0';
    }

    // Parse presets
    JsonArray presets = doc["presets"];
    if (!presets.isNull()) {
      Serial.println("Parsing presets...");
      for (int p = 0; p < 4 && p < (int)presets.size(); p++) {
        JsonObject pObj = presets[p];

        strncpy(presetNames[p], pObj["name"] | "Preset", 20);
        presetNames[p][20] = '\0';

        const char *pmStr = pObj["presetLedMode"] | "NORMAL";
        if (strcmp(pmStr, "SELECTION") == 0)
          presetLedModes[p] = PRESET_LED_SELECTION;
        else if (strcmp(pmStr, "HYBRID") == 0)
          presetLedModes[p] = PRESET_LED_HYBRID;
        else
          presetLedModes[p] = PRESET_LED_NORMAL;

        // Sync Mode setting (supports legacy syncSpm boolean + new syncMode
        // string)
        if (pObj.containsKey("syncMode")) {
          const char *syncStr = pObj["syncMode"] | "NONE";
          if (strcmp(syncStr, "SPM") == 0)
            presetSyncMode[p] = SYNC_SPM;
          else if (strcmp(syncStr, "GP5") == 0)
            presetSyncMode[p] = SYNC_GP5;
          else
            presetSyncMode[p] = SYNC_NONE;
        } else {
          // Legacy: syncSpm boolean (true = SYNC_SPM)
          presetSyncMode[p] = (pObj["syncSpm"] | false) ? SYNC_SPM : SYNC_NONE;
        }

        JsonArray buttons = pObj["buttons"];
        if (buttons.isNull())
          continue;

        int btnCount = min((int)buttons.size(), (int)systemConfig.buttonCount);
        for (int b = 0; b < btnCount; b++) {
          JsonObject bObj = buttons[b];
          ButtonConfig &cfg = buttonConfigs[p][b];

          strncpy(cfg.name, bObj["name"] | "BTN", 20);
          cfg.name[20] = '\0';

          const char *lmStr = bObj["ledMode"] | "MOMENTARY";
          cfg.ledMode =
              (strcmp(lmStr, "TOGGLE") == 0) ? LED_TOGGLE : LED_MOMENTARY;
          cfg.inSelectionGroup = bObj["inSelectionGroup"] | false;
          cfg.messageCount = 0;

          JsonArray msgs = bObj["messages"];
          if (!msgs.isNull()) {
            for (int m = 0; m < (int)msgs.size() && m < MAX_ACTIONS_PER_BUTTON;
                 m++) {
              JsonObject mObj = msgs[m];
              ActionMessage &msg = cfg.messages[m];
              memset(&msg, 0, sizeof(ActionMessage));

              const char *actStr = mObj["action"] | "PRESS";
              if (strcmp(actStr, "PRESS") == 0)
                msg.action = ACTION_PRESS;
              else if (strcmp(actStr, "2ND_PRESS") == 0)
                msg.action = ACTION_2ND_PRESS;
              else if (strcmp(actStr, "RELEASE") == 0)
                msg.action = ACTION_RELEASE;
              else if (strcmp(actStr, "2ND_RELEASE") == 0)
                msg.action = ACTION_2ND_RELEASE;
              else if (strcmp(actStr, "LONG_PRESS") == 0)
                msg.action = ACTION_LONG_PRESS;
              else if (strcmp(actStr, "2ND_LONG_PRESS") == 0)
                msg.action = ACTION_2ND_LONG_PRESS;
              else if (strcmp(actStr, "DOUBLE_TAP") == 0)
                msg.action = ACTION_DOUBLE_TAP;
              else if (strcmp(actStr, "COMBO") == 0)
                msg.action = ACTION_COMBO;
              else
                msg.action = ACTION_NO_ACTION;

              msg.type = parseCommandType(mObj["type"] | "OFF");
              msg.channel = mObj["channel"] | 1;
              msg.data1 = mObj["data1"] | 0;
              msg.data2 = mObj["data2"] | 0;
              hexToRgb(mObj["rgb"] | "#bb86fc", msg.rgb);

              // Parse action-specific data
              if (msg.action == ACTION_LONG_PRESS ||
                  msg.action == ACTION_2ND_LONG_PRESS) {
                msg.longPress.holdMs = mObj["holdMs"] | 500; // Default 500ms
              }
              if (msg.action == ACTION_COMBO) {
                msg.combo.partner = mObj["partner"] | 0;
              }
              // Parse label for ALL actions (not just COMBO)
              const char *label = mObj["label"] | "";
              strncpy(msg.label, label, 5);
              msg.label[5] = '\0';
              if (msg.type == TAP_TEMPO) {
                msg.tapTempo.rhythmPrev = mObj["rhythmPrev"] | 0;
                msg.tapTempo.rhythmNext = mObj["rhythmNext"] | 4;
                msg.tapTempo.tapLock = mObj["tapLock"] | 7;
              }
              if (msg.type == SYSEX) {
                const char *hex = mObj["sysex"] | "";
                size_t len = strlen(hex);
                msg.sysex.length = 0;
                for (size_t s = 0; s + 1 < len && msg.sysex.length < 48;
                     s += 2) {
                  char h[3] = {hex[s], hex[s + 1], 0};
                  msg.sysex.data[msg.sysex.length++] = strtol(h, NULL, 16);
                }
              }

              cfg.messageCount++;
            }
          }
        }
        yield();
      }
      savePresets();
      Serial.println("Presets saved!");
    }

    // CRITICAL: Ensure systemPrefs is fully closed before opening with new
    // namespace
    systemPrefs.end(); // Force close any open handle
    delay(300);        // Longer delay for NVS to settle
    yield();

    // Parse system config
    JsonObject sys = doc["system"];
    if (!sys.isNull()) {
      Serial.println("Parsing system config...");
      if (sys.containsKey("bleName")) {
        strncpy(systemConfig.bleDeviceName, sys["bleName"], 23);
      }
      if (sys.containsKey("apSSID")) {
        strncpy(systemConfig.apSSID, sys["apSSID"], 23);
      }
      if (sys.containsKey("apPass")) {
        strncpy(systemConfig.apPassword, sys["apPass"], 15);
      }
      if (sys.containsKey("buttonCount"))
        systemConfig.buttonCount = sys["buttonCount"];
      if (sys.containsKey("ledPin"))
        systemConfig.ledPin = sys["ledPin"];
      if (sys.containsKey("ledsPerButton"))
        systemConfig.ledsPerButton = sys["ledsPerButton"];
      if (sys.containsKey("brightness"))
        ledBrightnessOn = sys["brightness"];
      if (sys.containsKey("brightnessDim"))
        ledBrightnessDim = sys["brightnessDim"];
      if (sys.containsKey("brightnessTap"))
        ledBrightnessTap = sys["brightnessTap"];

      // Parse OLED Configuration (v1.5 - 128x32 support)
      if (sys.containsKey("oled")) {
        JsonObject oled = sys["oled"];

        // OLED Type
        if (oled.containsKey("type")) {
          String typeStr = oled["type"].as<String>();
          oledConfig.type = (typeStr == "128x32") ? OLED_128X32 : OLED_128X64;
        }
        oledConfig.rotation = oled["rotation"] | 0;

        // Per-screen settings
        if (oled.containsKey("screens")) {
          JsonObject screens = oled["screens"];

          // Main screen
          if (screens.containsKey("main")) {
            JsonObject main = screens["main"];
            oledConfig.main.labelSize = main["labelSize"] | 1;
            oledConfig.main.titleSize = main["titleSize"] | 2;
            oledConfig.main.statusSize = main["statusSize"] | 1;
            oledConfig.main.topRowY = main["topRowY"] | 0;
            oledConfig.main.titleY = main["titleY"] | 14;
            oledConfig.main.statusY = main["statusY"] | 32;
            oledConfig.main.bottomRowY = main["bottomRowY"] | 56;
            oledConfig.main.showBpm = main["showBpm"] | true;
            oledConfig.main.showAnalog = main["showAnalog"] | false;
          }

          // Menu screen
          if (screens.containsKey("menu")) {
            JsonObject menu = screens["menu"];
            oledConfig.menu.labelSize = menu["itemSize"] | 1;
            oledConfig.menu.titleSize = menu["headerSize"] | 1;
            oledConfig.menu.topRowY = menu["headerY"] | 0;
            oledConfig.menu.titleY = menu["itemStartY"] | 14;
          }

          // Tap tempo screen
          if (screens.containsKey("tap")) {
            JsonObject tap = screens["tap"];
            oledConfig.tap.labelSize = tap["labelSize"] | 1;
            oledConfig.tap.titleSize = tap["bpmSize"] | 3;
            oledConfig.tap.statusSize = tap["patternSize"] | 1;
            oledConfig.tap.topRowY = tap["labelTopY"] | 0;
            oledConfig.tap.titleY = tap["bpmY"] | 16;
            oledConfig.tap.statusY = tap["patternY"] | 46;
            oledConfig.tap.bottomRowY = tap["labelBottomY"] | 56;
          }

          // Overlay screen
          if (screens.containsKey("overlay")) {
            JsonObject overlay = screens["overlay"];
            oledConfig.overlay.titleSize = overlay["textSize"] | 2;
          }
        }

        Serial.printf("Parsed OLED config: type=%d, rotation=%d\n",
                      oledConfig.type, oledConfig.rotation);
      }

      Serial.printf("Before saveSystemSettings - Free heap: %d\n",
                    ESP.getFreeHeap());
      saveSystemSettings();
    }

    currentPreset = 0;
    pendingDisplayUpdate = true;
    displayOLED(); // Update OLED with new config
    updateLeds();  // Update LEDs with new config

    Serial.println(
        "=== Save Complete! Device will reboot after response... ===");
    Serial.flush();
    pendingRestart = true; // Set flag so response handler triggers restart
  }
}

void turnWifiOn() {
  if (isWifiOn)
    return;

  // CRITICAL: Stop BLE scan IMMEDIATELY before anything else
  doScan = false;
  BLEScan *pScan = BLEDevice::getScan();
  if (pScan) {
    pScan->stop();
  }
  delay(200);
  yield();

  Serial.println("=== WiFi Startup ===");
  Serial.flush();

  uint32_t freeHeap = ESP.getFreeHeap();
  Serial.printf("Free heap: %d bytes\n", freeHeap);
  Serial.flush();

  // Critical: Check if we have enough memory for WiFi
  if (freeHeap < 40000) {
    Serial.println("ERROR: Insufficient memory for WiFi!");
    Serial.flush();
    doScan = true;
    return;
  }

  // Disconnect client if connected
  if (clientConnected && pClient != nullptr) {
    pClient->disconnect();
    clientConnected = false;
    Serial.println("BLE Client: disconnected");
    Serial.flush();
  }
  delay(100);
  yield();

  // CRITICAL: Completely deinitialize BLE to release the radio
  // ESP32 BLE and WiFi share the same radio - they can't run together
  Serial.println("Deinitializing BLE (releasing radio for WiFi)...");
  Serial.flush();
  BLEDevice::deinit(
      false); // false = don't release memory (faster reinit later)
  delay(500);
  yield();

  Serial.printf("Free heap after BLE deinit: %d bytes\n", ESP.getFreeHeap());
  Serial.flush();

  // Start WiFi
  Serial.println("Setting WiFi mode to AP...");
  Serial.flush();
  WiFi.mode(WIFI_AP);
  delay(200);
  yield();

  Serial.println("Starting softAP...");
  Serial.flush();
  bool apStarted = WiFi.softAP(systemConfig.apSSID, systemConfig.apPassword);
  if (!apStarted) {
    Serial.println("ERROR: WiFi.softAP() failed!");
    Serial.flush();
    return;
  }
  delay(200);
  yield();

  Serial.println("Starting web server...");
  Serial.flush();
  server.begin();
  delay(100);
  yield();

  isWifiOn = true;
  Serial.printf("WiFi AP Started! Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("SSID: %s\n", systemConfig.apSSID);
  Serial.println("Access web editor at: http://192.168.4.1");
  Serial.flush();

  yield();
  displayOLED();
}

void turnWifiOff() {
  if (!isWifiOn)
    return;

  Serial.println("=== WiFi Shutdown ===");
  Serial.flush();

  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  isWifiOn = false;

  delay(200);
  yield();

  Serial.println("WiFi AP Stopped");
  Serial.printf("Free heap after WiFi off: %d bytes\n", ESP.getFreeHeap());
  Serial.flush();

  // Re-initialize BLE (it was completely deinitialized when WiFi started)
  Serial.println("Reinitializing BLE...");
  Serial.flush();
  setup_ble_midi(); // Full BLE reinit

  Serial.println("BLE reinitialized successfully");
  Serial.flush();

  // Resume scanning if client mode is enabled
  if (systemConfig.bleMode == BLE_CLIENT_ONLY ||
      systemConfig.bleMode == BLE_DUAL_MODE) {
    doScan = true;
    Serial.println("BLE scanning will resume");
  }

  displayOLED();
}

// Handle preset change from web editor - sync to controller display
void handlePreset() {
  if (!server.hasArg("p")) {
    server.send(400, "text/plain", "Missing preset parameter");
    return;
  }
  int preset = server.arg("p").toInt();
  if (preset < 0 || preset > 3) {
    server.send(400, "text/plain", "Invalid preset (0-3)");
    return;
  }
  currentPreset = preset;

  // Reset toggle states for new preset
  for (int i = 0; i < MAX_BUTTONS; i++) {
    ledToggleState[i] = false;
  }

  // Update display and LEDs
  Serial.printf("handlePreset: Changing to preset %d\n", preset);
  pendingDisplayUpdate = true;
  displayOLED(); // Update OLED immediately
  updateLeds();  // WiFi-safe LED update

  server.send(200, "text/plain", "OK");
}

void setup_web_server() {
  // Enable body collection for POST requests (critical for receiving POST
  // data!)
  const char *headerKeys[] = {"Content-Type", "Content-Length"};
  server.collectHeaders(headerKeys, 2);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/saveSystem", HTTP_POST, handleSaveSystem);
  server.on("/export", HTTP_GET, handleExport);
  server.on("/import", HTTP_GET, handleImport);
  // Register multipart upload handler: (path, method, responseHandler,
  // uploadHandler)
  server.on("/import", HTTP_POST, handleImportUploadResponse,
            handleImportUploadData);
  server.on("/preset", HTTP_GET, handlePreset);

  // Test endpoint for diagnosing POST issues
  server.on("/test", HTTP_POST, []() {
    Serial.println("TEST POST received!");
    Serial.printf("Content-Length: %s\n",
                  server.header("Content-Length").c_str());
    Serial.printf("Has plain: %d\n", server.hasArg("plain"));
    Serial.printf("Has json_data: %d\n", server.hasArg("json_data"));
    if (server.hasArg("json_data")) {
      Serial.printf("json_data length: %d\n", server.arg("json_data").length());
    }
    server.send(200, "text/plain", "OK - POST works!");
  });

  // Analog Input API Endpoints
  server.on("/api/expression", HTTP_GET, []() {
    String json = "{\"analogInputs\":[";
    for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
      if (i > 0)
        json += ",";
      AnalogInputConfig &cfg = analogInputs[i];

      json += "{\"enabled\":";
      json += cfg.enabled ? "true" : "false";
      json += ",\"pin\":";
      json += String(cfg.pin);
      json += ",\"name\":\"";
      json += escapeJson(cfg.name);
      json += "\",\"inputMode\":";
      json += String(cfg.inputMode);

      // Piezo / FSR params
      json += ",\"piezoThreshold\":";
      json += String(cfg.piezoThreshold);
      json += ",\"piezoScanTime\":";
      json += String(cfg.piezoScanTime);
      json += ",\"piezoMaskTime\":";
      json += String(cfg.piezoMaskTime);
      json += ",\"fsrThreshold\":";
      json += String(cfg.fsrThreshold);

      // Calibration
      json += ",\"minVal\":";
      json += String(cfg.minVal);
      json += ",\"maxVal\":";
      json += String(cfg.maxVal);
      json += ",\"inverted\":";
      json += cfg.inverted ? "true" : "false";
      json += ",\"emaAlpha\":";
      json += String(cfg.emaAlpha, 2);
      json += ",\"hysteresis\":";
      json += String(cfg.hysteresis);
      json += ",\"calibrating\":";
      json += cfg.calibrating ? "true" : "false";

      // Messages
      json += ",\"messages\":[";
      for (int m = 0; m < cfg.messageCount; m++) {
        if (m > 0)
          json += ",";
        ActionMessage &msg = cfg.messages[m];
        json += "{\"type\":\"";
        json += getCommandTypeString(msg.type);
        json += "\",\"channel\":";
        json += String(msg.channel);
        json += ",\"data1\":";
        json += String(msg.data1);
        json += ",\"data2\":";
        json += String(msg.data2); // Usually unused/velocity
        json += ",\"inMin\":";
        json += String(msg.minInput);
        json += ",\"inMax\":";
        json += String(msg.maxInput);
        json += "}";
      }
      json += "]}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/expression", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "Missing JSON body");
      return;
    }
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
      server.send(400, "text/plain", String("JSON error: ") + err.c_str());
      return;
    }
    int idx = doc["index"] | 0;
    if (idx < 0 || idx >= MAX_ANALOG_INPUTS) {
      server.send(400, "text/plain", "Invalid index");
      return;
    }
    AnalogInputConfig &cfg = analogInputs[idx];

    if (doc.containsKey("enabled"))
      cfg.enabled = doc["enabled"];
    if (doc.containsKey("pin"))
      cfg.pin = doc["pin"];
    if (doc.containsKey("name")) {
      strncpy(cfg.name, doc["name"] | "A1", 10);
      cfg.name[10] = 0;
    }
    if (doc.containsKey("inputMode"))
      cfg.inputMode = (AnalogInputMode)(doc["inputMode"] | 0);

    if (doc.containsKey("piezoThreshold"))
      cfg.piezoThreshold = doc["piezoThreshold"];
    if (doc.containsKey("piezoScanTime"))
      cfg.piezoScanTime = doc["piezoScanTime"];
    if (doc.containsKey("piezoMaskTime"))
      cfg.piezoMaskTime = doc["piezoMaskTime"];
    if (doc.containsKey("fsrThreshold"))
      cfg.fsrThreshold = doc["fsrThreshold"];

    if (doc.containsKey("minVal"))
      cfg.minVal = doc["minVal"];
    if (doc.containsKey("maxVal"))
      cfg.maxVal = doc["maxVal"];
    if (doc.containsKey("inverted"))
      cfg.inverted = doc["inverted"];
    if (doc.containsKey("emaAlpha"))
      cfg.emaAlpha = doc["emaAlpha"];
    if (doc.containsKey("hysteresis"))
      cfg.hysteresis = doc["hysteresis"];

    // Messages
    JsonArray msgs = doc["messages"];
    if (!msgs.isNull()) {
      cfg.messageCount = 0;
      for (JsonObject mObj : msgs) {
        if (cfg.messageCount >= 4)
          break;
        ActionMessage &msg = cfg.messages[cfg.messageCount];
        memset(&msg, 0, sizeof(ActionMessage));

        msg.action = ACTION_PRESS; // Contextual
        String typeStr = mObj["type"] | "CC";
        msg.type = parseCommandType(typeStr);
        msg.channel = mObj["channel"] | 1;
        msg.data1 = mObj["data1"] | 0;
        msg.data2 = mObj["data2"] | 0;
        msg.minInput = mObj["inMin"] | 0;
        msg.maxInput = mObj["inMax"] | 100;

        cfg.messageCount++;
      }
    }

    saveAnalogInputs();
    setupAnalogInputs();
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/expression/calibrate", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "Missing JSON body");
      return;
    }
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    int idx = doc["index"] | 0;
    bool start = doc["start"] | false;

    if (idx < 0 || idx >= MAX_ANALOG_INPUTS) {
      server.send(400, "text/plain", "Invalid index");
      return;
    }

    if (start) {
      startCalibration(idx);
      server.send(200, "text/plain", "Calibration started");
    } else {
      stopCalibration(idx);
      AnalogInputConfig &cfg = analogInputs[idx];
      saveAnalogInputs();
      String response = "{\"minVal\":";
      response += String(cfg.minVal);
      response += ",\"maxVal\":";
      response += String(cfg.maxVal);
      response += "}";
      server.send(200, "application/json", response);
    }
  });

  Serial.println("Web server routes configured");
}

// ============================================================================
// SHARED CONFIG FUNCTIONS - Used by Serial and BLE config handlers
// ============================================================================

String buildFullConfigJson() {
  // Build full config JSON in memory (Warning: uses significant heap!)
  String json = "{\"version\":3,\"configName\":\"";
  json += escapeJson(configProfileName);
  json += "\",\"lastModified\":\"";
  json += escapeJson(configLastModified);
  json += "\",\"presets\":[";

  for (int p = 0; p < 4; p++) {
    if (p > 0)
      json += ",";

    const char *ledModeStr = (presetLedModes[p] == PRESET_LED_NORMAL) ? "NORMAL"
                             : (presetLedModes[p] == PRESET_LED_SELECTION)
                                 ? "SELECTION"
                                 : "HYBRID";
    const char *syncModeStr = (presetSyncMode[p] == SYNC_SPM)   ? "SPM"
                              : (presetSyncMode[p] == SYNC_GP5) ? "GP5"
                                                                : "NONE";

    json += "{\"name\":\"";
    json += escapeJson(presetNames[p]);
    json += "\",\"presetLedMode\":\"";
    json += ledModeStr;
    json += "\",\"syncMode\":\"";
    json += syncModeStr;
    json += "\",\"buttons\":[";

    for (int b = 0; b < systemConfig.buttonCount; b++) {
      if (b > 0)
        json += ",";
      const ButtonConfig &cfg = buttonConfigs[p][b];

      json += "{\"name\":\"";
      json += escapeJson(cfg.name);
      json += "\",\"ledMode\":\"";
      json += (cfg.ledMode == LED_TOGGLE) ? "TOGGLE" : "MOMENTARY";
      json += "\",\"inSelectionGroup\":";
      json += cfg.inSelectionGroup ? "true" : "false";
      json += ",\"messages\":[";

      for (int m = 0; m < cfg.messageCount && m < MAX_ACTIONS_PER_BUTTON; m++) {
        if (m > 0)
          json += ",";
        const ActionMessage &msg = cfg.messages[m];

        const char *actionName = "PRESS";
        switch (msg.action) {
        case ACTION_PRESS:
          actionName = "PRESS";
          break;
        case ACTION_2ND_PRESS:
          actionName = "2ND_PRESS";
          break;
        case ACTION_RELEASE:
          actionName = "RELEASE";
          break;
        case ACTION_2ND_RELEASE:
          actionName = "2ND_RELEASE";
          break;
        case ACTION_LONG_PRESS:
          actionName = "LONG_PRESS";
          break;
        case ACTION_2ND_LONG_PRESS:
          actionName = "2ND_LONG_PRESS";
          break;
        case ACTION_DOUBLE_TAP:
          actionName = "DOUBLE_TAP";
          break;
        default:
          actionName = "NO_ACTION";
          break;
        }

        char hexColor[8];
        rgbToHex(hexColor, sizeof(hexColor), msg.rgb);

        json += "{\"action\":\"";
        json += actionName;
        json += "\",\"type\":\"";
        json += getCommandTypeString(msg.type);
        json += "\",\"channel\":";
        json += String(msg.channel);
        json += ",\"data1\":";
        json += String(msg.data1);
        json += ",\"data2\":";
        json += String(msg.data2);
        json += ",\"rgb\":\"";
        json += hexColor;
        json += "\",\"label\":\"";
        json += escapeJson(msg.label);
        json += "\",\"partner\":";
        json += String(msg.combo.partner);

        if (msg.action == ACTION_LONG_PRESS ||
            msg.action == ACTION_2ND_LONG_PRESS) {
          json += ",\"holdMs\":";
          json += String(msg.longPress.holdMs);
        }

        // Tap Tempo fields
        if (msg.type == TAP_TEMPO) {
          json += ",\"rhythmPrev\":";
          json += String(msg.tapTempo.rhythmPrev);
          json += ",\"rhythmNext\":";
          json += String(msg.tapTempo.rhythmNext);
          json += ",\"tapLock\":";
          json += String(msg.tapTempo.tapLock);
        }

        // Sysex fields
        if (msg.type == SYSEX && msg.sysex.length > 0) {
          json += ",\"sysex\":\"";
          for (int s = 0; s < msg.sysex.length; s++) {
            char hx[3];
            sprintf(hx, "%02x", msg.sysex.data[s]);
            json += hx;
          }
          json += "\"";
        }

        json += "}";
      }
      json += "]}";
    }
    json += "]}";
  }

  // System config
  json += "],\"system\":{";
  json += "\"buttonCount\":";
  json += String(systemConfig.buttonCount);
  json += ",\"ledsPerButton\":";
  json += String(systemConfig.ledsPerButton);
  json += ",\"bleDeviceName\":\"";
  json += systemConfig.bleDeviceName;
  json += "\",\"apSSID\":\"";
  json += systemConfig.apSSID;
  json += "\",\"brightness\":";
  json += String(ledBrightnessOn);
  json += ",\"brightnessDim\":";
  json += String(ledBrightnessDim);
  json += ",\"brightnessTap\":";
  json += String(ledBrightnessTap);

  // OLED Configuration (v1.5)
  json += ",\"oled\":{";
  json += "\"type\":";
  json += String(oledConfig.type); // 0=128x64, 1=128x32
  json += ",\"rotation\":";
  json += String(oledConfig.rotation);
  json += ",\"screens\":{";

  // Main screen
  json += "\"main\":{";
  json += "\"labelSize\":" + String(oledConfig.main.labelSize);
  json += ",\"titleSize\":" + String(oledConfig.main.titleSize);
  json += ",\"statusSize\":" + String(oledConfig.main.statusSize);
  json += ",\"topRowY\":" + String(oledConfig.main.topRowY);
  json += ",\"titleY\":" + String(oledConfig.main.titleY);
  json += ",\"statusY\":" + String(oledConfig.main.statusY);
  json += ",\"bottomRowY\":" + String(oledConfig.main.bottomRowY);
  json += ",\"showBpm\":";
  json += oledConfig.main.showBpm ? "true" : "false";
  json += ",\"showAnalog\":";
  json += oledConfig.main.showAnalog ? "true" : "false";
  json += "}";

  // Menu screen
  json += ",\"menu\":{";
  json += "\"itemSize\":" + String(oledConfig.menu.labelSize);
  json += ",\"headerSize\":" + String(oledConfig.menu.titleSize);
  json += ",\"headerY\":" + String(oledConfig.menu.topRowY);
  json += ",\"itemStartY\":" + String(oledConfig.menu.titleY);
  json += "}";

  // Tap tempo screen
  json += ",\"tap\":{";
  json += "\"labelSize\":" + String(oledConfig.tap.labelSize);
  json += ",\"bpmSize\":" + String(oledConfig.tap.titleSize);
  json += ",\"patternSize\":" + String(oledConfig.tap.statusSize);
  json += ",\"labelTopY\":" + String(oledConfig.tap.topRowY);
  json += ",\"bpmY\":" + String(oledConfig.tap.titleY);
  json += ",\"patternY\":" + String(oledConfig.tap.statusY);
  json += ",\"labelBottomY\":" + String(oledConfig.tap.bottomRowY);
  json += "}";

  // Overlay screen
  json += ",\"overlay\":{";
  json += "\"textSize\":" + String(oledConfig.overlay.titleSize);
  json += "}";

  json += "}}}"; // Close screens, oled, system

  // Analog Inputs
  json += ",\"analogInputs\":[";
  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    if (i > 0)
      json += ",";
    AnalogInputConfig &cfg = analogInputs[i];

    json += "{\"enabled\":";
    json += cfg.enabled ? "true" : "false";
    json += ",\"pin\":";
    json += String(cfg.pin);
    json += ",\"name\":\"";
    json += escapeJson(cfg.name);
    json += "\",\"inputMode\":";
    json += String(cfg.inputMode);

    json += ",\"piezoThreshold\":";
    json += String(cfg.piezoThreshold);
    json += ",\"piezoScanTime\":";
    json += String(cfg.piezoScanTime);
    json += ",\"piezoMaskTime\":";
    json += String(cfg.piezoMaskTime);
    json += ",\"fsrThreshold\":";
    json += String(cfg.fsrThreshold);

    json += ",\"minVal\":";
    json += String(cfg.minVal);
    json += ",\"maxVal\":";
    json += String(cfg.maxVal);
    json += ",\"inverted\":";
    json += cfg.inverted ? "true" : "false";
    json += ",\"emaAlpha\":";
    json += String(cfg.emaAlpha, 2);
    json += ",\"hysteresis\":";
    json += String(cfg.hysteresis);

    json += ",\"messages\":[";
    for (int m = 0; m < cfg.messageCount; m++) {
      if (m > 0)
        json += ",";
      ActionMessage &msg = cfg.messages[m];
      json += "{\"type\":\"";
      json += getCommandTypeString(msg.type);
      json += "\",\"channel\":";
      json += String(msg.channel);
      json += ",\"data1\":";
      json += String(msg.data1);
      json += ",\"data2\":";
      json += String(msg.data2);
      json += ",\"inMin\":";
      json += String(msg.minInput);
      json += ",\"inMax\":";
      json += String(msg.maxInput);
      json += "}";
    }
    json += "]}";
  }

  return json;
}

bool processConfigChunk(const String &jsonStr, int chunkNum) {
  // Parse and apply JSON config (same logic as multipart upload handler)
  StaticJsonDocument<512> filter;
  filter["version"] = true;
  filter["presets"] = true;
  filter["system"] = true;

  DynamicJsonDocument doc(24576); // 24KB for full config
  DeserializationError error = deserializeJson(doc, jsonStr);

  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return false;
  }

  // Parse config metadata (editor fields)
  const char *cfgName = doc["configName"] | "";
  if (strlen(cfgName) > 0) {
    strncpy(configProfileName, cfgName, sizeof(configProfileName) - 1);
    configProfileName[sizeof(configProfileName) - 1] = '\0';
  }
  const char *cfgModified = doc["lastModified"] | "";
  if (strlen(cfgModified) > 0) {
    strncpy(configLastModified, cfgModified, sizeof(configLastModified) - 1);
    configLastModified[sizeof(configLastModified) - 1] = '\0';
  }

  // Process presets
  JsonArray presets = doc["presets"];
  if (!presets.isNull()) {
    int p = 0;
    for (JsonObject pObj : presets) {
      if (p >= 4)
        break;

      const char *pName = pObj["name"] | "";
      strncpy(presetNames[p], pName, 20);
      presetNames[p][20] = '\0';

      // LED Mode
      const char *ledModeStr = pObj["presetLedMode"] | "NORMAL";
      if (strcmp(ledModeStr, "SELECTION") == 0)
        presetLedModes[p] = PRESET_LED_SELECTION;
      else if (strcmp(ledModeStr, "HYBRID") == 0)
        presetLedModes[p] = PRESET_LED_HYBRID;
      else
        presetLedModes[p] = PRESET_LED_NORMAL;

      // Sync Mode
      if (pObj.containsKey("syncMode")) {
        const char *syncStr = pObj["syncMode"] | "NONE";
        if (strcmp(syncStr, "SPM") == 0)
          presetSyncMode[p] = SYNC_SPM;
        else if (strcmp(syncStr, "GP5") == 0)
          presetSyncMode[p] = SYNC_GP5;
        else
          presetSyncMode[p] = SYNC_NONE;
      }

      // Buttons
      JsonArray buttons = pObj["buttons"];
      if (!buttons.isNull()) {
        int b = 0;
        for (JsonObject bObj : buttons) {
          if (b >= MAX_BUTTONS)
            break;
          ButtonConfig &cfg = buttonConfigs[p][b];

          const char *bName = bObj["name"] | "";
          strncpy(cfg.name, bName, 20);
          cfg.name[20] = '\0';

          const char *lmStr = bObj["ledMode"] | "TOGGLE";
          cfg.ledMode =
              (strcmp(lmStr, "TOGGLE") == 0) ? LED_TOGGLE : LED_MOMENTARY;
          cfg.inSelectionGroup = bObj["inSelectionGroup"] | false;
          cfg.messageCount = 0;

          JsonArray msgs = bObj["messages"];
          if (!msgs.isNull()) {
            for (int m = 0; m < (int)msgs.size() && m < MAX_ACTIONS_PER_BUTTON;
                 m++) {
              JsonObject mObj = msgs[m];
              ActionMessage &msg = cfg.messages[m];

              // Action type
              const char *actStr = mObj["action"] | "PRESS";
              if (strcmp(actStr, "PRESS") == 0)
                msg.action = ACTION_PRESS;
              else if (strcmp(actStr, "2ND_PRESS") == 0)
                msg.action = ACTION_2ND_PRESS;
              else if (strcmp(actStr, "RELEASE") == 0)
                msg.action = ACTION_RELEASE;
              else if (strcmp(actStr, "2ND_RELEASE") == 0)
                msg.action = ACTION_2ND_RELEASE;
              else if (strcmp(actStr, "LONG_PRESS") == 0)
                msg.action = ACTION_LONG_PRESS;
              else if (strcmp(actStr, "2ND_LONG_PRESS") == 0)
                msg.action = ACTION_2ND_LONG_PRESS;
              else if (strcmp(actStr, "DOUBLE_TAP") == 0)
                msg.action = ACTION_DOUBLE_TAP;
              else
                msg.action = ACTION_PRESS;

              // MIDI type
              const char *typeStr = mObj["type"] | "OFF";
              msg.type = parseCommandType(String(typeStr));

              msg.channel = mObj["channel"] | 1;
              msg.data1 = mObj["data1"] | 0;
              msg.data2 = mObj["data2"] | 127;

              // Color - web editor uses "rgb", check both
              const char *colorStr = mObj["rgb"] | (mObj["color"] | "#bb86fc");
              hexToRgb(String(colorStr), msg.rgb);

              // Metadata and Extended fields
              const char *labelStr = mObj["label"] | "";
              strncpy(msg.label, labelStr, 5);
              msg.label[5] = '\0';

              msg.combo.partner = mObj["partner"] | 255;

              if (msg.action == ACTION_LONG_PRESS ||
                  msg.action == ACTION_2ND_LONG_PRESS) {
                msg.longPress.holdMs = mObj["holdMs"] | 500;
              }

              // Tap Tempo
              if (msg.type == TAP_TEMPO) {
                msg.tapTempo.rhythmPrev = mObj["rhythmPrev"] | 255;
                msg.tapTempo.rhythmNext = mObj["rhythmNext"] | 255;
                msg.tapTempo.tapLock = mObj["tapLock"] | 255;
              }

              // SysEx
              if (msg.type == SYSEX && mObj.containsKey("sysex")) {
                const char *hex = mObj["sysex"];
                int len = strlen(hex) / 2;
                if (len > 32)
                  len = 32;
                msg.sysex.length = len;
                for (int i = 0; i < len; i++) {
                  char byteStr[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
                  msg.sysex.data[i] = (uint8_t)strtol(byteStr, NULL, 16);
                }
              }

              cfg.messageCount++;
            }
          }
          b++;
        }
      }
      p++;
    }
  }

  // Process system config
  JsonObject sys = doc["system"];
  if (!sys.isNull()) {
    if (sys.containsKey("buttonCount"))
      systemConfig.buttonCount = sys["buttonCount"];
    if (sys.containsKey("ledsPerButton"))
      systemConfig.ledsPerButton = sys["ledsPerButton"];
    if (sys.containsKey("bleDeviceName")) {
      strncpy(systemConfig.bleDeviceName, sys["bleDeviceName"] | "Chocotone",
              23);
    }
    if (sys.containsKey("brightness"))
      ledBrightnessOn = sys["brightness"];
    if (sys.containsKey("brightnessDim"))
      ledBrightnessDim = sys["brightnessDim"];
    if (sys.containsKey("brightnessTap"))
      ledBrightnessTap = sys["brightnessTap"];

    // Parse OLED Configuration (v1.5 - 128x32 support)
    if (sys.containsKey("oled")) {
      JsonObject oled = sys["oled"];

      // Handle both numeric (0/1) and string ("0"/"1"/"128x32")
      String typeStr = oled["type"].as<String>();
      oledConfig.type =
          (typeStr == "128x32" || typeStr == "1") ? OLED_128X32 : OLED_128X64;

      // Convert degrees to SSD1306 rotation (0=0°, 1=90°, 2=180°, 3=270°)
      int rotDeg = oled["rotation"] | 0;
      if (rotDeg >= 270)
        oledConfig.rotation = 3;
      else if (rotDeg >= 180)
        oledConfig.rotation = 2;
      else if (rotDeg >= 90)
        oledConfig.rotation = 1;
      else
        oledConfig.rotation = 0;

      // Per-screen settings
      if (oled.containsKey("screens")) {
        JsonObject screens = oled["screens"];

        if (screens.containsKey("main")) {
          JsonObject main = screens["main"];
          oledConfig.main.labelSize = main["labelSize"] | 1;
          oledConfig.main.titleSize = main["titleSize"] | 2;
          oledConfig.main.statusSize = main["statusSize"] | 1;
          oledConfig.main.topRowY = main["topRowY"] | 0;
          oledConfig.main.titleY = main["titleY"] | 14;
          oledConfig.main.statusY = main["statusY"] | 32;
          oledConfig.main.bottomRowY = main["bottomRowY"] | 56;
          oledConfig.main.showBpm = main["showBpm"] | true;
          oledConfig.main.showAnalog = main["showAnalog"] | false;
        }
        if (screens.containsKey("menu")) {
          JsonObject menu = screens["menu"];
          oledConfig.menu.labelSize = menu["itemSize"] | 1;
          oledConfig.menu.titleSize = menu["headerSize"] | 1;
          oledConfig.menu.topRowY = menu["headerY"] | 0;
          oledConfig.menu.titleY = menu["itemStartY"] | 14;
        }
        if (screens.containsKey("tap")) {
          JsonObject tap = screens["tap"];
          oledConfig.tap.labelSize = tap["labelSize"] | 1;
          oledConfig.tap.titleSize = tap["bpmSize"] | 3;
          oledConfig.tap.statusSize = tap["patternSize"] | 1;
          oledConfig.tap.topRowY = tap["labelTopY"] | 0;
          oledConfig.tap.titleY = tap["bpmY"] | 16;
          oledConfig.tap.statusY = tap["patternY"] | 46;
          oledConfig.tap.bottomRowY = tap["labelBottomY"] | 56;
        }
        if (screens.containsKey("overlay")) {
          JsonObject overlay = screens["overlay"];
          oledConfig.overlay.titleSize = overlay["textSize"] | 2;
        }
      }
    }
  }

  return true;
}

void finalizeConfigUpload() {
  // Save to NVS
  savePresets();
  saveSystemSettings();

  // Update display
  // Update display
  initDisplayHardware(); // Re-initialize in case type changed
  displayOLED();
  updateLeds();

  Serial.println("Config saved to NVS");
}

// ============================================================================
// SERIAL CONFIG HANDLER - For offline_editor_v2.html via USB Serial
// ============================================================================
static String serialBuffer = "";

void handleSerialConfig() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer.trim();

      // SET_PRESET - Change active preset from editor
      if (serialBuffer.startsWith("SET_PRESET:")) {
        int preset = serialBuffer.substring(11).toInt();
        if (preset >= 0 && preset < 4) {
          currentPreset = preset;
          displayOLED();
          updateLeds();
          Serial.println("OK:PRESET_SET");
        } else {
          Serial.println("ERR:INVALID_PRESET");
        }
        serialBuffer = "";
        return;
      }

      // GET_CONFIG - Send current config as JSON
      if (serialBuffer == "GET_CONFIG") {
        Serial.println("CONFIG_START");

        // Send config JSON with metadata
        Serial.print("{\"version\":3,\"configName\":\"");
        Serial.print(configProfileName);
        Serial.print("\",\"lastModified\":\"");
        Serial.print(configLastModified);
        Serial.print("\",\"presets\":[");
        for (int p = 0; p < 4; p++) {
          if (p > 0)
            Serial.print(",");

          const char *ledModeStr =
              (presetLedModes[p] == PRESET_LED_NORMAL)      ? "NORMAL"
              : (presetLedModes[p] == PRESET_LED_SELECTION) ? "SELECTION"
                                                            : "HYBRID";

          Serial.print("{\"name\":\"");
          Serial.print(presetNames[p]);
          Serial.print("\",\"presetLedMode\":\"");
          Serial.print(ledModeStr);

          // Export syncMode (NONE/SPM/GP5)
          const char *syncModeStr = (presetSyncMode[p] == SYNC_SPM)   ? "SPM"
                                    : (presetSyncMode[p] == SYNC_GP5) ? "GP5"
                                                                      : "NONE";
          Serial.print("\",\"syncMode\":\"");
          Serial.print(syncModeStr);

          Serial.print("\",\"buttons\":[");

          for (int b = 0; b < systemConfig.buttonCount; b++) {
            if (b > 0)
              Serial.print(",");
            const ButtonConfig &cfg = buttonConfigs[p][b];

            Serial.print("{\"name\":\"");
            Serial.print(cfg.name);
            Serial.print("\",\"ledMode\":\"");
            Serial.print(cfg.ledMode == LED_TOGGLE ? "TOGGLE" : "MOMENTARY");
            Serial.print("\",\"inSelectionGroup\":");
            Serial.print(cfg.inSelectionGroup ? "true" : "false");
            Serial.print(",\"messages\":[");

            for (int m = 0; m < cfg.messageCount && m < MAX_ACTIONS_PER_BUTTON;
                 m++) {
              if (m > 0)
                Serial.print(",");
              const ActionMessage &msg = cfg.messages[m];

              const char *actionName = "PRESS";
              switch (msg.action) {
              case ACTION_PRESS:
                actionName = "PRESS";
                break;
              case ACTION_2ND_PRESS:
                actionName = "2ND_PRESS";
                break;
              case ACTION_RELEASE:
                actionName = "RELEASE";
                break;
              case ACTION_2ND_RELEASE:
                actionName = "2ND_RELEASE";
                break;
              case ACTION_LONG_PRESS:
                actionName = "LONG_PRESS";
                break;
              case ACTION_2ND_LONG_PRESS:
                actionName = "2ND_LONG_PRESS";
                break;
              case ACTION_DOUBLE_TAP:
                actionName = "DOUBLE_TAP";
                break;
              case ACTION_COMBO:
                actionName = "COMBO";
                break;
              default:
                actionName = "NO_ACTION";
                break;
              }

              char hexColor[8];
              rgbToHex(hexColor, sizeof(hexColor), msg.rgb);

              Serial.print("{\"action\":\"");
              Serial.print(actionName);
              Serial.print("\",\"type\":\"");
              Serial.print(getCommandTypeString(msg.type));
              Serial.print("\",\"channel\":");
              Serial.print(msg.channel);
              Serial.print(",\"data1\":");
              Serial.print(msg.data1);
              Serial.print(",\"data2\":");
              Serial.print(msg.data2);
              Serial.print(",\"rgb\":\"");
              Serial.print(hexColor);
              Serial.print("\"");

              // Action-specific fields
              if (msg.action == ACTION_LONG_PRESS ||
                  msg.action == ACTION_2ND_LONG_PRESS) {
                Serial.print(",\"holdMs\":");
                Serial.print(msg.longPress.holdMs);
              }
              if (msg.action == ACTION_COMBO) {
                Serial.print(",\"partner\":");
                Serial.print(msg.combo.partner);
              }
              // Export label for ALL actions
              if (msg.label[0]) {
                Serial.print(",\"label\":\"");
                Serial.print(msg.label);
                Serial.print("\"");
              }
              if (msg.type == TAP_TEMPO) {
                Serial.print(",\"rhythmPrev\":");
                Serial.print(msg.tapTempo.rhythmPrev);
                Serial.print(",\"rhythmNext\":");
                Serial.print(msg.tapTempo.rhythmNext);
                Serial.print(",\"tapLock\":");
                Serial.print(msg.tapTempo.tapLock);
              }
              if (msg.type == SYSEX && msg.sysex.length > 0) {
                Serial.print(",\"sysex\":\"");
                for (int s = 0; s < msg.sysex.length; s++) {
                  char hx[3];
                  snprintf(hx, sizeof(hx), "%02x", msg.sysex.data[s]);
                  Serial.print(hx);
                }
                Serial.print("\"");
              }
              Serial.print("}");
            }
            Serial.print("]}");
          }
          Serial.print("]}");
        }

        // System config
        Serial.print("],\"system\":{");
        Serial.print("\"bleDeviceName\":\"");
        Serial.print(systemConfig.bleDeviceName);
        Serial.print("\",\"apSSID\":\"");
        Serial.print(systemConfig.apSSID);
        Serial.print("\",\"apPassword\":\"");
        Serial.print(systemConfig.apPassword);
        Serial.print("\",\"buttonCount\":");
        Serial.print(systemConfig.buttonCount);
        Serial.print(",\"buttonPins\":\"");
        for (int i = 0; i < systemConfig.buttonCount; i++) {
          if (i > 0)
            Serial.print(",");
          Serial.print(systemConfig.buttonPins[i]);
        }
        Serial.print("\",\"ledPin\":");
        Serial.print(systemConfig.ledPin);
        Serial.print(",\"ledsPerButton\":");
        Serial.print(systemConfig.ledsPerButton);
        Serial.print(",\"ledMap\":\"");
        for (int i = 0; i < 10; i++) {
          if (i > 0)
            Serial.print(",");
          Serial.print(systemConfig.ledMap[i]);
        }
        Serial.print("\",\"encoderA\":");
        Serial.print(systemConfig.encoderA);
        Serial.print(",\"encoderB\":");
        Serial.print(systemConfig.encoderB);
        Serial.print(",\"encoderBtn\":");
        Serial.print(systemConfig.encoderBtn);
        Serial.print(",\"bleMode\":\"");
        Serial.print(systemConfig.bleMode == 0
                         ? "CLIENT"
                         : (systemConfig.bleMode == 1 ? "SERVER" : "DUAL"));
        Serial.print("\",\"brightness\":");
        Serial.print(ledBrightnessOn);
        Serial.print(",\"brightnessDim\":");
        Serial.print(ledBrightnessDim);
        Serial.print(",\"brightnessTap\":");
        Serial.print(ledBrightnessTap);
        Serial.print("},\"oled\":{");
        Serial.print("\"type\":\"");
        Serial.print(oledConfig.type == OLED_128X32 ? "128x32" : "128x64");
        Serial.print("\",\"rotation\":");
        Serial.print(oledConfig.rotation * 90);
        Serial.print(",\"screens\":{");

        // Main Screen
        Serial.print("\"main\":{");
        Serial.print("\"labelSize\":");
        Serial.print(oledConfig.main.labelSize);
        Serial.print(",\"titleSize\":");
        Serial.print(oledConfig.main.titleSize);
        Serial.print(",\"statusSize\":");
        Serial.print(oledConfig.main.statusSize);
        Serial.print(",\"topRowY\":");
        Serial.print(oledConfig.main.topRowY);
        Serial.print(",\"titleY\":");
        Serial.print(oledConfig.main.titleY);
        Serial.print(",\"statusY\":");
        Serial.print(oledConfig.main.statusY);
        Serial.print(",\"bottomRowY\":");
        Serial.print(oledConfig.main.bottomRowY);
        Serial.print(",\"showBpm\":");
        Serial.print(oledConfig.main.showBpm ? "true" : "false");
        Serial.print(",\"showAnalog\":");
        Serial.print(oledConfig.main.showAnalog ? "true" : "false");

        // Menu Screen
        Serial.print("},\"menu\":{");
        Serial.print("\"itemSize\":");
        Serial.print(oledConfig.menu.labelSize);
        Serial.print(",\"headerSize\":");
        Serial.print(oledConfig.menu.titleSize);
        Serial.print(",\"headerY\":");
        Serial.print(oledConfig.menu.topRowY);
        Serial.print(",\"itemStartY\":");
        Serial.print(oledConfig.menu.titleY);

        // Tap Screen
        Serial.print("},\"tap\":{");
        Serial.print("\"labelSize\":");
        Serial.print(oledConfig.tap.labelSize);
        Serial.print(",\"bpmSize\":");
        Serial.print(oledConfig.tap.titleSize);
        Serial.print(",\"patternSize\":");
        Serial.print(oledConfig.tap.statusSize);
        Serial.print(",\"labelTopY\":");
        Serial.print(oledConfig.tap.topRowY);
        Serial.print(",\"bpmY\":");
        Serial.print(oledConfig.tap.titleY);
        Serial.print(",\"patternY\":");
        Serial.print(oledConfig.tap.statusY);
        Serial.print(",\"labelBottomY\":");
        Serial.print(oledConfig.tap.bottomRowY);

        // Overlay Screen
        Serial.print("},\"overlay\":{\"textSize\":");
        Serial.print(oledConfig.overlay.titleSize);
        Serial.print("}}}"); // Close overlay, screens, oled

        Serial.print(",\"analogInputs\":[");

        // Analog Inputs (Serial Export)
        for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
          if (i > 0)
            Serial.print(",");
          AnalogInputConfig &cfg = analogInputs[i];

          Serial.print("{\"enabled\":");
          Serial.print(cfg.enabled ? "true" : "false");
          Serial.print(",\"pin\":");
          Serial.print(cfg.pin);
          Serial.print(",\"name\":\"");
          Serial.print(cfg.name);
          Serial.print("\",\"inputMode\":");
          Serial.print(cfg.inputMode);

          Serial.print(",\"piezoThreshold\":");
          Serial.print(cfg.piezoThreshold);
          Serial.print(",\"piezoScanTime\":");
          Serial.print(cfg.piezoScanTime);
          Serial.print(",\"piezoMaskTime\":");
          Serial.print(cfg.piezoMaskTime);
          Serial.print(",\"fsrThreshold\":");
          Serial.print(cfg.fsrThreshold);

          Serial.print(",\"minVal\":");
          Serial.print(cfg.minVal);
          Serial.print(",\"maxVal\":");
          Serial.print(cfg.maxVal);
          Serial.print(",\"inverted\":");
          Serial.print(cfg.inverted ? "true" : "false");
          Serial.print(",\"emaAlpha\":");
          Serial.print(cfg.emaAlpha);
          Serial.print(",\"hysteresis\":");
          Serial.print(cfg.hysteresis);

          Serial.print(",\"messages\":[");
          for (int m = 0; m < cfg.messageCount; m++) {
            if (m > 0)
              Serial.print(",");
            ActionMessage &msg = cfg.messages[m];
            Serial.print("{\"type\":\"");
            Serial.print(getCommandTypeString(msg.type));
            Serial.print("\",\"channel\":");
            Serial.print(msg.channel);
            Serial.print(",\"data1\":");
            Serial.print(msg.data1);
            Serial.print(",\"data2\":");
            Serial.print(msg.data2);
            Serial.print(",\"inMin\":");
            Serial.print(msg.minInput);
            Serial.print(",\"inMax\":");
            Serial.print(msg.maxInput);
            Serial.print("}");
          }
          Serial.print("]}");
        }
        Serial.print("]}"); // Close analogInputs array AND the root object

        Serial.println();
        Serial.println("CONFIG_END");
      }
      // SET_CONFIG_START - Begin receiving config
      else if (serialBuffer == "SET_CONFIG_START") {
        // Pause BLE scan to prevent serial buffer overflow
        doScan = false;
        BLEScan *pScan = BLEDevice::getScan();
        if (pScan) {
          pScan->stop();
        }
        uploadBufferLen = 0;
        memset(uploadBuffer, 0, sizeof(uploadBuffer));
        Serial.println("READY");
      }
      // PAUSE_SCAN - Stop BLE scanning for reliable serial transfer
      else if (serialBuffer == "PAUSE_SCAN") {
        doScan = false;
        BLEScan *pScan = BLEDevice::getScan();
        if (pScan) {
          pScan->stop();
        }
        Serial.println("SCAN_PAUSED");
      }
      // RESUME_SCAN - Resume BLE scanning after transfer
      else if (serialBuffer == "RESUME_SCAN") {
        if (systemConfig.bleMode == BLE_CLIENT_ONLY ||
            systemConfig.bleMode == BLE_DUAL_MODE) {
          doScan = true;
        }
        Serial.println("SCAN_RESUMED");
      }
      // SET_CONFIG_CHUNK:{data} - Receive a chunk of config data
      else if (serialBuffer.startsWith("SET_CONFIG_CHUNK:")) {
        String chunk = serialBuffer.substring(17);
        size_t chunkLen = chunk.length();
        if (uploadBufferLen + chunkLen < sizeof(uploadBuffer)) {
          memcpy(uploadBuffer + uploadBufferLen, chunk.c_str(), chunkLen);
          uploadBufferLen += chunkLen;
          Serial.print("CHUNK_OK:");
          Serial.println(uploadBufferLen);
        } else {
          Serial.println("CHUNK_ERROR:Buffer full");
        }
      }
      // SET_CONFIG_END - Parse and save the received config
      else if (serialBuffer == "SET_CONFIG_END") {
        uploadBuffer[uploadBufferLen] = '\0';
        Serial.printf("Parsing %d bytes of config...\n", uploadBufferLen);

        JsonDocument doc;
        DeserializationError error =
            deserializeJson(doc, uploadBuffer, uploadBufferLen);

        if (error) {
          Serial.print("JSON_ERROR:");
          Serial.println(error.c_str());
        } else {
          // Parse config metadata (editor fields)
          const char *cfgName = doc["configName"] | "";
          if (strlen(cfgName) > 0) {
            strncpy(configProfileName, cfgName, sizeof(configProfileName) - 1);
            configProfileName[sizeof(configProfileName) - 1] = '\0';
          }
          const char *cfgModified = doc["lastModified"] | "";
          if (strlen(cfgModified) > 0) {
            strncpy(configLastModified, cfgModified,
                    sizeof(configLastModified) - 1);
            configLastModified[sizeof(configLastModified) - 1] = '\0';
          }

          // Parse presets (reuse logic from web import)
          JsonArray presets = doc["presets"];
          if (!presets.isNull()) {
            Serial.println("Parsing presets...");
            Serial.printf("  Found %d presets in JSON\n", presets.size());
            for (int p = 0; p < 4 && p < (int)presets.size(); p++) {
              JsonObject pObj = presets[p];
              strncpy(presetNames[p], pObj["name"] | "Preset", 20);
              presetNames[p][20] = '\0';
              Serial.printf("  Preset %d: name='%s'\n", p, presetNames[p]);

              const char *pmStr = pObj["presetLedMode"] | "NORMAL";
              if (strcmp(pmStr, "SELECTION") == 0)
                presetLedModes[p] = PRESET_LED_SELECTION;
              else if (strcmp(pmStr, "HYBRID") == 0)
                presetLedModes[p] = PRESET_LED_HYBRID;
              else
                presetLedModes[p] = PRESET_LED_NORMAL;

              // Parse syncMode (NONE/SPM/GP5)
              const char *syncStr = pObj["syncMode"] | "NONE";
              if (strcmp(syncStr, "SPM") == 0)
                presetSyncMode[p] = SYNC_SPM;
              else if (strcmp(syncStr, "GP5") == 0)
                presetSyncMode[p] = SYNC_GP5;
              else
                presetSyncMode[p] = SYNC_NONE;

              JsonArray buttons = pObj["buttons"];
              if (buttons.isNull())
                continue;

              int btnCount =
                  min((int)buttons.size(), (int)systemConfig.buttonCount);
              Serial.printf("    Buttons: %d in JSON, parsing %d\n",
                            buttons.size(), btnCount);
              for (int b = 0; b < btnCount; b++) {
                JsonObject bObj = buttons[b];
                ButtonConfig &cfg = buttonConfigs[p][b];

                strncpy(cfg.name, bObj["name"] | "BTN", 20);
                cfg.name[20] = '\0';
                Serial.printf("    Button %d: name='%s'\n", b, cfg.name);

                const char *lmStr = bObj["ledMode"] | "MOMENTARY";
                cfg.ledMode =
                    (strcmp(lmStr, "TOGGLE") == 0) ? LED_TOGGLE : LED_MOMENTARY;
                cfg.inSelectionGroup = bObj["inSelectionGroup"] | false;
                cfg.messageCount = 0;

                JsonArray msgs = bObj["messages"];
                if (!msgs.isNull()) {
                  for (int m = 0;
                       m < (int)msgs.size() && m < MAX_ACTIONS_PER_BUTTON;
                       m++) {
                    JsonObject mObj = msgs[m];
                    ActionMessage &msg = cfg.messages[m];
                    memset(&msg, 0, sizeof(ActionMessage));

                    const char *actStr = mObj["action"] | "PRESS";
                    if (strcmp(actStr, "PRESS") == 0)
                      msg.action = ACTION_PRESS;
                    else if (strcmp(actStr, "2ND_PRESS") == 0)
                      msg.action = ACTION_2ND_PRESS;
                    else if (strcmp(actStr, "RELEASE") == 0)
                      msg.action = ACTION_RELEASE;
                    else if (strcmp(actStr, "2ND_RELEASE") == 0)
                      msg.action = ACTION_2ND_RELEASE;
                    else if (strcmp(actStr, "LONG_PRESS") == 0)
                      msg.action = ACTION_LONG_PRESS;
                    else if (strcmp(actStr, "2ND_LONG_PRESS") == 0)
                      msg.action = ACTION_2ND_LONG_PRESS;
                    else if (strcmp(actStr, "DOUBLE_TAP") == 0)
                      msg.action = ACTION_DOUBLE_TAP;
                    else if (strcmp(actStr, "COMBO") == 0)
                      msg.action = ACTION_COMBO;
                    else
                      msg.action = ACTION_NO_ACTION;

                    msg.type = parseCommandType(mObj["type"] | "OFF");
                    msg.channel = mObj["channel"] | 1;
                    msg.data1 = mObj["data1"] | 0;
                    msg.data2 = mObj["data2"] | 0;
                    hexToRgb(mObj["rgb"] | "#bb86fc", msg.rgb);

                    if (msg.action == ACTION_LONG_PRESS) {
                      msg.longPress.holdMs = mObj["holdMs"] | 500;
                    }
                    if (msg.action == ACTION_COMBO) {
                      msg.combo.partner = mObj["partner"] | 0;
                    }
                    // Parse label for ALL actions
                    const char *label = mObj["label"] | "";
                    strncpy(msg.label, label, 5);
                    msg.label[5] = '\0';
                    if (msg.type == TAP_TEMPO) {
                      msg.tapTempo.rhythmPrev = mObj["rhythmPrev"] | 0;
                      msg.tapTempo.rhythmNext = mObj["rhythmNext"] | 4;
                      msg.tapTempo.tapLock = mObj["tapLock"] | 7;
                    }
                    if (msg.type == SYSEX) {
                      const char *hex = mObj["sysex"] | "";
                      size_t len = strlen(hex);
                      msg.sysex.length = 0;
                      for (size_t s = 0; s + 1 < len && msg.sysex.length < 48;
                           s += 2) {
                        char h[3] = {hex[s], hex[s + 1], 0};
                        msg.sysex.data[msg.sysex.length++] =
                            strtol(h, NULL, 16);
                      }
                    }
                    cfg.messageCount++;
                  }
                }
              }
            }
            savePresets();
            Serial.println("Presets saved!");
          }

          // Parse system config
          JsonObject sys = doc["system"];
          if (!sys.isNull()) {
            if (sys.containsKey("bleDeviceName"))
              strncpy(systemConfig.bleDeviceName, sys["bleDeviceName"], 23);
            if (sys.containsKey("apSSID"))
              strncpy(systemConfig.apSSID, sys["apSSID"], 23);
            if (sys.containsKey("apPassword"))
              strncpy(systemConfig.apPassword, sys["apPassword"], 15);
            if (sys.containsKey("buttonCount"))
              systemConfig.buttonCount = sys["buttonCount"];
            // Parse buttonPins string "14,27,26,..." into array
            if (sys.containsKey("buttonPins")) {
              String pinsStr = sys["buttonPins"].as<String>();
              int idx = 0;
              int start = 0;
              for (int i = 0; i <= pinsStr.length() && idx < 10; i++) {
                if (i == pinsStr.length() || pinsStr[i] == ',') {
                  if (i > start) {
                    systemConfig.buttonPins[idx++] =
                        pinsStr.substring(start, i).toInt();
                  }
                  start = i + 1;
                }
              }
            }
            if (sys.containsKey("ledPin"))
              systemConfig.ledPin = sys["ledPin"];
            if (sys.containsKey("ledsPerButton"))
              systemConfig.ledsPerButton = sys["ledsPerButton"];
            // Parse ledMap string "0,1,2,..." into array
            if (sys.containsKey("ledMap")) {
              String mapStr = sys["ledMap"].as<String>();
              int idx = 0;
              int start = 0;
              for (int i = 0; i <= mapStr.length() && idx < 10; i++) {
                if (i == mapStr.length() || mapStr[i] == ',') {
                  if (i > start) {
                    systemConfig.ledMap[idx++] =
                        mapStr.substring(start, i).toInt();
                  }
                  start = i + 1;
                }
              }
            }
            if (sys.containsKey("encoderA"))
              systemConfig.encoderA = sys["encoderA"];
            if (sys.containsKey("encoderB"))
              systemConfig.encoderB = sys["encoderB"];
            if (sys.containsKey("encoderBtn"))
              systemConfig.encoderBtn = sys["encoderBtn"];
            if (sys.containsKey("bleMode")) {
              String mode = sys["bleMode"].as<String>();
              if (mode == "CLIENT")
                systemConfig.bleMode = static_cast<BleMode>(0);
              else if (mode == "SERVER")
                systemConfig.bleMode = static_cast<BleMode>(1);
              else if (mode == "DUAL")
                systemConfig.bleMode = static_cast<BleMode>(2);
            }
            if (sys.containsKey("brightness"))
              ledBrightnessOn = sys["brightness"];
            if (sys.containsKey("brightnessDim"))
              ledBrightnessDim = sys["brightnessDim"];
            if (sys.containsKey("brightnessTap"))
              ledBrightnessTap = sys["brightnessTap"];

            // Parse OLED Configuration (v1.5 - 128x32 support)
            if (sys.containsKey("oled")) {
              JsonObject oled = sys["oled"];
              Serial.println("Parsing OLED config...");

              // Handle both string and numeric type
              if (oled.containsKey("type")) {
                String typeStr = oled["type"].as<String>();
                oledConfig.type = (typeStr == "128x32" || typeStr == "1")
                                      ? OLED_128X32
                                      : OLED_128X64;
                Serial.printf("  OLED type: %s -> %d\n", typeStr.c_str(),
                              oledConfig.type);
              }

              if (oled.containsKey("rotation")) {
                int rotDeg = oled["rotation"] | 0;
                // Convert degrees to SSD1306 rotation (0=0°, 1=90°, 2=180°,
                // 3=270°)
                if (rotDeg >= 270)
                  oledConfig.rotation = 3;
                else if (rotDeg >= 180)
                  oledConfig.rotation = 2;
                else if (rotDeg >= 90)
                  oledConfig.rotation = 1;
                else
                  oledConfig.rotation = 0;
                Serial.printf("  OLED rotation: %d° -> %d\n", rotDeg,
                              oledConfig.rotation);
              }

              // Per-screen settings
              if (oled.containsKey("screens")) {
                JsonObject screens = oled["screens"];

                if (screens.containsKey("main")) {
                  JsonObject main = screens["main"];
                  oledConfig.main.labelSize = main["labelSize"] | 1;
                  oledConfig.main.titleSize = main["titleSize"] | 2;
                  oledConfig.main.statusSize = main["statusSize"] | 1;
                  oledConfig.main.topRowY = main["topRowY"] | 0;
                  oledConfig.main.titleY = main["titleY"] | 14;
                  oledConfig.main.statusY = main["statusY"] | 32;
                  oledConfig.main.bottomRowY = main["bottomRowY"] | 56;
                  oledConfig.main.showBpm = main["showBpm"] | true;
                  oledConfig.main.showAnalog = main["showAnalog"] | false;
                }
                if (screens.containsKey("menu")) {
                  JsonObject menu = screens["menu"];
                  oledConfig.menu.labelSize = menu["itemSize"] | 1;
                  oledConfig.menu.titleSize = menu["headerSize"] | 1;
                  oledConfig.menu.topRowY = menu["headerY"] | 0;
                  oledConfig.menu.titleY = menu["itemStartY"] | 14;
                }
                if (screens.containsKey("tap")) {
                  JsonObject tap = screens["tap"];
                  oledConfig.tap.labelSize = tap["labelSize"] | 1;
                  oledConfig.tap.titleSize = tap["bpmSize"] | 3;
                  oledConfig.tap.statusSize = tap["patternSize"] | 1;
                  oledConfig.tap.topRowY = tap["labelTopY"] | 0;
                  oledConfig.tap.titleY = tap["bpmY"] | 16;
                  oledConfig.tap.statusY = tap["patternY"] | 46;
                  oledConfig.tap.bottomRowY = tap["labelBottomY"] | 56;
                }
                if (screens.containsKey("overlay")) {
                  JsonObject overlay = screens["overlay"];
                  oledConfig.overlay.titleSize = overlay["textSize"] | 2;
                }
              }
            }

            saveSystemSettings();
            Serial.println("System config saved!");

            // Reinitialize display with new config (important for OLED type
            // change)
            initDisplayHardware();
            displayOLED();
            updateLeds();
            Serial.println("Display reinitialized!");
          }

          // Parse Analog Inputs
          JsonArray analogs = doc["analogInputs"];
          if (!analogs.isNull()) {
            for (int i = 0; i < MAX_ANALOG_INPUTS && i < (int)analogs.size();
                 i++) {
              AnalogInputConfig &cfg = analogInputs[i];
              JsonObject aObj = analogs[i];

              if (aObj.containsKey("enabled"))
                cfg.enabled = aObj["enabled"];
              if (aObj.containsKey("pin"))
                cfg.pin = aObj["pin"];
              if (aObj.containsKey("name")) {
                strncpy(cfg.name, aObj["name"] | "A1", 10);
                cfg.name[10] = 0;
              }
              if (aObj.containsKey("inputMode"))
                cfg.inputMode = (AnalogInputMode)(aObj["inputMode"] | 0);

              if (aObj.containsKey("piezoThreshold"))
                cfg.piezoThreshold = aObj["piezoThreshold"];
              if (aObj.containsKey("piezoScanTime"))
                cfg.piezoScanTime = aObj["piezoScanTime"];
              if (aObj.containsKey("piezoMaskTime"))
                cfg.piezoMaskTime = aObj["piezoMaskTime"];
              if (aObj.containsKey("fsrThreshold"))
                cfg.fsrThreshold = aObj["fsrThreshold"];

              if (aObj.containsKey("minVal"))
                cfg.minVal = aObj["minVal"];
              if (aObj.containsKey("maxVal"))
                cfg.maxVal = aObj["maxVal"];
              if (aObj.containsKey("inverted"))
                cfg.inverted = aObj["inverted"];
              if (aObj.containsKey("emaAlpha"))
                cfg.emaAlpha = aObj["emaAlpha"];
              if (aObj.containsKey("hysteresis"))
                cfg.hysteresis = aObj["hysteresis"];

              JsonArray msgs = aObj["messages"];
              if (!msgs.isNull()) {
                cfg.messageCount = 0;
                for (JsonObject mObj : msgs) {
                  if (cfg.messageCount >= 4)
                    break;
                  ActionMessage &msg = cfg.messages[cfg.messageCount];
                  memset(&msg, 0, sizeof(ActionMessage));

                  msg.action = ACTION_PRESS;
                  String typeStr = mObj["type"] | "CC";
                  msg.type = parseCommandType(typeStr);
                  msg.channel = mObj["channel"] | 1;
                  msg.data1 = mObj["data1"] | 0;
                  msg.data2 = mObj["data2"] | 0;
                  msg.minInput = mObj["inMin"] | 0;
                  msg.maxInput = mObj["inMax"] | 100;

                  cfg.messageCount++;
                }
              }
            }
            saveAnalogInputs();
            setupAnalogInputs();
          }

          currentPreset = 0;
          displayOLED();
          updateLeds();
          Serial.println("SAVE_OK");
        }
      }
      // SET_PRESET:N - Change current preset and update display
      else if (serialBuffer.startsWith("SET_PRESET:")) {
        int preset = serialBuffer.substring(11).toInt();
        if (preset >= 0 && preset < 4) {
          currentPreset = preset;
          displayOLED();
          updateLeds();
          Serial.print("PRESET_OK:");
          Serial.println(preset);
        } else {
          Serial.println("PRESET_ERROR:Invalid preset");
        }
      }
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}

// ============================================================================
// BLUETOOTH SERIAL (SPP) - Wireless editor connection
// ============================================================================
// Similar to USB Serial, but over Classic Bluetooth.
// Note: Cannot run simultaneously with BLE MIDI (shared radio).

static String btSerialBuffer = "";

void turnBtSerialOn() {
  if (isBtSerialOn)
    return;
  if (isWifiOn) {
    Serial.println("Cannot enable BT Serial while WiFi is on");
    return;
  }

  Serial.println("=== Bluetooth Serial Startup ===");
  Serial.flush();
  yield();

  // Step 1: Stop BLE scan first
  Serial.println("Step 1: Stopping BLE scan...");
  doScan = false;
  BLEScan *pScan = BLEDevice::getScan();
  if (pScan) {
    pScan->stop();
    pScan->clearResults();
  }
  delay(100);
  yield();

  // Step 2: Stop BLE advertising
  Serial.println("Step 2: Stopping BLE advertising...");
  BLEDevice::stopAdvertising();
  delay(100);
  yield();

  // Step 3: Disconnect BLE client if connected
  Serial.println("Step 3: Disconnecting BLE client...");
  if (clientConnected && pClient != nullptr) {
    pClient->disconnect();
    clientConnected = false;
    delay(200);
    yield();
  }

  // Step 4: Wait for BLE stack to settle
  Serial.println("Step 4: Waiting for BLE to settle...");
  Serial.flush();
  for (int i = 0; i < 5; i++) {
    delay(100);
    yield();
  }

  // Step 5: Deinitialize BLE to free the radio
  Serial.println("Step 5: Deinitializing BLE...");
  Serial.flush();
  BLEDevice::deinit(false);

  // Wait for deinit to complete
  for (int i = 0; i < 8; i++) {
    delay(100);
    yield();
  }

  // Step 6: Start Bluetooth Serial
  Serial.printf("Step 6: Starting Bluetooth Serial as '%s'...\n",
                systemConfig.bleDeviceName);
  Serial.flush();

  if (!SerialBT.begin(systemConfig.bleDeviceName)) {
    Serial.println("ERROR: Failed to start Bluetooth Serial!");
    // Try to recover by reinitializing BLE
    setup_ble_midi();
    return;
  }

  delay(300);
  yield();

  isBtSerialOn = true;
  btSerialBuffer = "";

  Serial.printf("Bluetooth Serial Started! Pair with '%s' on your computer.\n",
                systemConfig.bleDeviceName);
  Serial.println("Then select the Bluetooth COM port in the editor.");
  Serial.flush();

  displayOLED();
}

void turnBtSerialOff() {
  if (!isBtSerialOn)
    return;

  Serial.println("=== Bluetooth Serial Shutdown ===");
  Serial.flush();

  SerialBT.end();
  isBtSerialOn = false;
  btSerialBuffer = "";

  delay(200);
  yield();

  Serial.println("Bluetooth Serial Stopped");
  Serial.println("Reinitializing BLE...");
  Serial.flush();

  // Reinitialize BLE
  setup_ble_midi();

  // Resume scanning if client mode
  if (systemConfig.bleMode == BLE_CLIENT_ONLY ||
      systemConfig.bleMode == BLE_DUAL_MODE) {
    doScan = true;
  }

  Serial.println("BLE reinitialized");
  Serial.flush();

  displayOLED();
}

// Handle Bluetooth Serial config commands - mirrors handleSerialConfig()
void handleBtSerialConfig() {
  if (!isBtSerialOn)
    return;

  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\n') {
      btSerialBuffer.trim();

      // GET_CONFIG - Send config as JSON
      if (btSerialBuffer == "GET_CONFIG") {
        SerialBT.println("CONFIG_START");
        yield(); // Feed watchdog

        // Send config JSON (same format as USB serial)
        SerialBT.print("{\"version\":3,\"configName\":\"");
        SerialBT.print(configProfileName);
        SerialBT.print("\",\"lastModified\":\"");
        SerialBT.print(configLastModified);
        SerialBT.print("\",\"presets\":[");

        for (int p = 0; p < 4; p++) {
          yield(); // Feed watchdog for each preset
          if (p > 0)
            SerialBT.print(",");

          const char *ledModeStr =
              (presetLedModes[p] == PRESET_LED_NORMAL)      ? "NORMAL"
              : (presetLedModes[p] == PRESET_LED_SELECTION) ? "SELECTION"
                                                            : "HYBRID";
          const char *syncModeStr = (presetSyncMode[p] == SYNC_SPM)   ? "SPM"
                                    : (presetSyncMode[p] == SYNC_GP5) ? "GP5"
                                                                      : "NONE";

          SerialBT.print("{\"name\":\"");
          SerialBT.print(presetNames[p]);
          SerialBT.print("\",\"presetLedMode\":\"");
          SerialBT.print(ledModeStr);
          SerialBT.print("\",\"syncMode\":\"");
          SerialBT.print(syncModeStr);
          SerialBT.print("\",\"buttons\":[");

          for (int b = 0; b < systemConfig.buttonCount; b++) {
            yield(); // Feed watchdog for each button
            if (b > 0)
              SerialBT.print(",");
            const ButtonConfig &cfg = buttonConfigs[p][b];

            SerialBT.print("{\"name\":\"");
            SerialBT.print(cfg.name);
            SerialBT.print("\",\"ledMode\":\"");
            SerialBT.print(cfg.ledMode == LED_TOGGLE ? "TOGGLE" : "MOMENTARY");
            SerialBT.print("\",\"inSelectionGroup\":");
            SerialBT.print(cfg.inSelectionGroup ? "true" : "false");
            SerialBT.print(",\"messages\":[");

            for (int m = 0; m < cfg.messageCount && m < MAX_ACTIONS_PER_BUTTON;
                 m++) {
              yield(); // Feed watchdog for each message
              if (m > 0)
                SerialBT.print(",");
              const ActionMessage &msg = cfg.messages[m];

              const char *actionName = "PRESS";
              switch (msg.action) {
              case ACTION_PRESS:
                actionName = "PRESS";
                break;
              case ACTION_2ND_PRESS:
                actionName = "2ND_PRESS";
                break;
              case ACTION_RELEASE:
                actionName = "RELEASE";
                break;
              case ACTION_2ND_RELEASE:
                actionName = "2ND_RELEASE";
                break;
              case ACTION_LONG_PRESS:
                actionName = "LONG_PRESS";
                break;
              case ACTION_2ND_LONG_PRESS:
                actionName = "2ND_LONG_PRESS";
                break;
              case ACTION_DOUBLE_TAP:
                actionName = "DOUBLE_TAP";
                break;
              case ACTION_COMBO:
                actionName = "COMBO";
                break;
              default:
                actionName = "NO_ACTION";
                break;
              }

              char hexColor[8];
              rgbToHex(hexColor, sizeof(hexColor), msg.rgb);

              SerialBT.print("{\"action\":\"");
              SerialBT.print(actionName);
              SerialBT.print("\",\"type\":\"");
              SerialBT.print(getCommandTypeString(msg.type));
              SerialBT.print("\",\"channel\":");
              SerialBT.print(msg.channel);
              SerialBT.print(",\"data1\":");
              SerialBT.print(msg.data1);
              SerialBT.print(",\"data2\":");
              SerialBT.print(msg.data2);
              SerialBT.print(",\"rgb\":\"");
              SerialBT.print(hexColor);
              SerialBT.print("\"");

              // Label (if set)
              if (msg.label[0] != '\0') {
                SerialBT.print(",\"label\":\"");
                SerialBT.print(msg.label);
                SerialBT.print("\"");
              }

              // COMBO partner
              if (msg.action == ACTION_COMBO) {
                SerialBT.print(",\"partner\":");
                SerialBT.print(msg.combo.partner);
              }

              // LONG_PRESS holdMs
              if (msg.action == ACTION_LONG_PRESS ||
                  msg.action == ACTION_2ND_LONG_PRESS) {
                SerialBT.print(",\"holdMs\":");
                SerialBT.print(msg.longPress.holdMs);
              }

              // TAP_TEMPO fields
              if (msg.type == TAP_TEMPO) {
                SerialBT.print(",\"rhythmPrev\":");
                SerialBT.print(msg.tapTempo.rhythmPrev);
                SerialBT.print(",\"rhythmNext\":");
                SerialBT.print(msg.tapTempo.rhythmNext);
                SerialBT.print(",\"tapLock\":");
                SerialBT.print(msg.tapTempo.tapLock);
              }

              // SYSEX data
              if (msg.type == SYSEX && msg.sysex.length > 0) {
                SerialBT.print(",\"sysex\":\"");
                for (int s = 0; s < msg.sysex.length; s++) {
                  char hx[3];
                  snprintf(hx, sizeof(hx), "%02x", msg.sysex.data[s]);
                  SerialBT.print(hx);
                }
                SerialBT.print("\"");
              }

              SerialBT.print("}");
            }
            SerialBT.print("]}");
          }
          SerialBT.print("]}");
        }

        yield(); // Feed watchdog before system config

        // System config - same as USB Serial
        SerialBT.print("],\"system\":{");
        SerialBT.print("\"bleDeviceName\":\"");
        SerialBT.print(systemConfig.bleDeviceName);
        SerialBT.print("\",\"apSSID\":\"");
        SerialBT.print(systemConfig.apSSID);
        SerialBT.print("\",\"apPassword\":\"");
        SerialBT.print(systemConfig.apPassword);
        SerialBT.print("\",\"buttonCount\":");
        SerialBT.print(systemConfig.buttonCount);
        SerialBT.print(",\"buttonPins\":\"");
        for (int i = 0; i < systemConfig.buttonCount; i++) {
          if (i > 0)
            SerialBT.print(",");
          SerialBT.print(systemConfig.buttonPins[i]);
        }
        SerialBT.print("\",\"ledPin\":");
        SerialBT.print(systemConfig.ledPin);
        SerialBT.print(",\"ledsPerButton\":");
        SerialBT.print(systemConfig.ledsPerButton);
        yield(); // Feed watchdog mid-system config
        SerialBT.print(",\"ledMap\":\"");
        for (int i = 0; i < 10; i++) {
          if (i > 0)
            SerialBT.print(",");
          SerialBT.print(systemConfig.ledMap[i]);
        }
        yield(); // Feed watchdog
        SerialBT.print("\",\"encoderA\":");
        SerialBT.print(systemConfig.encoderA);
        SerialBT.print(",\"encoderB\":");
        SerialBT.print(systemConfig.encoderB);
        SerialBT.print(",\"encoderBtn\":");
        SerialBT.print(systemConfig.encoderBtn);
        SerialBT.print(",\"bleMode\":\"");
        SerialBT.print(systemConfig.bleMode == 0
                           ? "CLIENT"
                           : (systemConfig.bleMode == 1 ? "SERVER" : "DUAL"));
        SerialBT.print("\",\"brightness\":");
        SerialBT.print(ledBrightnessOn);
        SerialBT.print(",\"brightnessDim\":");
        SerialBT.print(ledBrightnessDim);
        SerialBT.print(",\"brightnessTap\":");
        SerialBT.print(ledBrightnessTap);
        SerialBT.print("},\"oled\":{");
        SerialBT.print("\"type\":\"");
        SerialBT.print(oledConfig.type == OLED_128X32 ? "128x32" : "128x64");
        SerialBT.print("\",\"rotation\":");
        SerialBT.print(oledConfig.rotation * 90);
        SerialBT.print(",\"screens\":{");

        // Main Screen
        SerialBT.print("\"main\":{");
        SerialBT.print("\"labelSize\":");
        SerialBT.print(oledConfig.main.labelSize);
        SerialBT.print(",\"titleSize\":");
        SerialBT.print(oledConfig.main.titleSize);
        SerialBT.print(",\"statusSize\":");
        SerialBT.print(oledConfig.main.statusSize);
        SerialBT.print(",\"topRowY\":");
        SerialBT.print(oledConfig.main.topRowY);
        SerialBT.print(",\"titleY\":");
        SerialBT.print(oledConfig.main.titleY);
        SerialBT.print(",\"statusY\":");
        SerialBT.print(oledConfig.main.statusY);
        SerialBT.print(",\"bottomRowY\":");
        SerialBT.print(oledConfig.main.bottomRowY);
        SerialBT.print(",\"showBpm\":");
        SerialBT.print(oledConfig.main.showBpm ? "true" : "false");
        SerialBT.print(",\"showAnalog\":");
        SerialBT.print(oledConfig.main.showAnalog ? "true" : "false");

        // Menu Screen
        SerialBT.print("},\"menu\":{");
        SerialBT.print("\"itemSize\":");
        SerialBT.print(oledConfig.menu.labelSize);
        SerialBT.print(",\"headerSize\":");
        SerialBT.print(oledConfig.menu.titleSize);
        SerialBT.print(",\"headerY\":");
        SerialBT.print(oledConfig.menu.topRowY);
        SerialBT.print(",\"itemStartY\":");
        SerialBT.print(oledConfig.menu.titleY);

        // Tap Screen
        SerialBT.print("},\"tap\":{");
        SerialBT.print("\"labelSize\":");
        SerialBT.print(oledConfig.tap.labelSize);
        SerialBT.print(",\"bpmSize\":");
        SerialBT.print(oledConfig.tap.titleSize);
        SerialBT.print(",\"patternSize\":");
        SerialBT.print(oledConfig.tap.statusSize);
        SerialBT.print(",\"labelTopY\":");
        SerialBT.print(oledConfig.tap.topRowY);
        SerialBT.print(",\"bpmY\":");
        SerialBT.print(oledConfig.tap.titleY);
        SerialBT.print(",\"patternY\":");
        SerialBT.print(oledConfig.tap.statusY);
        SerialBT.print(",\"labelBottomY\":");
        SerialBT.print(oledConfig.tap.bottomRowY);

        // Overlay Screen
        SerialBT.print("},\"overlay\":{\"textSize\":");
        SerialBT.print(oledConfig.overlay.titleSize);
        SerialBT.print("}}}"); // Close overlay, screens, oled

        SerialBT.print(",\"analogInputs\":[");

        // Analog Inputs (BT Export)
        for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
          yield(); // Feed watchdog
          if (i > 0)
            SerialBT.print(",");
          AnalogInputConfig &cfg = analogInputs[i];

          SerialBT.print("{\"enabled\":");
          SerialBT.print(cfg.enabled ? "true" : "false");
          SerialBT.print(",\"pin\":");
          SerialBT.print(cfg.pin);
          SerialBT.print(",\"name\":\"");
          SerialBT.print(cfg.name);
          SerialBT.print("\",\"inputMode\":");
          SerialBT.print(cfg.inputMode);

          SerialBT.print(",\"piezoThreshold\":");
          SerialBT.print(cfg.piezoThreshold);
          SerialBT.print(",\"piezoScanTime\":");
          SerialBT.print(cfg.piezoScanTime);
          SerialBT.print(",\"piezoMaskTime\":");
          SerialBT.print(cfg.piezoMaskTime);
          SerialBT.print(",\"fsrThreshold\":");
          SerialBT.print(cfg.fsrThreshold);

          SerialBT.print(",\"minVal\":");
          SerialBT.print(cfg.minVal);
          SerialBT.print(",\"maxVal\":");
          SerialBT.print(cfg.maxVal);
          SerialBT.print(",\"inverted\":");
          SerialBT.print(cfg.inverted ? "true" : "false");
          SerialBT.print(",\"emaAlpha\":");
          SerialBT.print(cfg.emaAlpha);
          SerialBT.print(",\"hysteresis\":");
          SerialBT.print(cfg.hysteresis);

          SerialBT.print(",\"messages\":[");
          for (int m = 0; m < cfg.messageCount; m++) {
            if (m > 0)
              SerialBT.print(",");
            ActionMessage &msg = cfg.messages[m];
            SerialBT.print("{\"type\":\"");
            SerialBT.print(getCommandTypeString(msg.type));
            SerialBT.print("\",\"channel\":");
            SerialBT.print(msg.channel);
            SerialBT.print(",\"data1\":");
            SerialBT.print(msg.data1);
            SerialBT.print(",\"data2\":");
            SerialBT.print(msg.data2);
            SerialBT.print(",\"inMin\":");
            SerialBT.print(msg.minInput);
            SerialBT.print(",\"inMax\":");
            SerialBT.print(msg.maxInput);
            SerialBT.print("}");
          }
          SerialBT.print("]}");
        }
        SerialBT.print("]}"); // Close analogInputs array AND the root object

        SerialBT.println();
        SerialBT.println("CONFIG_END");
      }
      // SET_PRESET:N - Change preset
      else if (btSerialBuffer.startsWith("SET_PRESET:")) {
        int preset = btSerialBuffer.substring(11).toInt();
        if (preset >= 0 && preset < 4) {
          currentPreset = preset;
          displayOLED();
          updateLeds();
          SerialBT.print("PRESET_OK:");
          SerialBT.println(preset);
        } else {
          SerialBT.println("PRESET_ERROR:Invalid preset");
        }
      }
      // PING - Simple connection test
      else if (btSerialBuffer == "PING") {
        SerialBT.println("PONG");
      }
      // SET_CONFIG_START - Begin receiving config (uses same uploadBuffer as
      // USB)
      else if (btSerialBuffer == "SET_CONFIG_START") {
        uploadBufferLen = 0;
        memset(uploadBuffer, 0, sizeof(uploadBuffer));
        SerialBT.println("READY");
        Serial.println("BT: SET_CONFIG_START");
      }
      // SET_CONFIG_CHUNK:{data} - Receive a chunk of config data
      else if (btSerialBuffer.startsWith("SET_CONFIG_CHUNK:")) {
        String chunk = btSerialBuffer.substring(17);
        size_t chunkLen = chunk.length();
        if (uploadBufferLen + chunkLen < sizeof(uploadBuffer)) {
          memcpy(uploadBuffer + uploadBufferLen, chunk.c_str(), chunkLen);
          uploadBufferLen += chunkLen;
          SerialBT.print("CHUNK_OK:");
          SerialBT.println(uploadBufferLen);
          Serial.printf("BT: Chunk received, total: %d\n", uploadBufferLen);
        } else {
          SerialBT.println("CHUNK_ERROR:Buffer full");
          Serial.println("BT: Buffer full!");
        }
      }
      // SET_CONFIG_END - Parse and save the received config
      else if (btSerialBuffer == "SET_CONFIG_END") {
        uploadBuffer[uploadBufferLen] = '\0';
        Serial.printf("BT: Parsing %d bytes of config...\n", uploadBufferLen);
        yield(); // Feed watchdog before parsing

        JsonDocument doc;
        DeserializationError error =
            deserializeJson(doc, uploadBuffer, uploadBufferLen);
        yield(); // Feed watchdog after parsing

        if (error) {
          SerialBT.print("JSON_ERROR:");
          SerialBT.println(error.c_str());
          Serial.printf("BT: JSON error: %s\n", error.c_str());
        } else {
          // Parse config metadata
          const char *cfgName = doc["configName"] | "";
          if (strlen(cfgName) > 0) {
            strncpy(configProfileName, cfgName, sizeof(configProfileName) - 1);
            configProfileName[sizeof(configProfileName) - 1] = '\0';
          }
          const char *cfgModified = doc["lastModified"] | "";
          if (strlen(cfgModified) > 0) {
            strncpy(configLastModified, cfgModified,
                    sizeof(configLastModified) - 1);
            configLastModified[sizeof(configLastModified) - 1] = '\0';
          }
          yield(); // Feed watchdog

          // Parse presets
          JsonArray presets = doc["presets"];
          if (!presets.isNull()) {
            for (int p = 0; p < 4 && p < (int)presets.size(); p++) {
              yield(); // Feed watchdog for each preset
              JsonObject pObj = presets[p];
              strncpy(presetNames[p], pObj["name"] | "Preset", 20);
              presetNames[p][20] = '\0';

              const char *pmStr = pObj["presetLedMode"] | "NORMAL";
              if (strcmp(pmStr, "SELECTION") == 0)
                presetLedModes[p] = PRESET_LED_SELECTION;
              else if (strcmp(pmStr, "HYBRID") == 0)
                presetLedModes[p] = PRESET_LED_HYBRID;
              else
                presetLedModes[p] = PRESET_LED_NORMAL;

              const char *syncStr = pObj["syncMode"] | "NONE";
              if (strcmp(syncStr, "SPM") == 0)
                presetSyncMode[p] = SYNC_SPM;
              else if (strcmp(syncStr, "GP5") == 0)
                presetSyncMode[p] = SYNC_GP5;
              else
                presetSyncMode[p] = SYNC_NONE;

              JsonArray buttons = pObj["buttons"];
              if (buttons.isNull())
                continue;

              int btnCount =
                  min((int)buttons.size(), (int)systemConfig.buttonCount);
              for (int b = 0; b < btnCount; b++) {
                yield(); // Feed watchdog for each button
                JsonObject bObj = buttons[b];
                ButtonConfig &cfg = buttonConfigs[p][b];

                strncpy(cfg.name, bObj["name"] | "BTN", 20);
                cfg.name[20] = '\0';

                const char *lmStr = bObj["ledMode"] | "MOMENTARY";
                cfg.ledMode =
                    (strcmp(lmStr, "TOGGLE") == 0) ? LED_TOGGLE : LED_MOMENTARY;
                cfg.inSelectionGroup = bObj["inSelectionGroup"] | false;
                cfg.messageCount = 0;

                JsonArray msgs = bObj["messages"];
                if (!msgs.isNull()) {
                  for (int m = 0;
                       m < (int)msgs.size() && m < MAX_ACTIONS_PER_BUTTON;
                       m++) {
                    JsonObject mObj = msgs[m];
                    ActionMessage &msg = cfg.messages[m];
                    memset(&msg, 0, sizeof(ActionMessage));

                    const char *actStr = mObj["action"] | "PRESS";
                    if (strcmp(actStr, "PRESS") == 0)
                      msg.action = ACTION_PRESS;
                    else if (strcmp(actStr, "2ND_PRESS") == 0)
                      msg.action = ACTION_2ND_PRESS;
                    else if (strcmp(actStr, "RELEASE") == 0)
                      msg.action = ACTION_RELEASE;
                    else if (strcmp(actStr, "2ND_RELEASE") == 0)
                      msg.action = ACTION_2ND_RELEASE;
                    else if (strcmp(actStr, "LONG_PRESS") == 0)
                      msg.action = ACTION_LONG_PRESS;
                    else if (strcmp(actStr, "2ND_LONG_PRESS") == 0)
                      msg.action = ACTION_2ND_LONG_PRESS;
                    else if (strcmp(actStr, "DOUBLE_TAP") == 0)
                      msg.action = ACTION_DOUBLE_TAP;
                    else if (strcmp(actStr, "COMBO") == 0)
                      msg.action = ACTION_COMBO;
                    else
                      msg.action = ACTION_NO_ACTION;

                    msg.type = parseCommandType(mObj["type"] | "OFF");
                    msg.channel = mObj["channel"] | 1;
                    msg.data1 = mObj["data1"] | 0;
                    msg.data2 = mObj["data2"] | 0;
                    hexToRgb(mObj["rgb"] | "#bb86fc", msg.rgb);

                    // COMBO partner
                    if (msg.action == ACTION_COMBO) {
                      msg.combo.partner = mObj["partner"] | 0;
                    }

                    // LONG_PRESS holdMs
                    if (msg.action == ACTION_LONG_PRESS ||
                        msg.action == ACTION_2ND_LONG_PRESS) {
                      msg.longPress.holdMs = mObj["holdMs"] | 500;
                    }

                    // Label
                    const char *label = mObj["label"] | "";
                    strncpy(msg.label, label, 5);
                    msg.label[5] = '\0';

                    // TAP_TEMPO fields
                    if (msg.type == TAP_TEMPO) {
                      msg.tapTempo.rhythmPrev = mObj["rhythmPrev"] | 0;
                      msg.tapTempo.rhythmNext = mObj["rhythmNext"] | 4;
                      msg.tapTempo.tapLock = mObj["tapLock"] | 7;
                    }

                    // SYSEX data
                    if (msg.type == SYSEX) {
                      const char *hex = mObj["sysex"] | "";
                      size_t len = strlen(hex);
                      msg.sysex.length = 0;
                      for (size_t s = 0; s + 1 < len && msg.sysex.length < 48;
                           s += 2) {
                        char h[3] = {hex[s], hex[s + 1], 0};
                        msg.sysex.data[msg.sysex.length++] =
                            strtol(h, NULL, 16);
                      }
                    }
                    cfg.messageCount++;
                  }
                }
              }
            }
            yield(); // Feed watchdog before save
            savePresets();
            yield(); // Feed watchdog after save
            Serial.println("BT: Presets saved!");
          }

          // Parse system config
          JsonObject sys = doc["system"];
          if (!sys.isNull()) {
            if (sys.containsKey("bleDeviceName"))
              strncpy(systemConfig.bleDeviceName, sys["bleDeviceName"], 23);
            if (sys.containsKey("buttonCount"))
              systemConfig.buttonCount = sys["buttonCount"];
            if (sys.containsKey("brightness"))
              ledBrightnessOn = sys["brightness"];
            yield(); // Feed watchdog before save
            saveSystemSettings();
            yield(); // Feed watchdog after save
            Serial.println("BT: System config saved!");
          }

          currentPreset = 0;
          yield(); // Feed watchdog before display update
          displayOLED();
          updateLeds();
          SerialBT.println("SAVE_OK");
          Serial.println("BT: Config saved successfully!");
        }
      }

      btSerialBuffer = "";
    } else {
      btSerialBuffer += c;
      // Prevent buffer overflow
      if (btSerialBuffer.length() > 16000) {
        btSerialBuffer = "";
      }
    }
  }
}
