/**
 * DeviceProfiles.h - Multi-Device Support Abstraction Layer
 * 
 * Provides a unified interface for controlling different guitar processors:
 * - Sonicake Pocket Master (SPM)
 * - Valeton GP-5
 * - Generic MIDI devices
 * 
 * Each device has different CC mappings and sync protocols, but this
 * abstraction layer allows the button handling code to remain device-agnostic.
 * 
 * Copyright (C) 2024 Chocotone Project
 * Licensed under MIT License
 */

#ifndef DEVICE_PROFILES_H
#define DEVICE_PROFILES_H

#include <Arduino.h>

// ============================================
// DEVICE TYPE ENUMERATION
// ============================================

enum DeviceType : uint8_t {
    DEVICE_SPM = 0,       // Sonicake Pocket Master
    DEVICE_GP5 = 1,       // Valeton GP-5
    DEVICE_GENERIC = 255  // Generic MIDI (no sync, user-defined CCs)
};

// ============================================
// ABSTRACT EFFECT BLOCK ENUMERATION
// ============================================
// Unified effect block IDs that map to device-specific blocks

enum EffectBlock : uint8_t {
    EFFECT_NR = 0,    // Noise Reduction (SPM: NR, GP5: NR)
    EFFECT_FX1,       // Pre-effect 1 (SPM: FX1, GP5: PRE)
    EFFECT_DRV,       // Drive/Distortion (SPM: DRV, GP5: DST)
    EFFECT_AMP,       // Amplifier
    EFFECT_IR,        // Cabinet/IR (SPM: IR, GP5: CAB)
    EFFECT_EQ,        // EQ
    EFFECT_FX2,       // Effect 2 (SPM: FX2, GP5: MOD)
    EFFECT_DLY,       // Delay
    EFFECT_RVB,       // Reverb
    EFFECT_NS,        // Special (GP5 only: Snaptone)
    EFFECT_BLOCK_COUNT
};

// ============================================
// DEVICE PROFILE STRUCTURE
// ============================================

// Forward declare the profile callbacks
typedef void (*RequestStateFunc)();
typedef void (*ParseResponseFunc)(const uint8_t* data, size_t len);
typedef void (*EffectToggleFunc)(EffectBlock block, bool state);
typedef void (*TapTempoFunc)();
typedef void (*PresetChangeFunc)(uint8_t presetNum);

struct DeviceProfile {
    // Device identification
    const char* name;
    const char* shortName;     // 3-4 char abbreviation for display
    DeviceType type;
    
    // Capabilities
    uint8_t effectBlockCount;  // Number of effect blocks (SPM: 9, GP5: 10)
    uint8_t maxPresets;        // Number of presets (SPM: 100, GP5: 100)
    bool supportsStateSync;    // Can sync effect states from device
    bool supportsTapTempo;     // Has tap tempo functionality
    
    // CC mappings for effect toggles (index = EffectBlock enum)
    // Value of 0xFF means block not available on this device
    const uint8_t* effectToggleCCs;
    
    // CC for preset selection (or 0xFF if not supported)
    uint8_t presetSelectCC;
    
    // Function pointers for device-specific behavior
    RequestStateFunc requestPresetState;
    ParseResponseFunc parsePresetResponse;
    EffectToggleFunc onEffectToggle;
    TapTempoFunc sendTapTempo;
    PresetChangeFunc sendPresetChange;
};

// ============================================
// DEVICE PROFILE INSTANCES
// ============================================

extern const DeviceProfile PROFILE_SPM;
extern const DeviceProfile PROFILE_GP5;
extern const DeviceProfile PROFILE_GENERIC;

// ============================================
// PROFILE MANAGEMENT FUNCTIONS
// ============================================

/**
 * Get the currently active device profile
 */
const DeviceProfile* getCurrentProfile();

/**
 * Set the active device profile by type
 */
void setDeviceProfile(DeviceType type);

/**
 * Get profile by type without setting as active
 */
const DeviceProfile* getProfileByType(DeviceType type);

// ============================================
// CONVENIENCE FUNCTIONS
// ============================================

/**
 * Get the CC number for toggling an effect block on current profile
 * @param block Effect block to look up
 * @return CC number, or 0xFF if not available
 */
uint8_t getEffectCC(EffectBlock block);

/**
 * Get the effect block short name for display
 * @param block Effect block
 * @return Short name (e.g., "NR", "DRV", "AMP")
 */
const char* getEffectName(EffectBlock block);

/**
 * Map a CC number to an effect block for the current profile
 * @param cc MIDI CC number
 * @return Effect block, or EFFECT_BLOCK_COUNT if not found
 */
EffectBlock ccToEffectBlock(uint8_t cc);

/**
 * Check if an effect block is available on the current profile
 */
bool isEffectAvailable(EffectBlock block);

#endif // DEVICE_PROFILES_H
