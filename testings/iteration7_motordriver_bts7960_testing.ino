// CAR UNIT — ESP-NOW RECEIVER + BTS7960 MOTOR CONTROL

#include <WiFi.h>
#include <esp_now.h>
#include <math.h>

// BTS7960 Motor Driver Pin Configuration — ONE DRIVER PER SIDE
// Left side: both left motors shorted together, driven by a single BTS7960
// Right side: both right motors shorted together, driven by a single BTS7960
//
// ASSUMPTION: first pin you gave = RPWM, second = LPWM, for each side.
// If a side spins backward from what you expect during testing, swap that
// side's two pin numbers below — that's the fix, not a logic bug.
#define LEFT_RPWM  26
#define LEFT_LPWM  25
#define RIGHT_RPWM 18
#define RIGHT_LPWM 19

#define STOP      0
#define FORWARD   1
#define BACKWARD  2
#define LEFT      3
#define RIGHT     4

// PWM Configuration
const int freq = 5000;
const int resolution = 8;


typedef struct {
  uint8_t cmd;
} espnow_packet_t;

espnow_packet_t data;

unsigned long lastPacketTime = 0;
const unsigned long TIMEOUT = 1500;

float currentSpeed   = 0;
float targetSpeed    = 0;
const float maxSpeed  = 180;
const float turnSpeed = 120;
const float accelFactor = 0.05;
const float brakeFactor = 0.20;
uint8_t currentCommand   = STOP;
uint8_t requestedCommand = STOP;

// ESP-NOW Receive Callback 
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(data)) return;
  memcpy(&data, incomingData, sizeof(data));
  lastPacketTime = millis();

  Serial.print("Received Command: ");
  Serial.println(data.cmd);
}


void updateMotion() {
  requestedCommand = data.cmd;

  if (requestedCommand == currentCommand) {
    if (currentCommand == STOP) {
      targetSpeed = 0;
    } else if (currentCommand == LEFT || currentCommand == RIGHT) {
      targetSpeed = turnSpeed;
    } else {
      targetSpeed = maxSpeed;
    }
  } else {
    targetSpeed = 0;
    currentSpeed += (targetSpeed - currentSpeed) * brakeFactor;

    if (currentSpeed < 1) {
      currentSpeed = 0;
      currentCommand = requestedCommand;
    } else {
      return;
    }
  }

  currentSpeed += (targetSpeed - currentSpeed) * accelFactor;

  if (currentSpeed < 0) currentSpeed = 0;
  if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
  if (fabs(targetSpeed - currentSpeed) < 1) currentSpeed = targetSpeed;
}

void setup() {
  Serial.begin(115200);

  ledcAttach(LEFT_RPWM, freq, resolution);
  ledcAttach(LEFT_LPWM, freq, resolution);
  ledcAttach(RIGHT_RPWM, freq, resolution);
  ledcAttach(RIGHT_LPWM, freq, resolution);

  stopCar();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    while (1);
  }
  esp_now_register_recv_cb(OnDataRecv);

  lastPacketTime = millis();
  Serial.println("Car unit ready — waiting for glove commands.");
}

void loop() {
  if (millis() - lastPacketTime > TIMEOUT) {
    if (data.cmd != STOP) {
      Serial.println("Signal lost — forcing STOP");
      data.cmd = STOP;
    }
  }

  updateMotion();

  switch (currentCommand) {
    case FORWARD:  moveForward();  break;
    case BACKWARD: moveBackward(); break;
    case LEFT:     turnLeft();     break;
    case RIGHT:    turnRight();    break;
    default:       stopCar();      break;
  }
}

void stopCar() {
  ledcWrite(LEFT_RPWM, 0);  ledcWrite(LEFT_LPWM, 0);
  ledcWrite(RIGHT_RPWM, 0); ledcWrite(RIGHT_LPWM, 0);
}

void moveForward() {
  
  ledcWrite(LEFT_RPWM, (int)currentSpeed);  ledcWrite(LEFT_LPWM, 0);
  ledcWrite(RIGHT_RPWM, (int)currentSpeed); ledcWrite(RIGHT_LPWM, 0);
}

void moveBackward() {
  ledcWrite(LEFT_RPWM, 0);  ledcWrite(LEFT_LPWM, (int)currentSpeed);
  ledcWrite(RIGHT_RPWM, 0); ledcWrite(RIGHT_LPWM, (int)currentSpeed);
}


void turnLeft() {
  ledcWrite(LEFT_RPWM, 0); ledcWrite(LEFT_LPWM, (int)currentSpeed);   // left side ACW (reverse)
  ledcWrite(RIGHT_RPWM, (int)currentSpeed); ledcWrite(RIGHT_LPWM, 0); // right side CW (forward)
}

void turnRight() {
  ledcWrite(LEFT_RPWM, (int)currentSpeed); ledcWrite(LEFT_LPWM, 0);   // left side CW (forward)
  ledcWrite(RIGHT_RPWM, 0); ledcWrite(RIGHT_LPWM, (int)currentSpeed); // right side ACW (reverse)
}