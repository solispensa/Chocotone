#include "Storage.h"
#include "DefaultPresets.h"

#define PRESETS_NAMESPACE "midi_presets"
#define CONFIG_VERSION_KEY "cfg_ver"
#define CURRENT_CONFIG_VERSION 3  // v3: Action-based ButtonConfig with messages[] array

// ============================================
// SYSTEM SETTINGS (v2)
// ============================================

void saveSystemSettings() {
    Serial.println("=== Saving System Settings (v2) ===");
    
    // Use a FRESH LOCAL Preferences object to avoid global state issues
    Preferences prefs;
    if (!prefs.begin(PRESETS_NAMESPACE, false)) {
        Serial.println("ERROR: Cannot open presets namespace for system settings!");
        return;
    }
    
    // Write version marker (prefixed to avoid collision with preset version)
    prefs.putInt("sys_ver", CURRENT_CONFIG_VERSION);
    
    // UI Settings (prefixed with "s_" to distinguish from preset data)
    prefs.putInt("s_font", buttonNameFontSize);
    prefs.putInt("s_ledOn", ledBrightnessOn);
    prefs.putInt("s_ledDim", ledBrightnessDim);
    prefs.putInt("s_ledTap", ledBrightnessTap);
    prefs.putInt("s_debounce", buttonDebounce);
    prefs.putInt("s_rhythm", rhythmPattern);
    prefs.putInt("s_delay", currentDelayType);
    prefs.putInt("s_preset", currentPreset);
    
    // SystemConfig struct fields
    prefs.putString("s_bleName", systemConfig.bleDeviceName);
    prefs.putString("s_apSSID", systemConfig.apSSID);
    prefs.putString("s_apPass", systemConfig.apPassword);
    prefs.putInt("s_btnCount", systemConfig.buttonCount);
    prefs.putBytes("s_btnPins", systemConfig.buttonPins, sizeof(systemConfig.buttonPins));
    prefs.putInt("s_ledPin", systemConfig.ledPin);
    prefs.putInt("s_encA", systemConfig.encoderA);
    prefs.putInt("s_encB", systemConfig.encoderB);
    prefs.putInt("s_encBtn", systemConfig.encoderBtn);
    prefs.putBool("s_wifiBoot", systemConfig.wifiOnAtBoot);
    prefs.putUChar("s_bleMode", (uint8_t)systemConfig.bleMode);
    prefs.putUChar("s_ledsPerBtn", systemConfig.ledsPerButton);
    prefs.putBytes("s_ledMap", systemConfig.ledMap, sizeof(systemConfig.ledMap));
    
    prefs.end();
    Serial.println("=== System Settings Saved (v2) ===");
}

