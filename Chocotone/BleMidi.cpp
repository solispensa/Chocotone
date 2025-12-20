#include "BleMidi.h"
#include "UI_Display.h"
#include "Storage.h"
#include "WebInterface.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "esp_gap_ble_api.h"
#include "delay_time_sysex.h"

#define MIDI_SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// Config Service UUIDs (custom for Chocotone wireless configuration)
#define CONFIG_SERVICE_UUID      "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
#define CONFIG_RX_UUID           "a1b2c3d4-e5f6-7890-abcd-ef1234567891"  // Write (editor→ESP)
#define CONFIG_TX_UUID           "a1b2c3d4-e5f6-7890-abcd-ef1234567892"  // Notify (ESP→editor)

// Forward Declarations
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
static void serverMidiCallback(BLECharacteristic* pCharacteristic);

// Security Callbacks
class MySecurityCallbacks : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() { return 123456; }
    void onPassKeyNotify(uint32_t pass_key) {}
    bool onConfirmPIN(uint32_t pass_key) { return true; }
    bool onSecurityRequest() { 
        // Always allow security request - handle in onAuthenticationComplete
        return true; 
    }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
        if (auth_cmpl.success) {
            Serial.println("BLE Security: Auth Success");
        } else {
            // In config mode, auth failure is expected from web browsers
            if (bleConfigMode) {
                Serial.println("BLE Security: Auth skipped (Config Mode)");
            } else {
                Serial.println("BLE Security: Auth Failed");
            }
        }
    }
};

// Client Callbacks
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        clientConnected = true;
        Serial.println("BLE Client Connected");
    }
    void onDisconnect(BLEClient* pclient) {
        clientConnected = false;
        Serial.println("BLE Client Disconnected");
    }
};

// Global BLE Client variables
BLEAdvertisedDevice* myDevice;
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
bool doConnect = false;
bool clientConnected = false;
bool doScan = true;
bool bleConfigMode = false;  // When true, BLE scanning is paused for web editor config

// Global BLE Server variables
BLEServer* pServer = nullptr;
BLECharacteristic* pServerMidiCharacteristic = nullptr;
bool serverConnected = false;

// BLE Config Server variables
BLECharacteristic* pConfigRxCharacteristic = nullptr;
BLECharacteristic* pConfigTxCharacteristic = nullptr;
bool configClientConnected = false;
String bleConfigBuffer = "";  // Buffer for incoming config data

// ============================================
// BLE SERVER CALLBACKS (for DAW/App connections)
// ============================================

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        serverConnected = true;
        Serial.println("BLE Server: Device connected (DAW/App)");
        // Note: In dual mode, we may want to stop advertising to save power
        // But we'll keep advertising to allow reconnection
    }
    
    void onDisconnect(BLEServer* pServer) {
        serverConnected = false;
        configClientConnected = false;  // Also mark config client as disconnected
        Serial.println("BLE Server: Device disconnected");
        // Restart advertising for reconnection
        // Also restart if bleConfigMode is active (for web editor connections)
        if (systemConfig.bleMode != BLE_CLIENT_ONLY || bleConfigMode) {
            delay(100);  // Small delay before restarting advertising
            BLEDevice::startAdvertising();
            Serial.println("BLE Server: Advertising restarted");
        }
    }
};

// MIDI Characteristic Callbacks (for incoming MIDI from DAW)
class MidiCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        auto value = pCharacteristic->getValue();
        if (value.length() > 2) {
            // Skip BLE MIDI header bytes (timestamp)
            Serial.print("BLE Server: Received MIDI from DAW (");
            Serial.print(value.length());
            Serial.print(" bytes): ");
            for (size_t i = 0; i < value.length() && i < 10; i++) {
                Serial.printf("%02X ", (uint8_t)value[i]);
            }
            if (value.length() > 10) Serial.print("...");
            Serial.println();
            
            // TODO: Process incoming MIDI or forward to SPM if in dual mode
            // For now, just log it
        }
    }
};

// ============================================
// BLE CONFIG SERVER CALLBACKS (for Web Editor)
// ============================================

// Forward declaration for config processing
void processBleConfigCommand(const String& cmd);

class ConfigCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        auto value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.printf("BLE Config: Received %d bytes\n", (int)value.length());
            
            // Append to buffer and process complete lines
            bleConfigBuffer += value;
            
            // Process complete commands (newline-terminated)
            int nlPos;
            while ((nlPos = bleConfigBuffer.indexOf('\n')) >= 0) {
                String cmd = bleConfigBuffer.substring(0, nlPos);
                cmd.trim();
                bleConfigBuffer = bleConfigBuffer.substring(nlPos + 1);
                
                if (cmd.length() > 0) {
                    processBleConfigCommand(cmd);
                }
            }
            
            // Safety: prevent buffer overflow
            if (bleConfigBuffer.length() > 4096) {
                Serial.println("BLE Config: Buffer overflow, clearing");
                bleConfigBuffer = "";
            }
        }
    }
};

// ============================================
// BLE SERVER SETUP
// ============================================

