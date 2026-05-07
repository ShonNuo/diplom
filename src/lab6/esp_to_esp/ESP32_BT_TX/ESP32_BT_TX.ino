#include "BluetoothSerial.h"
#pragma GCC optimize("O3")

BluetoothSerial SerialBT;
uint8_t address[6] = {0x88, 0x57, 0x21, 0x6A, 0x0B, 0x8A};  // Your addr

void setup() {
  Serial.begin(115200);
  if (!SerialBT.begin("ESP32_BT_TX", true)) {
    Serial.println("BT init failed");
    while(1);
  }
  delay(500);
  
  Serial.print("Connecting...");
  while (!SerialBT.connect(address)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" OK");
}

void loop() {
  if (!SerialBT.connected()) {
    Serial.println("Disconnected. Restarting...");
    ESP.restart();
  }

  const uint32_t TEST_MS = 10000;
  const size_t CHUNK = 2048;
  uint8_t payload[CHUNK];
  for (size_t i = 0; i < CHUNK; i++) payload[i] = 0xA5;
  
  uint32_t bytes_sent = 0;
  uint32_t end_time = millis() + TEST_MS;
  
  while (millis() < end_time && SerialBT.connected()) {
    bytes_sent += SerialBT.write(payload, CHUNK);
  }
  
  uint32_t duration_ms = millis() - (end_time - TEST_MS);
  float kbps = (bytes_sent * 8.0) / (duration_ms * 1.0);
  float kBps = bytes_sent / (duration_ms * 1.0);
  
  Serial.printf("\n[TX] %u bytes | %u ms | %.1f kbps | %.1f KB/s\n", 
                bytes_sent, duration_ms, kbps, kBps);
  
  delay(3000);
}