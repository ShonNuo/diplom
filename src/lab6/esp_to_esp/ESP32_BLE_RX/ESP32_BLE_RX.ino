#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

uint32_t bytesReceived = 0;
uint32_t startTime = 0;
uint32_t lastDataTime = 0;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        if (startTime == 0) startTime = millis();
        bytesReceived += pCharacteristic->getValue().length();
        lastDataTime = millis();
    }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_BLE_RX");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE_NR // Higher throughput with no-response write
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("[RX] Waiting for BLE client connection...");
}

void loop() {
  if (startTime > 0 && (millis() - lastDataTime > 2000)) {
    float duration = (lastDataTime - startTime) / 1000.0;
    float speed = (bytesReceived / 1024.0) / duration;
    
    Serial.printf("[RX] Test complete. Received: %u bytes. Duration: %.2f sec. Speed: %.2f KB/s\n", 
                  bytesReceived, duration, speed);
    
    startTime = 0;
    bytesReceived = 0;
  }
  delay(10);
}