void setup_ble_server() {
    Serial.println("Setting up BLE Server (MIDI Peripheral)...");
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create MIDI Service
    BLEService* pService = pServer->createService(MIDI_SERVICE_UUID);
    
    // Create MIDI Characteristic with all required properties
    pServerMidiCharacteristic = pService->createCharacteristic(
        MIDI_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_WRITE_NR  // Write without response for low-latency
    );
    
    // Add Client Characteristic Configuration Descriptor (required for notifications)
    pServerMidiCharacteristic->addDescriptor(new BLE2902());
    
    // Set callbacks for incoming writes
    pServerMidiCharacteristic->setCallbacks(new MidiCharacteristicCallbacks());
    
    // Start the service
    pService->start();
    
    // Setup advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(MIDI_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Help with iPhone connection issues
    pAdvertising->setMinPreferred(0x12);
    
    // Start advertising
    BLEDevice::startAdvertising();
    
    Serial.println("BLE Server Started - Advertising as MIDI device");
    Serial.printf("  Device Name: %s\n", systemConfig.bleDeviceName);
}

// ============================================
// BLE CONFIG SERVER SETUP (always runs for web editor)
// ============================================

void setup_ble_config_server() {
    Serial.println("Setting up BLE Config Server (Web Editor)...");
    
    // Create server if not already created (may be created by MIDI server)
    if (pServer == nullptr) {
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
    }
    
    // Create Config Service
    BLEService* pConfigService = pServer->createService(CONFIG_SERVICE_UUID);
    
    // Create RX Characteristic (editor writes to ESP)
    // Set permissions to allow write without encryption (for web browser compatibility)
    pConfigRxCharacteristic = pConfigService->createCharacteristic(
        CONFIG_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    pConfigRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);  // No encryption required
    pConfigRxCharacteristic->setCallbacks(new ConfigCharacteristicCallbacks());
    
    // Create TX Characteristic (ESP notifies editor)
    // Set permissions to allow read/notify without encryption
    pConfigTxCharacteristic = pConfigService->createCharacteristic(
        CONFIG_TX_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pConfigTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);  // No encryption required
    pConfigTxCharacteristic->addDescriptor(new BLE2902());
    
    // Start the config service
    pConfigService->start();
    
    // Add config service to advertising (alongside MIDI if present)
    // Must restart advertising after adding new service UUID
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(CONFIG_SERVICE_UUID);
    
    // Restart advertising to include the new service UUID
    BLEDevice::startAdvertising();
    
    Serial.println("BLE Config Server Started - Advertising restarted");
}

// Toggle BLE Config Mode - called from menu
// When entering: stops scanning, disconnects, restarts advertising
// When exiting: resumes normal operations
void toggleBleConfigMode() {
    bleConfigMode = !bleConfigMode;
    
    if (bleConfigMode) {
        Serial.println("=== ENTERING BLE CONFIG MODE ===");
        
        // 1. Stop any ongoing BLE scan
        BLEScan* pScan = BLEDevice::getScan();
        if (pScan) {
            pScan->stop();
            Serial.println("  - Scanning stopped");
        }
        
        // 2. Disconnect BLE client if connected to SPM
        if (pClient && pClient->isConnected()) {
            pClient->disconnect();
            Serial.println("  - Client disconnected from SPM");
        }
        clientConnected = false;
        doConnect = false;
        
        // 3. Stop advertising briefly
        BLEDevice::stopAdvertising();
        delay(100);
        
        // 4. Configure and restart advertising with config service
        BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMaxPreferred(0x12);
        
        // Make sure config service UUID is advertised
        pAdvertising->addServiceUUID(CONFIG_SERVICE_UUID);
        
        // 5. Start advertising
        BLEDevice::startAdvertising();
        Serial.println("  - Advertising started for web editor");
        Serial.println("=== BLE CONFIG MODE READY - Connect from web editor ===");
        
    } else {
        Serial.println("=== EXITING BLE CONFIG MODE ===");
        
        // Normal operations will resume on next loop cycle
        // doScan and other flags will trigger appropriate behavior
        if (systemConfig.bleMode == BLE_CLIENT_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
            doScan = true;  // Re-enable scanning for SPM
            Serial.println("  - Scanning will resume for SPM");
        }
        
        Serial.println("=== NORMAL BLE OPERATIONS RESUMED ===");
    }
}

// Advertised Device Callbacks (for client scanning)
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(MIDI_SERVICE_UUID))) {
             BLEDevice::getScan()->stop();
             myDevice = new BLEAdvertisedDevice(advertisedDevice);
             doConnect = true;
             doScan = true;
        }
    }
};

