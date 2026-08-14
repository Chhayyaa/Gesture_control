// CAR UNIT — ESP-NOW RECEIVER + 4 x CYTRON MD20A DRIVERS
#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

// Pin Definitions for 4 MD20A Drivers
#define FL_PWM 18
#define FL_DIR 19

#define FR_PWM 16
#define FR_DIR 17

#define RL_PWM 22
#define RL_DIR 23

#define RR_PWM 32
#define RR_DIR 33

const int freq = 20000;      // 20kHz PWM Frequency
const int resolution = 8;    // 8-bit resolution (0-255)

const float HARD_MAX_SPEED = 220.0;
const float accelFactor = 0.06;
const float brakeFactor = 0.18;
const float STOP_THRESHOLD = 3.0;

const unsigned long TIMEOUT = 1500;
unsigned long lastPacketTime = 0;

typedef struct struct_message {
  char command;
  float pitch;
  float roll;
  float yaw;
  float targetSpeed;
} struct_message;

struct_message incoming;

char currentCommand = 'S';
char requestedCommand = 'S';
float currentSpeed = 0;
float targetSpeed = 0;

void setDriver(int pwmPin, int dirPin, int speed, bool reverse) {
  digitalWrite(dirPin, reverse ? HIGH : LOW);
  ledcWrite(pwmPin, constrain(speed, 0, 255));
}

void stopMotor() {
  setDriver(FL_PWM, FL_DIR, 0, false);
  setDriver(FR_PWM, FR_DIR, 0, false);
  setDriver(RL_PWM, RL_DIR, 0, false);
  setDriver(RR_PWM, RR_DIR, 0, false);
}

void moveForward(int s) {
  setDriver(FL_PWM, FL_DIR, s, false);
  setDriver(FR_PWM, FR_DIR, s, false);
  setDriver(RL_PWM, RL_DIR, s, false);
  setDriver(RR_PWM, RR_DIR, s, false);
}

void moveBackward(int s) {
  setDriver(FL_PWM, FL_DIR, s, true);
  setDriver(FR_PWM, FR_DIR, s, true);
  setDriver(RL_PWM, RL_DIR, s, true);
  setDriver(RR_PWM, RR_DIR, s, true);
}

// Tank Turn Left: Left side reverse, Right side forward
void turnLeft(int s) {
  setDriver(FL_PWM, FL_DIR, s, true);
  setDriver(RL_PWM, RL_DIR, s, true);
  setDriver(FR_PWM, FR_DIR, s, false);
  setDriver(RR_PWM, RR_DIR, s, false);
}

// Tank Turn Right: Left side forward, Right side reverse
void turnRight(int s) {
  setDriver(FL_PWM, FL_DIR, s, false);
  setDriver(RL_PWM, RL_DIR, s, false);
  setDriver(FR_PWM, FR_DIR, s, true);
  setDriver(RR_PWM, RR_DIR, s, true);
}

void applyMotors() {
  int s = (int)currentSpeed;
  switch (currentCommand) {
    case 'F': moveForward(s);  break;
    case 'B': moveBackward(s); break;
    case 'L': turnLeft(s);     break;
    case 'R': turnRight(s);    break;
    case 'S':
    default:  stopMotor();     break;
  }
}

void updateMotion() {
  requestedCommand = incoming.command;
  float requestedSpeed = min(incoming.targetSpeed, HARD_MAX_SPEED);

  if (requestedCommand == currentCommand) {
    targetSpeed = (currentCommand == 'S') ? 0 : requestedSpeed;
  } else {
    // Direction change: smooth deceleration to zero first
    targetSpeed = 0;
    currentSpeed += (targetSpeed - currentSpeed) * brakeFactor;

    if (currentSpeed < STOP_THRESHOLD) {
      currentSpeed = 0;
      currentCommand = requestedCommand;
    } else {
      applyMotors();
      return;
    }
  }

  currentSpeed += (targetSpeed - currentSpeed) * accelFactor;
  currentSpeed = constrain(currentSpeed, 0.0f, HARD_MAX_SPEED);
  if (fabs(targetSpeed - currentSpeed) < 1.0f) currentSpeed = targetSpeed;

  applyMotors();
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len != sizeof(incoming)) return;
  memcpy(&incoming, data, sizeof(incoming));
  lastPacketTime = millis();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Initialize DIR Pins
  pinMode(FL_DIR, OUTPUT);
  pinMode(FR_DIR, OUTPUT);
  pinMode(RL_DIR, OUTPUT);
  pinMode(RR_DIR, OUTPUT);

  // Configure PWM Channels (Arduino ESP32 Core 3.x)
  ledcAttach(FL_PWM, freq, resolution);
  ledcAttach(FR_PWM, freq, resolution);
  ledcAttach(RL_PWM, freq, resolution);
  ledcAttach(RR_PWM, freq, resolution);

  stopMotor();

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(OnDataRecv);

  lastPacketTime = millis();
}

void loop() {
  if (millis() - lastPacketTime > TIMEOUT) {
    if (incoming.command != 'S') {
      incoming.command = 'S';
      incoming.targetSpeed = 0;
    }
  }
  updateMotion();
}