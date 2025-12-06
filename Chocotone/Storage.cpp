#include "Storage.h"
#include "DefaultPresets.h"

#define PRESETS_NAMESPACE "midi_presets"

void saveSystemSettings() {
    Serial.println("=== Saving System Settings to NVS ===");
    Serial.printf("Current values: Font=%d, Preset=%d, LED=%d/%d\n", 
                  buttonNameFontSize, currentPreset, ledBrightnessOn, ledBrightnessDim);
    
    // FIRST: Clear dead namespaces to free space (only runs once)
    static bool cleaned = false;
    if (!cleaned) {
        Serial.println("Clearing corrupted/legacy namespaces to free NVS space...");
        Preferences cleanup;
        if (cleanup.begin("midi_system", false)) {
            cleanup.clear();
            cleanup.end();
            Serial.println("  Cleared: midi_system");
        }
        if (cleanup.begin("midi-presets", false)) {
            cleanup.clear();
            cleanup.end();
            Serial.println("  Cleared: midi-presets (legacy)");
        }
        cleaned = true;
        delay(100);
    }
    
    // Use separate Preferences object to avoid blob/key conflicts
    Preferences sysPrefs;
    if (!sysPrefs.begin("sys_cfg", false)) {
        Serial.println("ERROR: Cannot open sys_cfg namespace!");
        return;
    }
    
    sysPrefs.putInt("font", buttonNameFontSize);
    sysPrefs.putInt("ledOn", ledBrightnessOn);
    sysPrefs.putInt("ledDim", ledBrightnessDim);
    sysPrefs.putInt("debounce", buttonDebounce);
    sysPrefs.putInt("rhythm", rhythmPattern);
    sysPrefs.putInt("delay", currentDelayType);
    sysPrefs.putInt("preset", currentPreset);
    sysPrefs.putBool("wifi", wifiOnAtBoot);
    sysPrefs.putString("bleName", bleDeviceName);
    sysPrefs.putString("apSSID", apSSID);
    sysPrefs.putString("apPass", apPassword);
    
    sysPrefs.end();
    
    Serial.println("=== System Settings Saved (NVS closed) ===");
}

void loadSystemSettings() {
    Preferences sysPrefs;
    sysPrefs.begin("sys_cfg", true);
    
    buttonNameFontSize = sysPrefs.getInt("font", 4);
    ledBrightnessOn = sysPrefs.getInt("ledOn", 50);
    ledBrightnessDim = sysPrefs.getInt("ledDim", 90);
    buttonDebounce = sysPrefs.getInt("debounce", 120);
    rhythmPattern = sysPrefs.getInt("rhythm", 0);
    if(rhythmPattern < 0 || rhythmPattern > 3) rhythmPattern = 0;
    currentDelayType = sysPrefs.getInt("delay", 0);
    currentPreset = sysPrefs.getInt("preset", 0);
    if(currentPreset < 0 || currentPreset > 3) currentPreset = 0;
    wifiOnAtBoot = sysPrefs.getBool("wifi", false);
    
    String s_bleName = sysPrefs.getString("bleName", DEFAULT_BLE_NAME);
    strncpy(bleDeviceName, s_bleName.c_str(), 31); bleDeviceName[31] = '\0';
    
    String s_ssid = sysPrefs.getString("apSSID", DEFAULT_AP_SSID);
    strncpy(apSSID, s_ssid.c_str(), 31); apSSID[31] = '\0';
    
    String s_pass = sysPrefs.getString("apPass", DEFAULT_AP_PASS);
    strncpy(apPassword, s_pass.c_str(), 63); apPassword[63] = '\0';
    
    sysPrefs.end();
    
    Serial.println("=== System Settings Loaded ===");
    Serial.printf("  Font: %d, Preset: %d\n", buttonNameFontSize, currentPreset);
}

void savePresets() {
    systemPrefs.begin(PRESETS_NAMESPACE, false);
    systemPrefs.putBytes("presets", buttonConfigs, sizeof(buttonConfigs));
    systemPrefs.putBytes("presetNames", presetNames, sizeof(presetNames));
    systemPrefs.end();
    Serial.println("Presets Saved.");
}

void loadPresets() {
    // Try new blob format first
    systemPrefs.begin(PRESETS_NAMESPACE, true);
    size_t len = systemPrefs.getBytesLength("presets");
    
    if (len == sizeof(buttonConfigs)) {
        systemPrefs.getBytes("presets", buttonConfigs, len);
        size_t lenNames = systemPrefs.getBytesLength("presetNames");
        if (lenNames == sizeof(presetNames)) {
            systemPrefs.getBytes("presetNames", presetNames, lenNames);
        }
        systemPrefs.end();
        Serial.println("✓ Presets Loaded (Blob Format).");
        return;
    }
    systemPrefs.end();
    
    // Try legacy individual-key format (midi-presets namespace)
    Preferences legacyPrefs;
    legacyPrefs.begin("midi-presets", true);
    
    bool foundLegacyData = false;
    for (int p = 0; p < 4; p++) {
        String presetNameKey = "pName" + String(p);
        if (legacyPrefs.isKey(presetNameKey.c_str())) {
            String name = legacyPrefs.getString(presetNameKey.c_str(), "");
            if (name.length() > 0) {
                strncpy(presetNames[p], name.c_str(), 20);
                presetNames[p][20] = '\0';
                foundLegacyData = true;
            }
        }
        
        for (int t = 0; t < NUM_BUTTONS; t++) {
            String key = "cfg_p" + String(p) + "t" + String(t);
            if (legacyPrefs.isKey(key.c_str())) {
                size_t keyLen = legacyPrefs.getBytesLength(key.c_str());
                if (keyLen == sizeof(ButtonConfig)) {
                    legacyPrefs.getBytes(key.c_str(), &buttonConfigs[p][t], sizeof(ButtonConfig));
                    foundLegacyData = true;
                }
            }
        }
    }
    legacyPrefs.end();
    
    if (foundLegacyData) {
        Serial.println("✓ Legacy Presets Loaded (Individual Keys).");
        savePresets();
        return;
    }
    
    // No data found, create defaults
    Serial.println("⚠ No presets found. Loading Factory Defaults.");
    loadFactoryPresets();  // Load hardcoded user presets
    savePresets();
}

void saveCurrentPresetIndex() {
    Preferences sysPrefs;
    sysPrefs.begin("sys_cfg", false);
    sysPrefs.putInt("preset", currentPreset);
    sysPrefs.end();
}

void loadCurrentPresetIndex() {
    Preferences sysPrefs;
    sysPrefs.begin("sys_cfg", true);
    currentPreset = sysPrefs.getInt("preset", 0);
    sysPrefs.end();
    if (currentPreset < 0 || currentPreset > 3) currentPreset = 0;
}
