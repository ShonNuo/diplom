#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Run `make menuconfig` or check Arduino core settings.
#endif

BluetoothSerial SerialBT;

#define TEST_SIZE (1024UL * 1024 * 10) // 10 MB
#define CHUNK_SIZE 64 * 1024
uint8_t chunk[CHUNK_SIZE];

enum TestState { IDLE, RX_TEST, TX_TEST };
TestState state = IDLE;
uint32_t rxCount = 0;
uint32_t sentCount = 0;
unsigned long testStart = 0;
uint8_t cmdBuf[3];
uint8_t cmdIdx = 0;

void startTX() {
  state = TX_TEST;
  sentCount = 0;
  testStart = micros();
  Serial.println("[Classic] TX Test started (ESP -> PC)...");
  
  while (sentCount < TEST_SIZE) { 
    uint32_t remaining = TEST_SIZE - sentCount;
    uint32_t toSend = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
    
    // Ждём, пока буфер освободится (неблокирующая отправка)
    while (SerialBT.availableForWrite() < toSend) {
      vTaskDelay(1);  // Уступаем управление стеку
    }
    Serial.println("Hallo");
    size_t written = SerialBT.write(chunk, toSend);
    if (written > 0) {
      sentCount += written;
    }
  }

  SerialBT.flush();

  unsigned long elapsed = micros() - testStart;
  float sec = elapsed / 1000000.0;
  Serial.printf("[Classic] TX Done: %.2f Kbps (%.2f KB/s)\n", (TEST_SIZE * 8.0 / sec) / 1000.0, TEST_SIZE / sec / 1024.0);
  state = IDLE;
}

void loop() {
  if (!SerialBT.hasClient()) {
    if (state != IDLE) { state = IDLE; rxCount = 0; sentCount = 0; cmdIdx = 0; }
    delay(100);
    return;
  }

  while (SerialBT.available()) {
    uint8_t c = SerialBT.read();
    if (state == IDLE) {
      if (c == 'T' || c == 'R') cmdIdx = 0; // Сброс при новом старте
      cmdBuf[cmdIdx++] = c;
      
      if (cmdIdx >= 2) {
        if (cmdBuf[0] == 'T' && cmdBuf[1] == 'X') {
          startTX();
        } else if (cmdBuf[0] == 'R' && cmdBuf[1] == 'X') {
          state = RX_TEST; rxCount = 0; testStart = micros();
          Serial.println("[Classic] RX Test started. Waiting 1MB...");
        }
        cmdIdx = 0;
      }
    } 
    else if (state == RX_TEST) {
      rxCount++;
      if (rxCount >= TEST_SIZE) {
        unsigned long elapsed = micros() - testStart;
        float sec = elapsed / 1000000.0;
        Serial.printf("[Classic] RX Done: %.2f Kbps (%.2f KB/s)\n", (rxCount * 8.0 / sec) / 1000.0, rxCount / sec / 1024.0);
        state = IDLE; rxCount = 0; cmdIdx = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  memset(chunk, 0xAA, CHUNK_SIZE);
  Serial.setDebugOutput(false); // Отключает вывод в UART во время работы
  SerialBT.begin("ESP_SPP_BENCH");
  Serial.println("✅ Classic BT SPP Ready. Pair & send 'TX' or 'RX' via Bluetooth.");
}