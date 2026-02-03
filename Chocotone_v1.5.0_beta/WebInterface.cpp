#include "WebInterface.h"
#include "AnalogInput.h"
#include "BleMidi.h"
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#include "BluetoothSerial.h"
#endif
#include "Storage.h"
#include "UI_Display.h"
#include <ArduinoJson.h>
#include <WebServer.h> // Ensure WebServer is included

// Bluetooth Serial (SPP) for wireless editor connection
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
BluetoothSerial SerialBT;
#endif

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

// Helper: Check heap and print connection status
void checkHeapStatus() {
  uint32_t free = ESP.getFreeHeap();
  uint32_t max_free = ESP.getMaxAllocHeap();
  Serial.printf("[SYSTEM] Free Heap: %d bytes (Max Block: %d)\n", free,
                max_free);

  if (free < 20000) {
    Serial.println(
        "⚠️  WARNING: Low Memory! WiFi/BLE stability may be affected.");
  }
}

// Return System Info as JSON (Hardware aware)
void handleSysInfo() {
  String json = "{";

  // Chip Model
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  json += "\"chip\":\"ESP32-S3\",";
  json += "\"usb_midi\":true,";
  json += "\"safe_pins\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,33,34,"
          "35,36,37,38,39,40,41,42],"; // S3 Safe Pins
#else
  json += "\"chip\":\"ESP32\",";
  json += "\"usb_midi\":false,";
  json +=
      "\"safe_pins\":[12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33],"; // Classical
                                                                          // ESP32
                                                                          // Safe
                                                                          // Pins
#endif

  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"version\":\"1.5.0-beta\",";
  json += "\"ble_mode\":" + String((int)systemConfig.bleMode);
  json += "}";

  server.send(200, "application/json", json);
}

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
  case MENU_TOGGLE:
    return "MENU_TOGGLE";
  case MENU_UP:
    return "MENU_UP";
  case MENU_DOWN:
    return "MENU_DOWN";
  case MENU_ENTER:
    return "MENU_ENTER";
  default:
    return "OFF";
  }
}
const char *getActionTypeString(ActionType t) {
  switch (t) {
  case ACTION_PRESS:
    return "PRESS";
  case ACTION_2ND_PRESS:
    return "2ND_PRESS";
  case ACTION_RELEASE:
    return "RELEASE";
  case ACTION_2ND_RELEASE:
    return "2ND_RELEASE";
  case ACTION_LONG_PRESS:
    return "LONG_PRESS";
  case ACTION_2ND_LONG_PRESS:
    return "2ND_LONG_PRESS";
  case ACTION_DOUBLE_TAP:
    return "DOUBLE_TAP";
  case ACTION_COMBO:
    return "COMBO";
  default:
    return "PRESS";
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
  if (s == "SYSEX_SCROLL")
    return SYSEX_SCROLL;
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
  if (s == "MENU_TOGGLE")
    return MENU_TOGGLE;
  if (s == "MENU_UP")
    return MENU_UP;
  if (s == "MENU_DOWN")
    return MENU_DOWN;
  if (s == "MENU_ENTER")
    return MENU_ENTER;
  return MIDI_OFF;
}
ActionType parseActionType(String s) {
  if (s == "PRESS")
    return ACTION_PRESS;
  if (s == "2ND_PRESS")
    return ACTION_2ND_PRESS;
  if (s == "RELEASE")
    return ACTION_RELEASE;
  if (s == "2ND_RELEASE")
    return ACTION_2ND_RELEASE;
  if (s == "LONG_PRESS")
    return ACTION_LONG_PRESS;
  if (s == "2ND_LONG_PRESS")
    return ACTION_2ND_LONG_PRESS;
  if (s == "DOUBLE_TAP")
    return ACTION_DOUBLE_TAP;
  if (s == "COMBO")
    return ACTION_COMBO;
  return ACTION_NO_ACTION;
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

  if (server.hasArg("debugAnalogIn")) {
    bool dbg = server.arg("debugAnalogIn") == "true" ||
               server.arg("debugAnalogIn") == "1";
    if (dbg != systemConfig.debugAnalogIn) {
      systemConfig.debugAnalogIn = dbg;
      changed = true;
    }
  }

  if (server.hasArg("analogInputCount")) {
    uint8_t cnt = server.arg("analogInputCount").toInt();
    if (cnt > MAX_ANALOG_INPUTS)
      cnt = MAX_ANALOG_INPUTS;
    if (cnt != systemConfig.analogInputCount) {
      systemConfig.analogInputCount = cnt;
      changed = true;
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
  server.sendHeader("Access-Control-Allow-Origin", "*");
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
  sys += "\"encoderBtn\":" + String(systemConfig.encoderBtn) + ",";
  sys += "\"debugAnalogIn\":" +
         String(systemConfig.debugAnalogIn ? "true" : "false") + ",";

  // Add globalSpecialActions array
  sys += "\"globalSpecialActions\":[";
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    if (i > 0)
      sys += ",";
    const GlobalSpecialAction &gsa = globalSpecialActions[i];
    const ActionMessage &msg = gsa.comboAction;

    sys += "{\"enabled\":";
    sys += gsa.hasCombo ? "true" : "false";
    sys += ",\"partner\":";
    sys += String(msg.combo.partner);
    sys += ",\"type\":\"" + String(getCommandTypeString(msg.type)) + "\",";
    sys += "\"channel\":" + String(msg.channel) + ",";
    sys += "\"data1\":" + String(msg.data1) + ",";
    sys += "\"data2\":" + String(msg.data2) + ",";

    char hexColor[8];
    rgbToHex(hexColor, sizeof(hexColor), msg.rgb);
    sys += "\"rgb\":\"" + String(hexColor) + "\",";

    sys += "\"label\":\"";
    char labelBuf[6] = {0};
    strncpy(labelBuf, msg.label, 5);
    sys += String(labelBuf) + "\"";

    if (msg.type == TAP_TEMPO) {
      sys += ",\"rhythmPrev\":" + String(msg.tapTempo.rhythmPrev);
      sys += ",\"rhythmNext\":" + String(msg.tapTempo.rhythmNext);
      sys += ",\"tapLock\":" + String(msg.tapTempo.tapLock);
    }

    if (msg.type == SYSEX && msg.sysex.length > 0) {
      String sysexHex = "";
      for (int s = 0; s < msg.sysex.length; s++) {
        char hx[3];
        snprintf(hx, sizeof(hx), "%02x", msg.sysex.data[s]);
        sysexHex += hx;
      }
      sys += ",\"sysex\":\"" + sysexHex + "\"";
    }

    sys += "}";
  }
  sys += "]}";
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
// 20KB buffer for configs with SYSEX_SCROLL parameters
static char uploadBuffer[20480]; // 20KB static buffer for config uploads
static size_t uploadBufferLen = 0;
static bool pendingRestart = false; // Flag to trigger restart after response
static char uploadError[128] = "";  // Error message for browser

// Response handler - called when upload is complete
void handleImportUploadResponse() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
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

    // Consolidated parsing using applyConfigJson (v1.5.5)
    applyConfigJson(doc.as<JsonObject>());

    Serial.println("Saving configuration...");
    savePresets();
    saveSystemSettings();
    saveAnalogInputs();
    Serial.println("Configuration saved!");

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
  server.on("/sysinfo", HTTP_GET, handleSysInfo);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/saveSystem", HTTP_POST, handleSaveSystem);
  server.on("/export", HTTP_GET, handleExport);
  server.on("/import", HTTP_GET, handleImport);
  server.on("/import", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200);
  });
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
        msg.minOut = mObj["min"] | 0;   // Output range min (editor sends 'min')
        msg.maxOut = mObj["max"] | 127; // Output range max (editor sends 'max')

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
  json += "\",\"presetCount\":";
  json += String(presetCount);
  json += ",\"presets\":[";

  for (int p = 0; p < presetCount; p++) {
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
  json += ",\"debugAnalogIn\":";
  json += systemConfig.debugAnalogIn ? "true" : "false";
  json += ",\"batteryAdcPin\":";
  json += String(systemConfig.batteryAdcPin);

  // OLED Configuration (v1.5)
  json += ",\"oled\":{";
  json += "\"type\":";
  json += String(
      (int)oledConfig.type); // 0=none, 1=128x64, 2=128x32, 3=128x128, 4=128x160
  json += ",\"rotation\":";
  json += String(oledConfig.rotation);
  // Display pins (v1.5.2)
  json += ",\"sdaPin\":";
  json += String(oledConfig.sdaPin);
  json += ",\"sclPin\":";
  json += String(oledConfig.sclPin);
  json += ",\"csPin\":";
  json += String(oledConfig.csPin);
  json += ",\"dcPin\":";
  json += String(oledConfig.dcPin);
  json += ",\"rstPin\":";
  json += String(oledConfig.rstPin);
  json += ",\"mosiPin\":";
  json += String(oledConfig.mosiPin);
  json += ",\"sclkPin\":";
  json += String(oledConfig.sclkPin);
  json += ",\"ledPin\":";
  json += String(oledConfig.ledPin);
  json += ",\"screens\":{";

  // Main screen
  json += "\"main\":{";
  json += "\"labelSize\":" + String(oledConfig.main.labelSize);
  json += ",\"titleSize\":" + String(oledConfig.main.titleSize);
  json += ",\"statusSize\":" + String(oledConfig.main.statusSize);
  json += ",\"bpmSize\":" + String(oledConfig.main.bpmSize);
  json += ",\"topRowY\":" + String(oledConfig.main.topRowY);
  json += ",\"titleY\":" + String(oledConfig.main.titleY);
  json += ",\"statusY\":" + String(oledConfig.main.statusY);
  json += ",\"bpmY\":" + String(oledConfig.main.bpmY);
  json += ",\"bottomRowY\":" + String(oledConfig.main.bottomRowY);
  json += ",\"showBpm\":";
  json += oledConfig.main.showBpm ? "true" : "false";
  json += ",\"showAnalog\":";
  json += oledConfig.main.showAnalog ? "true" : "false";
  json += ",\"showStatus\":";
  json += oledConfig.main.showStatus ? "true" : "false";
  json += ",\"showTopRow\":";
  json += oledConfig.main.showTopRow ? "true" : "false";
  json += ",\"showBottomRow\":";
  json += oledConfig.main.showBottomRow ? "true" : "false";
  json += ",\"titleAlign\":" + String(oledConfig.main.titleAlign);
  json += ",\"statusAlign\":" + String(oledConfig.main.statusAlign);
  json += ",\"bpmAlign\":" + String(oledConfig.main.bpmAlign);
  json += ",\"topRowMap\":\"";
  json += escapeJson(oledConfig.main.topRowMap);
  json += "\",\"bottomRowMap\":\"";
  json += escapeJson(oledConfig.main.bottomRowMap);
  // TFT Color Strip settings (v1.5.2)
  json += "\",\"showColorStrips\":";
  json += oledConfig.main.showColorStrips ? "true" : "false";
  json += ",\"colorStripHeight\":" + String(oledConfig.main.colorStripHeight);
  json += ",\"topRowAlign\":" + String(oledConfig.main.topRowAlign);
  json += ",\"bottomRowAlign\":" + String(oledConfig.main.bottomRowAlign);
  // Battery Indicator (v1.5)
  json += ",\"showBattery\":";
  json += oledConfig.main.showBattery ? "true" : "false";
  json += ",\"batteryX\":" + String(oledConfig.main.batteryX);
  json += ",\"batteryY\":" + String(oledConfig.main.batteryY);
  json += ",\"batteryScale\":" + String(oledConfig.main.batteryScale);
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
  json += "}"; // Close overlay

  // Global Special Actions (v1.5.1 - move inside system)
  json += "}}}"; // Close screens, oled (Wait! We need to close screens and oled
                 // BEFORE globalSpecialActions if we want it in system)

  // FIX: Close oled and screens properly before adding globalSpecialActions to
  // system We need to rewrite the closing logic. Current open scopes: system ->
  // oled -> screens We want: system -> globalSpecialActions

  // Actually, let's just close screens and oled here.
  // json += "}}"; // Close screens, oled. System is still open.

  // RE-WRITING THE BLOCK for clarity:
  /*
     The previous code was:
     ... overlay ...
     json += "}"; // close overlay
     json += globalSpecialActions...
     json += "}}}"; // close screens, oled, system

     This put globalSpecialActions INSIDE screens.
  */

  // Correct implementation:
  json += "}}"; // Close screens, oled (system still open)

  // Global Special Actions (v1.5.1 - inside system)
  json += ",\"globalSpecialActions\":[";
  for (int i = 0; i < systemConfig.buttonCount; i++) {
    if (i > 0)
      json += ",";
    json += "{\"enabled\":";
    json += globalSpecialActions[i].hasCombo ? "true" : "false";
    json += ",\"partner\":";
    json += String(globalSpecialActions[i].partner);
    json += ",\"type\":\"";
    json += getCommandTypeString(globalSpecialActions[i].comboAction.type);
    json += "\",\"action\":\"";
    json += getActionTypeString(globalSpecialActions[i].comboAction.action);
    json += "\",\"channel\":";
    json += String(globalSpecialActions[i].comboAction.channel);
    json += ",\"data1\":";
    json += String(globalSpecialActions[i].comboAction.data1);
    json += ",\"data2\":";
    json += String(globalSpecialActions[i].comboAction.data2);
    json += ",\"holdMs\":" +
            String(globalSpecialActions[i].comboAction.longPress.holdMs);
    json += ",\"label\":\"";
    char labelBuf[6] = {0};
    strncpy(labelBuf, globalSpecialActions[i].comboAction.label, 5);
    json += escapeJson(labelBuf);
    json += "\"}";
  }
  json += "]";

  json += "}"; // Close system

  // Analog Inputs
  json += ",\"analogInputs\":[";
  for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
    if (i > 0)
      json += ",";
    AnalogInputConfig &cfg = analogInputs[i];

    json += "{\"enabled\":";
    json += cfg.enabled ? "true" : "false";
    json += ",\"source\":\"";
    json += (cfg.source == AIN_SOURCE_MUX) ? "mux" : "gpio";
    json += "\",\"pin\":";
    json += String(cfg.pin);
    json += ",\"name\":\"";
    json += escapeJson(cfg.name);
    json += "\",\"inputMode\":";
    json += String(cfg.inputMode);

    // Action/Curve Type (v1.5.5)
    json += ",\"actionType\":\"";
    const char *atStr = "linear_linear";
    switch (cfg.actionType) {
    case AIN_ACTION_LOG:
      atStr = "log_linear";
      break;
    case AIN_ACTION_EXP:
      atStr = "linear_log";
      break;
    case AIN_ACTION_JOYSTICK:
      atStr = "joystick";
      break;
    default:
      atStr = "linear_linear";
      break;
    }
    json += atStr;
    json += "\"";

    // Curve parameters
    json += ",\"curve\":";
    json += String(cfg.curve, 2);
    json += ",\"center\":";
    json += String(cfg.center);
    json += ",\"deadzone\":";
    json += String(cfg.deadzone);

    // LED Feedback
    char hexColor[8];
    rgbToHex(hexColor, sizeof(hexColor), cfg.rgb);
    json += ",\"rgb\":\"";
    json += hexColor;
    json += "\",\"ledIndex\":";
    json += String(cfg.ledIndex);

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

  json += "]}"; // Close analogInputs array and root object
  return json;
}

// Consolidated Configuration Parsing (v1.5)
bool applyConfigJson(JsonObject doc) {
  // Config Metadata
  if (doc.containsKey("configName")) {
    strncpy(configProfileName, doc["configName"] | "", 31);
    configProfileName[31] = '\0';
  }
  if (doc.containsKey("lastModified")) {
    strncpy(configLastModified, doc["lastModified"] | "", 31);
    configLastModified[31] = '\0';
  }

  // v1.5.2: Parse preset count
  if (doc.containsKey("presetCount")) {
    presetCount = doc["presetCount"] | 4;
    if (presetCount < 1)
      presetCount = 1;
    if (presetCount > CHOCO_MAX_PRESETS)
      presetCount = CHOCO_MAX_PRESETS;
  }

  // Process Presets
  JsonArray presets = doc["presets"];
  if (!presets.isNull()) {
    for (int p = 0; p < (int)presets.size() && p < 4; p++) {
      JsonObject pObj = presets[p];
      if (pObj.containsKey("name")) {
        strncpy(presetNames[p], pObj["name"] | "Preset", 20);
        presetNames[p][20] = '\0';
      }

      if (pObj.containsKey("presetLedMode")) {
        String m = pObj["presetLedMode"].as<String>();
        if (m == "SELECTION")
          presetLedModes[p] = PRESET_LED_SELECTION;
        else if (m == "HYBRID")
          presetLedModes[p] = PRESET_LED_HYBRID;
        else
          presetLedModes[p] = PRESET_LED_NORMAL;
      }

      if (pObj.containsKey("syncMode")) {
        String s = pObj["syncMode"].as<String>();
        if (s == "SPM")
          presetSyncMode[p] = SYNC_SPM;
        else if (s == "GP5")
          presetSyncMode[p] = SYNC_GP5;
        else
          presetSyncMode[p] = SYNC_NONE;
      } else if (pObj.containsKey("syncSpm")) {
        // Legacy support: syncSpm boolean
        presetSyncMode[p] = (pObj["syncSpm"] | false) ? SYNC_SPM : SYNC_NONE;
      }

      // Buttons
      JsonArray buttons = pObj["buttons"];
      if (!buttons.isNull()) {
        for (int b = 0; b < (int)buttons.size() && b < MAX_BUTTONS; b++) {
          JsonObject bObj = buttons[b];
          ButtonConfig &btn = buttonConfigs[p][b];

          if (bObj.containsKey("name")) {
            strncpy(btn.name, bObj["name"] | "BTN", 20);
            btn.name[20] = '\0';
          }
          if (bObj.containsKey("ledMode")) {
            btn.ledMode = (bObj["ledMode"].as<String>() == "TOGGLE")
                              ? LED_TOGGLE
                              : LED_MOMENTARY;
          } else {
            btn.ledMode =
                LED_MOMENTARY; // v1.5.5: Default to MOMENTARY if not specified
          }
          btn.inSelectionGroup = bObj["inSelectionGroup"] | false;

          JsonArray msgs = bObj["messages"];
          if (!msgs.isNull()) {
            btn.messageCount = 0;
            for (JsonObject mObj : msgs) {
              if (btn.messageCount >= MAX_ACTIONS_PER_BUTTON)
                break;
              ActionMessage &msg = btn.messages[btn.messageCount];
              memset(&msg, 0, sizeof(ActionMessage));

              msg.action = parseActionType(mObj["action"] | "PRESS");
              msg.type = parseCommandType(mObj["type"] | "CC");
              msg.channel = mObj["channel"] | 1;
              msg.data1 = mObj["data1"] | 0;
              msg.data2 = mObj["data2"] | 0;
              msg.minInput = mObj["inMin"] | 0;
              msg.maxInput = mObj["inMax"] | 100;
              msg.minOut = mObj["min"] | 0;
              msg.maxOut = mObj["max"] | 127;

              if (mObj.containsKey("rgb"))
                hexToRgb(mObj["rgb"] | "#bb86fc", msg.rgb);
              if (mObj.containsKey("label")) {
                strncpy(msg.label, mObj["label"] | "", 5);
                msg.label[5] = '\0';
              }

              if (msg.action == ACTION_COMBO)
                msg.combo.partner = mObj["partner"] | 0;
              if (msg.action == ACTION_LONG_PRESS ||
                  msg.action == ACTION_2ND_LONG_PRESS) {
                msg.longPress.holdMs = mObj["holdMs"] | 500;
              }

              if (msg.type == TAP_TEMPO) {
                msg.tapTempo.rhythmPrev = mObj["rhythmPrev"] | 0;
                msg.tapTempo.rhythmNext = mObj["rhythmNext"] | 4;
                msg.tapTempo.tapLock = mObj["tapLock"] | 7;
              }

              if (msg.type == SYSEX && mObj.containsKey("sysex")) {
                const char *hex = mObj["sysex"];
                int len = strlen(hex) / 2;
                if (len > 48)
                  len = 48;
                msg.sysex.length = len;
                for (int i = 0; i < len; i++) {
                  char bh[3] = {hex[i * 2], hex[i * 2 + 1], 0};
                  msg.sysex.data[i] = (uint8_t)strtol(bh, NULL, 16);
                }
              }
              btn.messageCount++;
            }
          }
        }
      }
    }
  }

  // System Configuration
  JsonObject sys = doc["system"];
  if (!sys.isNull()) {
    if (sys.containsKey("bleDeviceName")) {
      strncpy(systemConfig.bleDeviceName, sys["bleDeviceName"] | "Chocotone",
              23);
      systemConfig.bleDeviceName[23] = '\0';
    }
    if (sys.containsKey("buttonCount"))
      systemConfig.buttonCount = sys["buttonCount"];
    if (sys.containsKey("brightness"))
      ledBrightnessOn = sys["brightness"];
    if (sys.containsKey("brightnessDim"))
      ledBrightnessDim = sys["brightnessDim"];
    if (sys.containsKey("brightnessTap"))
      ledBrightnessTap = sys["brightnessTap"];
    if (sys.containsKey("debounce"))
      buttonDebounce = sys["debounce"];
    if (sys.containsKey("wifiOnAtBoot"))
      systemConfig.wifiOnAtBoot = sys["wifiOnAtBoot"];
    if (sys.containsKey("bleMode")) {
      String bm = sys["bleMode"].as<String>();
      if (bm == "SERVER")
        systemConfig.bleMode = BLE_SERVER_ONLY;
      else if (bm == "DUAL")
        systemConfig.bleMode = BLE_DUAL_MODE;
      else
        systemConfig.bleMode = BLE_CLIENT_ONLY;
    }
    if (sys.containsKey("targetDevice"))
      systemConfig.targetDevice = (DeviceType)(sys["targetDevice"] | 0);
    if (sys.containsKey("midiChannel"))
      systemConfig.midiChannel = sys["midiChannel"] | 1;
    if (sys.containsKey("debugAnalogIn"))
      systemConfig.debugAnalogIn = sys["debugAnalogIn"] | false;
    if (sys.containsKey("analogInputCount")) {
      uint8_t cnt = sys["analogInputCount"] | 0;
      if (cnt > MAX_ANALOG_INPUTS)
        cnt = MAX_ANALOG_INPUTS;
      systemConfig.analogInputCount = cnt;
    }
    if (sys.containsKey("batteryAdcPin"))
      systemConfig.batteryAdcPin = sys["batteryAdcPin"] | 0;

    if (sys.containsKey("buttonPins")) {
      String pinsStr = sys["buttonPins"].as<String>();
      // Simple comma parser
      int idx = 0;
      char *token = strtok((char *)pinsStr.c_str(), ", ");
      while (token != NULL && idx < 10) {
        systemConfig.buttonPins[idx++] = atoi(token);
        token = strtok(NULL, ", ");
      }
    }
    if (sys.containsKey("ledPin"))
      systemConfig.ledPin = sys["ledPin"];
    if (sys.containsKey("ledsPerButton"))
      systemConfig.ledsPerButton = sys["ledsPerButton"];
    if (sys.containsKey("ledMap")) {
      String mapStr = sys["ledMap"].as<String>();
      int idx = 0;
      char *token = strtok((char *)mapStr.c_str(), ", ");
      while (token != NULL && idx < 10) {
        systemConfig.ledMap[idx++] = atoi(token);
        token = strtok(NULL, ", ");
      }
    }
    if (sys.containsKey("encoderA"))
      systemConfig.encoderA = sys["encoderA"];
    if (sys.containsKey("encoderB"))
      systemConfig.encoderB = sys["encoderB"];
    if (sys.containsKey("encoderBtn"))
      systemConfig.encoderBtn = sys["encoderBtn"];

    // Multiplexer (v1.5)
    if (sys.containsKey("multiplexer")) {
      JsonObject mux = sys["multiplexer"];
      systemConfig.multiplexer.enabled = mux["enabled"] | false;
      if (mux.containsKey("type")) {
        strncpy(systemConfig.multiplexer.type, mux["type"] | "cd74hc4067", 15);
        systemConfig.multiplexer.type[15] = '\0';
      }
      if (mux.containsKey("signalPin"))
        systemConfig.multiplexer.signalPin = mux["signalPin"];
      if (mux.containsKey("selectPins")) {
        JsonArray pins = mux["selectPins"];
        for (int i = 0; i < 4 && i < (int)pins.size(); i++)
          systemConfig.multiplexer.selectPins[i] = pins[i];
      }
      if (mux.containsKey("useFor")) {
        strncpy(systemConfig.multiplexer.useFor, mux["useFor"] | "analog", 7);
        systemConfig.multiplexer.useFor[7] = '\0';
      }
      if (mux.containsKey("buttonChannels")) {
        String chStr = mux["buttonChannels"].as<String>();
        for (int i = 0; i < 10; i++)
          systemConfig.multiplexer.buttonChannels[i] = -1;
        int idx = 0;
        char *token = strtok((char *)chStr.c_str(), ", ");
        while (token != NULL && idx < 10) {
          systemConfig.multiplexer.buttonChannels[idx++] = atoi(token);
          token = strtok(NULL, ", ");
        }
      }
    }

    // Global Special Actions (v1.5.1)
    if (sys.containsKey("globalSpecialActions")) {
      JsonArray gsa = sys["globalSpecialActions"];
      for (int i = 0; i < (int)gsa.size() && i < MAX_BUTTONS; i++) {
        JsonObject gObj = gsa[i];
        globalSpecialActions[i].hasCombo = gObj["enabled"] | false;
        globalSpecialActions[i].partner = gObj["partner"] | -1;

        // Action Message
        globalSpecialActions[i].comboAction.action =
            parseActionType(gObj["action"] | "COMBO");
        globalSpecialActions[i].comboAction.type =
            parseCommandType(gObj["type"] | "OFF");
        globalSpecialActions[i].comboAction.channel = gObj["channel"] | 1;
        globalSpecialActions[i].comboAction.data1 = gObj["data1"] | 0;
        globalSpecialActions[i].comboAction.data2 = gObj["data2"] | 0;
        globalSpecialActions[i].comboAction.longPress.holdMs =
            gObj["holdMs"] | 500;

        if (gObj.containsKey("label")) {
          strncpy(globalSpecialActions[i].comboAction.label, gObj["label"] | "",
                  5);
          globalSpecialActions[i].comboAction.label[5] = '\0';
        }
      }
    }

    if (sys.containsKey("oled")) {
      JsonObject oled = sys["oled"];
      String t = oled["type"] | "128x64";
      // Handle string types
      if (t == "none" || t == "0")
        oledConfig.type = OLED_NONE;
      else if (t == "128x64" || t == "1")
        oledConfig.type = OLED_128X64;
      else if (t == "128x32" || t == "2")
        oledConfig.type = OLED_128X32;
      else if (t == "128x128" || t == "3")
        oledConfig.type = TFT_128X128;
      else if (t == "128x160" || t == "4")
        oledConfig.type = TFT_128X160;
      else
        oledConfig.type = OLED_128X64;

      int r = oled["rotation"] | 0;
      if (r >= 270)
        oledConfig.rotation = 3;
      else if (r >= 180)
        oledConfig.rotation = 2;
      else if (r >= 90)
        oledConfig.rotation = 1;
      else
        oledConfig.rotation = 0;

      // Display pins (v1.5.2)
      oledConfig.sdaPin = oled["sdaPin"] | 21;
      oledConfig.sclPin = oled["sclPin"] | 22;
      oledConfig.csPin = oled["csPin"] | 15;
      oledConfig.dcPin = oled["dcPin"] | 2;
      oledConfig.rstPin = oled["rstPin"] | 4;
      oledConfig.mosiPin = oled["mosiPin"] | 23;
      oledConfig.sclkPin = oled["sclkPin"] | 18;
      oledConfig.ledPin = oled["ledPin"] | 32;

      if (oled.containsKey("screens")) {
        JsonObject scs = oled["screens"];
        if (scs.containsKey("main")) {
          JsonObject m = scs["main"];
          oledConfig.main.labelSize = m["labelSize"] | 1;
          oledConfig.main.titleSize = m["titleSize"] | 2;
          oledConfig.main.statusSize = m["statusSize"] | 1;
          oledConfig.main.bpmSize = m["bpmSize"] | 1;
          oledConfig.main.topRowY = m["topRowY"] | 0;
          oledConfig.main.titleY = m["titleY"] | 14;
          oledConfig.main.statusY = m["statusY"] | 44;
          oledConfig.main.bpmY = m["bpmY"] | 32;
          oledConfig.main.bottomRowY = m["bottomRowY"] | 56;
          oledConfig.main.showBpm = m["showBpm"] | true;
          oledConfig.main.showAnalog = m["showAnalog"] | false;
          oledConfig.main.showTopRow = m["showTopRow"] | true;
          oledConfig.main.showBottomRow = m["showBottomRow"] | true;
          oledConfig.main.showStatus =
              m.containsKey("showStatus") ? m["showStatus"].as<bool>() : true;
          oledConfig.main.titleAlign = m["titleAlign"] | 1;
          oledConfig.main.statusAlign = m["statusAlign"] | 0;
          oledConfig.main.bpmAlign = m["bpmAlign"] | 1;
          if (m.containsKey("topRowMap")) {
            strncpy(oledConfig.main.topRowMap, m["topRowMap"] | "5,6,7,8", 31);
            oledConfig.main.topRowMap[31] = '\0';
          }
          if (m.containsKey("bottomRowMap")) {
            strncpy(oledConfig.main.bottomRowMap, m["bottomRowMap"] | "1,2,3,4",
                    31);
            oledConfig.main.bottomRowMap[31] = '\0';
          }
          // TFT Color Strip settings (v1.5.2)
          oledConfig.main.showColorStrips = m["showColorStrips"] | false;
          oledConfig.main.colorStripHeight = m["colorStripHeight"] | 4;
          oledConfig.main.topRowAlign = m["topRowAlign"] | 0;
          oledConfig.main.bottomRowAlign = m["bottomRowAlign"] | 0;
          // Battery Indicator (v1.5)
          oledConfig.main.showBattery = m["showBattery"] | false;
          oledConfig.main.batteryX = m["batteryX"] | 116;
          oledConfig.main.batteryY = m["batteryY"] | 0;
          oledConfig.main.batteryScale = m["batteryScale"] | 1;
        }
        if (scs.containsKey("menu")) {
          JsonObject m = scs["menu"];
          oledConfig.menu.labelSize = m["itemSize"] | 1;
          oledConfig.menu.titleSize = m["headerSize"] | 1;
          oledConfig.menu.topRowY = m["headerY"] | 0;
          oledConfig.menu.titleY = m["itemStartY"] | 14;
        }
        if (scs.containsKey("tap")) {
          JsonObject m = scs["tap"];
          oledConfig.tap.labelSize = m["labelSize"] | 1;
          oledConfig.tap.titleSize = m["bpmSize"] | 3;
          oledConfig.tap.statusSize = m["patternSize"] | 1;
          oledConfig.tap.topRowY = m["labelTopY"] | 0;
          oledConfig.tap.titleY = m["bpmY"] | 16;
          oledConfig.tap.statusY = m["patternY"] | 46;
          oledConfig.tap.bottomRowY = m["labelBottomY"] | 56;
        }
        if (scs.containsKey("overlay")) {
          JsonObject m = scs["overlay"];
          oledConfig.overlay.titleSize = m["textSize"] | 2;
        }
      } // End screens
    } // End oled

    // Global Special Actions (v1.5.2: optimized format with index field)
    // Reset all global special actions first, then apply only the enabled ones
    if (sys.containsKey("globalSpecialActions")) {
      JsonArray gsaArr = sys["globalSpecialActions"];
      // Reset all global special actions to disabled state first
      for (int i = 0; i < MAX_BUTTONS; i++) {
        globalSpecialActions[i].hasCombo = false;
      }
      // Apply only the enabled actions from the config (may have index field)
      for (int i = 0; i < (int)gsaArr.size(); i++) {
        JsonObject gsaObj = gsaArr[i];
        // Use index field if present (optimized export), otherwise use array
        // position
        int idx = gsaObj.containsKey("index") ? (int)gsaObj["index"] : i;
        if (idx < 0 || idx >= MAX_BUTTONS)
          continue;
        globalSpecialActions[idx].hasCombo = gsaObj["enabled"] | false;
        globalSpecialActions[idx].partner = gsaObj["partner"] | -1;
        globalSpecialActions[idx].comboAction.type =
            parseCommandType(gsaObj["type"] | "OFF");
        globalSpecialActions[idx].comboAction.action =
            parseActionType(gsaObj["action"] | "PRESS");

        // MIDI data fields (required for CC, NOTE, PC commands)
        globalSpecialActions[idx].comboAction.channel = gsaObj["channel"] | 1;
        globalSpecialActions[idx].comboAction.data1 = gsaObj["data1"] | 0;
        globalSpecialActions[idx].comboAction.data2 = gsaObj["data2"] | 0;

        if (globalSpecialActions[idx].comboAction.action == ACTION_LONG_PRESS ||
            globalSpecialActions[idx].comboAction.action ==
                ACTION_2ND_LONG_PRESS) {
          globalSpecialActions[idx].comboAction.longPress.holdMs =
              gsaObj["holdMs"] | 500;
        }

        if (gsaObj.containsKey("label")) {
          strncpy(globalSpecialActions[idx].comboAction.label,
                  gsaObj["label"] | "", 5);
          globalSpecialActions[idx].comboAction.label[5] = '\0';
        }

        // Debug output for global special actions
        Serial.printf(
            "GSA[%d]: enabled=%d, partner=%d, action=%d, type=%d, holdMs=%d\n",
            idx, globalSpecialActions[idx].hasCombo,
            globalSpecialActions[idx].partner,
            globalSpecialActions[idx].comboAction.action,
            globalSpecialActions[idx].comboAction.type,
            globalSpecialActions[idx].comboAction.longPress.holdMs);
      }
    }
  } // End sys

  // Analog Inputs (v1.5.2: optimized format with index field)
  // Reset all analog inputs first, then apply only the enabled ones
  JsonArray analogs = doc["analogInputs"];
  if (!analogs.isNull()) {
    // Reset all analog inputs to disabled state first
    for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
      analogInputs[i].enabled = false;
    }
    // Apply only the enabled inputs from the config (may have index field)
    for (int i = 0; i < (int)analogs.size(); i++) {
      JsonObject aObj = analogs[i];
      // Use index field if present (optimized export), otherwise use array
      // position
      int idx = aObj.containsKey("index") ? (int)aObj["index"] : i;
      if (idx < 0 || idx >= MAX_ANALOG_INPUTS)
        continue;
      AnalogInputConfig &acfg = analogInputs[idx];

      // If an input appears in the config, it's enabled (editor only exports
      // enabled inputs) Honor explicit enabled field if present, otherwise
      // default to true
      acfg.enabled = aObj.containsKey("enabled") ? (bool)aObj["enabled"] : true;
      if (aObj.containsKey("source")) {
        acfg.source = (aObj["source"].as<String>() == "mux") ? AIN_SOURCE_MUX
                                                             : AIN_SOURCE_GPIO;
      }
      if (aObj.containsKey("pin"))
        acfg.pin = aObj["pin"];
      if (aObj.containsKey("name")) {
        strncpy(acfg.name, aObj["name"] | "A1", 10);
        acfg.name[10] = '\0';
      }
      if (aObj.containsKey("inputMode"))
        acfg.inputMode = (AnalogInputMode)(aObj["inputMode"] | 0);

      if (aObj.containsKey("actionType")) {
        String at = aObj["actionType"].as<String>();
        if (at == "log_linear")
          acfg.actionType = AIN_ACTION_LOG;
        else if (at == "linear_log")
          acfg.actionType = AIN_ACTION_EXP;
        else if (at == "joystick")
          acfg.actionType = AIN_ACTION_JOYSTICK;
        else
          acfg.actionType = AIN_ACTION_LINEAR;
      }
      if (aObj.containsKey("actionOptions")) {
        JsonObject ao = aObj["actionOptions"];
        if (ao.containsKey("curve"))
          acfg.curve = ao["curve"];
        if (ao.containsKey("center"))
          acfg.center = ao["center"];
        if (ao.containsKey("deadzone"))
          acfg.deadzone = ao["deadzone"];
      }
      // Also check top-level curve/center/deadzone (v1.5.5 export format)
      if (aObj.containsKey("curve"))
        acfg.curve = aObj["curve"];
      if (aObj.containsKey("center"))
        acfg.center = aObj["center"];
      if (aObj.containsKey("deadzone"))
        acfg.deadzone = aObj["deadzone"];
      if (aObj.containsKey("rgb"))
        hexToRgb(aObj["rgb"].as<String>(), acfg.rgb);
      if (aObj.containsKey("ledIndex"))
        acfg.ledIndex = aObj["ledIndex"];

      if (aObj.containsKey("piezoThreshold"))
        acfg.piezoThreshold = aObj["piezoThreshold"];
      if (aObj.containsKey("piezoScanTime"))
        acfg.piezoScanTime = aObj["piezoScanTime"];
      if (aObj.containsKey("piezoMaskTime"))
        acfg.piezoMaskTime = aObj["piezoMaskTime"];
      if (aObj.containsKey("fsrThreshold"))
        acfg.fsrThreshold = aObj["fsrThreshold"];
      if (aObj.containsKey("minVal"))
        acfg.minVal = aObj["minVal"];
      if (aObj.containsKey("maxVal"))
        acfg.maxVal = aObj["maxVal"];
      else if (acfg.maxVal == 0)
        acfg.maxVal = 4095;
      if (aObj.containsKey("inverted"))
        acfg.inverted = aObj["inverted"];
      if (aObj.containsKey("emaAlpha"))
        acfg.emaAlpha = aObj["emaAlpha"];
      else if (acfg.emaAlpha == 0)
        acfg.emaAlpha = 0.05f; // DEFAULT_EMA_ALPHA

      if (aObj.containsKey("hysteresis"))
        acfg.hysteresis = aObj["hysteresis"];
      else if (acfg.hysteresis == 0)
        acfg.hysteresis = 3; // DEFAULT_HYSTERESIS

      JsonArray amsgs = aObj["messages"];
      if (!amsgs.isNull()) {
        acfg.messageCount = 0;
        for (JsonObject amObj : amsgs) {
          if (acfg.messageCount >= 4)
            break;
          ActionMessage &amsg = acfg.messages[acfg.messageCount];
          memset(&amsg, 0, sizeof(ActionMessage));
          amsg.action = ACTION_PRESS;
          String typeStr = amObj["type"] | "CC";
          amsg.type = parseCommandType(typeStr);
          amsg.channel = amObj["channel"] | 1;
          amsg.data1 = amObj["data1"] | 0;
          amsg.data2 = amObj["data2"] | 0;
          amsg.minInput = amObj["inMin"] | 0;
          amsg.maxInput = amObj["inMax"] | 100;
          amsg.minOut = amObj["min"] | 0;
          amsg.maxOut = amObj["max"] | 127;
          Serial.printf("AIN[%d] Msg[%d]: type='%s' -> %d, data1=%d\n", idx,
                        acfg.messageCount, typeStr.c_str(), amsg.type,
                        amsg.data1);
          acfg.messageCount++;
        }
      }
    }
  }

  return true;
}