void setup_ble_midi() {
    BLEDevice::init(systemConfig.bleDeviceName);
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    // Initialize based on configured BLE mode
    Serial.printf("BLE Mode: %s\n", 
        systemConfig.bleMode == BLE_CLIENT_ONLY ? "CLIENT_ONLY (SPM)" :
        systemConfig.bleMode == BLE_SERVER_ONLY ? "SERVER_ONLY (DAW/Apps)" : "DUAL_MODE");
    
    // Setup Server if Server-Only or Dual Mode
    if (systemConfig.bleMode == BLE_SERVER_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
        setup_ble_server();
    }
    
    // Client mode is implicitly ready (scanning handled separately in startBleScan)
    if (systemConfig.bleMode == BLE_CLIENT_ONLY || systemConfig.bleMode == BLE_DUAL_MODE) {
        Serial.println("BLE Client Configured (will scan for SPM)");
        doScan = true;  // Enable scanning for client mode
    } else {
        doScan = false;  // Disable scanning in server-only mode
        Serial.println("BLE Client Disabled (server-only mode)");
    }
    
    // ALWAYS setup config server for wireless configuration from web editor
    setup_ble_config_server();
    
    // Start advertising if not already started by MIDI server
    if (systemConfig.bleMode == BLE_CLIENT_ONLY) {
        BLEDevice::startAdvertising();
        Serial.println("BLE Advertising started (Config only)");
    }
}

void startBleScan() {
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(1, false);
}

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    yield();  // Allow other tasks to run

    // Try to connect with a reasonable timeout (BLE lib handles this internally)
    if (!pClient->connect(myDevice)) {
        Serial.println("✗ connect() returned false");
        return false;
    }
    Serial.println(" - Connected to server");
    yield();  // Allow other tasks to run
    
    // Request larger MTU for MIDI
    pClient->setMTU(517);
    yield();  // Allow other tasks to run

    // Quick service lookup with error handling
    BLERemoteService* pRemoteService = pClient->getService(MIDI_SERVICE_UUID);
    if (pRemoteService == nullptr) {
        Serial.println("✗ MIDI service not found");
        pClient->disconnect();
        return false;
    }
    yield();  // Allow other tasks to run

    pRemoteCharacteristic = pRemoteService->getCharacteristic(MIDI_CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("✗ MIDI characteristic not found");
        pClient->disconnect();
        return false;
    }
    yield();  // Allow other tasks to run

    if(pRemoteCharacteristic->canNotify())
        pRemoteCharacteristic->registerForNotify(notifyCallback);

    clientConnected = true;
    return true;
}

// NOTE: sendMidiMessage() removed in v3.0 - use executeActionMessage() in Input.cpp instead
// Individual MIDI send functions remain below for direct use

void sendMidiNoteOn(byte ch, byte n, byte v) {
    if(ch<1)ch=1; if(ch>16)ch=16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0x90 | ((ch-1) & 0x0F)), n, v};
    
    // Send to SPM via Client (if connected)
    if(clientConnected && pRemoteCharacteristic) {
        pRemoteCharacteristic->writeValue(m, 5, false);
        Serial.printf("→ SPM: Note On Ch%d N%d V%d\n", ch, n, v);
    }
    
    // Also send to DAW/App via Server (if connected)
    if(serverConnected && pServerMidiCharacteristic) {
        pServerMidiCharacteristic->setValue(m, 5);
        pServerMidiCharacteristic->notify();
        Serial.printf("→ DAW: Note On Ch%d N%d V%d\n", ch, n, v);
    }
    
    // Log if neither connected
    if (!clientConnected && !serverConnected) {
        Serial.println("! No BLE devices connected");
    }
}

void sendMidiNoteOff(byte ch, byte n, byte v) {
    if(ch<1)ch=1; if(ch>16)ch=16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0x80 | ((ch-1) & 0x0F)), n, v};
    
    // Send to SPM via Client
    if(clientConnected && pRemoteCharacteristic) {
        pRemoteCharacteristic->writeValue(m, 5, false);
        Serial.printf("→ SPM: Note Off Ch%d N%d V%d\n", ch, n, v);
    }
    
    // Send to DAW/App via Server
    if(serverConnected && pServerMidiCharacteristic) {
        pServerMidiCharacteristic->setValue(m, 5);
        pServerMidiCharacteristic->notify();
        Serial.printf("→ DAW: Note Off Ch%d N%d V%d\n", ch, n, v);
    }
}

void sendMidiCC(byte ch, byte n, byte v) {
    if(ch<1)ch=1; if(ch>16)ch=16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0xB0 | ((ch-1) & 0x0F)), n, v};
    
    // Send to SPM via Client
    if(clientConnected && pRemoteCharacteristic) {
        pRemoteCharacteristic->writeValue(m, 5, false);
        Serial.printf("→ SPM: CC Ch%d N%d V%d\n", ch, n, v);
    }
    
    // Send to DAW/App via Server
    if(serverConnected && pServerMidiCharacteristic) {
        pServerMidiCharacteristic->setValue(m, 5);
        pServerMidiCharacteristic->notify();
        Serial.printf("→ DAW: CC Ch%d N%d V%d\n", ch, n, v);
    }
}

void sendMidiPC(byte ch, byte n) {
    if(ch<1)ch=1; if(ch>16)ch=16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0xC0 | ((ch-1) & 0x0F)), n, 0};
    
    // Send to SPM via Client
    if(clientConnected && pRemoteCharacteristic) {
        pRemoteCharacteristic->writeValue(m, 5, false);
        Serial.printf("→ SPM: PC Ch%d N%d\n", ch, n);
    }
    
    // Send to DAW/App via Server
    if(serverConnected && pServerMidiCharacteristic) {
        pServerMidiCharacteristic->setValue(m, 5);
        pServerMidiCharacteristic->notify();
        Serial.printf("→ DAW: PC Ch%d N%d\n", ch, n);
    }
}

