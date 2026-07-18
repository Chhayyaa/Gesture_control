//CAR-SIDE INTEGRATED CODE
#include <esp_now.h>
#include <WiFi.h>
// direction: 1=FORWARD, 2=BACKWARD, 3=LEFT, 4=RIGHT, 0=STOP
struct GestureCommand {
  uint8_t direction;
  uint8_t speed;
}; 

volatile GestureCommand receivedCmd = {0, 0};   
volatile unsigned long lastPacketTime = 0;      // for safety check (signal loss)

// MOTOR DRIVER PINS — BTS7960 (one module per side)
// forward PWM channel, LPWM = reverse PWM channel
// R_EN/L_EN = enable pins, tie both HIGH to keep driver always active
#define LEFT_RPWM   25   // Left motor - forward PWM
#define LEFT_LPWM   26   // Left motor - reverse PWM
#define LEFT_R_EN   27   // Left driver enable (forward side)
#define LEFT_L_EN   14   // Left driver enable (reverse side)

#define RIGHT_RPWM  32   // Right motor - forward PWM
#define RIGHT_LPWM  33   // Right motor - reverse PWM
#define RIGHT_R_EN  12   // Right driver enable (forward side)
#define RIGHT_L_EN  13   // Right driver enable (reverse side)

// FIXED PWM LEVELS (speed binning)
#define PWM_STOP   0
#define PWM_SLOW   130
#define PWM_MEDIUM 190
#define PWM_FAST   255

// Turn dominant-wheel (example to slow the single wheel while turning)
#define TURN_FACTOR 0.4

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy((void*)&receivedCmd, incomingData, sizeof(receivedCmd));
  lastPacketTime = millis();
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

void safetyCheck() {
  if (millis() - lastPacketTime > 300) {
    receivedCmd.direction = 0;
    receivedCmd.speed = 0;
  }
}

// MOTOR SETUP — BTS7960
// R_EN and L_EN set HIGH once and left alone (always enabled),
 void setupMotors() {
  pinMode(LEFT_R_EN, OUTPUT);  pinMode(LEFT_L_EN, OUTPUT);
  pinMode(RIGHT_R_EN, OUTPUT); pinMode(RIGHT_L_EN, OUTPUT);

  digitalWrite(LEFT_R_EN, HIGH);  digitalWrite(LEFT_L_EN, HIGH);
  digitalWrite(RIGHT_R_EN, HIGH); digitalWrite(RIGHT_L_EN, HIGH);

  ledcAttach(LEFT_RPWM, 5000, 8);    // 5kHz, 8-bit resolution
  ledcAttach(LEFT_LPWM, 5000, 8);
  ledcAttach(RIGHT_RPWM, 5000, 8);
  ledcAttach(RIGHT_LPWM, 5000, 8);
}

// rpwmPin = forward channel, lpwmPin = reverse channel for that side
// Only ONE of the two channels is driven at a time, other stays 0
void setMotor(int rpwmPin, int lpwmPin, int speed, bool forward) {
  if (forward) {
    ledcWrite(rpwmPin, speed);
    ledcWrite(lpwmPin, 0);
  } else {
    ledcWrite(rpwmPin, 0);
    ledcWrite(lpwmPin, speed);
  }
}

void stopMotors() {
  ledcWrite(LEFT_RPWM, 0);  ledcWrite(LEFT_LPWM, 0);
  ledcWrite(RIGHT_RPWM, 0); ledcWrite(RIGHT_LPWM, 0);
}
// Raw speed (0-255 from glove) -> Fixed PWM level
// Hysteresis prevents jitter/flicker when value hovers near a
int getBinnedPWM(uint8_t rawSpeedValue) {
  static int lastPWM = PWM_STOP;   // remembers state between calls

  // Rising (speed increasing)
  if (lastPWM == PWM_STOP && rawSpeedValue > 65) {
    lastPWM = PWM_SLOW;
  } else if (lastPWM == PWM_SLOW && rawSpeedValue > 125) {
    lastPWM = PWM_MEDIUM;
  } else if (lastPWM == PWM_MEDIUM && rawSpeedValue > 195) {
    lastPWM = PWM_FAST;
  }
  // Falling (speed decreasing) — lower threshold than rising
  else if (lastPWM == PWM_FAST && rawSpeedValue < 185) {
    lastPWM = PWM_MEDIUM;
  } else if (lastPWM == PWM_MEDIUM && rawSpeedValue < 115) {
    lastPWM = PWM_SLOW;
  } else if (lastPWM == PWM_SLOW && rawSpeedValue < 55) {
    lastPWM = PWM_STOP;
  }

  return lastPWM;
}

// direction: 1=FORWARD, 2=BACKWARD, 3=LEFT, 4=RIGHT, 0=STOP
void executeCommand(GestureCommand cmd) {
  int pwmToApply = getBinnedPWM(cmd.speed);   // <-- binning step

  switch (cmd.direction) {
    case 0:   // STOP
      stopMotors();
      break;

    case 1:   // FORWARD
      setMotor(LEFT_RPWM, LEFT_LPWM, pwmToApply, true);
      setMotor(RIGHT_RPWM, RIGHT_LPWM, pwmToApply, true);
      break;

    case 2:   // BACKWARD
      setMotor(LEFT_RPWM, LEFT_LPWM, pwmToApply, false);
      setMotor(RIGHT_RPWM, RIGHT_LPWM, pwmToApply, false);
      break;

    case 3:   // LEFT turn — right wheel dominant, left wheel slower
      setMotor(LEFT_RPWM, LEFT_LPWM, pwmToApply * TURN_FACTOR, false);
      setMotor(RIGHT_RPWM, RIGHT_LPWM, pwmToApply, true);
      break;

    case 4:   // RIGHT turn — left wheel dominant, right wheel slower
      setMotor(LEFT_RPWM, LEFT_LPWM, pwmToApply, true);
      setMotor(RIGHT_RPWM, RIGHT_LPWM, pwmToApply * TURN_FACTOR, false);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  setupMotors();
  setupESPNOWReceiver();
}

void loop() {
  safetyCheck();
  GestureCommand cmd = receivedCmd;
  executeCommand(cmd);
  delay(20);
}