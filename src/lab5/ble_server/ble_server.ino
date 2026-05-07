/**
 * BLE Server (Peripheral) - ESP32_A
 * 
 * This sketch implements a BLE GATT server with a custom service.
 * It exposes two characteristics:
 *   - CHAR_RX: Write-only (receives data from Client)
 *   - CHAR_TX: Read + Notify (sends data to Client)
 * 
 * When data is written to CHAR_RX, the server echoes it back via CHAR_TX notification.
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ===== Custom Service and Characteristic UUIDs =====
// Generate your own UUIDs for production use!
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_RX   "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // Write (Client -> Server)
#define CHARACTERISTIC_TX   "beb5483e-36e1-4688-b7f5-ea07361b26a9"  // Read + Notify (Server -> Client)

// ===== Global variables =====
BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// ===== Server callbacks =====
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[SERVER] Client connected");
    // Allow reconnection after disconnect
    BLEDevice::startAdvertising();
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[SERVER] Client disconnected");
  }
};

// ===== Characteristic callbacks (for RX characteristic) =====
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue();
    
    if (rxValue.length() > 0) {
      Serial.print("[SERVER] Received: ");
      for (size_t i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      Serial.println();

      // Echo back via TX characteristic (Notify)
      String ack = "ACK: " + String(rxValue.c_str());
      pTxCharacteristic->setValue(ack.c_str());
      pTxCharacteristic->notify();
      Serial.println("[SERVER] Sent ACK via NOTIFY");
    }
  }
};

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for Serial Monitor
  Serial.println("\n[SERVER] Starting BLE Server...");

  // Initialize BLE with device name
  BLEDevice::init("ESP32_BLE_Server");

  // Create server and set callbacks
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create custom service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX characteristic (Read + Notify)
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_TX,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Add CCCD descriptor (required for Notify/Indicate) - FIXED for Core 3.x
  BLEDescriptor *pccd = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
  pTxCharacteristic->addDescriptor(pccd);

  // Create RX characteristic (Write)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start service and advertising
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[SERVER] Advertising started. Waiting for connection...");
}

// ===== Main loop =====
void loop() {
  // Handle reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);  // Give BLE stack time to reset
    pServer->startAdvertising();
    Serial.println("[SERVER] Restarted advertising");
    oldDeviceConnected = deviceConnected;
  }

  // Handle new connection
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("[SERVER] New client connected");
    oldDeviceConnected = deviceConnected;
  }

  // Optional: Send periodic data via NOTIFY (uncomment if needed)
  /*
  if (deviceConnected) {
    static uint32_t lastSend = 0;
    if (millis() - lastSend > 5000) {
      String msg = "Server heartbeat: " + String(millis() / 1000);
      pTxCharacteristic->setValue(msg.c_str());
      pTxCharacteristic->notify();
      Serial.println("[SERVER] Sent periodic message");
      lastSend = millis();
    }
  }
  */

  delay(10);  // Small delay for stability
}
