/**
 * GP5Protocol.h - Valeton GP-5 SysEx Protocol Implementation
 * 
 * Based on analysis of TonexOneController project and GP5Editor.html
 * The GP-5 uses a proprietary SysEx protocol with:
 * - CRC8 checksum (polynomial 0x07)
 * - Nibble encoding (each byte split into two 4-bit values)
 * - Multi-chunk message support
 * 
 * Copyright (C) 2024 Chocotone Project
 * Licensed under MIT License
 */

#ifndef GP5_PROTOCOL_H
#define GP5_PROTOCOL_H

#include <Arduino.h>

// ============================================
// GP-5 EFFECT BLOCKS
// ============================================

enum GP5EffectBlock : uint8_t {
    GP5_BLOCK_NR = 0,   // Noise Reduction / Gate
    GP5_BLOCK_PRE,      // Pre-effects (Comp, Boost, Wah, Octave, Pitch, Detune)
    GP5_BLOCK_DST,      // Distortion (Green OD, Yellow OD, SM Dist, Fuzz, etc.)
    GP5_BLOCK_AMP,      // Amplifier models (32 models)
    GP5_BLOCK_CAB,      // Cabinet/IR (20 models)
    GP5_BLOCK_EQ,       // Equalizer (5 types)
    GP5_BLOCK_MOD,      // Modulation (Chorus, Flanger, Phaser, Tremolo, etc.)
    GP5_BLOCK_DLY,      // Delay (10 types)
    GP5_BLOCK_RVB,      // Reverb (10 types)
    GP5_BLOCK_NS,       // Snaptone / NAM models (80 slots)
    GP5_BLOCK_COUNT
};

// ============================================
// GP-5 MIDI CC ASSIGNMENTS
// From TonexOneController MidiCommands.md
// ============================================

namespace GP5MidiCC {
    // Effect Module Enable/Disable (CC 0-9)
    constexpr uint8_t NR_ENABLE = 0;
    constexpr uint8_t PRE_ENABLE = 1;
    constexpr uint8_t DST_ENABLE = 2;
    constexpr uint8_t AMP_ENABLE = 3;
    constexpr uint8_t CAB_ENABLE = 4;
    constexpr uint8_t EQ_ENABLE = 5;
    constexpr uint8_t MOD_ENABLE = 6;
    constexpr uint8_t DLY_ENABLE = 7;
    constexpr uint8_t RVB_ENABLE = 8;
    constexpr uint8_t NS_ENABLE = 9;
    
    // Block Type Selection (CC 10-19)
    constexpr uint8_t NR_TYPE = 10;
    constexpr uint8_t PRE_TYPE = 11;
    constexpr uint8_t DST_TYPE = 12;
    constexpr uint8_t AMP_TYPE = 13;
    constexpr uint8_t CAB_TYPE = 14;
    constexpr uint8_t EQ_TYPE = 15;
    constexpr uint8_t MOD_TYPE = 16;
    constexpr uint8_t DLY_TYPE = 17;
    constexpr uint8_t RVB_TYPE = 18;
    constexpr uint8_t NS_TYPE = 19;
    
    // Patch and Global Controls
    constexpr uint8_t PATCH_VOLUME = 20;
    constexpr uint8_t PRESET_DN = 116;
    constexpr uint8_t PRESET_UP = 117;
    constexpr uint8_t BPM = 118;
    constexpr uint8_t TAP_TEMPO = 119;  // Not supported per docs
    constexpr uint8_t INPUT_LEVEL = 120;
    constexpr uint8_t CAB_BYPASS = 121;
    constexpr uint8_t MASTER_VOLUME = 122;
    constexpr uint8_t RECORD_LEVEL = 123;
    constexpr uint8_t MONITOR_LEVEL = 124;
    constexpr uint8_t BT_LEVEL = 125;
    constexpr uint8_t PRESET_SELECT = 127;  // 0-99
    
    // Block Parameters (8 params per block, CC 21-101)
    // NR Params: 21-28 (with gap at 22)
    // PRE Params: 30-37
    // DST Params: 38-45
    // AMP Params: 46-53
    // CAB Params: 54-61
    // EQ Params: 62-69
    // MOD Params: 70-77
    // DLY Params: 78-85
    // RVB Params: 86-93
    // NS Params: 94-101
}

// ============================================
// GP-5 SYSEX MESSAGE TYPES
// ============================================

namespace GP5SysEx {
    // Message type bytes (after length field)
    constexpr uint8_t MSG_TYPE_COMMAND = 0x01;   // Commands sent TO GP-5
    constexpr uint8_t MSG_TYPE_RESPONSE = 0x02;  // Responses FROM GP-5
    
