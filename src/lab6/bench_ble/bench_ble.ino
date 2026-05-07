#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "00002a56-0000-1000-8000-00805f9b34fb"

BLEServer* pServer = nullptr;
BLECharacteristic* pChar = nullptr;
bool deviceConnected = false;
uint16_t mtu = 23;

#define TEST_SIZE (1024UL * 1024)
uint8_t chunk[512];
enum TestState { IDLE, RX_TEST, TX_TEST };
TestState state = IDLE;
uint32_t rxCount = 0;
unsigned long testStart = 0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) override { deviceConnected = false; state = IDLE; }
};

class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    String val = pChar->getValue();
    if (val.length() == 0) return;

    if (state == IDLE && val.length() == 2) {
      if (val == "TX") {
        state = TX_TEST; testStart = micros(); rxCount = 0;
        Serial.println("[BLE] TX Test started (ESP -> PC via Notify)...");
        
        uint32_t sent = 0;
        uint16_t paySize = mtu > 3 ? mtu - 3 : 20;
        memset(chunk, 0xAA, sizeof(chunk));
        
        while (sent < TEST_SIZE) {
          uint16_t toSend = (TEST_SIZE - sent) > paySize ? paySize : (TEST_SIZE - sent);
          pChar->setValue(chunk, toSend);
          pChar->notify();
          sent += toSend;
          delay(2); // Критично для BLE-стека
        }
        unsigned long elapsed = micros() - testStart;
        float sec = elapsed / 1e6f;
        Serial.printf("[BLE] TX Done: %.2f Kbps (%.2f KB/s)\n", (TEST_SIZE * 8.0 / sec) / 1000.0, TEST_SIZE / sec / 1024.0);
        state = IDLE;
      }
      else if (val == "RX") {
        state = RX_TEST; rxCount = 0; testStart = micros();
        Serial.println("[BLE] RX Test started (PC -> ESP via Write). Waiting 1MB...");
      }
    } 
    else if (state == RX_TEST) {
      rxCount += val.length();
      if (rxCount >= TEST_SIZE) {
        unsigned long elapsed = micros() - testStart;
        float sec = elapsed / 1e6f;
        Serial.printf("[BLE] RX Done: %.2f Kbps (%.2f KB/s)\n", (rxCount * 8.0 / sec) / 1000.0, rxCount / sec / 1024.0);
        state = IDLE; rxCount = 0;
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP_BLE_BENCH");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pChar->addDescriptor(new BLE2902());
  pChar->setCallbacks(new CharCallbacks());
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("✅ BLE GATT Ready. Connect & send 'TX' or 'RX' to UUID 0x2A56");
}

void loop() {
  if (deviceConnected && pServer) {
    // Обновляем MTU при наличии соединения
    static bool mtuRequested = false;
    if (!mtuRequested) {
      mtu = pServer->getPeerMTU(pServer->getConnId());
      Serial.printf("📦 Negotiated MTU: %d\n", mtu);
      mtuRequested = true;
    }
    yield(); // Важно для BLE-тасков
  }
  delay(10);
}