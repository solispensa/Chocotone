/**
 * GP5Protocol.cpp - Valeton GP-5 SysEx Protocol Implementation
 * 
 * Implementation of CRC8 calculation, nibble encoding/decoding,
 * and SysEx message construction/parsing for GP-5 communication.
 * 
 * Based on analysis of:
 * - TonexOneController (https://github.com/Builty/TonexOneController)
 * - GP5Editor.html / GP5EditorBT.html reference implementations
 * 
 * Copyright (C) 2024 Chocotone Project
 * Licensed under MIT License
 */

#include "GP5Protocol.h"
#include "BleMidi.h"
#include "Globals.h"

// ============================================
// CRC8 CALCULATION
// ============================================

/**
 * Calculate CRC8 for GP-5 SysEx message
 * 
 * Algorithm from TonexOneController usb_valeton_gp5.c:
 * 1. Skip 0xF0 start and 2-byte CRC placeholder
 * 2. Pack nibble pairs into raw bytes
 * 3. Calculate CRC-8 with polynomial 0x07
 */
uint8_t gp5_crc8(const uint8_t* sysex_data, size_t length) {
    uint8_t raw_data[128];
    size_t raw_count = 0;
    uint8_t crc = 0;
    
    // Input validation
    if (length < 5 || sysex_data[0] != 0xF0) {
        return 0;
    }
    
    // Find data section (skip F0 and 2-byte CRC)
    const uint8_t* ptr = sysex_data + 3;
    size_t data_len = 0;
    
    // Count bytes until end marker 0xF7
    while (data_len < length - 3 && ptr[data_len] != 0xF7) {
        data_len++;
    }
    
    // Pack nibble pairs into bytes
    size_t nibble_pairs = data_len / 2;
    for (size_t i = 0; i < nibble_pairs && raw_count < sizeof(raw_data); i++) {
        uint8_t high = ptr[i * 2] & 0x0F;
        uint8_t low = ptr[i * 2 + 1] & 0x0F;
        raw_data[raw_count++] = (high << 4) | low;
    }
    
    // Calculate CRC-8 (polynomial 0x07, initial value 0x00)
    for (size_t i = 0; i < raw_count; i++) {
        crc ^= raw_data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = ((crc << 1) & 0xFF) ^ 0x07;
            } else {
                crc = (crc << 1) & 0xFF;
            }
        }
    }
    
    return crc;
}

// ============================================
// NIBBLE ENCODING/DECODING
// ============================================

void gp5_encode_nibbles(uint8_t value, uint8_t* highNibble, uint8_t* lowNibble) {
    *highNibble = (value >> 4) & 0x0F;
    *lowNibble = value & 0x0F;
}

uint8_t gp5_decode_nibbles(uint8_t highNibble, uint8_t lowNibble) {
    return ((highNibble & 0x0F) << 4) | (lowNibble & 0x0F);
}

int16_t gp5_decode_signed_nibbles(uint8_t byte1, uint8_t byte2) {
    int16_t value = (byte1 * 16) + byte2;
    
    // Handle signed values (if high bit set, it's negative)
    if (value & 0x80) {
        value = value - 0x100;
    }
    
    return value;
}

// ============================================
// SYSEX MESSAGE BUILDING
// ============================================

/**
 * Build a complete GP-5 SysEx command message
 * 
 * Message structure:
 * [F0] [CRC_HI] [CRC_LO] [CHUNK_TOT_HI] [CHUNK_TOT_LO] [CHUNK_IDX_HI] [CHUNK_IDX_LO]
 * [LEN_HI] [LEN_LO] [MSG_TYPE_01] [MSG_TYPE_02] [PAYLOAD...] [F7]
 * 
 * CRC is calculated over everything after CRC bytes, before recombining nibbles
 */
size_t gp5_build_sysex(uint8_t* output, const uint8_t* payload, size_t payloadLen, uint8_t msgType) {
    size_t idx = 0;
    
    // Start marker
    output[idx++] = 0xF0;
    
    // CRC placeholder (will fill after building rest of message)
    output[idx++] = 0x00;  // CRC high nibble
    output[idx++] = 0x00;  // CRC low nibble
    
    // Total chunks: 00 01 (single chunk for now)
    output[idx++] = 0x00;
    output[idx++] = 0x01;
    
    // Chunk index: 00 00 (first and only chunk)
    output[idx++] = 0x00;
    output[idx++] = 0x00;
    
    // Length field (nibble-encoded)
    // Length = (payload nibbles + 2 for message type) / 2 = payloadLen + 1
    size_t dataLen = payloadLen + 1;  // +1 for the message type byte when decoded
    output[idx++] = (dataLen >> 4) & 0x0F;
    output[idx++] = dataLen & 0x0F;
    
    // Message type: 01 01 for commands to GP-5
    output[idx++] = 0x01;
    output[idx++] = msgType;
    
    // Payload (caller provides nibble-encoded data)
    // For raw payload, we need to nibble-encode each byte
    for (size_t i = 0; i < payloadLen; i++) {
        uint8_t hi, lo;
        gp5_encode_nibbles(payload[i], &hi, &lo);
        output[idx++] = hi;
        output[idx++] = lo;
    }
    
    // End marker
    output[idx++] = 0xF7;
    
    // Calculate CRC over everything after CRC bytes, up to but not including F7
    uint8_t crc = gp5_crc8(output, idx);
    
    // Insert CRC as nibbles
    output[1] = (crc >> 4) & 0x0F;
    output[2] = crc & 0x0F;
    
    return idx;
}