bool processConfigChunk(const String &jsonStr, int chunkNum) {
  // Parse and apply JSON config (same logic as multipart upload handler)
  Serial.printf("processConfigChunk: Parsing %d bytes...\n", jsonStr.length());
  Serial.printf("Free heap before parse: %d\n", ESP.getFreeHeap());

  DeserializationError error;

#ifdef BOARD_HAS_PSRAM
  // Use PSRAM allocator for large configs (ESP32-S3 with 8MB PSRAM)
  struct SpiRamAllocator {
    void *allocate(size_t size) {
      return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    }
    void deallocate(void *ptr) { heap_caps_free(ptr); }
    void *reallocate(void *ptr, size_t new_size) {
      return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    }
  };
  BasicJsonDocument<SpiRamAllocator> doc(65536); // 64KB in PSRAM
  error = deserializeJson(doc, jsonStr);
  Serial.println("Using PSRAM for JSON parsing");
#else
  // Standard heap allocation - may fail for large configs
  JsonDocument doc;
  error = deserializeJson(doc, jsonStr);
  Serial.println("Using heap for JSON parsing");
#endif

  Serial.printf("Free heap after parse: %d\n", ESP.getFreeHeap());

  if (error) {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return false;
  }

  return applyConfigJson(doc.as<JsonObject>());
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
static unsigned long lastEditorActivityTime = 0;

bool isEditorConnected() {
  if (lastEditorActivityTime == 0)
    return false;
  return (millis() - lastEditorActivityTime < 30000); // 30 second timeout
}

void refreshEditorActivity() { lastEditorActivityTime = millis(); }

void handleSerialConfig() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer.trim();
      refreshEditorActivity(); // Any command refreshes activity

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
              Serial.print(",\"inMin\":");
              Serial.print(msg.minInput);
              Serial.print(",\"inMax\":");
              Serial.print(msg.maxInput);
              Serial.print(",\"min\":");
              Serial.print(msg.minOut);
              Serial.print(",\"max\":");
              Serial.print(msg.maxOut);
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
        Serial.print(",\"analogInputCount\":");
        Serial.print(systemConfig.analogInputCount);
        Serial.print(",\"targetDevice\":");
        Serial.print((uint8_t)systemConfig.targetDevice);
        Serial.print(",\"midiChannel\":");
        Serial.print(systemConfig.midiChannel);
        Serial.print(",\"debugAnalogIn\":");
        Serial.print(systemConfig.debugAnalogIn ? "true" : "false");
        Serial.print(",\"batteryAdcPin\":");
        Serial.print(systemConfig.batteryAdcPin);

        // Multiplexer Settings
        Serial.print(",\"multiplexer\":{");
        Serial.print("\"enabled\":");
        Serial.print(systemConfig.multiplexer.enabled ? "true" : "false");
        Serial.print(",\"type\":\"");
        Serial.print(systemConfig.multiplexer.type);
        Serial.print("\",\"signalPin\":");
        Serial.print(systemConfig.multiplexer.signalPin);
        Serial.print(",\"selectPins\":[");
        for (int i = 0; i < 4; i++) {
          if (i > 0)
            Serial.print(",");
          Serial.print(systemConfig.multiplexer.selectPins[i]);
        }
        Serial.print("],\"useFor\":\"");
        Serial.print(systemConfig.multiplexer.useFor);
        Serial.print("\",\"buttonChannels\":\"");
        for (int i = 0; i < 10; i++) {
          if (i > 0)
            Serial.print(",");
          Serial.print(systemConfig.multiplexer.buttonChannels[i]);
        }
        Serial.print("\"}");

        Serial.print(",\"oled\":{");
        Serial.print("\"type\":\"");
        Serial.print(oledConfig.type == OLED_128X32   ? "128x32"
                     : oledConfig.type == TFT_128X128 ? "128x128"
                                                      : "128x64");
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
        Serial.print(",\"bpmSize\":");
        Serial.print(oledConfig.main.bpmSize);
        Serial.print(",\"topRowY\":");
        Serial.print(oledConfig.main.topRowY);
        Serial.print(",\"titleY\":");
        Serial.print(oledConfig.main.titleY);
        Serial.print(",\"statusY\":");
        Serial.print(oledConfig.main.statusY);
        Serial.print(",\"bpmY\":");
        Serial.print(oledConfig.main.bpmY);
        Serial.print(",\"bottomRowY\":");
        Serial.print(oledConfig.main.bottomRowY);
        Serial.print(",\"showBpm\":");
        Serial.print(oledConfig.main.showBpm ? "true" : "false");
        Serial.print(",\"showAnalog\":");
        Serial.print(oledConfig.main.showAnalog ? "true" : "false");
        Serial.print(",\"showStatus\":");
        Serial.print(oledConfig.main.showStatus ? "true" : "false");
        Serial.print(",\"showTopRow\":");
        Serial.print(oledConfig.main.showTopRow ? "true" : "false");
        Serial.print(",\"showBottomRow\":");
        Serial.print(oledConfig.main.showBottomRow ? "true" : "false");
        Serial.print(",\"titleAlign\":");
        Serial.print(oledConfig.main.titleAlign);
        Serial.print(",\"statusAlign\":");
        Serial.print(oledConfig.main.statusAlign);
        Serial.print(",\"bpmAlign\":");
        Serial.print(oledConfig.main.bpmAlign);
        Serial.print(",\"topRowMap\":\"");
        Serial.print(oledConfig.main.topRowMap);
        Serial.print("\",\"bottomRowMap\":\"");
        Serial.print(oledConfig.main.bottomRowMap);
        Serial.print("\",\"showColorStrips\":");
        Serial.print(oledConfig.main.showColorStrips ? "true" : "false");
        Serial.print(",\"colorStripHeight\":");
        Serial.print(oledConfig.main.colorStripHeight);
        Serial.print(",\"topRowAlign\":");
        Serial.print(oledConfig.main.topRowAlign);
        Serial.print(",\"bottomRowAlign\":");
        Serial.print(oledConfig.main.bottomRowAlign);
        // Battery Indicator (v1.5)
        Serial.print(",\"showBattery\":");
        Serial.print(oledConfig.main.showBattery ? "true" : "false");
        Serial.print(",\"batteryX\":");
        Serial.print(oledConfig.main.batteryX);
        Serial.print(",\"batteryY\":");
        Serial.print(oledConfig.main.batteryY);
        Serial.print(",\"batteryScale\":");
        Serial.print(oledConfig.main.batteryScale);

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
        Serial.print("}}}}"); // Close overlay, screens, oled, system

        Serial.print(",\"analogInputs\":[");

        // Analog Inputs (Serial Export)
        for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
          if (i > 0)
            Serial.print(",");
          AnalogInputConfig &cfg = analogInputs[i];

          Serial.print("{\"enabled\":");
          Serial.print(cfg.enabled ? "true" : "false");
          Serial.print(",\"source\":\"");
          Serial.print(cfg.source == AIN_SOURCE_MUX ? "mux" : "gpio");
          Serial.print("\",\"pin\":");
          Serial.print(cfg.pin);
          Serial.print(",\"name\":\"");
          Serial.print(cfg.name);
          Serial.print("\",\"inputMode\":");
          Serial.print(cfg.inputMode);

          Serial.print(",\"actionType\":\"");
          Serial.print(cfg.actionType == AIN_ACTION_LOG
                           ? "log_linear"
                           : (cfg.actionType == AIN_ACTION_EXP
                                  ? "linear_log"
                                  : (cfg.actionType == AIN_ACTION_JOYSTICK
                                         ? "joystick"
                                         : "linear_linear")));
          Serial.print("\"");

          if (cfg.actionType == AIN_ACTION_LOG ||
              cfg.actionType == AIN_ACTION_EXP) {
            Serial.print(",\"actionOptions\":{\"curve\":");
            Serial.print(cfg.curve);
            Serial.print("}");
          } else if (cfg.actionType == AIN_ACTION_JOYSTICK) {
            Serial.print(",\"actionOptions\":{\"center\":");
            Serial.print(cfg.center);
            Serial.print(",\"deadzone\":");
            Serial.print(cfg.deadzone);
            Serial.print("}");
          }

          char hexColor[8];
          rgbToHex(hexColor, sizeof(hexColor), cfg.rgb);
          Serial.print(",\"rgb\":\"");
          Serial.print(hexColor);
          Serial.print("\"");
          Serial.print(",\"ledIndex\":");
          Serial.print(cfg.ledIndex);

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
            Serial.print(",\"min\":");
            Serial.print(msg.minOut);
            Serial.print(",\"max\":");
            Serial.print(msg.maxOut);
            Serial.print("}");
          }
          Serial.print("]}");
        }
        Serial.print("]"); // Close analogInputs array only (root still open)

        // Global Special Actions (Serial Export)
        Serial.print(",\"globalSpecialActions\":[");
        for (int i = 0; i < systemConfig.buttonCount; i++) {
          if (i > 0)
            Serial.print(",");
          const GlobalSpecialAction &gsa = globalSpecialActions[i];
          const ActionMessage &msg = gsa.comboAction;

          Serial.print("{\"enabled\":");
          Serial.print(gsa.hasCombo ? "true" : "false");
          Serial.print(",\"action\":\"");
          Serial.print(getActionTypeString(msg.action));
          Serial.print("\",\"partner\":");
          Serial.print(gsa.partner);

          Serial.print(",\"type\":\"");
          Serial.print(getCommandTypeString(msg.type));
          Serial.print("\",\"channel\":");
          Serial.print(msg.channel);
          Serial.print(",\"data1\":");
          Serial.print(msg.data1);
          Serial.print(",\"data2\":");
          Serial.print(msg.data2);

          char hexColor[8];
          rgbToHex(hexColor, sizeof(hexColor), msg.rgb);
          Serial.print(",\"rgb\":\"");
          Serial.print(hexColor);
          Serial.print("\"");

          if (msg.label[0] != '\0') {
            Serial.print(",\"label\":\"");
            Serial.print(msg.label);
            Serial.print("\"");
          }
          if (msg.action == ACTION_LONG_PRESS ||
              msg.action == ACTION_2ND_LONG_PRESS) {
            Serial.print(",\"holdMs\":");
            Serial.print(msg.longPress.holdMs);
          }
          Serial.print("}");
        }
        Serial.print("]}"); // Close globalSpecialActions array AND root object
        Serial.println();   // End line
        Serial.println("CONFIG_END");
      }
      // SET_CONFIG_START - Begin receiving config
      else if (serialBuffer == "SET_CONFIG_START") {
        // Pause BLE scan and clear results to free heap for JSON parsing
        doScan = false;
        BLEScan *pScan = BLEDevice::getScan();
        if (pScan) {
          pScan->stop();
          pScan->clearResults(); // Free memory from scan results
        }
        uploadBufferLen = 0;
        memset(uploadBuffer, 0, sizeof(uploadBuffer));
        Serial.printf("READY (Heap: %d)\n", ESP.getFreeHeap());
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
        Serial.printf("Free heap before cleanup: %d\n", ESP.getFreeHeap());

        // Clear existing config arrays to free memory before parsing
        // This returns ~10KB+ of RAM consumed by loaded config
        for (int p = 0; p < CHOCO_MAX_PRESETS; p++) {
          for (int b = 0; b < MAX_BUTTONS; b++) {
            memset(&buttonConfigs[p][b], 0, sizeof(ButtonConfig));
          }
        }
        for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
          memset(&analogInputs[i], 0, sizeof(AnalogInputConfig));
        }
        for (int i = 0; i < MAX_BUTTONS; i++) {
          globalSpecialActions[i].hasCombo = false;
        }

        Serial.printf("Free heap after cleanup: %d\n", ESP.getFreeHeap());

        // Try to use PSRAM for JSON parsing if available (ESP32-S3 with PSRAM)
        DeserializationError error;

