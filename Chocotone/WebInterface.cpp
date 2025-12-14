#include "WebInterface.h"
#include "Storage.h"
#include "UI_Display.h"
#include "BleMidi.h"
#include <ArduinoJson.h>

// Request throttling to prevent crashes from concurrent or rapid requests
static volatile bool requestInProgress = false;
static unsigned long lastRequestTime = 0;
const unsigned long MIN_REQUEST_INTERVAL = 500;  // Minimum 500ms between requests

const char* getCommandTypeName(MidiCommandType t){switch(t){case NOTE_MOMENTARY:return"Note (Momentary)";case NOTE_ON:return"Note On";case NOTE_OFF:return"Note Off";case CC:return"CC";case PC:return"PC";case TAP_TEMPO:return"Tap Tempo";case PRESET_UP:return"Preset Up";case PRESET_DOWN:return"Preset Down";case PRESET_1:return"Preset 1";case PRESET_2:return"Preset 2";case PRESET_3:return"Preset 3";case PRESET_4:return"Preset 4";case CLEAR_BLE_BONDS:return"Clear BLE Bonds";case WIFI_TOGGLE:return"WiFi Toggle";default:return"Off";}}
const char* getCommandTypeString(MidiCommandType t){switch(t){case NOTE_MOMENTARY:return"NOTE_MOMENTARY";case NOTE_ON:return"NOTE_ON";case NOTE_OFF:return"NOTE_OFF";case CC:return"CC";case PC:return"PC";case TAP_TEMPO:return"TAP_TEMPO";case PRESET_UP:return"PRESET_UP";case PRESET_DOWN:return"PRESET_DOWN";case PRESET_1:return"PRESET_1";case PRESET_2:return"PRESET_2";case PRESET_3:return"PRESET_3";case PRESET_4:return"PRESET_4";case CLEAR_BLE_BONDS:return"CLEAR_BLE_BONDS";case WIFI_TOGGLE:return"WIFI_TOGGLE";default:return"OFF";}}
MidiCommandType parseCommandType(String s){if(s=="NOTE_MOMENTARY")return NOTE_MOMENTARY;if(s=="NOTE_ON")return NOTE_ON;if(s=="NOTE_OFF")return NOTE_OFF;if(s=="CC")return CC;if(s=="PC")return PC;if(s=="TAP_TEMPO")return TAP_TEMPO;if(s=="PRESET_UP")return PRESET_UP;if(s=="PRESET_DOWN")return PRESET_DOWN;if(s=="PRESET_1")return PRESET_1;if(s=="PRESET_2")return PRESET_2;if(s=="PRESET_3")return PRESET_3;if(s=="PRESET_4")return PRESET_4;if(s=="CLEAR_BLE_BONDS")return CLEAR_BLE_BONDS;if(s=="WIFI_TOGGLE")return WIFI_TOGGLE;return OFF;}