// ============================================
// SYSEX MESSAGE PARSING
// ============================================

/**
 * Parse incoming GP-5 SysEx message
 * Extracts and decodes the payload section
 */
bool gp5_parse_sysex(const uint8_t* input, size_t inputLen, uint8_t* payload, size_t* payloadLen) {
    *payloadLen = 0;
    
    // Minimum valid message: F0 + CRC(2) + chunks(4) + len(2) + type(2) + F7 = 12 bytes
    if (inputLen < 12) {
        Serial.println("GP5: SysEx too short");
        return false;
    }
    
    // Verify start marker
    if (input[0] != 0xF0) {
        Serial.println("GP5: Missing F0 start");
        return false;
    }
    
    // Skip BLE MIDI header if present (80 80)
    size_t offset = 0;
    if (input[0] == 0x80 && input[1] == 0x80) {
        offset = 2;
        if (inputLen < 14) {
            Serial.println("GP5: SysEx with BLE header too short");
            return false;
        }
    }
    
    // Verify F0 at expected position
    if (input[offset] != 0xF0) {
        Serial.printf("GP5: Expected F0 at offset %d, got %02X\n", offset, input[offset]);
        return false;
    }
    
    // Skip F0 (1) + CRC (2) + chunk info (4) + length (2) + message type (2) = 11 bytes
    size_t payloadStart = offset + 11;
    
    // Find end marker
    size_t payloadEnd = payloadStart;
    while (payloadEnd < inputLen && input[payloadEnd] != 0xF7) {
        payloadEnd++;
    }
    
    if (payloadEnd >= inputLen) {
        Serial.println("GP5: Missing F7 end marker");
        return false;
    }
    
    // Decode nibble pairs in payload
    size_t nibblePairs = (payloadEnd - payloadStart) / 2;
    for (size_t i = 0; i < nibblePairs; i++) {
        payload[i] = gp5_decode_nibbles(
            input[payloadStart + i * 2],
            input[payloadStart + i * 2 + 1]
        );
    }
    
    *payloadLen = nibblePairs;
    return true;
}

// ============================================
// HIGH-LEVEL PROTOCOL COMMANDS
// ============================================

// Request SysEx command templates (pre-nibble-encoded payloads)
// These are derived from GP5EditorBT.html analysis

/**
 * Request current preset state
 * Sends command that triggers FUNC_PRESET_PARAMS (0x41) response
 * 
 * Raw command from GP5EditorBT.html: 8080F0 000900010000000201020401 F7
 */
void gp5_request_current_preset() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request preset - not connected");
        return;
    }
    
    // Exact command from GP5EditorBT.html to request preset parameters
    // sendSysex("8080f0000900010000000201020401f7")
    static const uint8_t cmd[] = {
        0x80, 0x80,                             // BLE MIDI header
        0xF0,                                   // SysEx start
        0x00, 0x09, 0x00, 0x01, 0x00, 0x00,     // Chunk info
        0x00, 0x02, 0x01, 0x02, 0x04, 0x01,     // Request preset params command
        0xF7                                    // SysEx end
    };
    
    pRemoteCharacteristic->writeValue((uint8_t*)cmd, sizeof(cmd), false);
    
    Serial.println("GP5: Requested current preset (raw SysEx)");
}

/**
 * Request all preset names (100 presets)
 * 
 * Raw command from GP5EditorBT.html: 8080F0 000E00010000000201020400 F7
 */
void gp5_request_preset_list() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request preset list - not connected");
        return;
    }
    
    // Exact command from GP5EditorBT.html to request patch names
    // sendSysex("8080F0000E00010000000201020400F7")
    static const uint8_t cmd[] = {
        0x80, 0x80,                             // BLE MIDI header
        0xF0,                                   // SysEx start
        0x00, 0x0E, 0x00, 0x01, 0x00, 0x00,     // Chunk info
        0x00, 0x02, 0x01, 0x02, 0x04, 0x00,     // Request preset list command
        0xF7                                    // SysEx end
    };
    
    pRemoteCharacteristic->writeValue((uint8_t*)cmd, sizeof(cmd), false);
    
    Serial.println("GP5: Requested preset list (raw SysEx)");
}

/**
 * Request global settings
 * 
 * Raw command from GP5EditorBT.html: 8080F0 0B0900010000000201020100 F7
 */