#ifdef BOARD_HAS_PSRAM
        // Use PSRAM allocator for large configs (ESP32-S3 with 8MB PSRAM)
        struct SpiRamAllocator {
          void *allocate(size_t size) {
            return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
          }
          void deallocate(void *ptr) { heap_caps_free(ptr); }
          void *reallocate(void *ptr, size_t new_size) {
            return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
          }
        };
        BasicJsonDocument<SpiRamAllocator> doc(65536); // 64KB in PSRAM
        error = deserializeJson(doc, uploadBuffer);
        Serial.println("Using PSRAM for JSON parsing");
#else
        // MEMORY OPTIMIZATION FOR ESP32 CLASSIC:
        // BLE consumes huge amount of RAM (30KB+). When parsing large JSON,
        // we often run out of heap (NoMemory).
        // Solution: Temporarily de-initialize BLE to free RAM.
        // We are going to reboot anyway if valid, or we can restart BLE if
        // invalid.

        Serial.println("Releasing BLE memory for JSON parsing...");
        BLEDevice::deinit(true);
        delay(200); // Allow time for memory to be freed

        // ZERO-COPY IN-PLACE PARSING
        // By passing the mutable char* buffer, ArduinoJson modifies strings
        // in-place. This drastically reduces memory usage.
        size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        Serial.printf("Largest free block: %d bytes\n", largestBlock);

        // For in-place parsing, we need memory for the JSON tree structure
        // Use largest available block minus margin for safety
        size_t docSize = largestBlock - 1024; // Leave 1KB margin
        // If we have plenty of RAM (after freeing BLE), cap it reasonably
        if (docSize > 32768)
          docSize = 32768; // Cap at 32KB (should be plenty for 8KB JSON)

        DynamicJsonDocument doc(docSize);
        Serial.printf("Allocated DynamicJsonDocument(%d), heap now: %d\n",
                      docSize, ESP.getFreeHeap());

        // Pass mutable buffer for zero-copy/in-place parsing
        error = deserializeJson(doc, uploadBuffer);
        Serial.println("Using zero-copy in-place JSON parsing");
