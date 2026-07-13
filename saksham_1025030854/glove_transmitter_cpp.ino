/*
  GLOVE UNIT (TRANSMITTER) -- C++ OOP VERSION
  Hardware : ESP32-S3 + BNO055 (I2C)
  Comm     : ESP-NOW

  Same logic as the Arduino sketch version, but restructured as a
  proper C++ class (GloveUnit) for cleaner, reusable, testable code.

  DUMMY MODE:
    If BNO055 is not detected, GloveUnit automatically falls back
    to simulated pitch/roll values so the ESP-NOW link / car unit
    can be tested without the sensor wired up.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <esp_now.h>
#include <WiFi.h>

// ============================================================
//  Message structure sent over ESP-NOW
// ============================================================
struct GestureMessage {
  char  command;   // 'F','B','L','R','S'
  float pitch;
  float roll;
};

// ============================================================
//  GloveUnit class
// ============================================================
class GloveUnit {
public:
    GloveUnit(uint8_t peerMac[6],
              float pitchThreshold = 20.0f,
              float rollThreshold  = 20.0f,
              bool  forceDummy     = false)
        : bno(55, 0x28),
          pitchThreshold_(pitchThreshold),
          rollThreshold_(rollThreshold),
          forceDummy_(forceDummy),
          sensorAvailable_(false),
          dummyStep_(0)
    {
        memcpy(peerAddress_, peerMac, 6);
    }

    // Call once in setup()
    bool begin() {
        Serial.begin(115200);
        delay(500);
        Wire.begin();

        if (!forceDummy_ && bno.begin()) {
            sensorAvailable_ = true;
            bno.setExtCrystalUse(true);
            Serial.println("BNO055 detected. Using REAL sensor data.");
        } else {
            sensorAvailable_ = false;
            Serial.println("BNO055 NOT detected (or dummy forced). Using DUMMY values.");
        }

        WiFi.mode(WIFI_STA);

        if (esp_now_init() != ESP_OK) {
            Serial.println("ESP-NOW init failed!");
            return false;
        }

        esp_now_register_send_cb(GloveUnit::onDataSentStatic);

        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peerAddress_, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add ESP-NOW peer");
            return false;
        }

        return true;
    }

    // Call repeatedly in loop()
    void update() {
        float pitch, roll;
        readOrientation(pitch, roll);

        char gesture = classify(pitch, roll);

        GestureMessage msg;
        msg.command = gesture;
        msg.pitch   = pitch;
        msg.roll    = roll;

        esp_err_t result = esp_now_send(peerAddress_,
                                         reinterpret_cast<uint8_t*>(&msg),
                                         sizeof(msg));

        Serial.print("Pitch: ");   Serial.print(pitch);
        Serial.print(" | Roll: "); Serial.print(roll);
        Serial.print(" | Gesture: "); Serial.println(gesture);

        if (result != ESP_OK) {
            Serial.println("Error sending data");
        }
    }

private:
    Adafruit_BNO055 bno;
    uint8_t peerAddress_[6];
    float pitchThreshold_;
    float rollThreshold_;
    bool  forceDummy_;
    bool  sensorAvailable_;
    int   dummyStep_;

    // Reads real sensor data, or generates dummy cycling values
    void readOrientation(float &pitch, float &roll) {
        if (sensorAvailable_) {
            sensors_event_t event;
            bno.getEvent(&event);
            roll  = event.orientation.y;
            pitch = event.orientation.z;
        } else {
            switch (dummyStep_) {
                case 0: pitch = 0;   roll = 0;   break;  // stop
                case 1: pitch = 30;  roll = 0;   break;  // forward
                case 2: pitch = 0;   roll = 0;   break;  // stop
                case 3: pitch = -30; roll = 0;   break;  // backward
                case 4: pitch = 0;   roll = 0;   break;  // stop
                case 5: pitch = 0;   roll = -30; break;  // left
                case 6: pitch = 0;   roll = 0;   break;  // stop
                case 7: pitch = 0;   roll = 30;  break;  // right
            }
            dummyStep_ = (dummyStep_ + 1) % 8;
        }
    }

    char classify(float pitch, float roll) const {
        if (pitch > pitchThreshold_)   return 'F';
        if (pitch < -pitchThreshold_)  return 'B';
        if (roll  < -rollThreshold_)   return 'L';
        if (roll  >  rollThreshold_)   return 'R';
        return 'S';
    }

    static void onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status) {
        Serial.print("Send status: ");
        Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
    }
};

// ============================================================
//  Global instance + Arduino entry points
// ============================================================
uint8_t carAddress[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};

// Set forceDummy = true to always simulate, regardless of sensor
GloveUnit glove(carAddress, /*pitchThreshold=*/20.0f, /*rollThreshold=*/20.0f, /*forceDummy=*/false);

void setup() {
    glove.begin();
}

void loop() {
    glove.update();
    delay(800);
}
