#include "WebInterface.h"
#include "Storage.h"
#include "UI_Display.h"
#include "BleMidi.h"
#include <ArduinoJson.h>

const char* getCommandTypeName(MidiCommandType t){switch(t){case NOTE_MOMENTARY:return"Note (Momentary)";case NOTE_ON:return"Note On";case NOTE_OFF:return"Note Off";case CC:return"CC";case PC:return"PC";case TAP_TEMPO:return"Tap Tempo";default:return"Off";}}
const char* getCommandTypeString(MidiCommandType t){switch(t){case NOTE_MOMENTARY:return"NOTE_MOMENTARY";case NOTE_ON:return"NOTE_ON";case NOTE_OFF:return"NOTE_OFF";case CC:return"CC";case PC:return"PC";case TAP_TEMPO:return"TAP_TEMPO";default:return"OFF";}}
MidiCommandType parseCommandType(String s){if(s=="NOTE_MOMENTARY")return NOTE_MOMENTARY;if(s=="NOTE_ON")return NOTE_ON;if(s=="NOTE_OFF")return NOTE_OFF;if(s=="CC")return CC;if(s=="PC")return PC;if(s=="TAP_TEMPO")return TAP_TEMPO;return OFF;}

void rgbToHex(char* buffer, size_t size, const byte rgb[3]) { snprintf(buffer, size, "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]); }
void hexToRgb(const String& hex, byte rgb[3]) { long c = strtol(hex.substring(1).c_str(), NULL, 16); rgb[0]=(c>>16)&0xFF; rgb[1]=(c>>8)&0xFF; rgb[2]=c&0xFF; }

void sendOptions(String& out, MidiCommandType currentType) {
    const char* types[] = {"NOTE_MOMENTARY", "NOTE_ON", "NOTE_OFF", "CC", "PC", "TAP_TEMPO"};
    const char* names[] = {"Note (Mom)", "Note On", "Note Off", "CC", "PC", "Tap Tempo"};
    const MidiCommandType typeEnums[] = {NOTE_MOMENTARY, NOTE_ON, NOTE_OFF, CC, PC, TAP_TEMPO};
    
    for(int i=0; i<6; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "<option value='%s'%s>%s</option>", 
            types[i], 
            (typeEnums[i] == currentType ? " selected" : ""), 
            names[i]);
        out += buf;
    }
}

void sendMessageFields(String& out, const char* id, const MidiMessage& msg) {
    char buf[256];
    char hex[8];
    rgbToHex(hex, sizeof(hex), msg.rgb);
    
    // Type Select
    out += F("<div class='f'><label>Type:</label><select name='");
    out += id;
    out += F("_type'>");
    sendOptions(out, msg.type);
    out += F("</select></div>");
    
    // Channel
    snprintf(buf, sizeof(buf), "<div class='f'><label>Ch:</label><input type='number' name='%s_ch' min='1' max='16' value='%d'></div>", id, msg.channel);
    out += buf;
    
    // Data 1
    snprintf(buf, sizeof(buf), "<div class='f'><label>D1:</label><input type='number' name='%s_d1' min='0' max='127' value='%d'></div>", id, msg.data1);
    out += buf;
    
    // Data 2
    snprintf(buf, sizeof(buf), "<div class='f'><label>D2:</label><input type='number' name='%s_d2' min='0' max='127' value='%d'></div>", id, msg.data2);
    out += buf;
    
    // Color
    snprintf(buf, sizeof(buf), "<div class='f'><label>RGB:</label><input type='color' name='%s_rgb' value='%s'></div>", id, hex);
    out += buf;
}

void handleRoot() {
    int pagePreset = 0;
    if (server.hasArg("preset")) pagePreset = server.arg("preset").toInt();
    if (pagePreset < 0 || pagePreset > 3) pagePreset = 0;

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");

    String out = "";
    out.reserve(2048);

    out += F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Chocotone MIDI Editor</title>");
    // Minified CSS
    out += F("<style>:root{--bg:#121212;--card:#1e1e1e;--text:#e0e0e0;--acc:#bb86fc;--inp:#2c2c2c;--brd:#333;--sp:15px}body{font-family:'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);margin:0;padding:20px;line-height:1.5}*{box-sizing:border-box}header{text-align:center;margin-bottom:20px}h1{margin:0;font-weight:300;color:var(--acc);letter-spacing:1px;font-size:1.8rem}.sub{font-size:0.8rem;color:#888;margin-top:2px}.nav{background:var(--card);padding:15px;border-radius:12px;margin-bottom:var(--sp);display:flex;gap:15px;align-items:center;box-shadow:0 4px 6px #0000004d}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:var(--sp)}.card{background:var(--card);padding:15px;border-radius:12px;border:1px solid var(--brd);box-shadow:0 2px 4px #00000033}.head{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;border-bottom:1px solid #333;padding-bottom:8px}h4{margin:0;color:var(--acc);font-weight:500}.f{display:grid;grid-template-columns:60px 1fr;gap:10px;align-items:center;margin-bottom:8px}label{font-size:0.9em;color:#bbb}input,select{width:100%;background:var(--inp);border:1px solid var(--brd);color:#fff;padding:8px 10px;border-radius:6px;font-size:14px;outline:none}input:focus,select:focus{border-color:var(--acc)}input[type=color]{padding:0;height:36px;cursor:pointer}input[type=checkbox]{width:auto;accent-color:var(--acc)}.msg-h{background:#252525;padding:6px 10px;margin:12px -15px 12px -15px;font-size:11px;font-weight:bold;color:#888;text-transform:uppercase;letter-spacing:1px;border-left:3px solid var(--acc)}button{width:100%;padding:14px;background:#3700b3;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:25px;box-shadow:0 4px 6px #0000004d}button:hover{background:#4e00ff}.foot{text-align:center;margin:30px}.foot a{color:var(--acc);text-decoration:none;margin:0 10px;font-size:0.9em}.foot a:hover{text-decoration:underline}.disabled{opacity:0.3;pointer-events:none}.card>h4{margin-bottom:15px}@media(max-width:600px){body{padding:10px}.grid{grid-template-columns:1fr 1fr;gap:8px}.card{padding:10px;border-radius:8px}.f{grid-template-columns:1fr;gap:2px;margin-bottom:6px}.f label{font-size:11px}input,select{padding:4px 6px;font-size:12px;height:28px}input[type=color]{height:28px}h4{font-size:13px}.head{margin-bottom:8px;padding-bottom:4px}.msg-h{margin:8px -10px 8px -10px;padding:4px 8px;font-size:10px}.card>h4{margin-bottom:15px}}</style>");
    
    // Minified JS
    out += F("<script>function chgPreset(s){window.location='/?preset='+s.value}function toggleAlt(btn,id){var alt=document.getElementById('alt_'+id);alt.style.display=alt.style.display=='none'?'block':'none'}function toggleFields(s){var base=s.name.replace('_type','');var isTap=(s.value==='TAP_TEMPO');var f=['ch','d1','d2'];f.forEach(function(x){var i=document.getElementsByName(base+'_'+x)[0];if(i){var r=i.parentElement;if(isTap){r.classList.add('disabled');i.disabled=true}else{r.classList.remove('disabled');i.disabled=false}}})}window.onload=function(){var s=document.querySelectorAll('select[name$=\"_type\"]');s.forEach(function(x){toggleFields(x)})};</script>");
    out += F("</head><body>");

    // Header
    out += F("<header><h1>Chocotone MIDI Editor</h1><div class='sub'>made by André Solis - 2025 - v1.0</div></header>");
    
    // Navigation
    out += F("<div class='nav'><label>Preset:</label><select onchange='chgPreset(this)'>");
    for (int p=0; p<4; p++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "<option value='%d'%s>%s</option>", p, (p==pagePreset?" selected":""), presetNames[p]);
        out += buf;
    }
    out += F("</select></div>");
    
    // Form Start
    out += F("<form method='POST' action='/save'>");
    out += "<input type='hidden' name='preset' value='" + String(pagePreset) + "'>";
    
    char buf[256];
    snprintf(buf, sizeof(buf), "<div class='card' style='margin-bottom:15px'><div class='f'><label>Name:</label><input name='name' value='%s' maxlength='20'></div></div>", presetNames[pagePreset]);
    out += buf;
    
    out += F("<div class='grid'>");
    server.sendContent(out); 
    out = "";

    // Buttons
    for (int b = 0; b < NUM_BUTTONS; b++) {
        yield(); 
        
        const ButtonConfig& cfg = buttonConfigs[pagePreset][b];
        char id[8]; snprintf(id, sizeof(id), "b%d", b);
        
        out = "";
        out.reserve(2048);
        
        out += "<div class='card'><div class='head'>";
        snprintf(buf, sizeof(buf), "<h4>Button %d</h4>", b+1);
        out += buf;
        snprintf(buf, sizeof(buf), "<label><input type='checkbox' name='%s_alt' style='width:auto' onchange='toggleAlt(this,\"%s\")'%s> Alt</label>", id, id, cfg.isAlternate?" checked":"");
        out += buf;
        out += F("</div>");
        
        // Name
        snprintf(buf, sizeof(buf), "<div class='f'><label>Label:</label><input name='%s_n' value='%s' maxlength='20'></div>", id, cfg.name);
        out += buf;
        
        // Message A
        out += F("<div class='msg-h'>PRIMARY MSG</div>");
        char ida[12]; snprintf(ida, sizeof(ida), "%s_a", id);
        sendMessageFields(out, ida, cfg.messageA); 
        
        // Message B
        char styleDisplay[16];
        snprintf(styleDisplay, sizeof(styleDisplay), cfg.isAlternate ? "block" : "none");
        snprintf(buf, sizeof(buf), "<div id='alt_%s' style='display:%s;margin-top:15px'>", id, styleDisplay);
        out += buf;
        out += F("<div class='msg-h'>ALTERNATE MSG</div>");
        char idb[12]; snprintf(idb, sizeof(idb), "%s_b", id);
        sendMessageFields(out, idb, cfg.messageB); 
        out += F("</div>"); 
        
        out += F("</div>"); // End card
        
        server.sendContent(out); 
    }
    
    out = "";
    out.reserve(1024);
    out += F("</div>"); // End grid
    out += F("<button type='submit'>Save Configuration</button>");
    out += F("</form>");
    
    // System Settings
    out += F("<div class='card' style='margin-top:20px'><h4>System Settings</h4><form method='POST' action='/saveSystem'>");
    snprintf(buf, sizeof(buf), "<div class='f'><label style='min-width:60px'>BLE:</label><input name='ble' value='%s'></div>", bleDeviceName);
    out += buf;
    snprintf(buf, sizeof(buf), "<div class='f'><label style='min-width:60px'>SSID:</label><input name='ssid' value='%s'></div>", apSSID);
    out += buf;
    snprintf(buf, sizeof(buf), "<div class='f'><label style='min-width:60px'>Pass:</label><input name='pass' value='%s'></div>", apPassword);
    out += buf;
    out += F("<button style='background:#17a2b8;margin-top:10px'>Save System & Reboot</button></form></div>");
    
    out += F("<div class='foot'><a href='/export'>Export JSON</a> <a href='/import'>Import JSON</a></div>");
    out += F("</body></html>");
    
    server.sendContent(out); 
}