void sendDelayTime(int delayMs) {
    if(!clientConnected || !pRemoteCharacteristic) {
        Serial.println("! SPM not connected - cannot send delay time");
        return;
    }
    
    // Constrain to valid range (20-1000ms)
    delayMs = constrain(delayMs, 20, 1000);
    
    // Find closest match in lookup table
    int closestIdx = 0;
    int minDiff = abs(pgm_read_word(&DELAY_TIME_LOOKUP[0].delayMs) - delayMs);
    
    for(int i = 1; i < DELAY_TIME_LOOKUP_SIZE; i++) {
        int diff = abs(pgm_read_word(&DELAY_TIME_LOOKUP[i].delayMs) - delayMs);
        if(diff < minDiff) {
            minDiff = diff;
            closestIdx = i;
        }
    }
    
    // Read SysEx from PROGMEM (all entries are 38 bytes)
    uint8_t sysex[38];
    for(int i = 0; i < 38; i++) {
        sysex[i] = pgm_read_byte(&DELAY_TIME_LOOKUP[closestIdx].data[i]);
    }
    
    // DEBUG: Print the SysEx we're about to send
    Serial.print("SysEx to send: ");
    for(int i = 0; i < 38; i++) {
        Serial.printf("%02X ", sysex[i]);
    }
    Serial.println();
    
    // Send entire SysEx as ONE packet (we negotiated MTU=517, so 38 bytes + 2 header fits easily)
    uint8_t blePacket[40];
    blePacket[0] = 0x80;  // BLE MIDI Timestamp High
    blePacket[1] = 0x80;  // BLE MIDI Timestamp Low
    memcpy(blePacket + 2, sysex, 38);
    
    Serial.printf("Sending as single packet (%d bytes): ", 40);
    for(int i = 0; i < 40; i++) {
        Serial.printf("%02X ", blePacket[i]);
    }
    Serial.println();
    
    pRemoteCharacteristic->writeValue(blePacket, 40, false);
    
    int actualDelay = pgm_read_word(&DELAY_TIME_LOOKUP[closestIdx].delayMs);
    Serial.printf("→ SPM: Delay Time = %dms (requested: %dms)\n", actualDelay, delayMs);
}

void sendSysex(const uint8_t* data, size_t length) {
    if(!clientConnected || !pRemoteCharacteristic) return;
    
    // BLE MIDI SysEx needs timestamps
    uint8_t blePacket[length + 2];
    blePacket[0] = 0x80;  // Timestamp high
    blePacket[1] = 0x80;  // Timestamp low
    memcpy(blePacket + 2, data, length);
    
    pRemoteCharacteristic->writeValue(blePacket, length + 2, false);
    
    Serial.print("SysEx sent (");
    Serial.print(length);
    Serial.print(" bytes): ");
    for(size_t i=0; i<length && i<20; i++) {
        if(data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        if(i < length-1) Serial.print(" ");
    }
    if(length > 20) Serial.print("...");
    Serial.println();
}

// ============================================
// BLE SERVER MIDI OUTPUT (to DAW/Apps)
// ============================================

void sendMidiToServer(byte* data, size_t length) {
    if (!serverConnected || !pServerMidiCharacteristic) {
        return;  // No connected client to send to
    }
    
    // Set the value and notify the connected client
    pServerMidiCharacteristic->setValue(data, length);
    pServerMidiCharacteristic->notify();
    
    Serial.print("→ DAW: ");
    for (size_t i = 0; i < length && i < 10; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();
}

// Helper to send a Note On to connected DAW/Apps
void sendMidiNoteOnToServer(byte ch, byte n, byte v) {
    if (ch < 1) ch = 1; if (ch > 16) ch = 16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0x90 | ((ch-1) & 0x0F)), n, v};
    sendMidiToServer(m, 5);
}

// Helper to send a Note Off to connected DAW/Apps
void sendMidiNoteOffToServer(byte ch, byte n, byte v) {
    if (ch < 1) ch = 1; if (ch > 16) ch = 16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0x80 | ((ch-1) & 0x0F)), n, v};
    sendMidiToServer(m, 5);
}

// Helper to send CC to connected DAW/Apps
void sendMidiCCToServer(byte ch, byte n, byte v) {
    if (ch < 1) ch = 1; if (ch > 16) ch = 16;
    uint8_t m[5] = {0x80, 0x80, (uint8_t)(0xB0 | ((ch-1) & 0x0F)), n, v};
    sendMidiToServer(m, 5);
}

// Helper to send PC to connected DAW/Apps
void sendMidiPCToServer(byte ch, byte n) {
    if (ch < 1) ch = 1; if (ch > 16) ch = 16;
    uint8_t m[4] = {0x80, 0x80, (uint8_t)(0xC0 | ((ch-1) & 0x0F)), n};
    sendMidiToServer(m, 4);
}

