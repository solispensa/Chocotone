#include "Storage.h"
#include "DefaultPresets.h"

#define PRESETS_NAMESPACE "midi_presets"
#define CONFIG_VERSION_KEY "cfg_ver"
#define CURRENT_CONFIG_VERSION 2

// ============================================
// SYSTEM SETTINGS (v2)
// ============================================

void saveSystemSettings() {
    Serial.println("=== Saving System Settings (v2) ===");
    
    Preferences sysPrefs;
    if (!sysPrefs.begin("sys_cfg", false)) {
        Serial.println("ERROR: Cannot open sys_cfg namespace!");
        return;
    }
    
    // Write version marker
    sysPrefs.putInt(CONFIG_VERSION_KEY, CURRENT_CONFIG_VERSION);
    
    // UI Settings
    sysPrefs.putInt("font", buttonNameFontSize);
    sysPrefs.putInt("ledOn", ledBrightnessOn);
    sysPrefs.putInt("ledDim", ledBrightnessDim);
    sysPrefs.putInt("debounce", buttonDebounce);
    sysPrefs.putInt("rhythm", rhythmPattern);
    sysPrefs.putInt("delay", currentDelayType);
    sysPrefs.putInt("preset", currentPreset);
    
    // SystemConfig struct fields
    sysPrefs.putString("bleName", systemConfig.bleDeviceName);
    sysPrefs.putString("apSSID", systemConfig.apSSID);
    sysPrefs.putString("apPass", systemConfig.apPassword);
    sysPrefs.putInt("btnCount", systemConfig.buttonCount);
    sysPrefs.putBytes("btnPins", systemConfig.buttonPins, sizeof(systemConfig.buttonPins));
    sysPrefs.putInt("ledPin", systemConfig.ledPin);
    sysPrefs.putInt("encA", systemConfig.encoderA);
    sysPrefs.putInt("encB", systemConfig.encoderB);
    sysPrefs.putInt("encBtn", systemConfig.encoderBtn);
    sysPrefs.putBool("wifiBoot", systemConfig.wifiOnAtBoot);
    sysPrefs.putUChar("bleMode", (uint8_t)systemConfig.bleMode);
    sysPrefs.putUChar("ledsPerBtn", systemConfig.ledsPerButton);
    
    sysPrefs.end();
    Serial.println("=== System Settings Saved (v2) ===");
}

void loadSystemSettings() {
    Preferences sysPrefs;
    sysPrefs.begin("sys_cfg", true);
    
    int version = sysPrefs.getInt(CONFIG_VERSION_KEY, 1);
    Serial.printf("Loading System Settings (v%d)...\n", version);
    
    // UI Settings (same in v1 and v2)
    buttonNameFontSize = sysPrefs.getInt("font", 5);
    ledBrightnessOn = sysPrefs.getInt("ledOn", 220);
    ledBrightnessDim = sysPrefs.getInt("ledDim", 120);
    buttonDebounce = sysPrefs.getInt("debounce", 120);
    rhythmPattern = sysPrefs.getInt("rhythm", 0);
    if (rhythmPattern < 0 || rhythmPattern > 3) rhythmPattern = 0;
    currentDelayType = sysPrefs.getInt("delay", 0);
    currentPreset = sysPrefs.getInt("preset", 0);
    if (currentPreset < 0 || currentPreset > 3) currentPreset = 0;
    
    if (version >= 2) {
        // v2 SystemConfig fields
        String s_bleName = sysPrefs.getString("bleName", DEFAULT_BLE_NAME);
        strncpy(systemConfig.bleDeviceName, s_bleName.c_str(), 23);
        systemConfig.bleDeviceName[23] = '\0';
        
        String s_ssid = sysPrefs.getString("apSSID", DEFAULT_AP_SSID);
        strncpy(systemConfig.apSSID, s_ssid.c_str(), 23);
        systemConfig.apSSID[23] = '\0';
        
        String s_pass = sysPrefs.getString("apPass", DEFAULT_AP_PASS);
        strncpy(systemConfig.apPassword, s_pass.c_str(), 15);
        systemConfig.apPassword[15] = '\0';
        
        systemConfig.buttonCount = sysPrefs.getInt("btnCount", DEFAULT_BUTTON_COUNT);
        if (systemConfig.buttonCount < 4) systemConfig.buttonCount = 4;
        if (systemConfig.buttonCount > 10) systemConfig.buttonCount = 10;
        
        sysPrefs.getBytes("btnPins", systemConfig.buttonPins, sizeof(systemConfig.buttonPins));
        systemConfig.ledPin = sysPrefs.getInt("ledPin", NEOPIXEL_PIN);
        systemConfig.encoderA = sysPrefs.getInt("encA", DEFAULT_ENCODER_A);
        systemConfig.encoderB = sysPrefs.getInt("encB", DEFAULT_ENCODER_B);
        systemConfig.encoderBtn = sysPrefs.getInt("encBtn", DEFAULT_ENCODER_BTN);
        systemConfig.wifiOnAtBoot = sysPrefs.getBool("wifiBoot", false);
        systemConfig.bleMode = (BleMode)sysPrefs.getUChar("bleMode", BLE_CLIENT_ONLY);
        systemConfig.ledsPerButton = sysPrefs.getUChar("ledsPerBtn", 1);  // Default 1 LED per button
    } else {
        // v1 MIGRATION: Read old individual fields into SystemConfig
        Serial.println("Migrating v1 settings to v2...");
        
        String s_bleName = sysPrefs.getString("bleName", DEFAULT_BLE_NAME);
        strncpy(systemConfig.bleDeviceName, s_bleName.c_str(), 23);
        systemConfig.bleDeviceName[23] = '\0';
        
        String s_ssid = sysPrefs.getString("apSSID", DEFAULT_AP_SSID);
        strncpy(systemConfig.apSSID, s_ssid.c_str(), 23);
        systemConfig.apSSID[23] = '\0';
        
        String s_pass = sysPrefs.getString("apPass", DEFAULT_AP_PASS);
        strncpy(systemConfig.apPassword, s_pass.c_str(), 15);
        systemConfig.apPassword[15] = '\0';
        
        // v1 didn't have these - use defaults
        systemConfig.buttonCount = DEFAULT_BUTTON_COUNT;
        memcpy(systemConfig.buttonPins, DEFAULT_BUTTON_PINS, sizeof(systemConfig.buttonPins));
        systemConfig.ledPin = NEOPIXEL_PIN;
        systemConfig.encoderA = DEFAULT_ENCODER_A;
        systemConfig.encoderB = DEFAULT_ENCODER_B;
        systemConfig.encoderBtn = DEFAULT_ENCODER_BTN;
        systemConfig.wifiOnAtBoot = sysPrefs.getBool("wifi", false);
        systemConfig.bleMode = BLE_CLIENT_ONLY;  // Default for v1 migration
        systemConfig.ledsPerButton = 1;  // Default for v1 migration
    }
    
    sysPrefs.end();
    Serial.println("=== System Settings Loaded ===");
}

