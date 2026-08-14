// ============================================================================
// GLOVE UNIT — ESP-NOW TRANSMITTER + BOSCH BNO055 IMU
// ============================================================================

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// --------------------------- ESP-NOW Config --------------------------------
// Broadcasts to all nearby receivers (or replace with Car Unit's specific MAC)
uint8_t broadcastAddress[] = {0xC0, 0xCD, 0xD6, 0xCA, 0x5D, 0x90};

// --------------------------- Struct Definition -----------------------------
// Must match the Car Unit's struct layout exactly
typedef struct struct_message {
  char command;
  float pitch;
  float roll;
  float yaw;
  float targetSpeed;
} struct_message;

struct_message outgoingPacket;
esp_now_peer_info_t peerInfo;

// --------------------------- IMU & Gesture Config --------------------------
// Default I2C address is 0x28 (or 0x29 if ADR pin is pulled HIGH)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// Gesture Deadzones & Scaling (in degrees)
const float TILT_DEADZONE = 12.0f;  // Flat hand zone where car stops ('S')
const float TILT_MAX      = 45.0f;  // Tilt angle corresponding to maximum speed

const float MIN_SPEED     = 80.0f;  // Starting torque speed to overcome inertia
const float HARD_MAX_SPEED = 220.0f; // Matches Car Unit speed cap

const unsigned long SEND_INTERVAL_MS = 20; // 50 Hz transmission rate
unsigned long lastSendTick = 0;

// --------------------------- Gesture Translation ---------------------------
void computeGestures(float pitch, float roll, float yaw) {
  outgoingPacket.pitch = pitch;
  outgoingPacket.roll  = roll;
  outgoingPacket.yaw   = yaw;

  float absPitch = fabs(pitch);
  float absRoll  = fabs(roll);

  // 1. Check Neutral / Flat Position -> Stop
  if (absPitch < TILT_DEADZONE && absRoll < TILT_DEADZONE) {
    outgoingPacket.command = 'S';
    outgoingPacket.targetSpeed = 0.0f;
    return;
  }

  // 2. Dominant Axis Selection
  if (absPitch >= absRoll) {
    // Pitch dominant: Forward / Backward
    float clampedAngle = constrain(absPitch, TILT_DEADZONE, TILT_MAX);
    outgoingPacket.targetSpeed = map(clampedAngle * 10, TILT_DEADZONE * 10, TILT_MAX * 10, MIN_SPEED, HARD_MAX_SPEED);

    // Depending on orientation/mounting:
    // Positive pitch = Tilt Forward ('F'), Negative pitch = Tilt Backward ('B')
    if (pitch > 0) {
      outgoingPacket.command = 'F';
    } else {
      outgoingPacket.command = 'B';
    }
  } else {
    // Roll dominant: Left / Right Turn
    float clampedAngle = constrain(absRoll, TILT_DEADZONE, TILT_MAX);
    outgoingPacket.targetSpeed = map(clampedAngle * 10, TILT_DEADZONE * 10, TILT_MAX * 10, MIN_SPEED, HARD_MAX_SPEED);

    // Positive roll = Tilt Right ('R'), Negative roll = Tilt Left ('L')
    if (roll > 0) {
      outgoingPacket.command = 'R';
    } else {
      outgoingPacket.command = 'L';
    }
  }
}

// --------------------------- Arduino Setup ---------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // 1. Initialize BNO055
  if (!bno.begin()) {
    Serial.println("Error: BNO055 not detected! Check wiring / I2C address (0x28 or 0x29).");
    while (1) {
      delay(100);
    }
  }
  Serial.println("BNO055 Initialized Successfully.");

  // Use external crystal for higher orientation precision
  bno.setExtCrystalUse(true);

  // 2. Initialize WiFi in Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // 3. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // 4. Register Peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    return;
  }
}

// --------------------------- Arduino Main Loop -----------------------------
void loop() {
  unsigned long now = millis();

  // Transmit at 50 Hz (every 20 ms)
  if (now - lastSendTick >= SEND_INTERVAL_MS) {
    lastSendTick = now;

    // Read fused Euler orientation vectors directly from BNO055 DSP
    // x = Yaw (0 to 360), y = Pitch (-180 to 180), z = Roll (-90 to 90)
    imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);

    float yaw   = euler.x();
    float pitch = euler.y();
    float roll  = euler.z();

    // Map tilt angles to car commands and proportional speeds
    computeGestures(pitch, roll, yaw);

    // Send packet
    esp_now_send(broadcastAddress, (uint8_t *)&outgoingPacket, sizeof(outgoingPacket));
  }

  // FreeRTOS yield to keep core cool
  delay(1);
}