void clearBLEBonds() {
    int devNum = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * devNum);
    esp_ble_get_bond_device_list(&devNum, dev_list);
    for (int i = 0; i < devNum; i++) {
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
    }
    free(dev_list);
    Serial.println("BLE Bonds Cleared");
}

// Buffer for incoming SysEx
volatile bool sysexReceived = false;
uint8_t sysexBuffer[256];
size_t sysexLen = 0;

void processBufferedSysex() {
    // Log incoming SysEx 
    Serial.print("Received SysEx (");
    Serial.print(sysexLen);
    Serial.print(" bytes): ");
    for(size_t i=0; i<min(sysexLen, (size_t)40); i++) {
        if(sysexBuffer[i] < 0x10) Serial.print("0");
        Serial.print(sysexBuffer[i], HEX);
        Serial.print(" ");
    }
    if(sysexLen > 40) Serial.print("...");
    Serial.println();
    
    // Only process SysEx messages (start with 80 80 F0)
    if (sysexLen < 20 || sysexBuffer[0] != 0x80 || sysexBuffer[1] != 0x80 || sysexBuffer[2] != 0xF0) {
        return;
    }
    
    // SPM sends 5 packets for a preset dump. Only the FIRST packet (sequence 0) contains
    // the module enable states. Bytes 7-8 contain the packet sequence number:
    // Packet 0: 00 05 00 00 - sequence 00 (contains states - PROCESS THIS ONE)
    // Packet 1: 00 05 00 01 - sequence 01 (effect types)
    // Packet 2: 00 05 00 02 - sequence 02 (parameters)
    // Packet 3: 00 05 00 03 - sequence 03 (parameters)
    // Packet 4: 00 05 00 04 - sequence 04 (parameters, shorter packet)
    
    // Check for 212-byte preset dump packets with sequence number 00
    // Pattern: bytes 5-8 should be "00 05 00 00" for the first packet
    if (sysexLen >= 200 && sysexBuffer[5] == 0x00 && sysexBuffer[6] == 0x05 &&
        sysexBuffer[7] == 0x00 && sysexBuffer[8] == 0x00) {
        
        Serial.println("SPM: First preset dump packet (seq=00) - processing states...");
        
        // POCKETEDIT ALGORITHM:
        // Search for "0A 00 00 00" chain marker from END (like JavaScript lastIndexOf)
        // State bytes are at offsets -13, -12, -10 relative to marker (in payload coordinates)
        
        // Search from END of buffer for chain marker
        int chainMarkerPosRaw = -1;
        for (int i = (int)sysexLen - 5; i >= 10; i--) {
            if (sysexBuffer[i] == 0x0A && sysexBuffer[i+1] == 0x00 && 
                sysexBuffer[i+2] == 0x00 && sysexBuffer[i+3] == 0x00) {
                chainMarkerPosRaw = i;
                break;
            }
        }
        
        if (chainMarkerPosRaw >= 0) {
            // Convert to payload position (pocketedit's payload starts at raw[5])
            int chainByteIndex = chainMarkerPosRaw - 5;  // Position in payload
        
            Serial.printf("SPM: Chain marker at raw[%d], payload[%d]\n", chainMarkerPosRaw, chainByteIndex);
            
            // Calculate positions in PAYLOAD (matching pocketedit)
            int nibble1PayloadPos = chainByteIndex - 13;
            int nibble2PayloadPos = chainByteIndex - 12;
            int globalStatePayloadPos = chainByteIndex - 10;
            
            // Convert back to RAW buffer positions
            int nibble1Pos = nibble1PayloadPos + 5;
            int nibble2Pos = nibble2PayloadPos + 5;
            int globalStatePos = globalStatePayloadPos + 5;
            
            Serial.printf("  Positions - nibble1: payload[%d]=raw[%d], nibble2: payload[%d]=raw[%d], globalState: payload[%d]=raw[%d]\n",
                nibble1PayloadPos, nibble1Pos, nibble2PayloadPos, nibble2Pos, globalStatePayloadPos, globalStatePos);
            
            if (nibble1Pos >= 5 && nibble1Pos < (int)sysexLen && globalStatePos >= 5 && globalStatePos < (int)sysexLen) {
                uint8_t nibble1Byte = sysexBuffer[nibble1Pos];
                uint8_t nibble2Byte = sysexBuffer[nibble2Pos];
                uint8_t globalStateByte = sysexBuffer[globalStatePos];
                
                Serial.printf("  Raw values: nibble1=0x%02X nibble2=0x%02X globalState=0x%02X\n", 
                    nibble1Byte, nibble2Byte, globalStateByte);
                
                // CRITICAL: pocketedit uses only the LOW NIBBLE of each state byte
                // JavaScript: parseInt(nibble1[1] + nibble2[1], 16) -> "F" + "E" = "FE" = 0xFE
                // So nibble1 is HIGH nibble, nibble2 is LOW nibble!
                uint8_t lowNibble1 = nibble1Byte & 0x0F;
                uint8_t lowNibble2 = nibble2Byte & 0x0F;
                uint8_t mainBitmask = (lowNibble1 << 4) | lowNibble2;
                
                Serial.printf("  Low nibbles: 0x%X + 0x%X = bitmask 0x%02X (binary: ", 
                    lowNibble1, lowNibble2, mainBitmask);
                for (int b = 7; b >= 0; b--) Serial.print((mainBitmask >> b) & 1);
                Serial.println(")");
                
                // RVB and Clone mode from globalStateByte
                bool isRvbOn = (globalStateByte & 0x01) != 0;
                bool isCloneMode = (globalStateByte & 0x02) != 0;
                
                Serial.printf("  globalStateByte: RVB=%d, CloneMode=%d\n", isRvbOn, isCloneMode);
                
                // Decode effect states
                spmEffectStates[0] = (mainBitmask & (1 << 0)) != 0;  // NR
                spmEffectStates[1] = (mainBitmask & (1 << 1)) != 0;  // FX1
                spmEffectStates[2] = (mainBitmask & (1 << 2)) != 0;  // DRV
                spmEffectStates[3] = (mainBitmask & (1 << 3)) != 0;  // AMP
                spmEffectStates[4] = (mainBitmask & (1 << 4)) != 0;  // IR
                spmEffectStates[5] = (mainBitmask & (1 << 5)) != 0;  // EQ
                spmEffectStates[6] = (mainBitmask & (1 << 6)) != 0;  // FX2
                spmEffectStates[7] = (mainBitmask & (1 << 7)) != 0;  // DLY
                spmEffectStates[8] = isRvbOn;  // RVB from globalStateByte bit 0
                
                // If Clone mode is active, IR is always ON (pocketedit line 12847)
                if (isCloneMode) {
                    spmEffectStates[4] = true;  // IR forced ON in clone mode
                }
                
                spmStateReceived = true;
                
                Serial.println("SPM State Decoded:");
                Serial.printf("  NR=%d FX1=%d DRV=%d AMP=%d IR=%d EQ=%d FX2=%d DLY=%d RVB=%d\n",
                    spmEffectStates[0], spmEffectStates[1], spmEffectStates[2], 
                    spmEffectStates[3], spmEffectStates[4], spmEffectStates[5],
                    spmEffectStates[6], spmEffectStates[7], spmEffectStates[8]);
            } else {
                Serial.printf("SPM: State byte positions out of range (nibble1=%d, globalState=%d)\n", 
                    nibble1Pos, globalStatePos);
            }
        }
        return;  // Processed first packet, exit
    }
    
    // Skip subsequent preset dump packets (seq 01-04) - they don't contain module states
    if (sysexLen >= 150 && sysexBuffer[5] == 0x00 && sysexBuffer[6] == 0x05 &&
        sysexBuffer[7] == 0x00 && sysexBuffer[8] > 0x00) {
        // Silently ignore packets 1-4 of the preset dump
        return;
    }
    
    // FALLBACK: Check for PRESET CHANGE messages (24 bytes with 06 01 02 04 03 pattern)
    // These are sent when the preset is changed on the SPM
    // Pattern: 80 80 F0 xx xx 00 01 00 00 00 06 01 02 04 03 00 [preset#] ...
    if (sysexLen >= 20 && sysexLen <= 30) {
        bool isPresetChange = (sysexBuffer[10] == 0x06 && 
                              sysexBuffer[11] == 0x01 && 
                              sysexBuffer[12] == 0x02 && 
                              sysexBuffer[13] == 0x04 &&
                              sysexBuffer[14] == 0x03);
        
        if (isPresetChange) {
            uint8_t presetNum = sysexBuffer[16];
            Serial.printf("SPM Preset Changed! New preset: %d\n", presetNum);
            
            // Request fresh state for the new preset
            if (presetSyncMode[currentPreset] != SYNC_NONE) {
                Serial.println("SPM Sync: Requesting state for new preset...");
                delay(50);  // Short delay for SPM to stabilize
                requestPresetState();
            }
            return;
        }
    }
    
    // Also check for 32-byte update packets (0A 01 02 04 pattern)
    // These are sent when effects are toggled in real-time
    // DISABLED: This parser was incorrectly treating 32-byte packets as full state updates.
    // The byte16=0x00 was being decoded as "all effects OFF" which corrupted button states
    // after any single effect toggle. These packets are likely just acknowledgements.
    // TODO: Research the actual SPM 32-byte packet format if real-time sync is needed.
    /*
    if (sysexLen >= 30 && sysexLen <= 40) {
        bool isUpdatePacket = (sysexBuffer[10] == 0x0A && 
                              sysexBuffer[11] == 0x01 && 
                              sysexBuffer[12] == 0x02 && 
                              sysexBuffer[13] == 0x04);
        
        if (isUpdatePacket) {
            Serial.println("SPM State Update packet detected (IGNORED - parser disabled)");
        }
    }
    */
}

