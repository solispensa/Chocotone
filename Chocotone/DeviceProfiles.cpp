/**
 * DeviceProfiles.cpp - Multi-Device Support Abstraction Layer Implementation
 * 
 * Implements device profiles for SPM, GP-5, and generic MIDI devices.
 * Each profile defines CC mappings and device-specific protocol handling.
 * 
 * Copyright (C) 2024 Chocotone Project
 * Licensed under MIT License
 */

#include "DeviceProfiles.h"
#include "BleMidi.h"
#include "GP5Protocol.h"
#include "Globals.h"

// ============================================
// EFFECT BLOCK NAMES
// ============================================

static const char* EFFECT_NAMES[EFFECT_BLOCK_COUNT] = {
    "NR",   // EFFECT_NR
    "FX1",  // EFFECT_FX1 (PRE on GP5)
    "DRV",  // EFFECT_DRV (DST on GP5)
    "AMP",  // EFFECT_AMP
    "IR",   // EFFECT_IR (CAB on GP5)
    "EQ",   // EFFECT_EQ
    "FX2",  // EFFECT_FX2 (MOD on GP5)
    "DLY",  // EFFECT_DLY
    "RVB",  // EFFECT_RVB
    "NS"    // EFFECT_NS (Snaptone, GP5 only)
};

// ============================================
// SPM PROFILE IMPLEMENTATION
// ============================================

// SPM CC mappings for effect blocks
static const uint8_t SPM_EFFECT_CCS[EFFECT_BLOCK_COUNT] = {
    43,     // EFFECT_NR
    44,     // EFFECT_FX1
    45,     // EFFECT_DRV
    46,     // EFFECT_AMP
    47,     // EFFECT_IR
    48,     // EFFECT_EQ
    49,     // EFFECT_FX2
    50,     // EFFECT_DLY
    51,     // EFFECT_RVB
    0xFF    // EFFECT_NS (not available on SPM)
};

// SPM protocol handlers
static void spm_request_state() {
    // Existing implementation in BleMidi.cpp
    requestPresetState();
}

static void spm_parse_response(const uint8_t* data, size_t len) {
    // Parsing handled in BleMidi.cpp::processBufferedSysex()
    // This is called after basic validation
}

static void spm_toggle_effect(EffectBlock block, bool state) {
    if (block >= EFFECT_BLOCK_COUNT) return;
    uint8_t cc = SPM_EFFECT_CCS[block];
    if (cc != 0xFF) {
        sendMidiCC(1, cc, state ? 127 : 0);
        Serial.printf("SPM: Toggle %s = %s (CC %d)\n", 
            EFFECT_NAMES[block], state ? "ON" : "OFF", cc);
    }
}

static void spm_tap_tempo() {
    // SPM uses delay time SysEx, not standard tap tempo
    // This is handled by the tap tempo logic in Input.cpp
}

static void spm_preset_change(uint8_t presetNum) {
    // SPM uses CC 1 for preset selection (1-100)
    sendMidiCC(1, 1, presetNum + 1);  // +1 because SPM is 1-indexed
    Serial.printf("SPM: Preset change to %d\n", presetNum + 1);
}

const DeviceProfile PROFILE_SPM = {
    .name = "Sonicake Pocket Master",
    .shortName = "SPM",
    .type = DEVICE_SPM,
    .effectBlockCount = 9,
    .maxPresets = 100,
    .supportsStateSync = true,
    .supportsTapTempo = true,
    .effectToggleCCs = SPM_EFFECT_CCS,
    .presetSelectCC = 1,
    .requestPresetState = spm_request_state,
    .parsePresetResponse = spm_parse_response,
    .onEffectToggle = spm_toggle_effect,
    .sendTapTempo = spm_tap_tempo,
    .sendPresetChange = spm_preset_change
};

// ============================================
// GP-5 PROFILE IMPLEMENTATION
// ============================================