#endif

        Serial.printf("Free heap after parse: %d\n", ESP.getFreeHeap());
        Serial.printf("JSON doc memory: %d bytes\n", doc.memoryUsage());

        if (error) {
          Serial.print("JSON_ERROR:");
          Serial.println(error.c_str());

#ifndef BOARD_HAS_PSRAM
          // If we failed and are on Classic ESP32, we killed BLE.
          // We must restart it to keep device functional without reboot.
          Serial.println("Parsing failed. Restoring BLE...");
          setup_ble_midi();
          if (systemConfig.bleMode == BLE_CLIENT_ONLY ||
              systemConfig.bleMode == BLE_DUAL_MODE) {
            doScan = true; // Will prompt loop to scan
          }
#endif
        } else {
          applyConfigJson(doc.as<JsonObject>());

          savePresets();
          saveSystemSettings();
          saveAnalogInputs();

          // Reinitialize display
          initDisplayHardware();
          displayOLED();
          updateLeds();

          Serial.println("SAVE_OK");
          Serial.println("Config saved successfully!");
          Serial.println("Rebooting in 1 second...");
          delay(1000);
          ESP.restart();
        }
        // doc goes out of scope, memory freed automatically
      }
      // Clear buffer after any command processed
      serialBuffer = "";
    } else {
      // Not a newline - add to buffer
      serialBuffer += c;
      // Prevent buffer overflow
      if (serialBuffer.length() > 16000) {
        serialBuffer = "";
      }
    }
  }
}