void loadSystemSettings() {
    // Use SAME namespace as presets to avoid namespace switch issues
    systemPrefs.begin(PRESETS_NAMESPACE, true);
    
    // Check for new format (sys_ver key) or old format
    int version = systemPrefs.getInt("sys_ver", 0);  // New unified format
    if (version == 0) {
        // Try old format - might not exist yet
        version = 1;
    }
    Serial.printf("Loading System Settings (v%d)...\n", version);
    
    // UI Settings (prefixed keys in new format)
    buttonNameFontSize = systemPrefs.getInt("s_font", 5);
    ledBrightnessOn = systemPrefs.getInt("s_ledOn", 220);
    ledBrightnessDim = systemPrefs.getInt("s_ledDim", 20);
    ledBrightnessTap = systemPrefs.getInt("s_ledTap", 240);
    buttonDebounce = systemPrefs.getInt("s_debounce", 120);
    rhythmPattern = systemPrefs.getInt("s_rhythm", 0);
    if (rhythmPattern < 0 || rhythmPattern > 3) rhythmPattern = 0;
    currentDelayType = systemPrefs.getInt("s_delay", 0);
    currentPreset = systemPrefs.getInt("s_preset", 0);
    if (currentPreset < 0 || currentPreset > 3) currentPreset = 0;
    
    // SystemConfig fields (prefixed keys)
    String s_bleName = systemPrefs.getString("s_bleName", DEFAULT_BLE_NAME);
    strncpy(systemConfig.bleDeviceName, s_bleName.c_str(), 23);
    systemConfig.bleDeviceName[23] = '\0';
    
    String s_ssid = systemPrefs.getString("s_apSSID", DEFAULT_AP_SSID);
    strncpy(systemConfig.apSSID, s_ssid.c_str(), 23);
    systemConfig.apSSID[23] = '\0';
    
    String s_pass = systemPrefs.getString("s_apPass", DEFAULT_AP_PASS);
    strncpy(systemConfig.apPassword, s_pass.c_str(), 15);
    systemConfig.apPassword[15] = '\0';
    
    systemConfig.buttonCount = systemPrefs.getInt("s_btnCount", DEFAULT_BUTTON_COUNT);
    if (systemConfig.buttonCount < 4) systemConfig.buttonCount = 4;
    if (systemConfig.buttonCount > 10) systemConfig.buttonCount = 10;
    
    systemPrefs.getBytes("s_btnPins", systemConfig.buttonPins, sizeof(systemConfig.buttonPins));
    systemConfig.ledPin = systemPrefs.getInt("s_ledPin", NEOPIXEL_PIN);
    systemConfig.encoderA = systemPrefs.getInt("s_encA", DEFAULT_ENCODER_A);
    systemConfig.encoderB = systemPrefs.getInt("s_encB", DEFAULT_ENCODER_B);
    systemConfig.encoderBtn = systemPrefs.getInt("s_encBtn", DEFAULT_ENCODER_BTN);
    systemConfig.wifiOnAtBoot = systemPrefs.getBool("s_wifiBoot", false);
    systemConfig.bleMode = (BleMode)systemPrefs.getUChar("s_bleMode", BLE_CLIENT_ONLY);
    systemConfig.ledsPerButton = systemPrefs.getUChar("s_ledsPerBtn", 1);
    
    // Load ledMap or use defaults
    if (systemPrefs.getBytesLength("s_ledMap") == sizeof(systemConfig.ledMap)) {
        systemPrefs.getBytes("s_ledMap", systemConfig.ledMap, sizeof(systemConfig.ledMap));
    } else {
        // Default ledMap: {0, 1, 2, 3, 7, 6, 5, 4, 8, 9}
        uint8_t defaultMap[] = {0, 1, 2, 3, 7, 6, 5, 4, 8, 9};
        memcpy(systemConfig.ledMap, defaultMap, sizeof(systemConfig.ledMap));
    }
    
    systemPrefs.end();
    Serial.println("=== System Settings Loaded ===");
}

// ============================================
// PRESETS - SPIFFS Storage (v4)
// NVS has ~20KB limit, not enough for 16KB preset data
// SPIFFS has 1MB available in the Huge APP partition
// ============================================

#include <SPIFFS.h>

#define PRESETS_FILE "/presets.bin"

void savePresets() {
    Serial.println("Saving Presets (SPIFFS storage)...");
    
    // Initialize SPIFFS if needed
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS mount failed!");
        return;
    }
    
    // Open file for writing
    File file = SPIFFS.open(PRESETS_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("ERROR: Failed to open presets file for writing!");
        return;
    }
    
    // Write version marker
    uint8_t version = CURRENT_CONFIG_VERSION;
    file.write(&version, 1);
    
    // Write all button configs (4 presets × 10 buttons)
    size_t written = file.write((uint8_t*)buttonConfigs, sizeof(buttonConfigs));
    Serial.printf("  buttonConfigs: %d/%d bytes\n", written, sizeof(buttonConfigs));
    
    // Write preset names
    written = file.write((uint8_t*)presetNames, sizeof(presetNames));
    Serial.printf("  presetNames: %d/%d bytes\n", written, sizeof(presetNames));
    
    // Write LED modes
    written = file.write((uint8_t*)presetLedModes, sizeof(presetLedModes));
    Serial.printf("  ledModes: %d/%d bytes\n", written, sizeof(presetLedModes));
    
    // Write sync modes
    written = file.write((uint8_t*)presetSyncMode, sizeof(presetSyncMode));
    Serial.printf("  syncModes: %d/%d bytes\n", written, sizeof(presetSyncMode));
    
    // Write global special actions
    written = file.write((uint8_t*)globalSpecialActions, sizeof(globalSpecialActions));
    Serial.printf("  specials: %d/%d bytes\n", written, sizeof(globalSpecialActions));
    
    // Write config metadata (editor fields)
    file.write((uint8_t*)configProfileName, sizeof(configProfileName));
    file.write((uint8_t*)configLastModified, sizeof(configLastModified));
    Serial.printf("  metadata: configName='%s'\n", configProfileName);
    
    size_t totalSize = file.size();
    file.close();
    
    Serial.printf("Presets Saved - %d bytes total\n", totalSize);
}