// GP-5 CC mappings for effect blocks (CC 0-9)
static const uint8_t GP5_EFFECT_CCS[EFFECT_BLOCK_COUNT] = {
    0,      // EFFECT_NR (GP5MidiCC::NR_ENABLE)
    1,      // EFFECT_FX1 -> PRE (GP5MidiCC::PRE_ENABLE)
    2,      // EFFECT_DRV -> DST (GP5MidiCC::DST_ENABLE)
    3,      // EFFECT_AMP (GP5MidiCC::AMP_ENABLE)
    4,      // EFFECT_IR -> CAB (GP5MidiCC::CAB_ENABLE)
    5,      // EFFECT_EQ (GP5MidiCC::EQ_ENABLE)
    6,      // EFFECT_FX2 -> MOD (GP5MidiCC::MOD_ENABLE)
    7,      // EFFECT_DLY (GP5MidiCC::DLY_ENABLE)
    8,      // EFFECT_RVB (GP5MidiCC::RVB_ENABLE)
    9       // EFFECT_NS (GP5MidiCC::NS_ENABLE)
};

// GP-5 protocol handlers
static void gp5_request_state_wrapper() {
    gp5_request_current_preset();
}

static void gp5_parse_response(const uint8_t* data, size_t len) {
    // Extract effect states from GP-5 preset params response
    // This is called from BleMidi.cpp after identifying message type
    
    if (len < 150) {
        Serial.println("GP5: Preset response too short");
        return;
    }
    
    // Effect states are stored as bit flags
    // Offset needs verification with actual hardware
    // Based on TonexOneController analysis:
    // Skip 96 bytes to patch volume, then +40 to effect states
    size_t stateOffset = 68;  // Approximate offset in decoded payload
    
    uint8_t states1 = data[stateOffset];
    uint8_t states2 = data[stateOffset + 1];
    
    // Decode to global effect states array
    extern bool effectStates[EFFECT_BLOCK_COUNT];
    effectStates[EFFECT_NR]  = (states1 & 0x01) != 0;
    effectStates[EFFECT_FX1] = (states1 & 0x02) != 0;
    effectStates[EFFECT_DRV] = (states1 & 0x04) != 0;
    effectStates[EFFECT_AMP] = (states1 & 0x08) != 0;
    effectStates[EFFECT_IR]  = (states1 & 0x10) != 0;
    effectStates[EFFECT_EQ]  = (states1 & 0x20) != 0;
    effectStates[EFFECT_FX2] = (states1 & 0x40) != 0;
    effectStates[EFFECT_DLY] = (states1 & 0x80) != 0;
    effectStates[EFFECT_RVB] = (states2 & 0x01) != 0;
    effectStates[EFFECT_NS]  = (states2 & 0x02) != 0;
    
    Serial.println("GP5: Effect states decoded");
    Serial.printf("  NR=%d PRE=%d DST=%d AMP=%d CAB=%d EQ=%d MOD=%d DLY=%d RVB=%d NS=%d\n",
        effectStates[0], effectStates[1], effectStates[2], effectStates[3],
        effectStates[4], effectStates[5], effectStates[6], effectStates[7],
        effectStates[8], effectStates[9]);
}

static void gp5_toggle_effect(EffectBlock block, bool state) {
    if (block >= EFFECT_BLOCK_COUNT) return;
    uint8_t cc = GP5_EFFECT_CCS[block];
    if (cc != 0xFF) {
        // Use configured MIDI channel for GP-5
        sendMidiCC(systemConfig.midiChannel, cc, state ? 127 : 0);
        Serial.printf("GP5: Toggle %s = %s (CC %d)\n",
            EFFECT_NAMES[block], state ? "ON" : "OFF", cc);
    }
}

static void gp5_tap_tempo() {
    // GP-5 tap tempo via CC 119 (though docs say not supported)
    // BPM can be set via CC 118 (0-127)
    sendMidiCC(systemConfig.midiChannel, 119, 127);
    Serial.println("GP5: Tap tempo sent");
}

static void gp5_preset_change(uint8_t presetNum) {
    if (presetNum >= 100) presetNum = 99;
    sendMidiCC(systemConfig.midiChannel, 127, presetNum);
    Serial.printf("GP5: Preset change to %d\n", presetNum);
}

