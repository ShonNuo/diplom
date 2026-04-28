#include "BluetoothSerial.h"

// Check if Bluetooth Classic is supported
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

BluetoothSerial SerialBT;

// Command constants
const String CMD_HELP = "HELP";
const String CMD_LED_ON = "LED_ON";
const String CMD_LED_OFF = "LED_OFF";
const String CMD_STATUS = "STATUS";

// Built-in LED pin (GPIO 2 for most ESP32 boards)
const int LED_PIN = 2;
// ADC pin for reading analog values (e.g., GPIO 34)
const int ADC_PIN = 34; 

void setup() {
  // Initialize USB serial for debugging
  Serial.begin(115200);
  
  // Configure LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // HIGH = OFF for most ESP32 boards

  // Initialize Bluetooth with device name
  // Ensure the name is unique in your network
  if (!SerialBT.begin("ESP32-SPP-Terminal")) {
    Serial.println("Bluetooth initialization failed!");
    while (true);
  }
  
  Serial.println("Bluetooth SPP server started. Waiting for connection...");
  Serial.println("Send 'HELP' for a list of commands.");
}

void loop() {
  // Check if data is available from the connected Bluetooth device
  if (SerialBT.available()) {
    // Read string until newline ('\n') or carriage return ('\r')
    String command = SerialBT.readStringUntil('\n');
    
    // Remove extra spaces and carriage returns ('\r')
    command.trim();
    
    // If the command is not empty, process it
    if (command.length() > 0) {
      Serial.print("Received BT command: ");
      Serial.println(command);
      
      processCommand(command);
    }
  }
  
  // Small delay for stability
  delay(10);
}

/**
 * Command processing function
 */
void processCommand(String cmd) {
  // Convert command to uppercase for consistency
  cmd.toUpperCase();

  if (cmd == CMD_HELP) {
    String response = "Available commands:\n";
    response += "LED_ON   - Turn on LED\n";
    response += "LED_OFF  - Turn off LED\n";
    response += "STATUS   - Show LED status and ADC value\n";
    response += "HELP     - Show this message\n";
    SerialBT.print(response);
  } 
  else if (cmd == CMD_LED_ON) {
    digitalWrite(LED_PIN, LOW); // LOW turns ON the LED on most ESP32s
    SerialBT.println("OK: LED ON");
  } 
  else if (cmd == CMD_LED_OFF) {
    digitalWrite(LED_PIN, HIGH); // HIGH turns OFF the LED
    SerialBT.println("OK: LED OFF");
  } 
  else if (cmd == CMD_STATUS) {
    int adcValue = analogRead(ADC_PIN);
    bool ledState = digitalRead(LED_PIN);
    
    String response = "STATUS:\n";
    // Invert logic for display: LOW means ON
    response += "LED: " + String(ledState ? "OFF" : "ON") + "\n";
    response += "ADC (GPIO34): " + String(adcValue) + "\n";
    SerialBT.print(response);
  } 
  else {
    SerialBT.println("ERROR: Unknown command. Send HELP.");
  }
}