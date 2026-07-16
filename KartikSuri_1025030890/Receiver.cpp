// CAR UNIT - ESP32 + ESP-NOW + BTS7960
#include <WiFi.h>      //Provides Wi-Fi functionality required for ESP-NOW communication.
#include <esp_now.h>
// BTS7960 Motor Driver Pin Configuration
// Left Front Motor
#define LF_RPWM 25
#define LF_LPWM 26
// Left Rear Motor
#define LR_RPWM 27
#define LR_LPWM 14
// Right Front Motor
#define RF_RPWM 33
#define RF_LPWM 32
// Right Rear Motor
#define RR_RPWM 18
#define RR_LPWM 19
// Enable Pins
#define L_EN 21
#define R_EN 22

// Movement Commands
#define STOP      0
#define FORWARD   1
#define BACKWARD  2
#define LEFT      3
#define RIGHT     4

// PWM Configuration
// PWM frequency in Hertz.
const int freq = 5000;
// 8-bit resolution provides a duty cycle range of 0-255.
const int resolution = 8;
// Default motor speed (can later be adjusted using PID).
int motorSpeed = 180;


// This structure must exactly match the structure used by the transmitter.
typedef struct
{
    int command;
    float pitch;
    float roll;
    float yaw;

} SensorData;

SensorData data;

// ESP-NOW Receive Callback
// Automatically executed whenever a data packet is received.
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len)
{
    // Verify that the received packet size is correct.
    if(len == sizeof(data)){
        // Copy the received bytes into the SensorData structure.
        memcpy(&data,
               incomingData,
               sizeof(data));
    }
}

void setup()
{
    Serial.begin(115200);

    // Configure BTS7960 Enable Pins
    pinMode(L_EN, OUTPUT);
    pinMode(R_EN, OUTPUT);

    digitalWrite(L_EN, HIGH);
    digitalWrite(R_EN, HIGH);

    // Configure PWM Outputs
    ledcAttach(LF_RPWM, freq, resolution);
    ledcAttach(LF_LPWM, freq, resolution);

    ledcAttach(LR_RPWM, freq, resolution);
    ledcAttach(LR_LPWM, freq, resolution);

    ledcAttach(RF_RPWM, freq, resolution);
    ledcAttach(RF_LPWM, freq, resolution);

    ledcAttach(RR_RPWM, freq, resolution);
    ledcAttach(RR_LPWM, freq, resolution);

    // Configure Wi-Fi Hardware
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW
    if(esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW Initialization Failed");

        while(1);
    }
    // Register Receive Callback
    esp_now_register_recv_cb(OnDataRecv);

    Serial.println("Receiver Ready");
}
void loop()
{
    switch(data.command)
    {
        case FORWARD:

            moveForward();

            break;

        case BACKWARD:

            moveBackward();

            break;

        case LEFT:

            turnLeft();

            break;

        case RIGHT:

            turnRight();

            break;

        default:

            stopCar();

            break;
    }
}

// Stop Vehicle
// Stops all motors by setting both PWM signals to zero.
void stopCar()
{
    ledcWrite(LF_RPWM, 0);
    ledcWrite(LF_LPWM, 0);

    ledcWrite(LR_RPWM, 0);
    ledcWrite(LR_LPWM, 0);

    ledcWrite(RF_RPWM, 0);
    ledcWrite(RF_LPWM, 0);

    ledcWrite(RR_RPWM, 0);
    ledcWrite(RR_LPWM, 0);
}

// Move Forward
// Drives all motors in the forward direction.
void moveForward()
{
    ledcWrite(LF_RPWM, motorSpeed);
    ledcWrite(LF_LPWM, 0);

    ledcWrite(LR_RPWM, motorSpeed);
    ledcWrite(LR_LPWM, 0);

    ledcWrite(RF_RPWM, motorSpeed);
    ledcWrite(RF_LPWM, 0);

    ledcWrite(RR_RPWM, motorSpeed);
    ledcWrite(RR_LPWM, 0);
}

// Move Backward
// Drives all motors in the reverse direction.
void moveBackward()
{
    ledcWrite(LF_RPWM, 0);
    ledcWrite(LF_LPWM, motorSpeed);

    ledcWrite(LR_RPWM, 0);
    ledcWrite(LR_LPWM, motorSpeed);

    ledcWrite(RF_RPWM, 0);
    ledcWrite(RF_LPWM, motorSpeed);

    ledcWrite(RR_RPWM, 0);
    ledcWrite(RR_LPWM, motorSpeed);
}

// Turn Left
// Stops the left-side motors while driving the right-side motors forward.
void turnLeft()
{
    ledcWrite(LF_RPWM, 0);
    ledcWrite(LF_LPWM, 0);

    ledcWrite(LR_RPWM, 0);
    ledcWrite(LR_LPWM, 0);

    ledcWrite(RF_RPWM, motorSpeed);
    ledcWrite(RF_LPWM, 0);

    ledcWrite(RR_RPWM, motorSpeed);
    ledcWrite(RR_LPWM, 0);
}

// Turn Right
// Stops the right-side motors while driving the left-side motors forward.
void turnRight()
{
    ledcWrite(LF_RPWM, motorSpeed);
    ledcWrite(LF_LPWM, 0);

    ledcWrite(LR_RPWM, motorSpeed);
    ledcWrite(LR_LPWM, 0);

    ledcWrite(RF_RPWM, 0);
    ledcWrite(RF_LPWM, 0);

    ledcWrite(RR_RPWM, 0);
    ledcWrite(RR_LPWM, 0);
}