    // Function codes (in response messages)
    constexpr uint8_t FUNC_CONFIRM = 0x08;           // Parameter change confirmation
    constexpr uint8_t FUNC_GLOBALS = 0x10;           // Global settings
    constexpr uint8_t FUNC_PRESET_CHANGED = 0x1B;    // Preset changed notification
    constexpr uint8_t FUNC_IRS = 0x20;               // IR data
    constexpr uint8_t FUNC_SNAPTONES = 0x24;         // Snaptone/NAM data
    constexpr uint8_t FUNC_PRESET_NAMES = 0x40;      // Preset name list
    constexpr uint8_t FUNC_PRESET_PARAMS = 0x41;     // Current preset parameters
    
    // Maximum values
    constexpr uint8_t MAX_PRESETS = 100;
    constexpr size_t MAX_SYSEX_SIZE = 512;
    constexpr uint8_t INTER_MESSAGE_DELAY_MS = 20;
}

// ============================================
// PROTOCOL UTILITY FUNCTIONS
// ============================================

/**
 * Calculate CRC8 checksum for GP-5 SysEx message
 * Uses polynomial 0x07 with initial value 0x00
 * 
 * @param sysex_data Complete SysEx message starting with 0xF0
 * @param length Total length of message
 * @return CRC8 checksum value
 */
uint8_t gp5_crc8(const uint8_t* sysex_data, size_t length);

/**
 * Encode a byte value as two nibbles
 * Example: 0x63 becomes highNibble=0x06, lowNibble=0x03
 * 
 * @param value Byte to encode
 * @param highNibble Output: upper 4 bits
 * @param lowNibble Output: lower 4 bits
 */
void gp5_encode_nibbles(uint8_t value, uint8_t* highNibble, uint8_t* lowNibble);

/**
 * Decode two nibbles back to a single byte
 * Example: highNibble=0x06, lowNibble=0x03 becomes 0x63
 * 
 * @param highNibble Upper 4 bits (in low nibble of this byte)
 * @param lowNibble Lower 4 bits (in low nibble of this byte)
 * @return Decoded byte value
 */
uint8_t gp5_decode_nibbles(uint8_t highNibble, uint8_t lowNibble);

/**
 * Convert signed 16-bit value from nibble pair
 * Handles negative values (>= 0x80)
 * 
 * @param byte1 High nibble byte
 * @param byte2 Low nibble byte
 * @return Signed 16-bit value
 */
int16_t gp5_decode_signed_nibbles(uint8_t byte1, uint8_t byte2);

/**
 * Build a complete GP-5 SysEx message with proper structure
 * Adds header, CRC, chunk info, and end marker
 * 
 * @param output Buffer to write complete message (must be large enough)
 * @param payload Raw payload bytes (NOT nibble-encoded)
 * @param payloadLen Length of payload
 * @param msgType GP5SysEx::MSG_TYPE_COMMAND or MSG_TYPE_RESPONSE
 * @return Total length of output message
 */
size_t gp5_build_sysex(uint8_t* output, const uint8_t* payload, size_t payloadLen, uint8_t msgType);

/**
 * Parse a received GP-5 SysEx message
 * Extracts payload after removing headers and decoding nibbles
 * 
 * @param input Raw received SysEx data (with BLE MIDI header stripped)
 * @param inputLen Length of input
 * @param payload Buffer for extracted payload
 * @param payloadLen Output: length of extracted payload
 * @return true if parsing succeeded, false on error
 */
bool gp5_parse_sysex(const uint8_t* input, size_t inputLen, uint8_t* payload, size_t* payloadLen);

// ============================================
// HIGH-LEVEL PROTOCOL FUNCTIONS
// ============================================

/**
 * Request current preset state from GP-5
 * Triggers a FUNC_PRESET_PARAMS response
 */
void gp5_request_current_preset();

/**
 * Request list of all preset names
 * Triggers a FUNC_PRESET_NAMES response
 */
void gp5_request_preset_list();

/**
 * Request global settings from GP-5
 * Triggers a FUNC_GLOBALS response
 */
void gp5_request_globals();

/**
 * Set effect block enable state
 * Uses standard MIDI CC (more reliable than SysEx)
 * 
 * @param block Effect block to control
 * @param enabled true=ON, false=OFF
 */
void gp5_set_effect_state(GP5EffectBlock block, bool enabled);

/**
 * Select a preset by number
 * 
 * @param presetNum Preset number (0-99)
 */
void gp5_select_preset(uint8_t presetNum);

/**
 * Request current preset number from GP-5
 * Triggers a response with the active preset index
 */
void gp5_request_current_preset_number();

/**
 * Parse effect states from preset data response
 * 
 * @param data Combined preset data from multi-chunk response
 * @param len Length of data
 * @param effectStates Output array of 10 effect states (GP5_BLOCK_COUNT)
 * @return true if parsing succeeded
 */
bool gp5_parse_preset_effect_states(const uint8_t* data, size_t len, bool effectStates[GP5_BLOCK_COUNT]);

#endif // GP5_PROTOCOL_H
