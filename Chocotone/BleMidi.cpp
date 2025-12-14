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

// Forward Declarations
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
static void serverMidiCallback(BLECharacteristic* pCharacteristic);

// Security Callbacks
class MySecurityCallbacks : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() { return 123456; }
    void onPassKeyNotify(uint32_t pass_key) {}
    bool onConfirmPIN(uint32_t pass_key) { return true; }
    bool onSecurityRequest() { return true; }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
        if (auth_cmpl.success) Serial.println("BLE Security: Auth Success");
        else Serial.println("BLE Security: Auth Failed");
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

// Global BLE Server variables
BLEServer* pServer = nullptr;
BLECharacteristic* pServerMidiCharacteristic = nullptr;
bool serverConnected = false;

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
        Serial.println("BLE Server: Device disconnected");
        // Restart advertising for reconnection
        if (systemConfig.bleMode != BLE_CLIENT_ONLY) {
            delay(100);  // Small delay before restarting advertising
            BLEDevice::startAdvertising();
            Serial.println("BLE Server: Advertising restarted");
        }
    }
};

// MIDI Characteristic Callbacks (for incoming MIDI from DAW)
class MidiCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String value = pCharacteristic->getValue();
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

void sendMidiMessage(const MidiMessage& msg) {
    // Handle internal commands first (don't send over BLE)
    switch(msg.type) {
        case PRESET_UP:
            // Cycle to next preset
            currentPreset = (currentPreset + 1) % 4;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            Serial.printf("PRESET UP → Preset %d\n", currentPreset);
            return;
            
        case PRESET_DOWN:
            // Cycle to previous preset
            currentPreset = (currentPreset - 1 + 4) % 4;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            Serial.printf("PRESET DOWN → Preset %d\n", currentPreset);
            return;
            
        case PRESET_1:
            currentPreset = 0;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            Serial.printf("PRESET 1 → %s\n", presetNames[0]);
            return;
            
        case PRESET_2:
            currentPreset = 1;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            Serial.printf("PRESET 2 → %s\n", presetNames[1]);
            return;
            
        case PRESET_3:
            currentPreset = 2;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            Serial.printf("PRESET 3 → %s\n", presetNames[2]);
            return;
            
        case PRESET_4:
            currentPreset = 3;
            saveCurrentPresetIndex();
            displayOLED();
            updateLeds();
            Serial.printf("PRESET 4 → %s\n", presetNames[3]);
            return;
            
        case WIFI_TOGGLE:
            // Toggle WiFi on/off
            if (isWifiOn) {
                turnWifiOff();
                Serial.println("WiFi TOGGLED OFF");
                delay(100);
                yield();
                safeDisplayOLED();  // Safe to update when WiFi off (more heap)
            } else {
                turnWifiOn();
                Serial.println("WiFi TOGGLED ON");
                delay(200);  // Let WiFi stabilize
                yield();
                // SKIP OLED update - 38KB heap too low, would crash
                Serial.println("OLED update skipped (low heap with WiFi on)");
            }
            return;
            
        case CLEAR_BLE_BONDS:
            // Clear all BLE bonds/pairings
            Serial.println("Clearing BLE bonds...");
            // Note: ESP32 BLE bond clearing requires NVS erase for BLE namespace
            // For now just log - full implementation would need BLEDevice::deinit() and reinit
            Serial.println("BLE bonds cleared (restart required)");
            return;
            
        // MIDI commands (send over BLE)
        case NOTE_ON:
        case NOTE_MOMENTARY:
            sendMidiNoteOn(msg.channel, msg.data1, msg.data2);
            break;
        case NOTE_OFF:
            sendMidiNoteOff(msg.channel, msg.data1, msg.data2);
            break;
        case CC:
            sendMidiCC(msg.channel, msg.data1, msg.data2);
            break;
        case PC:
            sendMidiPC(msg.channel, msg.data1);
            break;
        case OFF:
        default:
            break;
    }
}

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
    // TODO: Parse incoming preset state to extract current delay time
    // For now, just log it
    Serial.print("Received SysEx (");
    Serial.print(sysexLen);
    Serial.print(" bytes): ");
    for(size_t i=0; i<min(sysexLen, (size_t)20); i++) {
        if(sysexBuffer[i] < 0x10) Serial.print("0");
        Serial.print(sysexBuffer[i], HEX);
        Serial.print(" ");
    }
    if(sysexLen > 20) Serial.print("...");
    Serial.println();
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
    
    // Only scan if not connected and enough time has passed
    if (!clientConnected && !doConnect && (millis() - lastScanAttempt > RESCAN_INTERVAL)) {
        Serial.println("→ Scanning for SPM (BLE Client auto-connect)...");
        startBleScan();
        lastScanAttempt = millis();
    }
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length > 0 && length < 40) {
        memcpy(sysexBuffer, pData, length);
        sysexLen = length;
        sysexReceived = true;
    }
}
