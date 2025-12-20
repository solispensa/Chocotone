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
 */
void gp5_request_current_preset() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request preset - not connected");
        return;
    }
    
    // Command to request current preset parameters
    // Based on GP5EditorBT.html sync sequence
    uint8_t payload[] = { 0x02, 0x01 };  // Request preset params command
    
    uint8_t sysex[64];
    size_t len = gp5_build_sysex(sysex, payload, sizeof(payload), GP5SysEx::MSG_TYPE_COMMAND);
    
    // Wrap in BLE MIDI packet
    uint8_t blePkt[66];
    blePkt[0] = 0x80;
    blePkt[1] = 0x80;
    memcpy(blePkt + 2, sysex, len);
    
    pRemoteCharacteristic->writeValue(blePkt, len + 2, false);
    
    Serial.println("GP5: Requested current preset");
}

/**
 * Request all preset names (100 presets)
 */
void gp5_request_preset_list() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request preset list - not connected");
        return;
    }
    
    // Command to request preset names
    uint8_t payload[] = { 0x01, 0x01 };  // Request preset list command
    
    uint8_t sysex[64];
    size_t len = gp5_build_sysex(sysex, payload, sizeof(payload), GP5SysEx::MSG_TYPE_COMMAND);
    
    // Wrap in BLE MIDI packet
    uint8_t blePkt[66];
    blePkt[0] = 0x80;
    blePkt[1] = 0x80;
    memcpy(blePkt + 2, sysex, len);
    
    pRemoteCharacteristic->writeValue(blePkt, len + 2, false);
    
    Serial.println("GP5: Requested preset list");
}

/**
 * Request global settings
 */
void gp5_request_globals() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("GP5: Cannot request globals - not connected");
        return;
    }
    
    // Command to request global parameters
    uint8_t payload[] = { 0x03, 0x01 };  // Request globals command
    
    uint8_t sysex[64];
    size_t len = gp5_build_sysex(sysex, payload, sizeof(payload), GP5SysEx::MSG_TYPE_COMMAND);
    
    // Wrap in BLE MIDI packet
    uint8_t blePkt[66];
    blePkt[0] = 0x80;
    blePkt[1] = 0x80;
    memcpy(blePkt + 2, sysex, len);
    
    pRemoteCharacteristic->writeValue(blePkt, len + 2, false);
    
    Serial.println("GP5: Requested globals");
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
