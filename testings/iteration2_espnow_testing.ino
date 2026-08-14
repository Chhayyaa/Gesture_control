#include <esp_now.h>
#include <WiFi.h>

uint8_t peerMAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};  // other board's MAC

typedef struct struct_message {
  char text[20];
} struct_message;

struct_message myData;
struct_message incoming;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send Failed");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming, incomingData, sizeof(incoming));
  Serial.print("Received: ");
  Serial.println(incoming.text);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  strcpy(myData.text, "Hello");
  esp_now_send(peerMAC, (uint8_t *)&myData, sizeof(myData));
  delay(1000);
}