void loadPresets() {
    Serial.println("Loading Presets (SPIFFS storage)...");
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS mount failed!");
        loadFactoryPresets();
        return;
    }
    
    // Check if file exists
    if (!SPIFFS.exists(PRESETS_FILE)) {
        Serial.println("No presets file found - loading factory defaults");
        loadFactoryPresets();
        savePresets();  // Save defaults to SPIFFS
        return;
    }
    
    // Open file for reading
    File file = SPIFFS.open(PRESETS_FILE, FILE_READ);
    if (!file) {
        Serial.println("ERROR: Failed to open presets file!");
        loadFactoryPresets();
        return;
    }
    
    // Read version marker
    uint8_t version = 0;
    file.read(&version, 1);
    Serial.printf("  File version: %d\n", version);
    
    if (version < CURRENT_CONFIG_VERSION) {
        Serial.println("  Old version - loading defaults and migrating");
        file.close();
        loadFactoryPresets();
        savePresets();
        return;
    }
    
    // Read button configs
    size_t read = file.read((uint8_t*)buttonConfigs, sizeof(buttonConfigs));
    Serial.printf("  buttonConfigs: %d/%d bytes\n", read, sizeof(buttonConfigs));
    if (read != sizeof(buttonConfigs)) {
        Serial.println("  ERROR: Size mismatch - using defaults");
        file.close();
        loadFactoryPresets();
        return;
    }
    
    // Read preset names
    file.read((uint8_t*)presetNames, sizeof(presetNames));
    
    // Read LED modes
    file.read((uint8_t*)presetLedModes, sizeof(presetLedModes));
    
    // Read sync modes
    file.read((uint8_t*)presetSyncMode, sizeof(presetSyncMode));
    Serial.printf("  Sync Mode: P1=%d P2=%d P3=%d P4=%d\n", 
        presetSyncMode[0], presetSyncMode[1], presetSyncMode[2], presetSyncMode[3]);
    
    // Read global special actions
    size_t specialsRead = file.read((uint8_t*)globalSpecialActions, sizeof(globalSpecialActions));
    if (specialsRead == sizeof(globalSpecialActions)) {
        Serial.println("  ✓ Special actions loaded");
    } else {
        Serial.println("  Special actions missing - using defaults");
        for (int i = 0; i < MAX_BUTTONS; i++) {
            globalSpecialActions[i].hasCombo = false;
        }
    }
    
    // Read config metadata (editor fields) - optional, may not exist in old files
    size_t metadataRead = file.read((uint8_t*)configProfileName, sizeof(configProfileName));
    if (metadataRead == sizeof(configProfileName)) {
        file.read((uint8_t*)configLastModified, sizeof(configLastModified));
        Serial.printf("  metadata: configName='%s'\n", configProfileName);
    }
    
    file.close();
    Serial.println("✓ Presets Loaded from SPIFFS");
}

// initializeGlobalOverrides() removed - globalOverrides no longer used

// ============================================
// PRESET INDEX
// ============================================

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