void gp5_request_globals() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request globals - not connected");
        return;
    }
    
    // Exact command from GP5EditorBT.html to request global parameters
    // sendSysex("8080f00b0900010000000201020100f7")
    static const uint8_t cmd[] = {
        0x80, 0x80,                             // BLE MIDI header
        0xF0,                                   // SysEx start
        0x0B, 0x09, 0x00, 0x01, 0x00, 0x00,     // Chunk info
        0x00, 0x02, 0x01, 0x02, 0x01, 0x00,     // Request globals command
        0xF7                                    // SysEx end
    };
    
    pRemoteCharacteristic->writeValue((uint8_t*)cmd, sizeof(cmd), false);
    
    Serial.println("GP5: Requested globals (raw SysEx)");
}

/**
 * Toggle effect block on/off using MIDI CC
 * This is more reliable than SysEx for real-time control
 */
void gp5_set_effect_state(GP5EffectBlock block, bool enabled) {
    if (block >= GP5_BLOCK_COUNT) return;
    
    // CC numbers for effect enables are 0-9 (matching block enum)
    uint8_t cc = static_cast<uint8_t>(block);
    uint8_t value = enabled ? 127 : 0;
    
    // Send via standard MIDI CC (channel configured in systemConfig)
    sendMidiCC(systemConfig.midiChannel, cc, value);
    
    Serial.printf("GP5: Set block %d = %s (CC %d = %d)\n", 
        block, enabled ? "ON" : "OFF", cc, value);
}

/**
 * Select a specific preset
 */
void gp5_select_preset(uint8_t presetNum) {
    if (presetNum >= GP5SysEx::MAX_PRESETS) {
        presetNum = GP5SysEx::MAX_PRESETS - 1;
    }
    
    // Use CC 127 for preset selection
    sendMidiCC(systemConfig.midiChannel, GP5MidiCC::PRESET_SELECT, presetNum);
    
    Serial.printf("GP5: Selected preset %d\n", presetNum);
}

/**
 * Request current preset number
 * 
 * Raw command from GP5EditorBT.html: 8080F0 000700010000000201020403 F7
 */
void gp5_request_current_preset_number() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request preset number - not connected");
        return;
    }
    
    // Exact command from GP5EditorBT.html to request current preset number
    // sendSysex("8080f0000700010000000201020403f7")
    static const uint8_t cmd[] = {
        0x80, 0x80,                             // BLE MIDI header
        0xF0,                                   // SysEx start
        0x00, 0x07, 0x00, 0x01, 0x00, 0x00,     // Chunk info
        0x00, 0x02, 0x01, 0x02, 0x04, 0x03,     // Request current preset number
        0xF7                                    // SysEx end
    };
    
    pRemoteCharacteristic->writeValue((uint8_t*)cmd, sizeof(cmd), false);
    
    Serial.println("GP5: Requested current preset number (raw SysEx)");
}

/**
 * Parse effect states from preset data response
 * 
 * Effect state bits are located at specific byte offsets in the combined
 * preset data response (after reassembling multi-chunk messages):
 * 
 * data[140]: bit 0=CAB, bit 1=EQ, bit 2=MOD, bit 3=DLY
 * data[141]: bit 0=NR, bit 1=PRE, bit 2=DST, bit 3=AMP
 * data[143]: bit 0=RVB, bit 1=NS
 * 
 * @param data Combined preset data from multi-chunk response
 * @param len Length of data
 * @param effectStates Output array of 10 effect states (GP5_BLOCK_COUNT)
 * @return true if parsing succeeded
 */
bool gp5_parse_preset_effect_states(const uint8_t* data, size_t len, bool effectStates[GP5_BLOCK_COUNT]) {
    // Need at least 144 bytes to access effect state bits
    if (len < 144) {
        Serial.printf("GP5: Preset data too short (%d bytes, need 144)\n", len);
        return false;
    }
    
    // Extract effect states from bit flags
    // From GP5EditorBT.html handleNotification parsing logic
    effectStates[GP5_BLOCK_NR]  = (data[141] & (1 << 0)) != 0;
    effectStates[GP5_BLOCK_PRE] = (data[141] & (1 << 1)) != 0;
    effectStates[GP5_BLOCK_DST] = (data[141] & (1 << 2)) != 0;
    effectStates[GP5_BLOCK_AMP] = (data[141] & (1 << 3)) != 0;
    effectStates[GP5_BLOCK_CAB] = (data[140] & (1 << 0)) != 0;
    effectStates[GP5_BLOCK_EQ]  = (data[140] & (1 << 1)) != 0;
    effectStates[GP5_BLOCK_MOD] = (data[140] & (1 << 2)) != 0;
    effectStates[GP5_BLOCK_DLY] = (data[140] & (1 << 3)) != 0;
    effectStates[GP5_BLOCK_RVB] = (data[143] & (1 << 0)) != 0;
    effectStates[GP5_BLOCK_NS]  = (data[143] & (1 << 1)) != 0;
    
    Serial.print("GP5: Parsed effect states: ");
    const char* blockNames[] = {"NR", "PRE", "DST", "AMP", "CAB", "EQ", "MOD", "DLY", "RVB", "NS"};
    for (int i = 0; i < GP5_BLOCK_COUNT; i++) {
        if (effectStates[i]) {
            Serial.printf("%s ", blockNames[i]);
        }
    }
    Serial.println();
    
    return true;
}
