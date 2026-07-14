#include <esp_now.h>
#include <WiFi.h>

// L298N pins — 
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12
#define ENA 25   // PWM speed pin, left motor
#define ENB 33   // PWM speed pin, right motor

#define CMD_STOP     0
#define CMD_FORWARD  1
#define CMD_BACKWARD 2

typedef struct struct_message {
  int command;
} struct_message;

struct_message receivedData;

void motorStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void motorForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void motorBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  switch (receivedData.command) {
    case CMD_FORWARD:
      motorForward();
      break;
    case CMD_BACKWARD:
      motorBackward();
      break;
    case CMD_STOP:
    default:
      motorStop();
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  digitalWrite(ENA, HIGH);   
  digitalWrite(ENB, HIGH);

  motorStop();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // callback
}
