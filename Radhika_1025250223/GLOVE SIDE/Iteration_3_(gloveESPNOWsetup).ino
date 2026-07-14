// GestureCommand is the structure that we want to send via ESP-NOW
//Iteration 3 (Glove Side ESP-NOW Code)
#include <esp_now.h>
#include <WiFi.h>
uint8_t carMAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};  // Here the adress of the car ESP32 will be stored to pair them 
void setupESPNOW() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {          // initialize the ESP-NOW
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};  // Here the details of the reciever will be stored
  memcpy(peerInfo.peer_addr, carMAC, 6);
  peerInfo.channel = 0;        // Both car and glove should at the ssame channel for the communication
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("ESP-NOW Sender Ready");
}

void sendCommand(GestureCommand cmd) {   // if above steps get successfulthe the command will be send 
  esp_now_send(carMAC, (uint8_t *)&cmd, sizeof(cmd));
}


// Now all the functions of the glove side is ready so to call them we required the main loop which will be next iteration