// ============================================
// PRESETS (v2 with expanded ButtonConfig)
// ============================================

void savePresets() {
    Serial.println("Saving Presets (v2)...");
    
    yield();  // Let WDT breathe before starting
    systemPrefs.begin(PRESETS_NAMESPACE, false);
    
    // Save config version
    systemPrefs.putInt(CONFIG_VERSION_KEY, CURRENT_CONFIG_VERSION);
    
    // Save button count to help with migration
    systemPrefs.putInt("btnCount", systemConfig.buttonCount);
    
    yield();  // Yield before large blob write
    
    // Save presets as blob WITH ERROR CHECKING
    Serial.printf("Saving presets blob (%d bytes)...\n", sizeof(buttonConfigs));
    size_t written = systemPrefs.putBytes("presets", buttonConfigs, sizeof(buttonConfigs));
    if (written != sizeof(buttonConfigs)) {
        Serial.printf("ERROR: Only wrote %d of %d bytes!\n", written, sizeof(buttonConfigs));
    }
    
    yield();  // Yield between writes
    
    systemPrefs.putBytes("presetNames", presetNames, sizeof(presetNames));
    
    yield();
    
    // Save global special actions (hold/combo)
    systemPrefs.putBytes("specials", globalSpecialActions, sizeof(globalSpecialActions));
    
    yield();
    
    // CRITICAL: Verify data before closing
    size_t verify = systemPrefs.getBytesLength("presets");
    Serial.printf("Verification: NVS reports %d bytes stored\n", verify);
    
    systemPrefs.end();
    
    // Re-open to force commit and verify persistence
    delay(100);
    systemPrefs.begin(PRESETS_NAMESPACE, true);  // Read-only
    size_t finalCheck = systemPrefs.getBytesLength("presets");
    systemPrefs.end();
    
    Serial.printf("Final check: %d bytes in NVS\n", finalCheck);
    if (finalCheck == sizeof(buttonConfigs)) {
        Serial.println("Presets Saved (v2) - SUCCESS");
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
        

        
        // Load global special actions
        size_t lenSpecials = systemPrefs.getBytesLength("specials");
        Serial.printf("Loading specials: stored=%d, expected=%d\n", lenSpecials, sizeof(globalSpecialActions));
        if (lenSpecials == sizeof(globalSpecialActions)) {
            systemPrefs.getBytes("specials", globalSpecialActions, lenSpecials);
            Serial.println("✓ Special actions loaded");
            // Debug: print first button's hold enabled status
            Serial.printf("  Button 0 hold enabled: %s\n", globalSpecialActions[0].hold.enabled ? "YES" : "NO");
        } else {
            // Size mismatch (struct changed) - initialize combo labels to empty for safety
            Serial.printf("Special actions size mismatch (%d vs %d) - using defaults\n", lenSpecials, sizeof(globalSpecialActions));
            for (int i = 0; i < MAX_BUTTONS; i++) {
                globalSpecialActions[i].combo.label[0] = '\0';
            }
        }
        
        systemPrefs.end();
        Serial.println("✓ Presets Loaded (v2 Blob Format).");
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
