#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <esp_now.h>
#include <WiFi.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
uint8_t peerMAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

typedef struct struct_message {
  float x;
  float y;
  float z;
} struct_message;

struct_message myData;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send Failed");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);   // adjust SDA/SCL for your board

  if (!bno.begin()) {
    Serial.println("BNO055 not detected");
    while (1);
  }
  bno.setExtCrystalUse(true);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);

  myData.x = event.orientation.x;
  myData.y = event.orientation.y;
  myData.z = event.orientation.z;

  esp_now_send(peerMAC, (uint8_t *)&myData, sizeof(myData));
  delay(200);
}