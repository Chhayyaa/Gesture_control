#include <esp_now.h>
#include <WiFi.h>
struct GestureCommand {     // As struct on both glove and car side should be identical
  uint8_t direction;
  uint8_t speed;
};
// volatile keyword is used to prevent the compiler to use old cache value
volatile GestureCommand receivedCmd = {0, 0};  // Global variable to recieve the command which is initialise with value 0
volatile unsigned long lastPacketTime = 0;  // it is for safety check like if the new data packet not recieved means signal get lost and car can get stop 

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {  // call back function
  memcpy((void*)&receivedCmd, incomingData, sizeof(receivedCmd));  // unpack the data
  lastPacketTime = millis();  // store the timestamp when the last packet was recieved
}

void setupESPNOWReceiver() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("ESP-NOW Receiver Ready");
}