const DeviceProfile PROFILE_GP5 = {
    .name = "Valeton GP-5",
    .shortName = "GP5",
    .type = DEVICE_GP5,
    .effectBlockCount = 10,
    .maxPresets = 100,
    .supportsStateSync = true,
    .supportsTapTempo = true,  // Via BPM CC
    .effectToggleCCs = GP5_EFFECT_CCS,
    .presetSelectCC = 127,
    .requestPresetState = gp5_request_state_wrapper,
    .parsePresetResponse = gp5_parse_response,
    .onEffectToggle = gp5_toggle_effect,
    .sendTapTempo = gp5_tap_tempo,
    .sendPresetChange = gp5_preset_change
};

// ============================================
// GENERIC MIDI PROFILE
// ============================================

// Generic profile uses no effect CCs by default (user configures in editor)
static const uint8_t GENERIC_EFFECT_CCS[EFFECT_BLOCK_COUNT] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static void generic_noop() {
    // No-op for generic profile
}

static void generic_noop_parse(const uint8_t* data, size_t len) {
    // No-op
}

static void generic_toggle(EffectBlock block, bool state) {
    // Generic toggle uses whatever CC the user configured in the button action
    // This stub is not typically called - button actions are direct
}

static void generic_preset_change(uint8_t presetNum) {
    // Use Program Change for generic devices
    sendMidiPC(1, presetNum);
}

const DeviceProfile PROFILE_GENERIC = {
    .name = "Generic MIDI",
    .shortName = "MIDI",
    .type = DEVICE_GENERIC,
    .effectBlockCount = 0,  // No predefined blocks
    .maxPresets = 128,
    .supportsStateSync = false,
    .supportsTapTempo = false,
    .effectToggleCCs = GENERIC_EFFECT_CCS,
    .presetSelectCC = 0xFF,  // Uses Program Change
    .requestPresetState = generic_noop,
    .parsePresetResponse = generic_noop_parse,
    .onEffectToggle = generic_toggle,
    .sendTapTempo = generic_noop,
    .sendPresetChange = generic_preset_change
};

// ============================================
// PROFILE MANAGEMENT
// ============================================

static const DeviceProfile* activeProfile = &PROFILE_SPM;

const DeviceProfile* getCurrentProfile() {
    return activeProfile;
}

void setDeviceProfile(DeviceType type) {
    switch (type) {
        case DEVICE_GP5:
            activeProfile = &PROFILE_GP5;
            Serial.println("Device profile set to: Valeton GP-5");
            break;
        case DEVICE_GENERIC:
            activeProfile = &PROFILE_GENERIC;
            Serial.println("Device profile set to: Generic MIDI");
            break;
        case DEVICE_SPM:
        default:
            activeProfile = &PROFILE_SPM;
            Serial.println("Device profile set to: Sonicake Pocket Master");
            break;
    }
}

const DeviceProfile* getProfileByType(DeviceType type) {
    switch (type) {
        case DEVICE_GP5:
            return &PROFILE_GP5;
        case DEVICE_GENERIC:
            return &PROFILE_GENERIC;
        case DEVICE_SPM:
        default:
            return &PROFILE_SPM;
    }
}

// ============================================
// CONVENIENCE FUNCTIONS
// ============================================

uint8_t getEffectCC(EffectBlock block) {
    if (block >= EFFECT_BLOCK_COUNT) return 0xFF;
    return activeProfile->effectToggleCCs[block];
}

const char* getEffectName(EffectBlock block) {
    if (block >= EFFECT_BLOCK_COUNT) return "???";
    return EFFECT_NAMES[block];
}

EffectBlock ccToEffectBlock(uint8_t cc) {
    for (uint8_t i = 0; i < EFFECT_BLOCK_COUNT; i++) {
        if (activeProfile->effectToggleCCs[i] == cc) {
            return static_cast<EffectBlock>(i);
        }
    }
    return EFFECT_BLOCK_COUNT;  // Not found
}

bool isEffectAvailable(EffectBlock block) {
    if (block >= EFFECT_BLOCK_COUNT) return false;
    return activeProfile->effectToggleCCs[block] != 0xFF;
}
