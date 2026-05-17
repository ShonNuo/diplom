#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Unique UUIDs for service and data channels (you can generate your own)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Client Channel
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Server Channel

BLEServer *pServer = NULL;
BLECharacteristic *pClientChar = NULL;
BLECharacteristic *pServerChar = NULL;
bool deviceConnected = false;

// Handler class for data coming from the PC Client
class ClientCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("[BLE Bridge] Client -> Server: ");
            for (int i = 0; i < value.length(); i++) Serial.print(value[i]);
            Serial.println();
            
            // FORWARD DATA: send received client data to Server characteristic
            pServerChar->setValue(value);
            pServerChar->notify(); // Notify server about new data
        }
    }
};

// Handler class for data coming from the PC Server
class ServerCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("[BLE Bridge] Server -> Client: ");
            for (int i = 0; i < value.length(); i++) Serial.print(value[i]);
            Serial.println();
            
            // FORWARD DATA: send received server data to Client characteristic
            pClientChar->setValue(value);
            pClientChar->notify(); // Notify client about new data
        }
    }
};

// General connection status handler
class ServerConnectionCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("[+] New BLE connection detected");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("[-] Device disconnected. Restarting advertising...");
      pServer->startAdvertising(); // Make device visible again
    }
};

void setup() {
  Serial.begin(115200);

  // Initialize BLE device
  BLEDevice::init("ESP32-BLE-Bridge");

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerConnectionCallbacks());

  // Create Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 1. Create characteristic for CLIENT communication (Read/Write/Notify)
  pClientChar = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pClientChar->addDescriptor(new BLE2902());
  pClientChar->setCallbacks(new ClientCallbacks()); // Attach event handler

  // 2. Create characteristic for SERVER communication
  pServerChar = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pServerChar->addDescriptor(new BLE2902());
  pServerChar->setCallbacks(new ServerCallbacks()); // Attach event handler

  // Start service
  pService->start();

  // Start advertising so devices can discover ESP32
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Settings for iOS/Android compatibility
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("[+] BLE Bridge is ready. Name: 'ESP32-BLE-Bridge'");
}

void loop() {
  // Main loop remains completely empty!
  // All processing happens instantly in background via callbacks.
  delay(1000); 
}