void handleSave() {
    bool changed = false;
    int preset = server.hasArg("preset") ? server.arg("preset").toInt() : 0;

    if(server.hasArg("name")) {
        String newName = server.arg("name");
        if(strncmp(presetNames[preset], newName.c_str(), 20) != 0) {
            strncpy(presetNames[preset], newName.c_str(), 20);
            presetNames[preset][20] = '\0';
            changed = true;
        }
    }
    
    yield();  // Allow WDT to reset

    for (int t = 0; t < NUM_BUTTONS; t++) {
        char id[10]; snprintf(id, sizeof(id), "b%d", t);
        ButtonConfig &config = buttonConfigs[preset][t];

        if(server.hasArg(String(id)+"_n")) {
            String newName = server.arg(String(id)+"_n");
            if(strncmp(config.name, newName.c_str(), 20) != 0) {
                strncpy(config.name, newName.c_str(), 20);
                config.name[20] = '\0';
                changed = true;
            }
        }

        bool newIsAlternate = server.hasArg(String(id) + "_alt");
        if(config.isAlternate != newIsAlternate) { config.isAlternate = newIsAlternate; changed = true; }

        char ida[15]; snprintf(ida, sizeof(ida), "%s_a", id);
        MidiMessage &msgA = config.messageA;
        MidiCommandType newTypeA=parseCommandType(server.arg(String(ida)+"_type"));
        byte newChA=server.arg(String(ida)+"_ch").toInt();
        byte newD1A=server.arg(String(ida)+"_d1").toInt();
        byte newD2A=server.arg(String(ida)+"_d2").toInt();
        byte newRgbA[3]; hexToRgb(server.arg(String(ida)+"_rgb"), newRgbA);
        if(msgA.type!=newTypeA||msgA.channel!=newChA||msgA.data1!=newD1A||msgA.data2!=newD2A||memcmp(msgA.rgb,newRgbA,3)!=0){
            msgA={newTypeA,newChA,newD1A,newD2A,{newRgbA[0],newRgbA[1],newRgbA[2]}}; changed=true;
        }

        if(newIsAlternate) {
            char idb[15]; snprintf(idb, sizeof(idb), "%s_b", id);
            MidiMessage &msgB = config.messageB;
            MidiCommandType newTypeB=parseCommandType(server.arg(String(idb)+"_type"));
            byte newChB=server.arg(String(idb)+"_ch").toInt();
            byte newD1B=server.arg(String(idb)+"_d1").toInt();
            byte newD2B=server.arg(String(idb)+"_d2").toInt();
            byte newRgbB[3]; hexToRgb(server.arg(String(idb)+"_rgb"), newRgbB);
            if(msgB.type!=newTypeB||msgB.channel!=newChB||msgB.data1!=newD1B||msgB.data2!=newD2B||memcmp(msgB.rgb,newRgbB,3)!=0){
                msgB={newTypeB,newChB,newD1B,newD2B,{newRgbB[0],newRgbB[1],newRgbB[2]}}; changed=true;
            }
        }
        
        yield();  // Allow WDT to reset after each button
    }

    if (changed) {
        yield();  // Before blocking NVS write
        savePresets();
        
        // Defer display/LED updates to main loop for stability
        pendingDisplayUpdate = true;
        
        server.send(200, "text/html", "<html><body style='background:#222;color:#fff;text-align:center;padding:50px'>Saved! <a href='/?preset="+String(preset)+"' style='color:#8af'>Back</a></body></html>");
    } else {
        server.send(200, "text/html", "<html><body style='background:#222;color:#fff;text-align:center;padding:50px'>No changes. <a href='/?preset="+String(preset)+"' style='color:#8af'>Back</a></body></html>");
    }
}

