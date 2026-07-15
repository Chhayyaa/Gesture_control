#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <esp_now.h>
#include <WiFi.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

uint8_t carMAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

#define CMD_STOP     0
#define CMD_FORWARD  1
#define CMD_BACKWARD 2

typedef struct struct_message {
  int command;
} struct_message;

struct_message myData;

// Thresholds — 
const float FORWARD_THRESHOLD  = 20.0;   // pitch tilts forward past this = move forward
const float BACKWARD_THRESHOLD = -20.0;  // pitch tilts back past this = move backward

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send Failed");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);

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
  memcpy(peerInfo.peer_addr, carMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);

  float pitch = event.orientation.z;   

  if (pitch > FORWARD_THRESHOLD) {
    myData.command = CMD_FORWARD;
  } else if (pitch < BACKWARD_THRESHOLD) {
    myData.command = CMD_BACKWARD;
  } else {
    myData.command = CMD_STOP;
  }

  esp_now_send(carMAC, (uint8_t *)&myData, sizeof(myData));
  delay(200);
}
