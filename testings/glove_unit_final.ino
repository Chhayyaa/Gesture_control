// GLOVE UNIT — ESP-NOW TRANSMITTER (BNO055 + Proportional Speed + Safety Cutoff)
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <esp_now.h>
#include <WiFi.h>

#define LIMIT_SWITCH 25

// ---- Configuration Parameters ----
#define THRESHOLD_DEG    20.0f
#define MAX_ANGLE_DEG    55.0f
#define HYSTERESIS_DEG   5.0f
#define MIN_SPEED        70.0f
#define MAX_SPEED        220.0f
#define SAMPLE_MS        30
#define HEARTBEAT_MS     1000

// Update with actual Car Unit ESP32 MAC Address
uint8_t receiverMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

typedef struct struct_message {
  char command;
  float pitch;
  float roll;
  float yaw;
  float targetSpeed;
} struct_message;

struct_message packet;

char lastSentCmd = 'S';
unsigned long lastSampleTime = 0;
unsigned long lastHeartbeatTime = 0;

bool limitSwitchPressed(int pin) {
  return digitalRead(pin) == LOW; // Active LOW with INPUT_PULLUP
}

float computeSpeed(float angle) {
  float mag = fabs(angle);
  if (mag < THRESHOLD_DEG) return 0;
  float clamped = min(mag, MAX_ANGLE_DEG);
  float t = (clamped - THRESHOLD_DEG) / (MAX_ANGLE_DEG - THRESHOLD_DEG);
  return MIN_SPEED + t * (MAX_SPEED - MIN_SPEED);
}

#if ESP_IDF_VERSION_MAJOR >= 5
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "TX OK" : "TX FAIL");
}

void setup() {
  Serial.begin(115200);
  pinMode(LIMIT_SWITCH, INPUT_PULLUP);

  Wire.begin(21, 22);
  if (!bno.begin()) {
    Serial.println("BNO055 Error!");
    while (1) delay(1000);
  }
  bno.setExtCrystalUse(true);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void sendCommand(char cmd, float pitch, float roll, float yaw, float speed) {
  packet.command = cmd;
  packet.pitch = pitch;
  packet.roll = roll;
  packet.yaw = yaw;
  packet.targetSpeed = speed;
  esp_now_send(receiverMac, (uint8_t *)&packet, sizeof(packet));
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_MS) return;
  lastSampleTime = now;

  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float yaw   = euler.x();
  float roll  = euler.y();
  float pitch = euler.z();

  char currentCmd;
  float speed = 0;

  if (limitSwitchPressed(LIMIT_SWITCH)) {
    currentCmd = 'S';
    speed = 0;
  } else {
    float pThresh = (lastSentCmd == 'F' || lastSentCmd == 'B') ? (THRESHOLD_DEG - HYSTERESIS_DEG) : THRESHOLD_DEG;
    float rThresh = (lastSentCmd == 'L' || lastSentCmd == 'R') ? (THRESHOLD_DEG - HYSTERESIS_DEG) : THRESHOLD_DEG;

    bool pitchActive = fabs(pitch) >= pThresh;
    bool rollActive  = fabs(roll)  >= rThresh;

    if (pitchActive && (!rollActive || fabs(pitch) >= fabs(roll))) {
      currentCmd = (pitch > 0) ? 'F' : 'B';
      speed = computeSpeed(pitch);
    } else if (rollActive) {
      currentCmd = (roll > 0) ? 'R' : 'L';
      speed = computeSpeed(roll);
    } else {
      currentCmd = 'S';
      speed = 0;
    }
  }

  if (currentCmd != lastSentCmd) {
    sendCommand(currentCmd, pitch, roll, yaw, speed);
    lastSentCmd = currentCmd;
    lastHeartbeatTime = now;
  } else if (now - lastHeartbeatTime >= HEARTBEAT_MS) {
    sendCommand(currentCmd, pitch, roll, yaw, speed);
    lastHeartbeatTime = now;
  }
}