void checkForSysex() {
    if (sysexReceived) {
        sysexReceived = false;
        processBufferedSysex();
    }
}

void handleBleConnection() {
    // Don't scan if WiFi is active (doScan is false when WiFi on)
    if (!doScan) {
        return;  // BLE paused for WiFi stability
    }
    
    static unsigned long lastScanAttempt = 0;
    static unsigned long lastFailedAttempt = 0;
    static int consecutiveFailures = 0;
    static bool bondsCleared = false;  // Track if we've cleared bonds this session
    const unsigned long RESCAN_INTERVAL = 3000;
    const unsigned long BACKOFF_SHORT = 5000;      // 5 sec after 1-2 failures
    const unsigned long BACKOFF_LONG = 15000;      // 15 sec after 3+ failures
    
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("✓ Connected to SPM (BLE Client)");
            doConnect = false;
            consecutiveFailures = 0;  // Reset on success
            bondsCleared = false;     // Reset bond clear flag
            
            // Startup handshake: Request initial state if sync is enabled
            if (presetSyncMode[currentPreset] != SYNC_NONE) {
                Serial.println("SPM Sync: Requesting initial state...");
                delay(100);  // Short delay to let connection stabilize
                requestPresetState();
            }
        } else {
            Serial.println("✗ Connection/Service Discovery failed");
            doConnect = false;
            consecutiveFailures++;
            lastFailedAttempt = millis();
            
            // Automatically clear bonds after 3 consecutive failures
            if (consecutiveFailures == 3 && !bondsCleared) {
                Serial.println("⚠️  3 connection failures detected - likely stale BLE bonds");
                Serial.println("🔧 Auto-clearing BLE bonds...");
                clearBLEBonds();
                bondsCleared = true;
                consecutiveFailures = 0;  // Reset to give fresh bond a chance
                Serial.println("✅ Bonds cleared, will retry connection with fresh pairing");
                lastScanAttempt = millis();
                return;
            }
            
            // Don't retry immediately - give the SPM time to stabilize
            if (consecutiveFailures >= 3) {
                Serial.printf("⏸ Too many failures (%d) - backing off for 15 seconds\n", consecutiveFailures);
                lastScanAttempt = millis();  // Reset scan timer to enforce backoff
            } else {
                Serial.printf("⏸ Waiting 5 seconds before retry (failure %d/3)\n", consecutiveFailures);
                lastScanAttempt = millis();
            }
        }
        return;  // Don't continue to scan logic
    }
    
    // Don't scan if we recently failed
    if (consecutiveFailures > 0) {
        unsigned long backoffTime = (consecutiveFailures >= 3) ? BACKOFF_LONG : BACKOFF_SHORT;
        if (millis() - lastFailedAttempt < backoffTime) {
            return;  // Still in backoff period
        }
        // Backoff period over, reset failure count
        if (consecutiveFailures >= 3) {
            consecutiveFailures = 0;  // Full reset after long backoff
            Serial.println("Backoff period over, will retry connection");
        }
    }
    
    // Only scan if not connected, config mode OFF, and enough time has passed
    // bleConfigMode pauses scanning to allow web editor BLE connection
    if (!clientConnected && !doConnect && !bleConfigMode && (millis() - lastScanAttempt > RESCAN_INTERVAL)) {
        Serial.println("→ Scanning for SPM (BLE Client auto-connect)...");
        startBleScan();
        lastScanAttempt = millis();
    }
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length > 0 && length < 256) {
        memcpy(sysexBuffer, pData, length);
        sysexLen = length;
        sysexReceived = true;
    }
}