void handleSaveSystem() {
    bool changed = false;
    if(server.hasArg("ble")) {
        String newName = server.arg("ble");
        if(strncmp(bleDeviceName, newName.c_str(), 31) != 0) {
            strncpy(bleDeviceName, newName.c_str(), 31); bleDeviceName[31] = '\0'; changed = true;
        }
    }
    if(server.hasArg("ssid")) {
        String newSSID = server.arg("ssid");
        if(strncmp(apSSID, newSSID.c_str(), 31) != 0) {
            strncpy(apSSID, newSSID.c_str(), 31); apSSID[31] = '\0'; changed = true;
        }
    }
    if(server.hasArg("pass")) {
        String newPass = server.arg("pass");
        if(newPass.length() >= 8 && newPass.length() < 64 && strncmp(apPassword, newPass.c_str(), 63) != 0) {
            strncpy(apPassword, newPass.c_str(), 63); apPassword[63] = '\0'; changed = true;
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
    String html = F("<!DOCTYPE html><html><head><title>Rebooting...</title><style>body{font-family:Inter,sans-serif;background-color:#1a1a2e;color:#e0e0e0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;text-align:center;}.message-box{background-color:#2a2a4a;padding:30px;border-radius:12px;}h1{color:#8d8dff;}</style></head><body><div class='message-box'><h1>");
    html += message;
    html += F("</h1><p>Device is rebooting. Please reconnect to the new Wi-Fi AP if you changed it.</p></div></body></html>");
    server.send(200, "text/html", html);
    delay(1000);
    ESP.restart();
}

void handleExport() {
    server.sendHeader("Content-Disposition", "attachment; filename=\"midi_presets.json\"");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");

    // Stream the JSON structure manually to avoid massive memory allocation
    server.sendContent("{\"version\":1,\"presets\":[");

    for (int p = 0; p < 4; p++) {
        if (p > 0) server.sendContent(",");

        // Start Preset Object
        String pHeader = "{\"name\":\"" + String(presetNames[p]) + "\",\"buttons\":[";
        server.sendContent(pHeader);

        for (int t = 0; t < NUM_BUTTONS; t++) {
            if (t > 0) server.sendContent(",");

            // Use a small StaticJsonDocument for just ONE button to ensure valid JSON
            // and minimal memory usage
            StaticJsonDocument<768> doc;
            JsonObject bObj = doc.to<JsonObject>();
            
            const ButtonConfig& config = buttonConfigs[p][t];
            bObj["name"] = config.name;
            bObj["alt"] = config.isAlternate; // Changed to match offline editor (was isAlternate)

            JsonObject msgA = bObj.createNestedObject("msgA"); // Changed to match offline editor (was messageA)
            msgA["type"] = getCommandTypeString(config.messageA.type);
            msgA["ch"] = config.messageA.channel; // Changed to match offline editor
            msgA["d1"] = config.messageA.data1;
            msgA["d2"] = config.messageA.data2;
            char hexColorA[8];
            rgbToHex(hexColorA, sizeof(hexColorA), config.messageA.rgb);
            msgA["rgb"] = hexColorA; // Changed to match offline editor

            JsonObject msgB = bObj.createNestedObject("msgB"); // Changed to match offline editor (was messageB)
            msgB["type"] = getCommandTypeString(config.messageB.type);
            msgB["ch"] = config.messageB.channel;
            msgB["d1"] = config.messageB.data1;
            msgB["d2"] = config.messageB.data2;
            char hexColorB[8];
            rgbToHex(hexColorB, sizeof(hexColorB), config.messageB.rgb);
            msgB["rgb"] = hexColorB;

            String buttonJson;
            serializeJson(bObj, buttonJson);
            server.sendContent(buttonJson);
            
            // Yield to prevent WDT reset during long exports
            yield();
        }

        // End Buttons Array and Preset Object
        server.sendContent("]}");
    }

    // End Presets Array 
    server.sendContent("],");
    
    // Add System Settings
    String sys = "\"system\":{\"ble\":\"" + String(bleDeviceName) + "\",\"ssid\":\"" + String(apSSID) + "\",\"pass\":\"" + String(apPassword) + "\"}";
    server.sendContent(sys);

    // End Root Object
    server.sendContent("}");
    
    // Terminate chunked transfer
    server.sendContent("");
}

void handleImport() {
    String html = F("<!DOCTYPE html><html><head><title>Import Presets</title>");
    html += F("<style>body{font-family:Inter,sans-serif;background-color:#1a1a2e;color:#e0e0e0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;text-align:center;}.container{background-color:#2a2a4a;padding:30px;border-radius:12px;width:80%;max-width:600px;}h1{color:#8d8dff;}textarea{width:100%;box-sizing:border-box;background-color:#3a3a5a;color:#e0e0e0;border:1px solid #5a5a7a;border-radius:8px;padding:10px;height:200px;margin-top:15px;font-family:monospace;}button{width:100%;padding:10px 18px;background-color:#8d8dff;color:white;border:none;border-radius:8px;cursor:pointer;font-size:1em;margin-top: 15px;font-weight:bold;} button:hover{background-color:#7a7aff;}</style>");
    html += F("</head><body><div class='container'><h1>Import Presets</h1><p>Paste your JSON content below. This will <strong style='color:#ffccaa;'>OVERWRITE</strong> all presets.</p>");
    html += F("<form action='/import' method='POST'>");
    html += F("<textarea name='json_data' placeholder='Paste midi_presets.json content here...'></textarea>");
    html += F("<button type='submit'>Import & Save All Presets</button></form>");
    html += F("<p style='margin-top:15px;font-size:0.9em;'><a href='/' style='color:#aaaaff;'>Cancel and go back</a></p>");
    html += F("</div></body></html>");
    server.send(200, "text/html", html);
}

void handleImportUpload() {
    if (!server.hasArg("json_data")) {
        server.send(400, "text/plain", "Bad Request: No json_data");
        return;
    }

    String jsonData = server.arg("json_data");
    
    // Limit JSON size to prevent memory issues
    if (jsonData.length() > 16384) {  // 16KB max
        server.send(413, "text/plain", "JSON too large (max 16KB)");
        return;
    }
    
    yield();  // Allow WDT to reset before heavy parsing
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error) {
        server.send(400, "text/plain", "JSON Deserialization Failed: " + String(error.c_str()));
        return;
    }

    JsonArray presets = doc["presets"];
    if (presets.isNull() || presets.size() != 4) {
        server.send(400, "text/plain", "Invalid JSON structure or preset count != 4");
        return;
    }

    for (int p = 0; p < 4; p++) {
        yield();  // Allow WDT to reset for each preset
        
        JsonObject pObj = presets[p];
        strncpy(presetNames[p], pObj["name"] | "Imported Preset", 20);
        presetNames[p][20] = '\0';

        JsonArray bArray = pObj["buttons"];
        if (bArray.isNull() || bArray.size() != NUM_BUTTONS) continue;

        for (int t = 0; t < NUM_BUTTONS; t++) {
            JsonObject bObj = bArray[t];
            ButtonConfig& config = buttonConfigs[p][t];

            strncpy(config.name, bObj["name"] | "Imported Btn", 20);
            config.name[20] = '\0';
            config.isAlternate = bObj["alt"] | false; // Updated key
            config.nextIsB = false;

            JsonObject msgA = bObj["msgA"]; // Updated key
            config.messageA.type = parseCommandType(msgA["type"] | "CC");
            config.messageA.channel = msgA["ch"] | 1; // Updated key
            config.messageA.data1 = msgA["d1"] | 0; // Updated key
            config.messageA.data2 = msgA["d2"] | 0; // Updated key
            hexToRgb(msgA["rgb"] | "#0000FF", config.messageA.rgb); // Updated key

            JsonObject msgB = bObj["msgB"]; // Updated key
            config.messageB.type = parseCommandType(msgB["type"] | "CC");
            config.messageB.channel = msgB["ch"] | 1; // Updated key
            config.messageB.data1 = msgB["d1"] | 0; // Updated key
            config.messageB.data2 = msgB["d2"] | 0; // Updated key
            hexToRgb(msgB["rgb"] | "#FF0000", config.messageB.rgb); // Updated key
            
            yield();  // Allow WDT to reset after each button
        }
    }
    
    yield();  // Before system settings
    
    // Import System Settings if present
    JsonObject sys = doc["system"];
    if (!sys.isNull()) {
        const char* ble = sys["ble"];
        const char* ssid = sys["ssid"];
        const char* pass = sys["pass"];
        
        if (ble) { strncpy(bleDeviceName, ble, 31); bleDeviceName[31] = '\0'; }
        if (ssid) { strncpy(apSSID, ssid, 31); apSSID[31] = '\0'; }
        if (pass) { strncpy(apPassword, pass, 63); apPassword[63] = '\0'; }
        
        yield();
        saveSystemSettings(); // Save to NVS
    }

    yield();  // Before final save
    savePresets();
    
    // Defer display/LED updates to main loop for stability
    pendingDisplayUpdate = true;
    
    server.send(200, "text/plain", "Import successful! System settings updated. Redirecting...");
    delay(2000);
    server.sendHeader("Location", "/");
    server.send(302);
}


void turnWifiOn() {
    if (isWifiOn) return;
    
    // Pause BLE scanning/connection to avoid WiFi conflicts
    Serial.println("Pausing BLE for WiFi stability...");
    if (clientConnected && pClient) {
        pClient->disconnect();
        Serial.println("BLE client disconnected");
    }
    BLEDevice::getScan()->stop();
    doScan = false;  // Prevent auto-scanning
    Serial.println("BLE scan stopped");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    server.begin();
    isWifiOn = true;
    Serial.println("WiFi AP Started (BLE paused)");
}

void turnWifiOff() {
    if (!isWifiOn) return;
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    isWifiOn = false;
    Serial.println("WiFi AP Stopped");
    
    // Resume BLE scanning
    Serial.println("Resuming BLE...");
    doScan = true;
    startBleScan();
    Serial.println("BLE scan resumed");
}

void setup_web_server() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/saveSystem", HTTP_POST, handleSaveSystem);
    server.on("/export", HTTP_GET, handleExport);
    server.on("/import", HTTP_GET, handleImport);
    server.on("/import", HTTP_POST, handleImportUpload);
    Serial.println("Web server routes configured");
}
