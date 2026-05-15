/**
 * BLE Client (Central) - ESP32_B
 * Compatible with ESP32 Arduino Core 3.x
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ===== Target server configuration =====
#define TARGET_SERVER_NAME    "ESP32_BLE_Server"
#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_RX     "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // Write
#define CHARACTERISTIC_TX     "beb5483e-36e1-4688-b7f5-ea07361b26a9"  // Read + Notify

// ===== Global variables =====
BLEAddress *pServerAddress = nullptr;
BLERemoteCharacteristic *pRemoteCharRx = nullptr;
BLERemoteCharacteristic *pRemoteCharTx = nullptr;
BLEClient *pClient = nullptr;  // Keep client reference global
bool doConnect = false;
bool connected = false;

// ===== Callback: Handle notifications (standalone function) =====
void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    
  Serial.print("[CLIENT] Received NOTIFY: ");
  for (size_t i = 0; i < length; i++) {
    Serial.print((char)pData[i]);
  }
  Serial.println();
}

// ===== Callback: Scan results =====
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (advertisedDevice.haveName() && 
        advertisedDevice.getName() == TARGET_SERVER_NAME) {
      Serial.println("[CLIENT] Found target server!");
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

// ===== Connect to server and discover services =====
bool connectToServer() {
  Serial.println("[CLIENT] Connecting to server...");
  
  pClient = BLEDevice::createClient();
  if (!pClient->connect(*pServerAddress)) {
    Serial.println("[CLIENT] Connection failed!");
    return false;
  }
  Serial.println("[CLIENT] Connected!");
  connected = true;

  // Get remote service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("[CLIENT] Service not found!");
    pClient->disconnect();
    return false;
  }
  Serial.println("[CLIENT] Service found");

  // Get RX characteristic (Write)
  pRemoteCharRx = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_RX));
  if (pRemoteCharRx == nullptr) {
    Serial.println("[CLIENT] RX characteristic not found!");
    return false;
  }
  Serial.println("[CLIENT] RX characteristic found");

  // Get TX characteristic (Read + Notify)
  pRemoteCharTx = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_TX));
  if (pRemoteCharTx == nullptr) {
    Serial.println("[CLIENT] TX characteristic not found!");
    return false;
  }
  Serial.println("[CLIENT] TX characteristic found");

  // Subscribe to notifications
  if (pRemoteCharTx->canNotify()) {
    pRemoteCharTx->registerForNotify(notifyCallback);
    Serial.println("[CLIENT] Subscribed to notifications");
  }

  return true;
}

// ===== Send data to server via RX characteristic =====
void sendDataToServer(const String& message) {
  if (connected && pRemoteCharRx && pRemoteCharRx->canWrite()) {
    pRemoteCharRx->writeValue(message.c_str());
    Serial.print("[CLIENT] Sent: ");
    Serial.println(message);
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[CLIENT] Starting BLE Client...");

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  
  Serial.println("[CLIENT] Starting scan...");
  pBLEScan->start(5, false);
}

// ===== Main loop =====
void loop() {
  if (doConnect) {
    doConnect = false;
    if (connectToServer()) {
      Serial.println("[CLIENT] Discovery complete. Ready for data exchange.");
    } else {
      Serial.println("[CLIENT] Failed to connect. Restarting scan...");
      connected = false;
      BLEDevice::getScan()->start(5, false);
    }
  }

  if (connected) {
    static uint32_t lastSend = 0;
    if (millis() - lastSend > 3000) {
      String msg = "Hello from Client @" + String(millis() / 1000) + "s";
      sendDataToServer(msg);
      lastSend = millis();
    }
  }

  delay(10);
}