// ============================================
// SPM STATE SYNC FUNCTIONS
// ============================================

void requestPresetState() {
    if (!clientConnected || !pRemoteCharacteristic) {
        Serial.println("SPM: Cannot request state - not connected");
        return;
    }
    
    // Debounce - don't request too frequently
    if (millis() - lastSpmStateRequest < 500) {
        return;
    }
    
    // SysEx request for current preset state (from pocketedit.html analysis)
    // This requests the SPM to send its current preset dump
    uint8_t requestState[] = {
        0x80, 0x80,  // BLE MIDI timestamp header
        0xF0,        // SysEx start
        0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00,  // Header
        0x02, 0x01, 0x02, 0x04, 0x01,              // Request preset dump command
        0xF7         // SysEx end
    };
    
    pRemoteCharacteristic->writeValue(requestState, sizeof(requestState), false);
    lastSpmStateRequest = millis();
    Serial.println("→ SPM: Requested preset state");
}

void applySpmStateToButtons() {
    if (presetSyncMode[currentPreset] == SYNC_NONE) {
        return;  // Sync not enabled for this preset
    }
    
    Serial.println("SPM Sync: Applying state to buttons...");
    
    // CC number to effect index mapping:
    // CC 43 = NR (index 0)
    // CC 44 = FX1 (index 1)
    // CC 45 = DRV (index 2)
    // CC 46 = AMP (index 3)
    // CC 47 = IR (index 4)
    // CC 48 = EQ (index 5)
    // CC 49 = FX2 (index 6)
    // CC 50 = DLY (index 7)
    // CC 51 = RVB (index 8)
    
    for (int btn = 0; btn < systemConfig.buttonCount; btn++) {
        ButtonConfig& config = buttonConfigs[currentPreset][btn];
        
        // Skip buttons that don't have 2ND_PRESS (not toggle buttons)
        if (!hasAction(config, ACTION_2ND_PRESS)) {
            continue;
        }
        
        // Find the PRESS action to check if it's a CC for an effect
        ActionMessage* pressAction = findAction(config, ACTION_PRESS);
        if (!pressAction || pressAction->type != CC) {
            continue;
        }
        
        // Check if this CC is in range 43-51 (effect toggles)
        uint8_t ccNum = pressAction->data1;
        if (ccNum >= 43 && ccNum <= 51) {
            int effectIndex = ccNum - 43;
            bool effectIsOn = spmEffectStates[effectIndex];
            
            // Set button state:
            // If effect is ON in SPM, next press should turn it OFF (send 2ND_PRESS action)
            // So isAlternate = true means "we're in the ON state, next press sends OFF"
            config.isAlternate = effectIsOn;
            ledToggleState[btn] = effectIsOn;
            
            Serial.printf("  BTN %d (%s): CC%d -> Effect[%d] = %s\n", 
                btn, config.name, ccNum, effectIndex, effectIsOn ? "ON" : "OFF");
        }
    }
    
    // Update LEDs to reflect new state
    extern void updateLeds();  // From UI_Display.cpp
    updateLeds();
    
    Serial.println("SPM Sync: State applied successfully");
}

