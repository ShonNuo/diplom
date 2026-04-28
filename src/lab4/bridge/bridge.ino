#include "BluetoothSerial.h"

// MAC-адрес ПК-Сервера (куда ESP будет подключаться как клиент)
#define SERVER_MAC "AA:BB:CC:DD:EE:FF"

BluetoothSerial SerialToClient; // Интерфейс для связи с ПК-Клиентом (Server role)
BluetoothSerial SerialToServer; // Интерфейс для связи с ПК-Сервером (Client role)

void setup() {
  Serial.begin(115200); // USB для отладки

  // 1. Запуск SPP Server для ПК-Клиента
  if (!SerialToClient.begin("ESP32-Bridge")) {
    Serial.println("Error starting Client-facing server");
    while(1);
  }
  Serial.println("Waiting for Client connection...");

  // 2. Подключение к ПК-Серверу как клиент
  // Примечание: connect() может блокировать выполнение, если сервер недоступен.
  // В продакшене лучше использовать неблокирующий подход с таймаутами.
  if (SerialToServer.connect(SERVER_MAC)) {
    Serial.println("Connected to Server PC");
  } else {
    Serial.println("Failed to connect to Server PC. Retrying in loop...");
  }
}

void loop() {
  // Логика моста:
  // Если есть данные от Клиента -> шлем Серверу
  if (SerialToClient.available()) {
    String cmd = SerialToClient.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      Serial.print("[Bridge] Client -> Server: ");
      Serial.println(cmd);
      SerialToServer.println(cmd);
    }
  }

  // Если есть ответ от Сервера -> шлем Клиенту
  if (SerialToServer.available()) {
    String resp = SerialToServer.readStringUntil('\n');
    // Внимание: readStringUntil может ждать таймаута, если нет '\n'
    // Для потоковой передачи лучше читать посимвольно или блоками
    Serial.print("[Bridge] Server -> Client: ");
    Serial.println(resp);
    SerialToClient.print(resp);
    SerialToClient.println(); // Добавляем перевод строки для клиента
  }

  delay(10);
}