// ============================================================================
// BLUETOOTH SERIAL (SPP) - Wireless editor connection
// ============================================================================
// Similar to USB Serial, but over Classic Bluetooth.
// Note: Cannot run simultaneously with BLE MIDI (shared radio).

static String btSerialBuffer = "";

// Bluetooth Serial Startup
void turnBtSerialOn() {
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
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
#else
  Serial.println("BT Serial not supported on ESP32-S3");
#endif
}

void turnBtSerialOff() {
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
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
#endif
}

// Handle Bluetooth Serial config commands - mirrors handleSerialConfig()
void handleBtSerialConfig() {
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
  if (!isBtSerialOn)
    return;

  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\n') {
      btSerialBuffer.trim();
      refreshEditorActivity(); // Any BT command refreshes activity

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
              SerialBT.print(",\"inMin\":");
              SerialBT.print(msg.minInput);
              SerialBT.print(",\"inMax\":");
              SerialBT.print(msg.maxInput);
              SerialBT.print(",\"min\":");
              SerialBT.print(msg.minOut);
              SerialBT.print(",\"max\":");
              SerialBT.print(msg.maxOut);
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
        SerialBT.print(",\"analogInputCount\":");
        SerialBT.print(systemConfig.analogInputCount);
        SerialBT.print(",\"targetDevice\":");
        SerialBT.print((uint8_t)systemConfig.targetDevice);
        SerialBT.print(",\"midiChannel\":");
        SerialBT.print(systemConfig.midiChannel);
        SerialBT.print(",\"debugAnalogIn\":");
        SerialBT.print(systemConfig.debugAnalogIn ? "true" : "false");

        // Multiplexer Settings
        SerialBT.print(",\"multiplexer\":{");
        SerialBT.print("\"enabled\":");
        SerialBT.print(systemConfig.multiplexer.enabled ? "true" : "false");
        SerialBT.print(",\"type\":\"");
        SerialBT.print(systemConfig.multiplexer.type);
        SerialBT.print("\",\"signalPin\":");
        SerialBT.print(systemConfig.multiplexer.signalPin);
        SerialBT.print(",\"selectPins\":[");
        for (int i = 0; i < 4; i++) {
          if (i > 0)
            SerialBT.print(",");
          SerialBT.print(systemConfig.multiplexer.selectPins[i]);
        }
        SerialBT.print("],\"useFor\":\"");
        SerialBT.print(systemConfig.multiplexer.useFor);
        SerialBT.print("\",\"buttonChannels\":\"");
        for (int i = 0; i < 10; i++) {
          if (i > 0)
            SerialBT.print(",");
          SerialBT.print(systemConfig.multiplexer.buttonChannels[i]);
        }
        SerialBT.print("\"}");

        SerialBT.print(",\"oled\":{");
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
        SerialBT.print("}}}}"); // Close overlay, screens, oled, system

        SerialBT.print(",\"analogInputs\":[");

        // Analog Inputs (BT Export)
        for (int i = 0; i < MAX_ANALOG_INPUTS; i++) {
          yield(); // Feed watchdog
          if (i > 0)
            SerialBT.print(",");
          AnalogInputConfig &cfg = analogInputs[i];

          SerialBT.print("{\"enabled\":");
          SerialBT.print(cfg.enabled ? "true" : "false");
          SerialBT.print(",\"source\":\"");
          SerialBT.print(cfg.source == AIN_SOURCE_MUX ? "mux" : "gpio");
          SerialBT.print("\",\"pin\":");
          SerialBT.print(cfg.pin);
          SerialBT.print(",\"name\":\"");
          SerialBT.print(cfg.name);
          SerialBT.print("\",\"inputMode\":");
          SerialBT.print(cfg.inputMode);

          SerialBT.print(",\"actionType\":\"");
          SerialBT.print(cfg.actionType == AIN_ACTION_LOG
                             ? "log_linear"
                             : (cfg.actionType == AIN_ACTION_EXP
                                    ? "linear_log"
                                    : (cfg.actionType == AIN_ACTION_JOYSTICK
                                           ? "joystick"
                                           : "linear_linear")));

          if (cfg.actionType == AIN_ACTION_LOG ||
              cfg.actionType == AIN_ACTION_EXP) {
            SerialBT.print(",\"actionOptions\":{\"curve\":");
            SerialBT.print(cfg.curve);
            SerialBT.print("}");
          } else if (cfg.actionType == AIN_ACTION_JOYSTICK) {
            SerialBT.print(",\"actionOptions\":{\"center\":");
            SerialBT.print(cfg.center);
            SerialBT.print(",\"deadzone\":");
            SerialBT.print(cfg.deadzone);
            SerialBT.print("}");
          }

          char hexColor[8];
          rgbToHex(hexColor, sizeof(hexColor), cfg.rgb);
          SerialBT.print(",\"rgb\":\"");
          SerialBT.print(hexColor);
          SerialBT.print("\"");
          SerialBT.print(",\"ledIndex\":");
          SerialBT.print(cfg.ledIndex);

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
            SerialBT.print(",\"min\":");
            SerialBT.print(msg.minOut);
            SerialBT.print(",\"max\":");
            SerialBT.print(msg.maxOut);
            SerialBT.print("}");
          }
          SerialBT.print("]}");
        }
        SerialBT.print("]}"); // Close analogInputs array AND the root object

        // Global Special Actions (BT Serial Export)
        SerialBT.print(",\"globalSpecialActions\":[");
        for (int i = 0; i < systemConfig.buttonCount; i++) {
          if (i > 0)
            SerialBT.print(",");
          const GlobalSpecialAction &gsa = globalSpecialActions[i];
          const ActionMessage &msg = gsa.comboAction;

          SerialBT.print("{\"enabled\":");
          SerialBT.print(gsa.hasCombo ? "true" : "false");
          SerialBT.print(",\"action\":\"");
          SerialBT.print(getActionTypeString(msg.action));
          SerialBT.print("\",\"partner\":");
          SerialBT.print(gsa.partner);

          SerialBT.print(",\"type\":\"");
          SerialBT.print(getCommandTypeString(msg.type));
          SerialBT.print("\",\"channel\":");
          SerialBT.print(msg.channel);
          SerialBT.print(",\"data1\":");
          SerialBT.print(msg.data1);
          SerialBT.print(",\"data2\":");
          SerialBT.print(msg.data2);

          char hexColor[8];
          rgbToHex(hexColor, sizeof(hexColor), msg.rgb);
          SerialBT.print(",\"rgb\":\"");
          SerialBT.print(hexColor);
          SerialBT.print("\"");

          if (msg.label[0] != '\0') {
            SerialBT.print(",\"label\":\"");
            SerialBT.print(msg.label);
            SerialBT.print("\"");
          }
          if (msg.action == ACTION_LONG_PRESS ||
              msg.action == ACTION_2ND_LONG_PRESS) {
            SerialBT.print(",\"holdMs\":");
            SerialBT.print(msg.longPress.holdMs);
          }
          SerialBT.print("}");
        }
        SerialBT.print("]");

        SerialBT.println(); // End root object
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
        Serial.printf("BT: Free heap before parse: %d\n", ESP.getFreeHeap());
        yield(); // Feed watchdog before parsing

        // Use zero-copy in-place parsing - ArduinoJson modifies buffer directly
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, uploadBuffer);
        yield(); // Feed watchdog after parsing

        Serial.printf("BT: Free heap after parse: %d\n", ESP.getFreeHeap());
        Serial.printf("BT: JSON doc memory: %d bytes\n", doc.memoryUsage());

        if (error) {
          SerialBT.print("JSON_ERROR:");
          SerialBT.println(error.c_str());
          Serial.printf("BT: JSON error: %s\n", error.c_str());
        } else {
          applyConfigJson(doc.as<JsonObject>());

          savePresets();
          saveSystemSettings();
          saveAnalogInputs();

          // Reinitialize display
          initDisplayHardware();
          displayOLED();
          updateLeds();

          currentPreset = 0;
          SerialBT.println("SAVE_OK");
          Serial.println("BT: Config saved successfully!");
        }
        // doc goes out of scope, memory freed automatically
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
#endif
}
