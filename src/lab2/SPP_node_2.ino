/*
 * ESP32 Bluetooth Serial - Node 2
 * Connects to PC via /dev/rfcomm1
 */

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#define LED_PIN 2          // Built-in LED
#define BOARD_ID "NODE2"   // Board Identifier

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Indication of start
  
  Serial.begin(115200);
  SerialBT.begin("ESP32-Node2");  // Bluetooth device name (UNIQUE!)
  
  Serial.println();
  Serial.println("================================"); 
  Serial.println("[NODE2] Bluetooth Serial Ready!");
  Serial.println("[NODE2] Device Name: ESP32-Node2");
  Serial.println("[NODE2] Waiting for connection...");
  Serial.println("================================");
  
  // Blink LED to indicate successful startup
  digitalWrite(LED_PIN, LOW);
  delay(200);
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  // === Data from PC (USB) → to Bluetooth ===
  if (Serial.available()) {
    char c = Serial.read();
    SerialBT.write(c);
    
    // Echo to serial monitor for debugging
    Serial.print("[USB→BT] ");
    Serial.println(c);
  }
  
  // === Data from Bluetooth → to PC (USB) ===
  if (SerialBT.available()) {
    String msg = SerialBT.readStringUntil('\n');
    msg.trim();  // Remove whitespace and newlines
    
    if (msg.length() > 0) {
      // Blink LED upon receiving data
      digitalWrite(LED_PIN, LOW);
      delay(50);
      digitalWrite(LED_PIN, HIGH);
      
      // Variable to store the response status/message
      String response = "YOUR MESSAGE";
      
      // Simple command processing
      if (msg.equalsIgnoreCase("PING")) {
        response = "PONG!";
      }
      else if (msg.equalsIgnoreCase("LED ON")) {
        digitalWrite(LED_PIN, LOW);  // LED active LOW
        response = "LED ON";
      }
      else if (msg.equalsIgnoreCase("LED OFF")) {
        digitalWrite(LED_PIN, HIGH);
        response = "LED OFF";
      }
      
      // Send debug info to PC via USB Serial
      Serial.print("[NODE2←BT] ");
      Serial.println(msg);
      
      // Send unified ACK response back via Bluetooth
      // Format: [ACK-NODE2] Received: <msg> | Status: <response>
      SerialBT.printf("[ACK-NODE2] Received: %s | Status: %s\n", msg.c_str(), response.c_str());
    }
  }
  
  delay(10);  // Small pause for stability
}