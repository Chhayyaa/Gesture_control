// Glove Side full integrated code
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <esp_now.h>
#include <WiFi.h>


#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

void setupIMU() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  if (!bno.begin()) {
    Serial.println("BNO055 not detected");
    while (1);
  }
  bno.setExtCrystalUse(true);
  Serial.println("BNO055 Ready");
}

struct GestureCommand {
  uint8_t direction;   // 0=STOP, 1=FORWARD, 2=BACKWARD, 3=LEFT, 4=RIGHT
  uint8_t speed;        // 0-255
};

// gesture logic
GestureCommand analyseGesture(float roll, float pitch) {
  GestureCommand cmd;
  const float DEADZONE = 8.0;
  const float MAX_ANGLE = 45.0;

  if (abs(pitch) < DEADZONE && abs(roll) < DEADZONE) {
    cmd.direction = 0;
    cmd.speed = 0;
    return cmd;
  }

  if (abs(pitch) > abs(roll)) {
    cmd.direction = (pitch > 0) ? 1 : 2;
    cmd.speed = map(constrain(abs(pitch), DEADZONE, MAX_ANGLE),
                     DEADZONE, MAX_ANGLE, 100, 255);
  } else {
    cmd.direction = (roll > 0) ? 4 : 3;
    cmd.speed = map(constrain(abs(roll), DEADZONE, MAX_ANGLE),
                     DEADZONE, MAX_ANGLE, 100, 255);
  }
  return cmd;
}

//ESP-NOW sender
uint8_t carMAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};  // apna car MAC daalo

void setupESPNOW() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, carMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("ESP-NOW Sender Ready");
}

void sendCommand(GestureCommand cmd) {
  esp_now_send(carMAC, (uint8_t *)&cmd, sizeof(cmd));
}

// main loop
void setup() {
  Serial.begin(115200);
  setupIMU();
  setupESPNOW();
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);
  float roll  = event.orientation.y;
  float pitch = event.orientation.z;

  GestureCommand cmd = analyseGesture(roll, pitch);
  sendCommand(cmd);

  delay(50);
}