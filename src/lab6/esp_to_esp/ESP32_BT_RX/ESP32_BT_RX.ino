#include "BluetoothSerial.h"
#pragma GCC optimize("O3")

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  if (!SerialBT.begin("ESP32_BT_RX")) {
    Serial.println("BT init failed");
    while(1);
  }
  Serial.println("[RX] Waiting...");
}

void loop() {
  if (!SerialBT.connected()) {
    delay(50);
    return;
  }
  
  Serial.println("[RX] Receiving...");
  
  const size_t BUF_SIZE = 2048;
  uint8_t buffer[BUF_SIZE];
  uint32_t bytes_rx = 0;
  uint32_t start_ms = 0;
  uint32_t last_ms = 0;
  const uint32_t TIMEOUT_MS = 1500;

  while (SerialBT.connected()) {
    int avail = SerialBT.available();
    if (avail > 0) {
      if (start_ms == 0) start_ms = millis();
      
      int to_read = (avail > BUF_SIZE) ? BUF_SIZE : avail;
      SerialBT.readBytes(buffer, to_read);
      
      bytes_rx += to_read;
      last_ms = millis();
    }
    
    if (start_ms > 0 && (millis() - last_ms > TIMEOUT_MS)) break;
  }
  
  if (bytes_rx > 0) {
    uint32_t duration_ms = last_ms - start_ms;
    float kbps = (bytes_rx * 8.0) / (duration_ms * 1.0);
    float kBps = bytes_rx / (duration_ms * 1.0);
    
    Serial.printf("[RX] %u bytes | %u ms | %.1f kbps | %.1f KB/s\n", 
                  bytes_rx, duration_ms, kbps, kBps);
  }
  
  delay(1000);
}