// ============================================
// BLE CONFIG COMMAND PROCESSING
// ============================================

void sendBleConfigResponse(const String& response) {
    if (pConfigTxCharacteristic != nullptr) {
        // Web Bluetooth negotiates higher MTU (usually 512), but use safe value
        // for maximum compatibility. 180 bytes with delay works reliably.
        int len = response.length();
        int chunkSize = 180;  // Safe chunk size for negotiated BLE MTU
        
        Serial.printf("BLE Config: Sending %d bytes in %d chunks...\n", len, (len + chunkSize - 1) / chunkSize);
        
        for (int i = 0; i < len; i += chunkSize) {
            String chunk = response.substring(i, min(i + chunkSize, len));
            pConfigTxCharacteristic->setValue(chunk.c_str());
            pConfigTxCharacteristic->notify();
            delay(15);  // Delay between chunks for reliable transfer
        }
        Serial.printf("BLE Config: Sent response (%d bytes)\n", len);
    }
}

// External declarations for config functions (from WebInterface.cpp)
extern String buildFullConfigJson();
extern bool processConfigChunk(const String& chunk, int chunkNum);
extern void finalizeConfigUpload();

// Static variables for chunked config upload
static String bleConfigUploadBuffer = "";
static int bleConfigChunkCount = 0;
static bool bleConfigUploadActive = false;

void processBleConfigCommand(const String& cmd) {
    Serial.printf("BLE Config Command: %s\n", cmd.substring(0, 50).c_str());
    
    if (cmd == "GET_CONFIG") {
        // Return full configuration as JSON
        String configJson = buildFullConfigJson();
        sendBleConfigResponse(configJson);
        Serial.println("BLE Config: Sent full config");
    }
    else if (cmd == "SET_CONFIG_START") {
        // Start chunked config upload
        bleConfigUploadBuffer = "";
        bleConfigChunkCount = 0;
        bleConfigUploadActive = true;
        sendBleConfigResponse("CONFIG_READY");
        Serial.println("BLE Config: Upload started");
    }
    else if (cmd.startsWith("SET_CONFIG_CHUNK:")) {
        if (!bleConfigUploadActive) {
            sendBleConfigResponse("ERROR:Upload not started");
            return;
        }
        // Extract chunk data after the colon
        String chunkData = cmd.substring(17);  // Skip "SET_CONFIG_CHUNK:"
        bleConfigUploadBuffer += chunkData;
        bleConfigChunkCount++;
        sendBleConfigResponse("CHUNK_OK:" + String(bleConfigChunkCount));
    }
    else if (cmd == "SET_CONFIG_END") {
        if (!bleConfigUploadActive) {
            sendBleConfigResponse("ERROR:Upload not started");
            return;
        }
        // Process the complete config
        bleConfigUploadActive = false;
        Serial.printf("BLE Config: Processing %d bytes from %d chunks\n", 
            bleConfigUploadBuffer.length(), bleConfigChunkCount);
        
        // Process using the same logic as serial upload
        if (processConfigChunk(bleConfigUploadBuffer, 0)) {
            finalizeConfigUpload();
            sendBleConfigResponse("CONFIG_SAVED");
            Serial.println("BLE Config: Upload complete, saved to NVS");
        } else {
            sendBleConfigResponse("ERROR:Parse failed");
            Serial.println("BLE Config: Parse error");
        }
        bleConfigUploadBuffer = "";
        bleConfigChunkCount = 0;
    }
    else if (cmd == "PING") {
        sendBleConfigResponse("PONG");
    }
    else if (cmd == "GET_VERSION") {
        sendBleConfigResponse("Chocotone v1.0");
    }
    else {
        Serial.printf("BLE Config: Unknown command: %s\n", cmd.c_str());
        sendBleConfigResponse("ERROR:Unknown command");
    }
}
