#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEClient* pClient;
const String RX_ADDR = "88:57:21:6A:0B:8A";  // Replace with your MAC address

bool connectToServer() {
    Serial.println("[TX] Connecting to server...");
    pClient = BLEDevice::createClient();
    
    pClient->setMTU(517); 
    
    if (!pClient->connect(BLEAddress(RX_ADDR))) return false;

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) return false;

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    return (pRemoteCharacteristic != nullptr);
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_BLE_TX");
  Serial.println("[TX] Starting...");
}

void loop() {
  if (connectToServer()) {
    Serial.println("[TX] Connected! Starting test (10 sec)...");
    
    uint8_t payload[490];
    memset(payload, 0xAA, sizeof(payload));
    
    uint32_t start = millis();
    uint32_t sent = 0;

    while (millis() - start < 10000) {
      // афдыу = Write Without Response (maximum throughput)
      pRemoteCharacteristic->writeValue(payload, sizeof(payload), true);
      sent += sizeof(payload);
      yield();
    }

    Serial.printf("[TX] Sent: %u bytes in 10 sec.\n", sent);
    pClient->disconnect();
  } else {
    Serial.println("[TX] Failed to connect.");
  }
  delay(5000);
}