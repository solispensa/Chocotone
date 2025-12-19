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
// PRESETS (v2 with expanded ButtonConfig)
// ============================================

void savePresets() {
    Serial.println("Saving Presets (v3 action-based)...");
    
    yield();  // Let WDT breathe before starting
    
    // Use a FRESH LOCAL Preferences object to avoid global state issues
    Preferences prefs;
    if (!prefs.begin(PRESETS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to open presets namespace for saving!");
        return;
    }
    
    // CRITICAL: Remove old blob first to ensure fresh write
    // This fixes the issue where putBytes returns 0 when overwriting same-size data
    prefs.remove("presets");
    yield();
    
    // Save config version
    prefs.putInt(CONFIG_VERSION_KEY, CURRENT_CONFIG_VERSION);
    
    // Save button count to help with migration
    prefs.putInt("btnCount", systemConfig.buttonCount);
    
    yield();  // Yield before large blob write
    
    // Save presets as blob WITH ERROR CHECKING
    Serial.printf("Saving presets blob (%d bytes)...\n", sizeof(buttonConfigs));
    size_t written = prefs.putBytes("presets", buttonConfigs, sizeof(buttonConfigs));
    if (written != sizeof(buttonConfigs)) {
        Serial.printf("ERROR: Only wrote %d of %d bytes!\n", written, sizeof(buttonConfigs));
    } else {
        Serial.printf("OK: Wrote %d bytes\n", written);
    }
    
    yield();  // Yield between writes
    
    prefs.putBytes("presetNames", presetNames, sizeof(presetNames));
    prefs.putBytes("ledModes", presetLedModes, sizeof(presetLedModes));
    prefs.putBytes("syncSpm", presetSyncSpm, sizeof(presetSyncSpm));  // SPM sync settings
    
    yield();
    
    // Save global special actions (hold/combo)
    prefs.putBytes("specials", globalSpecialActions, sizeof(globalSpecialActions));
    
    // CRITICAL: Close to commit
    prefs.end();
    
    yield();
    delay(100);  // Let NVS commit
    
    // Verify with a fresh read-only session
    Preferences verifyPrefs;
    verifyPrefs.begin(PRESETS_NAMESPACE, true);
    size_t finalCheck = verifyPrefs.getBytesLength("presets");
    verifyPrefs.end();
    
    Serial.printf("Final check: %d bytes in NVS\n", finalCheck);
    if (finalCheck == sizeof(buttonConfigs)) {
        Serial.println("Presets Saved - SUCCESS");
    } else {
        Serial.printf("ERROR: NVS persistence failed! Expected %d, got %d\n", sizeof(buttonConfigs), finalCheck);
    }
}

void loadPresets() {
    systemPrefs.begin(PRESETS_NAMESPACE, true);
    
    int version = systemPrefs.getInt(CONFIG_VERSION_KEY, 1);
    size_t len = systemPrefs.getBytesLength("presets");
    
    Serial.printf("Loading Presets (v%d, blob size=%d, expected=%d)...\n", 
                  version, len, sizeof(buttonConfigs));
    
    if (version >= 2 && len == sizeof(buttonConfigs)) {
        // v2 format matches current struct size
        Serial.println("✓ Size match - loading from NVS...");
        systemPrefs.getBytes("presets", buttonConfigs, len);
        
        size_t lenNames = systemPrefs.getBytesLength("presetNames");
        if (lenNames == sizeof(presetNames)) {
            systemPrefs.getBytes("presetNames", presetNames, lenNames);
        }
        
        size_t lenModes = systemPrefs.getBytesLength("ledModes");
        if (lenModes == sizeof(presetLedModes)) {
            systemPrefs.getBytes("ledModes", presetLedModes, lenModes);
        }
        
        // Load SPM sync settings
        size_t lenSync = systemPrefs.getBytesLength("syncSpm");
        if (lenSync == sizeof(presetSyncSpm)) {
            systemPrefs.getBytes("syncSpm", presetSyncSpm, lenSync);
            Serial.printf("SPM Sync: P1=%d P2=%d P3=%d P4=%d\n", 
                presetSyncSpm[0], presetSyncSpm[1], presetSyncSpm[2], presetSyncSpm[3]);
        }
        
        // Load global special actions
        size_t lenSpecials = systemPrefs.getBytesLength("specials");
        Serial.printf("Loading specials: stored=%d, expected=%d\n", lenSpecials, sizeof(globalSpecialActions));
        if (lenSpecials == sizeof(globalSpecialActions)) {
            systemPrefs.getBytes("specials", globalSpecialActions, lenSpecials);
            Serial.println("✓ Special actions loaded");
            // Debug: print first button's combo status
            Serial.printf("  Button 0 hasCombo: %s\n", globalSpecialActions[0].hasCombo ? "YES" : "NO");
        } else {
            // Size mismatch (struct changed) - initialize hasCombo to false for safety
            Serial.printf("Special actions size mismatch (%d vs %d) - using defaults\n", lenSpecials, sizeof(globalSpecialActions));
            for (int i = 0; i < MAX_BUTTONS; i++) {
                globalSpecialActions[i].hasCombo = false;
            }
        }
        
        systemPrefs.end();
        Serial.println("✓ Presets Loaded (v3 Action-Based Format).");
        return;
    }
    
    // Size mismatch or old version - try migration or defaults
    Serial.printf("⚠ SIZE MISMATCH! Saved=%d, Expected=%d, Diff=%d\n", len, sizeof(buttonConfigs), (int)len - (int)sizeof(buttonConfigs));
    Serial.println("Falling back to factory defaults (THIS OVERWRITES YOUR CHANGES!)");
    systemPrefs.end();
    
    // Try loading v1 legacy format
    Serial.println("Attempting v1 migration or fallback...");
    
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
    }
    legacyPrefs.end();
    
    if (!foundLegacyData) {
        // No legacy data - load factory defaults
        Serial.println("⚠ No compatible presets. Loading Factory Defaults.");
        loadFactoryPresets();
    } else {
        Serial.println("✓ Migrated preset names from v1.");
        // Note: v1 ButtonConfig was different size, can't migrate button data
        // Factory defaults will be used for button configs
        loadFactoryPresets();
    }
    
    // initializeGlobalOverrides(); // Removed - function no longer exists
    
    // Save in v2 format
    savePresets();
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