void rgbToHex(char* buffer, size_t size, const byte rgb[3]) { snprintf(buffer, size, "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]); }
void hexToRgb(const String& hex, byte rgb[3]) { long c = strtol(hex.substring(1).c_str(), NULL, 16); rgb[0]=(c>>16)&0xFF; rgb[1]=(c>>8)&0xFF; rgb[2]=c&0xFF; }

void sendOptions(String& out, MidiCommandType currentType) {
    const char* types[] = {"OFF", "NOTE_MOMENTARY", "NOTE_ON", "NOTE_OFF", "CC", "PC", "TAP_TEMPO", "PRESET_UP", "PRESET_DOWN", "PRESET_1", "PRESET_2", "PRESET_3", "PRESET_4", "CLEAR_BLE_BONDS", "WIFI_TOGGLE"};
    const char* names[] = {"Off", "Note (Mom)", "Note On", "Note Off", "CC", "PC", "Tap Tempo", "Preset +", "Preset -", "Preset 1", "Preset 2", "Preset 3", "Preset 4", "Clear BLE Bonds", "WiFi Toggle"};
    const MidiCommandType typeEnums[] = {OFF, NOTE_MOMENTARY, NOTE_ON, NOTE_OFF, CC, PC, TAP_TEMPO, PRESET_UP, PRESET_DOWN, PRESET_1, PRESET_2, PRESET_3, PRESET_4, CLEAR_BLE_BONDS, WIFI_TOGGLE};
    
    for(int i=0; i<15; i++) {
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
    out += F("_type' onchange='toggleFields(this)'>");
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
    
    // Tap Tempo Mode Controls (for TAP_TEMPO type only)
    // Rhythm PREV Button
    int8_t safePrev = msg.rhythmPrevButton;
    if (safePrev < -1 || safePrev > 7) safePrev = 0;  // Default: button 1
    int displayPrev = safePrev == -1 ? 0 : safePrev + 1;
    snprintf(buf, sizeof(buf), "<div class='f'><label>R.Prev:</label><input type='number' name='%s_rprev' min='0' max='8' value='%d' placeholder='0=None'></div>", id, displayPrev);
    out += buf;
    
    // Rhythm NEXT Button
    int8_t safeNext = msg.rhythmNextButton;
    if (safeNext < -1 || safeNext > 7) safeNext = 4;  // Default: button 5
    int displayNext = safeNext == -1 ? 0 : safeNext + 1;
    snprintf(buf, sizeof(buf), "<div class='f'><label>R.Next:</label><input type='number' name='%s_rnext' min='0' max='8' value='%d' placeholder='0=None'></div>", id, displayNext);
    out += buf;
    
    // Tap Mode LOCK Button
    int8_t safeLock = msg.tapModeLockButton;
    if (safeLock < -1 || safeLock > 7) safeLock = 7;  // Default: button 8
    int displayLock = safeLock == -1 ? 0 : safeLock + 1;
    snprintf(buf, sizeof(buf), "<div class='f'><label>Lock:</label><input type='number' name='%s_lock' min='0' max='8' value='%d' placeholder='0=None'></div>", id, displayLock);
    out += buf;
    
    // Color
    snprintf(buf, sizeof(buf), "<div class='f'><label>RGB:</label><input type='color' name='%s_rgb' value='%s'></div>", id, hex);
    out += buf;
}

void handleRoot() {
    // Request throttling: prevent concurrent or rapid requests from crashing
    unsigned long now = millis();
    if (requestInProgress) {
        server.send(200, "text/html", 
            "<html><head><meta http-equiv='refresh' content='1'></head>"
            "<body style='background:#121212;color:#e0e0e0;text-align:center;padding:50px'>"
            "<p>Loading... please wait</p></body></html>");
        return;
    }
    if (now - lastRequestTime < MIN_REQUEST_INTERVAL) {
        server.send(200, "text/html", 
            "<html><head><meta http-equiv='refresh' content='1'></head>"
            "<body style='background:#121212;color:#e0e0e0;text-align:center;padding:50px'>"
            "<p>Please wait...</p></body></html>");
        return;
    }
    
    requestInProgress = true;
    lastRequestTime = now;
    
    // Memory safety: yield and check heap before generating large page
    yield();
    int freeHeap = ESP.getFreeHeap();
    Serial.printf("handleRoot: Free heap = %d bytes\n", freeHeap);
    
    // If heap is critically low (<20KB), show a simple error page
    // Note: Editor uses chunked sending, so 25KB+ is usually enough
    if (freeHeap < 20000) {
        char errorPage[256];
        snprintf(errorPage, sizeof(errorPage),
            "<html><body style='background:#121212;color:#e0e0e0;text-align:center;padding:50px'>"
            "<h2 style='color:#ff6b6b'>Low Memory</h2>"
            "<p>Free heap: %d bytes</p>"
            "<p>Please wait a moment and refresh.</p>"
            "<a href='/' style='color:#bb86fc'>Retry</a>"
            "</body></html>", freeHeap);
        server.send(200, "text/html", errorPage);
        requestInProgress = false;
        return;
    }
    
    int pagePreset = 0;
    if (server.hasArg("preset")) pagePreset = server.arg("preset").toInt();
    if (pagePreset < 0 || pagePreset > 3) pagePreset = 0;
    
    // Switch controller to selected preset if different (defer display update for memory safety)
    if (currentPreset != pagePreset) {
        currentPreset = pagePreset;
        saveCurrentPresetIndex();
        pendingDisplayUpdate = true;  // Defer to main loop - safer with WiFi on
        Serial.printf("Editor: Switched to preset %d (%s)\n", pagePreset, presetNames[pagePreset]);
    }
    
    // View parameter: "main", "special", or "system"
    String currentView = "main";
    if (server.hasArg("view")) currentView = server.arg("view");
    if (currentView != "main" && currentView != "special" && currentView != "system") currentView = "main";

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");

    String out = "";
    out.reserve(2048);

    out += F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Chocotone MIDI Editor</title>");
    // Minified CSS with gradient header like presentation.html
    out += F("<style>:root{--bg:#121212;--card:#1e1e1e;--text:#e0e0e0;--acc:#bb86fc;--inp:#2c2c2c;--brd:#333;--sp:15px}body{font-family:'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);margin:0;padding:20px;line-height:1.5}*{box-sizing:border-box}header{text-align:center;margin-bottom:20px;padding:20px 0}.hero-title{font-size:2.5rem;font-weight:700;margin:0;background:linear-gradient(to right,#fff,var(--acc));-webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}.sub{font-size:0.8rem;color:#888;margin-top:6px}.nav{background:var(--card);padding:15px;border-radius:12px;margin-bottom:var(--sp);display:flex;gap:15px;align-items:center;box-shadow:0 4px 6px #0000004d}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:var(--sp)}.card{background:var(--card);padding:15px;border-radius:12px;border:1px solid var(--brd);box-shadow:0 2px 4px #00000033}.o1{order:5}.o2{order:6}.o3{order:7}.o4{order:8}.o5{order:1}.o6{order:2}.o7{order:3}.o8{order:4}.head{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;border-bottom:1px solid #333;padding-bottom:8px}h4{margin:0;color:var(--acc);font-weight:500}.f{display:grid;grid-template-columns:60px 1fr;gap:10px;align-items:center;margin-bottom:8px}label{font-size:0.9em;color:#bbb}input,select{width:100%;background:var(--inp);border:1px solid var(--brd);color:#fff;padding:8px 10px;border-radius:6px;font-size:14px;outline:none}input:focus,select:focus{border-color:var(--acc)}input[type=color]{padding:0;height:36px;cursor:pointer}input[type=checkbox]{width:auto;accent-color:var(--acc)}.msg-h{background:#252525;padding:6px 10px;margin:12px -15px 12px -15px;font-size:11px;font-weight:bold;color:#888;text-transform:uppercase;letter-spacing:1px;border-left:3px solid var(--acc)}button{width:100%;padding:14px;background:#3700b3;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:25px;box-shadow:0 4px 6px #0000004d}button:hover{background:#4e00ff}.foot{text-align:center;margin:30px}.foot a{color:var(--acc);text-decoration:none;margin:0 10px;font-size:0.9em}.foot a:hover{text-decoration:underline}.disabled{opacity:0.3;pointer-events:none}.card>h4{margin-bottom:15px}.vtog{display:flex;gap:8px;margin:15px 0}.vtog a{flex:1;padding:10px;text-align:center;background:#444;border-radius:8px;color:#fff;text-decoration:none;font-weight:600}.vtog a.on{background:#3700b3}.main-v,.spec-v,.sys-v{display:none}.main-v.show,.spec-v.show,.sys-v.show{display:block}@media(max-width:600px){body{padding:10px}.hero-title{font-size:1.8rem}.grid{grid-template-columns:1fr 1fr;gap:8px}.card{padding:10px;border-radius:8px}.f{grid-template-columns:1fr;gap:2px;margin-bottom:6px}.f label{font-size:11px}input,select{padding:4px 6px;font-size:12px;height:28px}input[type=color]{height:28px}h4{font-size:13px}.head{margin-bottom:8px;padding-bottom:4px}.msg-h{margin:8px -10px 8px -10px;padding:4px 8px;font-size:10px}.card>h4{margin-bottom:15px}}</style>");
    
    
    // Simplified JS - views are now separate pages, not show/hide
    out += F("<script>function chgPreset(s){var u=new URLSearchParams(window.location.search);u.set('preset',s.value);window.location='/?'+u.toString()}function toggleAlt(btn,id){var alt=document.getElementById('alt_'+id);alt.style.display=alt.style.display=='none'?'block':'none'}function toggleFields(s){var base=s.name.replace('_type','');var internal=['OFF','TAP_TEMPO','PRESET_UP','PRESET_DOWN','PRESET_1','PRESET_2','PRESET_3','PRESET_4','CLEAR_BLE_BONDS','WIFI_TOGGLE'].indexOf(s.value)>=0;var f=['ch','d1','d2'];f.forEach(function(x){var i=document.getElementsByName(base+'_'+x)[0];if(i){var r=i.parentElement;if(internal){r.classList.add('disabled');i.disabled=true}else{r.classList.remove('disabled');i.disabled=false}}});var tapFields=['rprev','rnext','lock'];tapFields.forEach(function(x){var i=document.getElementsByName(base+'_'+x)[0];if(i){var r=i.parentElement;if(s.value=='TAP_TEMPO'){r.style.display='grid'}else{r.style.display='none'}}})}function toggleHoldCombo(cb,type){var base=cb.name.replace('_'+type+'_en','');var fields=type=='hold'?['hold_ms','hold_type','hold_ch','hold_d1','hold_d2']:['combo_label','combo_partner','combo_type','combo_ch','combo_d1','combo_d2'];fields.forEach(function(f){var inp=document.getElementsByName(base+'_'+f)[0];if(inp){inp.disabled=!cb.checked;inp.parentElement.style.opacity=cb.checked?'1':'0.5'}})}window.onload=function(){var s=document.querySelectorAll('select[name$=\"_type\"]');s.forEach(function(x){toggleFields(x)});document.querySelectorAll('input[name$=\"_hold_en\"]').forEach(function(c){toggleHoldCombo(c,'hold')});document.querySelectorAll('input[name$=\"_combo_en\"]').forEach(function(c){toggleHoldCombo(c,'combo')});};</script>");
    out += F("</head><body>");

    // Header - Gradient Chocotone title with subtitle
    out += F("<header><h1 class='hero-title'>Chocotone</h1><div class='sub'>MIDI Editor - by André Solis - v1.1</div></header>");
    
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
    out += "<input type='hidden' name='view' value='" + currentView + "'>";
    
    char buf[256];
    snprintf(buf, sizeof(buf), "<div class='card' style='margin-bottom:15px'><div class='f'><label>Name:</label><input name='name' value='%s' maxlength='20'></div>", presetNames[pagePreset]);
    out += buf;
    
    // View Toggle Buttons (real links, not JS show/hide)
    out += "<div class='vtog'>";
    out += String("<a href='/?preset=") + String(pagePreset) + "&view=main'" + (currentView=="main"?" class='on'":"") + ">Main Messages</a>";
    out += String("<a href='/?preset=") + String(pagePreset) + "&view=special'" + (currentView=="special"?" class='on'":"") + ">Special Actions</a>";
    out += String("<a href='/?preset=") + String(pagePreset) + "&view=system'" + (currentView=="system"?" class='on'":"") + ">System Config</a>";
    out += "</div></div>";
    
    out += F("<div class='grid'>");
    server.sendContent(out); 
    out = "";

    // Buttons (use dynamic button count)
    for (int b = 0; b < systemConfig.buttonCount; b++) {
        yield(); 
        
        const ButtonConfig& cfg = buttonConfigs[pagePreset][b];
        char id[8]; snprintf(id, sizeof(id), "b%d", b);
        
        out = "";
        out.reserve(3000);
        
        // Add order class for grid layout (5678 top row, 1234 bottom row)
        snprintf(buf, sizeof(buf), "<div class='card o%d'><div class='head'>", b+1);
        out += buf;
        snprintf(buf, sizeof(buf), "<h4>Button %d</h4>", b+1);
        out += buf;
        out += F("</div>");
        
        // Name (always visible)
        snprintf(buf, sizeof(buf), "<div class='f'><label>Label:</label><input name='%s_n' value='%s' maxlength='20'></div>", id, cfg.name);
        out += buf;
        
        // ===== MAIN VIEW: Primary/Alternate Messages with RGB =====
        if (currentView == "main") {
            // Alt toggle
            snprintf(buf, sizeof(buf), "<div class='f'><label>Alt:</label><input type='checkbox' name='%s_alt' onchange='toggleAlt(this,\"%s\")'%s></div>", id, id, cfg.isAlternate?" checked":"");
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
        }
        
        // ===== SPECIAL VIEW: Hold/Combo Actions =====
        if (currentView == "special") {
            // Hold Action (read from globalSpecialActions, not preset-specific cfg)
            out += F("<div class='msg-h'>HOLD ACTION</div>");
            snprintf(buf, sizeof(buf), "<div class='f'><label>Enabled:</label><input type='checkbox' name='%s_hold_en' onchange='toggleHoldCombo(this,\"hold\")'%s></div>", id, globalSpecialActions[b].hold.enabled?" checked":"");
            out += buf;
            snprintf(buf, sizeof(buf), "<div class='f'><label>Hold ms:</label><input type='number' name='%s_hold_ms' min='200' max='3000' value='%d'></div>", id, globalSpecialActions[b].hold.thresholdMs);
            out += buf;
            // Hold message type
            out += F("<div class='f'><label>Type:</label><select name='");
            out += id;
            out += F("_hold_type'>");
            sendOptions(out, globalSpecialActions[b].hold.type);
            out += F("</select></div>");
            snprintf(buf, sizeof(buf), "<div class='f'><label>Ch:</label><input type='number' name='%s_hold_ch' value='%d'></div>", id, globalSpecialActions[b].hold.channel);
            out += buf;
            snprintf(buf, sizeof(buf), "<div class='f'><label>D1:</label><input type='number' name='%s_hold_d1' value='%d'></div>", id, globalSpecialActions[b].hold.data1);
            out += buf;
            snprintf(buf, sizeof(buf), "<div class='f'><label>D2:</label><input type='number' name='%s_hold_d2' value='%d'></div>", id, globalSpecialActions[b].hold.data2);
            out += buf;
            
            // Combo Action (read from globalSpecialActions, not preset-specific cfg)
            out += F("<div class='msg-h'>COMBO ACTION</div>");
            snprintf(buf, sizeof(buf), "<div class='f'><label>Enabled:</label><input type='checkbox' name='%s_combo_en' onchange='toggleHoldCombo(this,\"combo\")'%s></div>", id, globalSpecialActions[b].combo.enabled?" checked":"");
            out += buf;
            // Safety: ensure label is a valid string (prevent garbage data from breaking page)
            char safeLabel[7] = {0};
            if (globalSpecialActions[b].combo.label[0] >= 32 && globalSpecialActions[b].combo.label[0] <= 126) {  // Printable ASCII
                strncpy(safeLabel, globalSpecialActions[b].combo.label, 6);
                safeLabel[6] = '\0';
            }
            snprintf(buf, sizeof(buf), "<div class='f'><label>Label:</label><input type='text' name='%s_combo_label' maxlength='6' value='%s' placeholder='Auto'></div>", id, safeLabel);
            out += buf;
            snprintf(buf, sizeof(buf), "<div class='f'><label>Partner:</label><input type='number' name='%s_combo_partner' min='1' max='8' value='%d'></div>", id, globalSpecialActions[b].combo.partner >= 0 ? globalSpecialActions[b].combo.partner + 1 : 1);
            out += buf;
            out += F("<div class='f'><label>Type:</label><select name='");
            out += id;
            out += F("_combo_type'>");
            sendOptions(out, globalSpecialActions[b].combo.type);
            out += F("</select></div>");
            snprintf(buf, sizeof(buf), "<div class='f'><label>Ch:</label><input type='number' name='%s_combo_ch' value='%d'></div>", id, globalSpecialActions[b].combo.channel);
            out += buf;
            snprintf(buf, sizeof(buf), "<div class='f'><label>D1:</label><input type='number' name='%s_combo_d1' value='%d'></div>", id, globalSpecialActions[b].combo.data1);
            out += buf;
            snprintf(buf, sizeof(buf), "<div class='f'><label>D2:</label><input type='number' name='%s_combo_d2' value='%d'></div>", id, globalSpecialActions[b].combo.data2);
            out += buf;
        }
        
        out += F("</div>"); // End card
        
        server.sendContent(out); 
    }
    
    out = "";
    out.reserve(1024);
    out += F("</div>"); // End grid
    
    // Save button only for main and special views (button forms)
    if (currentView == "main" || currentView == "special") {
        out += F("<button type='submit'>Save Configuration</button>");
    }
    out += F("</form>");
    
    // System Settings (only for system view)
    if (currentView == "system") {
        out += F("<div class='card' style='margin-top:20px'><h4>System Settings</h4><form method='POST' action='/saveSystem'>");
        snprintf(buf, sizeof(buf), "<div class='f'><label>BLE Name:</label><input name='ble' value='%s'></div>", systemConfig.bleDeviceName);
        out += buf;
        
        // BLE Mode dropdown
        out += F("<div class='f'><label>BLE Mode:</label><select name='bleMode'>");
        out += F("<option value='0'"); if(systemConfig.bleMode == BLE_CLIENT_ONLY) out += F(" selected"); out += F(">Client Only (→SPM)</option>");
        out += F("<option value='1'"); if(systemConfig.bleMode == BLE_SERVER_ONLY) out += F(" selected"); out += F(">Server Only (DAW→)</option>");
        out += F("<option value='2'"); if(systemConfig.bleMode == BLE_DUAL_MODE) out += F(" selected"); out += F(">Dual Mode (DAW→ESP→SPM)</option>");
        out += F("</select></div>");
        
        snprintf(buf, sizeof(buf), "<div class='f'><label>SSID:</label><input name='ssid' value='%s'></div>", systemConfig.apSSID);
        out += buf;
        snprintf(buf, sizeof(buf), "<div class='f'><label>Pass:</label><input name='pass' value='%s'></div>", systemConfig.apPassword);
        out += buf;
        
        // Hardware Configuration
        out += F("<div style='border-top:1px solid #333;margin:15px 0;padding-top:15px'>");
        snprintf(buf, sizeof(buf), "<div class='f'><label>Buttons:</label><input type='number' name='btnCount' min='4' max='10' value='%d'></div>", systemConfig.buttonCount);
        out += buf;
        
        // Button pins as comma-separated string
        char pinStr[64] = "";
        for (int i = 0; i < systemConfig.buttonCount; i++) {
            if (i > 0) strcat(pinStr, ",");
            char p[4]; snprintf(p, 4, "%d", systemConfig.buttonPins[i]);
            strcat(pinStr, p);
        }
        snprintf(buf, sizeof(buf), "<div class='f'><label>Pins:</label><input name='btnPins' value='%s' placeholder='32,33,25,...'></div>", pinStr);
        out += buf;
        
        snprintf(buf, sizeof(buf), "<div class='f'><label>LED Pin:</label><input type='number' name='ledPin' min='0' max='39' value='%d'></div>", systemConfig.ledPin);
        out += buf;
        snprintf(buf, sizeof(buf), "<div class='f'><label>LEDs/Btn:</label><input type='number' name='ledsPerBtn' min='1' max='32' value='%d' title='LEDs per button (for LED strips)'></div>", systemConfig.ledsPerButton);
        out += buf;
        out += F("</div>");
        
        // Encoder Configuration
        out += F("<div style='border-top:1px solid #333;margin:15px 0;padding-top:15px'>");
        snprintf(buf, sizeof(buf), "<div class='f'><label>Enc A:</label><input type='number' name='encA' min='0' max='39' value='%d'></div>", systemConfig.encoderA);
        out += buf;
        snprintf(buf, sizeof(buf), "<div class='f'><label>Enc B:</label><input type='number' name='encB' min='0' max='39' value='%d'></div>", systemConfig.encoderB);
        out += buf;
        snprintf(buf, sizeof(buf), "<div class='f'><label>Enc Btn:</label><input type='number' name='encBtn' min='0' max='39' value='%d'></div>", systemConfig.encoderBtn);
        out += buf;
        out += F("</div>");
        
        // LED Map Configuration (for single LED mode)
        out += F("<div style='border-top:1px solid #333;margin:15px 0;padding-top:15px'>");
        out += F("<div class='f'><label>LED Map:</label><input name='ledMap' value='");
        // Output LED map as comma-separated values
        for (int i = 0; i < 10; i++) {
            if (i > 0) out += F(",");
            char n[4]; snprintf(n, sizeof(n), "%d", systemConfig.ledMap[i]);
            out += n;
        }
        out += F("' title='Button to LED mapping. Ignored if LEDs/Btn > 1 (sequential mode)'></div>");
        out += F("</div>");
        
        out += F("<button style='background:#17a2b8;margin-top:10px'>Save System & Reboot</button></form></div>");
    }
    
    out += F("<div class='foot'><a href='/export'>Export JSON</a> <a href='/import'>Import JSON</a></div>");
    out += F("<div style='text-align:center;color:#666;font-size:11px;margin-top:10px'>Chocotone v2.0</div>");
    out += F("</body></html>");
    
    server.sendContent(out);
    
    // Release request lock
    requestInProgress = false;
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

    for (int t = 0; t < systemConfig.buttonCount; t++) {
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

        // Message A - only process if the fields exist in the form (main view)
        char ida[15]; snprintf(ida, sizeof(ida), "%s_a", id);
        bool newIsAlternate = config.isAlternate;  // Keep existing value by default
        if(server.hasArg(String(ida) + "_type")) {  // Only update if Message A fields exist (main view)
            // Alt checkbox is only in main view, so update it here
            newIsAlternate = server.hasArg(String(id) + "_alt");
            if(config.isAlternate != newIsAlternate) { config.isAlternate = newIsAlternate; changed = true; }
            
            MidiMessage &msgA = config.messageA;
            MidiCommandType newTypeA = parseCommandType(server.arg(String(ida)+"_type"));
            byte newChA = server.arg(String(ida)+"_ch").toInt();
            byte newD1A = server.arg(String(ida)+"_d1").toInt();
            byte newD2A = server.arg(String(ida)+"_d2").toInt();
            byte newRgbA[3]; hexToRgb(server.arg(String(ida)+"_rgb"), newRgbA);
            
            // Update fields individually (can't use struct initializer with char array)
            if(msgA.type != newTypeA) { msgA.type = newTypeA; changed = true; }
            if(msgA.channel != newChA) { msgA.channel = newChA; changed = true; }
            if(msgA.data1 != newD1A) { msgA.data1 = newD1A; changed = true; }
            if(msgA.data2 != newD2A) { msgA.data2 = newD2A; changed = true; }
            
            // Tap Tempo control buttons
            int8_t newRPrevA = server.arg(String(ida)+"_rprev").toInt();
            if (newRPrevA == 0) newRPrevA = -1; else newRPrevA = newRPrevA - 1;  // 1-8 → 0-7
            if(msgA.rhythmPrevButton != newRPrevA) { msgA.rhythmPrevButton = newRPrevA; changed = true; }
            
            int8_t newRNextA = server.arg(String(ida)+"_rnext").toInt();
            if (newRNextA == 0) newRNextA = -1; else newRNextA = newRNextA - 1;
            if(msgA.rhythmNextButton != newRNextA) { msgA.rhythmNextButton = newRNextA; changed = true; }
            
            int8_t newLockA = server.arg(String(ida)+"_lock").toInt();
            if (newLockA == 0) newLockA = -1; else newLockA = newLockA - 1;
            if(msgA.tapModeLockButton != newLockA) { msgA.tapModeLockButton = newLockA; changed = true; }
            
            if(memcmp(msgA.rgb, newRgbA, 3) != 0) {
                memcpy(msgA.rgb, newRgbA, 3); changed = true;
            }
        }

        // Message B (only if alternate AND fields exist in form)
        char idb[15]; snprintf(idb, sizeof(idb), "%s_b", id);
        if(newIsAlternate && server.hasArg(String(idb) + "_type")) {
            MidiMessage &msgB = config.messageB;
            MidiCommandType newTypeB = parseCommandType(server.arg(String(idb)+"_type"));
            byte newChB = server.arg(String(idb)+"_ch").toInt();
            byte newD1B = server.arg(String(idb)+"_d1").toInt();
            byte newD2B = server.arg(String(idb)+"_d2").toInt();
            byte newRgbB[3]; hexToRgb(server.arg(String(idb)+"_rgb"), newRgbB);
            
            if(msgB.type != newTypeB) { msgB.type = newTypeB; changed = true; }
            if(msgB.channel != newChB) { msgB.channel = newChB; changed = true; }
            if(msgB.data1 != newD1B) { msgB.data1 = newD1B; changed = true; }
            if(msgB.data2 != newD2B) { msgB.data2 = newD2B; changed = true; }
            
            // Tap Tempo control buttons
            int8_t newRPrevB = server.arg(String(idb)+"_rprev").toInt();
            if (newRPrevB == 0) newRPrevB = -1; else newRPrevB = newRPrevB - 1;
            if(msgB.rhythmPrevButton != newRPrevB) { msgB.rhythmPrevButton = newRPrevB; changed = true; }
            
            int8_t newRNextB = server.arg(String(idb)+"_rnext").toInt();
            if (newRNextB == 0) newRNextB = -1; else newRNextB = newRNextB - 1;
            if(msgB.rhythmNextButton != newRNextB) { msgB.rhythmNextButton = newRNextB; changed = true; }
            
            int8_t newLockB = server.arg(String(idb)+"_lock").toInt();
            if (newLockB == 0) newLockB = -1; else newLockB = newLockB - 1;
            if(msgB.tapModeLockButton != newLockB) { msgB.tapModeLockButton = newLockB; changed = true; }
            
            if(memcmp(msgB.rgb, newRgbB, 3) != 0) {
                memcpy(msgB.rgb, newRgbB, 3); changed = true;
            }
        }

        
        // Hold Action - save to globalSpecialActions (NOT buttonConfigs!)
        // Only update if the hold form section exists (prevents overwriting from other preset saves)
        HoldAction& hold = globalSpecialActions[t].hold;
        if(server.hasArg(String(id) + "_hold_ms")) {  // If hold section exists in form
            bool newHoldEn = server.hasArg(String(id) + "_hold_en");
            if(hold.enabled != newHoldEn) { hold.enabled = newHoldEn; changed = true; }
            
            uint16_t newMs = server.arg(String(id) + "_hold_ms").toInt();
            if(hold.thresholdMs != newMs) { hold.thresholdMs = newMs; changed = true; }
            
            if(server.hasArg(String(id) + "_hold_type")) {
                MidiCommandType newType = parseCommandType(server.arg(String(id) + "_hold_type"));
                if(hold.type != newType) { hold.type = newType; changed = true; }
            }
            if(server.hasArg(String(id) + "_hold_ch")) {
                byte newCh = server.arg(String(id) + "_hold_ch").toInt();
                if(hold.channel != newCh) { hold.channel = newCh; changed = true; }
            }
            if(server.hasArg(String(id) + "_hold_d1")) {
                byte newD1 = server.arg(String(id) + "_hold_d1").toInt();
                if(hold.data1 != newD1) { hold.data1 = newD1; changed = true; }
            }
            if(server.hasArg(String(id) + "_hold_d2")) {
                byte newD2 = server.arg(String(id) + "_hold_d2").toInt();
                if(hold.data2 != newD2) { hold.data2 = newD2; changed = true; }
            }
        }  // End of hold section check
        
        // Combo Action - save to globalSpecialActions (NOT buttonConfigs!)
        // Only update if the combo form section exists
        ComboAction& combo = globalSpecialActions[t].combo;
        if(server.hasArg(String(id) + "_combo_partner")) {  // If combo section exists in form
            bool newComboEn = server.hasArg(String(id) + "_combo_en");
            if(combo.enabled != newComboEn) { combo.enabled = newComboEn; changed = true; }
        
            if(server.hasArg(String(id) + "_combo_label")) {
                String newLabel = server.arg(String(id) + "_combo_label");
                if(strncmp(combo.label, newLabel.c_str(), 6) != 0) {
                    strncpy(combo.label, newLabel.c_str(), 6);
                    combo.label[6] = '\0';
                    changed = true;
                }
            }
            
            int8_t newPartner = server.arg(String(id) + "_combo_partner").toInt() - 1;
            if(combo.partner != newPartner) { combo.partner = newPartner; changed = true; }
            
            if(server.hasArg(String(id) + "_combo_type")) {
                MidiCommandType newType = parseCommandType(server.arg(String(id) + "_combo_type"));
                if(combo.type != newType) { combo.type = newType; changed = true; }
            }
            if(server.hasArg(String(id) + "_combo_ch")) {
                byte newCh = server.arg(String(id) + "_combo_ch").toInt();
                if(combo.channel != newCh) { combo.channel = newCh; changed = true; }
            }
            if(server.hasArg(String(id) + "_combo_d1")) {
                byte newD1 = server.arg(String(id) + "_combo_d1").toInt();
                if(combo.data1 != newD1) { combo.data1 = newD1; changed = true; }
            }
            if(server.hasArg(String(id) + "_combo_d2")) {
                byte newD2 = server.arg(String(id) + "_combo_d2").toInt();
                if(combo.data2 != newD2) { combo.data2 = newD2; changed = true; }
            }
        }  // End of combo section check
        
        yield();  // Allow WDT to reset after each button
    }

    if (changed) {
        yield();  // Before blocking NVS write
        savePresets();
        
        Serial.println("Save complete - deferring display update...");
        Serial.printf("Free heap after save: %d bytes\n", ESP.getFreeHeap());
        
        // DEFER display/LED updates to main loop - calling them here crashes due to low heap with WiFi
        pendingDisplayUpdate = true;
        
        // Give time for memory to stabilize before redirect
        delay(150);
        yield();
        
        // Get view parameter to redirect back to same view
        String view = server.hasArg("view") ? server.arg("view") : "main";
        
        // Simple success page - NO auto-redirect to prevent memory crash
        server.send(200, "text/html", 
            "<html><head><meta charset='UTF-8'></head>"
            "<body style='background:#121212;color:#e0e0e0;font-family:sans-serif;text-align:center;padding:80px'>"
            "<h2 style='color:#bb86fc'>✓ Saved!</h2>"
            "<p>Settings saved to NVS successfully.</p>"
            "<a href='/?preset="+String(preset)+"&view="+view+"' style='display:inline-block;margin-top:30px;padding:12px 30px;background:#3700b3;color:#fff;text-decoration:none;border-radius:8px'>Back to Editor</a>"
            "</body></html>");
    } else {
        String view = server.hasArg("view") ? server.arg("view") : "main";
        server.send(200, "text/html", "<html><body style='background:#222;color:#fff;text-align:center;padding:50px'>No changes. <a href='/?preset="+String(preset)+"&view="+view+"' style='color:#8af'>Back</a></body></html>");
    }
}

void handleSaveSystem() {
    bool changed = false;
    if(server.hasArg("ble")) {
        String newName = server.arg("ble");
        if(strncmp(systemConfig.bleDeviceName, newName.c_str(), 23) != 0) {
            strncpy(systemConfig.bleDeviceName, newName.c_str(), 23); systemConfig.bleDeviceName[23] = '\0'; changed = true;
        }
    }
    if(server.hasArg("ssid")) {
        String newSSID = server.arg("ssid");
        if(strncmp(systemConfig.apSSID, newSSID.c_str(), 23) != 0) {
            strncpy(systemConfig.apSSID, newSSID.c_str(), 23); systemConfig.apSSID[23] = '\0'; changed = true;
        }
    }
    if(server.hasArg("pass")) {
        String newPass = server.arg("pass");
        if(newPass.length() >= 8 && newPass.length() < 16 && strncmp(systemConfig.apPassword, newPass.c_str(), 15) != 0) {
            strncpy(systemConfig.apPassword, newPass.c_str(), 15); systemConfig.apPassword[15] = '\0'; changed = true;
        }
    }
    
    // BLE Mode
    if(server.hasArg("bleMode")) {
        int newMode = server.arg("bleMode").toInt();
        if(newMode >= 0 && newMode <= 2 && (BleMode)newMode != systemConfig.bleMode) {
            systemConfig.bleMode = (BleMode)newMode;
            changed = true;
            Serial.printf("BLE Mode changed to: %d\n", newMode);
        }
    }
    
    // v2 Hardware Configuration
    if(server.hasArg("btnCount")) {
        int newCount = server.arg("btnCount").toInt();
        if(newCount >= 4 && newCount <= 10 && newCount != systemConfig.buttonCount) {
            systemConfig.buttonCount = newCount; changed = true;
        }
    }
    if(server.hasArg("btnPins")) {
        String pins = server.arg("btnPins");
        // Parse comma-separated pins
        int idx = 0;
        int start = 0;
        for(int i = 0; i <= pins.length() && idx < 10; i++) {
            if(i == pins.length() || pins[i] == ',') {
                String p = pins.substring(start, i);
                p.trim();
                if(p.length() > 0) {
                    int pin = p.toInt();
                    if(pin >= 0 && pin <= 39) {
                        systemConfig.buttonPins[idx++] = pin;
                        changed = true;
                    }
                }
                start = i + 1;
            }
        }
    }
    if(server.hasArg("ledPin")) {
        int pin = server.arg("ledPin").toInt();
        if(pin >= 0 && pin <= 39 && pin != systemConfig.ledPin) {
            systemConfig.ledPin = pin; changed = true;
        }
    }
    if(server.hasArg("ledsPerBtn")) {
        int lpb = server.arg("ledsPerBtn").toInt();
        if(lpb >= 1 && lpb <= 32 && lpb != systemConfig.ledsPerButton) {
            systemConfig.ledsPerButton = lpb; changed = true;
        }
    }
    if(server.hasArg("encA")) {
        int pin = server.arg("encA").toInt();
        if(pin >= 0 && pin <= 39 && pin != systemConfig.encoderA) {
            systemConfig.encoderA = pin; changed = true;
        }
    }
    if(server.hasArg("encB")) {
        int pin = server.arg("encB").toInt();
        if(pin >= 0 && pin <= 39 && pin != systemConfig.encoderB) {
            systemConfig.encoderB = pin; changed = true;
        }
    }
    if(server.hasArg("encBtn")) {
        int pin = server.arg("encBtn").toInt();
        if(pin >= 0 && pin <= 39 && pin != systemConfig.encoderBtn) {
            systemConfig.encoderBtn = pin; changed = true;
        }
    }
    if(server.hasArg("ledMap")) {
        String mapStr = server.arg("ledMap");
        // Parse comma-separated LED map values
        int idx = 0;
        int start = 0;
        for(int i = 0; i <= mapStr.length() && idx < 10; i++) {
            if(i == mapStr.length() || mapStr[i] == ',') {
                String v = mapStr.substring(start, i);
                v.trim();
                if(v.length() > 0) {
                    int val = v.toInt();
                    if(val >= 0 && val <= 255) {
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
    String html = F("<!DOCTYPE html><html><head><title>Rebooting...</title><style>body{font-family:Inter,sans-serif;background-color:#1a1a2e;color:#e0e0e0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;text-align:center;}.message-box{background-color:#2a2a4a;padding:30px;border-radius:12px;}h1{color:#8d8dff;}</style></head><body><div class='message-box'><h1>");
    html += message;
    html += F("</h1><p>Device is rebooting. Please reconnect to the new Wi-Fi AP if you changed it.</p></div></body></html>");
    server.send(200, "text/html", html);
    delay(1000);
    ESP.restart();
}

void handleExport() {
    server.sendHeader("Content-Disposition", "attachment; filename=\"midi_presets_v2.json\"");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");

    // Stream the JSON structure manually to avoid massive memory allocation
    server.sendContent("{\"version\":2,\"presets\":[");

    for (int p = 0; p < 4; p++) {
        if (p > 0) server.sendContent(",");

        // Start Preset Object
        String pHeader = "{\"name\":\"" + String(presetNames[p]) + "\",\"buttons\":[";
        server.sendContent(pHeader);

        for (int t = 0; t < systemConfig.buttonCount; t++) {
            if (t > 0) server.sendContent(",");

            // Use a small StaticJsonDocument for just ONE button to ensure valid JSON
            // and minimal memory usage
            StaticJsonDocument<1024> doc;  // Increased for sysex
            JsonObject bObj = doc.to<JsonObject>();
            
            const ButtonConfig& config = buttonConfigs[p][t];
            bObj["name"] = config.name;
            bObj["alt"] = config.isAlternate;

            JsonObject msgA = bObj.createNestedObject("msgA");
            msgA["type"] = getCommandTypeString(config.messageA.type);
            msgA["ch"] = config.messageA.channel;
            msgA["d1"] = config.messageA.data1;
            msgA["d2"] = config.messageA.data2;
            msgA["sysex"] = config.messageA.sysex;  // v2 sysex field
            char hexColorA[8];
            rgbToHex(hexColorA, sizeof(hexColorA), config.messageA.rgb);
            msgA["rgb"] = hexColorA;

            JsonObject msgB = bObj.createNestedObject("msgB");
            msgB["type"] = getCommandTypeString(config.messageB.type);
            msgB["ch"] = config.messageB.channel;
            msgB["d1"] = config.messageB.data1;
            msgB["d2"] = config.messageB.data2;
            msgB["sysex"] = config.messageB.sysex;  // v2 sysex field
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
    
    // Add System Settings (v2 extended)
    String sys = "\"system\":{";
    sys += "\"ble\":\"" + String(systemConfig.bleDeviceName) + "\",";
    sys += "\"ssid\":\"" + String(systemConfig.apSSID) + "\",";
    sys += "\"pass\":\"" + String(systemConfig.apPassword) + "\",";
    sys += "\"buttonCount\":" + String(systemConfig.buttonCount) + ",";
    sys += "\"buttonPins\":\"";
    for (int i = 0; i < systemConfig.buttonCount; i++) {
        if (i > 0) sys += ",";
        sys += String(systemConfig.buttonPins[i]);
    }
    sys += "\",";
    sys += "\"ledPin\":" + String(systemConfig.ledPin) + ",";
    sys += "\"encoderA\":" + String(systemConfig.encoderA) + ",";
    sys += "\"encoderB\":" + String(systemConfig.encoderB) + ",";
    sys += "\"encoderBtn\":" + String(systemConfig.encoderBtn) + "},";
    server.sendContent(sys);
    
    // Add Global Special Actions (hold/combo) - v2.1
    server.sendContent("\"globalSpecialActions\":[");
    for (int b = 0; b < systemConfig.buttonCount; b++) {
        if (b > 0) server.sendContent(",");
        const SpecialAction& spec = globalSpecialActions[b];
        String sa = "{\"hold\":{";
        sa += "\"enabled\":" + String(spec.hold.enabled ? "true" : "false") + ",";
        sa += "\"ms\":" + String(spec.hold.thresholdMs) + ",";
        sa += "\"type\":\"" + String(getCommandTypeString(spec.hold.type)) + "\",";
        sa += "\"ch\":" + String(spec.hold.channel) + ",";
        sa += "\"d1\":" + String(spec.hold.data1) + ",";
        sa += "\"d2\":" + String(spec.hold.data2) + "},";
        sa += "\"combo\":{";
        sa += "\"enabled\":" + String(spec.combo.enabled ? "true" : "false") + ",";
        sa += "\"partner\":" + String(spec.combo.partner) + ",";
        sa += "\"label\":\"" + String(spec.combo.label) + "\",";
        sa += "\"type\":\"" + String(getCommandTypeString(spec.combo.type)) + "\",";
        sa += "\"ch\":" + String(spec.combo.channel) + ",";
        sa += "\"d1\":" + String(spec.combo.data1) + ",";
        sa += "\"d2\":" + String(spec.combo.data2) + "}}";
        server.sendContent(sa);
        yield();
    }
    server.sendContent("]");

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
        if (bArray.isNull()) continue;
        
        // Import up to buttonCount buttons (dynamic)
        int importCount = min((int)bArray.size(), (int)systemConfig.buttonCount);

        for (int t = 0; t < importCount; t++) {
            JsonObject bObj = bArray[t];
            ButtonConfig& config = buttonConfigs[p][t];

            strncpy(config.name, bObj["name"] | "Imported Btn", 20);
            config.name[20] = '\0';
            config.isAlternate = bObj["alt"] | false;
            config.nextIsB = false;

            JsonObject msgA = bObj["msgA"];
            config.messageA.type = parseCommandType(msgA["type"] | "CC");
            config.messageA.channel = msgA["ch"] | 1;
            config.messageA.data1 = msgA["d1"] | 0;
            config.messageA.data2 = msgA["d2"] | 0;
            // v2 sysex field (max 11 chars + null = 12 bytes)
            const char* sysexA = msgA["sysex"] | "";
            strncpy(config.messageA.sysex, sysexA, 11);
            config.messageA.sysex[11] = '\0';
            hexToRgb(msgA["rgb"] | "#0000FF", config.messageA.rgb);

            JsonObject msgB = bObj["msgB"];
            config.messageB.type = parseCommandType(msgB["type"] | "CC");
            config.messageB.channel = msgB["ch"] | 1;
            config.messageB.data1 = msgB["d1"] | 0;
            config.messageB.data2 = msgB["d2"] | 0;
            // v2 sysex field (max 11 chars + null = 12 bytes)
            const char* sysexB = msgB["sysex"] | "";
            strncpy(config.messageB.sysex, sysexB, 11);
            config.messageB.sysex[11] = '\0';
            hexToRgb(msgB["rgb"] | "#FF0000", config.messageB.rgb);
            
            yield();  // Allow WDT to reset after each button
        }
    }
    
    yield();  // Before system settings
    
    // Import System Settings if present (v2 extended)
    JsonObject sys = doc["system"];
    if (!sys.isNull()) {
        const char* ble = sys["ble"];
        const char* ssid = sys["ssid"];
        const char* pass = sys["pass"];
        
        if (ble) { strncpy(systemConfig.bleDeviceName, ble, 23); systemConfig.bleDeviceName[23] = '\0'; }
        if (ssid) { strncpy(systemConfig.apSSID, ssid, 23); systemConfig.apSSID[23] = '\0'; }
        if (pass) { strncpy(systemConfig.apPassword, pass, 15); systemConfig.apPassword[15] = '\0'; }
        
        // v2 extended system settings
        if (sys.containsKey("buttonCount")) {
            int btnCount = sys["buttonCount"];
            if (btnCount >= 4 && btnCount <= 10) systemConfig.buttonCount = btnCount;
        }
        if (sys.containsKey("buttonPins")) {
            String pins = sys["buttonPins"].as<String>();
            int idx = 0;
            int start = 0;
            for (int i = 0; i <= pins.length() && idx < 10; i++) {
                if (i == pins.length() || pins[i] == ',') {
                    String p = pins.substring(start, i);
                    p.trim();
                    if (p.length() > 0) {
                        int pin = p.toInt();
                        if (pin >= 0 && pin <= 39) systemConfig.buttonPins[idx++] = pin;
                    }
                    start = i + 1;
                }
            }
        }
        if (sys.containsKey("ledPin")) systemConfig.ledPin = sys["ledPin"];
        if (sys.containsKey("encoderA")) systemConfig.encoderA = sys["encoderA"];
        if (sys.containsKey("encoderB")) systemConfig.encoderB = sys["encoderB"];
        if (sys.containsKey("encoderBtn")) systemConfig.encoderBtn = sys["encoderBtn"];
        
        yield();
        saveSystemSettings(); // Save to NVS
    }

    // Import Global Special Actions if present (v2.1)
    JsonArray specials = doc["globalSpecialActions"];
    if (!specials.isNull()) {
        int importCount = min((int)specials.size(), (int)systemConfig.buttonCount);
        for (int b = 0; b < importCount; b++) {
            JsonObject sObj = specials[b];
            JsonObject hold = sObj["hold"];
            JsonObject combo = sObj["combo"];
            
            if (!hold.isNull()) {
                globalSpecialActions[b].hold.enabled = hold["enabled"] | false;
                globalSpecialActions[b].hold.thresholdMs = hold["ms"] | 500;
                globalSpecialActions[b].hold.type = parseCommandType(hold["type"] | "OFF");
                globalSpecialActions[b].hold.channel = hold["ch"] | 1;
                globalSpecialActions[b].hold.data1 = hold["d1"] | 0;
                globalSpecialActions[b].hold.data2 = hold["d2"] | 0;
            }
            if (!combo.isNull()) {
                globalSpecialActions[b].combo.enabled = combo["enabled"] | false;
                globalSpecialActions[b].combo.partner = combo["partner"] | 0;
                const char* lbl = combo["label"] | "";
                strncpy(globalSpecialActions[b].combo.label, lbl, 6);
                globalSpecialActions[b].combo.label[6] = '\0';
                globalSpecialActions[b].combo.type = parseCommandType(combo["type"] | "OFF");
                globalSpecialActions[b].combo.channel = combo["ch"] | 1;
                globalSpecialActions[b].combo.data1 = combo["d1"] | 0;
                globalSpecialActions[b].combo.data2 = combo["d2"] | 0;
            }
            yield();
        }
        Serial.println("Imported globalSpecialActions");
    }

    yield();  // Before final save
    savePresets();
    
    // Defer display/LED updates to main loop for stability
    pendingDisplayUpdate = true;
    
    // Single response with JavaScript redirect (no double-send error)
    String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    response += "<meta http-equiv='refresh' content='2;url=/' />";
    response += "<style>body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;";
    response += "display:flex;justify-content:center;align-items:center;height:100vh;margin:0;}";
    response += ".box{background:#2a2a4a;padding:40px;border-radius:12px;text-align:center;}";
    response += "h2{color:#8d8dff;}</style></head><body>";
    response += "<div class='box'><h2>✓ Import Successful!</h2>";
    response += "<p>Redirecting to editor...</p></div></body></html>";
    server.send(200, "text/html", response);
}


void turnWifiOn() {
    if (isWifiOn) return;
    
    Serial.printf("Free heap before WiFi: %d bytes\n", ESP.getFreeHeap());
    
    // Pause ALL BLE activity to avoid WiFi conflicts
    Serial.println("Pausing BLE for WiFi stability...");
    
    // Stop BLE Client scanning first (always safe to call)
    doScan = false;  // Prevent auto-scanning
    
    // Only try to stop scan if BLE was initialized
    try {
        BLEScan* pScan = BLEDevice::getScan();
        if (pScan) {
            pScan->stop();
            Serial.println("BLE: scan stopped");
        }
    } catch (...) {
        Serial.println("BLE: scan stop failed (ignored)");
    }
    
    // Disconnect client if connected AND pClient exists
    if (clientConnected && pClient != nullptr) {
        try {
            pClient->disconnect();
            Serial.println("BLE Client: disconnected");
        } catch (...) {
            Serial.println("BLE Client: disconnect failed (ignored)");
        }
    }
    
    // Stop BLE Server advertising (if that mode is active)
    if (systemConfig.bleMode == BLE_SERVER_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
        try {
            BLEDevice::stopAdvertising();
            Serial.println("BLE Server: advertising stopped");
        } catch (...) {
            Serial.println("BLE Server: stop advertising failed (ignored)");
        }
    }
    
    // Give BLE time to fully release resources
    delay(300);
    yield();
    
    Serial.printf("Free heap after BLE stop: %d bytes\n", ESP.getFreeHeap());
    
    WiFi.mode(WIFI_AP);
    yield();
    
    WiFi.softAP(systemConfig.apSSID, systemConfig.apPassword);
    yield();
    
    server.begin();
    yield();
    
    isWifiOn = true;
    Serial.printf("WiFi AP Started (BLE paused) - Free heap: %d bytes\n", ESP.getFreeHeap());
}

void turnWifiOff() {
    if (!isWifiOn) return;
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    isWifiOn = false;
    Serial.println("WiFi AP Stopped");
    
    // Resume BLE based on mode
    Serial.println("Resuming BLE...");
    
    // Resume scanning only if client mode is enabled
    if (systemConfig.bleMode == BLE_CLIENT_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
        doScan = true;
        startBleScan();
        Serial.println("BLE Client: scan resumed");
    }
    
    // Server advertising restarts automatically when a client disconnects
    // But if server was stopped, it needs to be restarted
    if (systemConfig.bleMode == BLE_SERVER_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
        BLEDevice::startAdvertising();
        Serial.println("BLE Server: advertising resumed");
    }
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
