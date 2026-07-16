#include <WiFi.h> //Provides Wi-Fi functionality required for ESP-NOW communication.
#include <esp_now.h>

#include <Wire.h> //Provides I2C communication support.
#include <Adafruit_Sensor.h> //Provides a common sensor interface used by the BNO055 library.
#include <Adafruit_BNO055.h> //Provides functions for initializing and reading data from the BNO055 sensor.

// BNO055 Object
Adafruit_BNO055 bno = Adafruit_BNO055(55);

// Receiver MAC Address
// Replace with the MAC address of the Car ESP32 (6-byte address)
uint8_t receiverMAC[] =
{
  0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC
};

// Structure to send
typedef struct
{
    int command;      // Movement command
    float pitch;      // Pitch angle (used for debugging)
    float roll;       // Roll angle (used for debugging)
    float yaw;        // Yaw angle (used for debugging)

} SensorData;

SensorData data; //Stores all sensor values and the movement command to be transmitted.

// Callback after sending (Automatically invoked by the ESP-NOW library after each transmission.)
void OnDataSent(const uint8_t *mac_addr, //Contains the MAC address of the receiver.
                esp_now_send_status_t status)
{
  Serial.print("Send Status : ");

//Checks whether the data packet was transmitted successfully.
  if (status == ESP_NOW_SEND_SUCCESS)
    Serial.println("Success");

  else
    Serial.println("Failed");
}

// Movement Commands
#define STOP      0
#define FORWARD   1
#define BACKWARD  2
#define LEFT      3
#define RIGHT     4

const float pitchThreshold = 20;
const float rollThreshold  = 20;
const float deadZone = 8; //Ignores hand movements smaller than ±8°.

void setup()
{
  Serial.begin(115200);

  //Initializes the I2C interface using the default ESP32 SDA and SCL pins.
  Wire.begin(21,22);

  // Initialize BNO055
  if(!bno.begin())
  {
    Serial.println("BNO055 NOT DETECTED");

    while(1);
  }

  delay(1000);

  bno.setExtCrystalUse(true);

  Serial.println("BNO055 Ready");

  WiFi.mode(WIFI_STA); //Configures the ESP32 to operate in Station mode.

  // Initialize ESP-NOW protocol
  if(esp_now_init()!=ESP_OK)
  {
    Serial.println("ESP NOW Init Failed");

    while(1);
  }

  //Registers the callback function that is executed after each data transmission.
  esp_now_register_send_cb(OnDataSent);

  // Add Receiver
  //Creates a structure to store the receiver's information.
  esp_now_peer_info_t peerInfo={};

  memcpy(peerInfo.peer_addr, //Copies the receiver MAC address into the peer information structure.
         receiverMAC,
         6);

  peerInfo.channel=0; //Uses the current Wi-Fi channel automatically.
  peerInfo.encrypt=false; //Disables data encryption.

  //Registers the receiver as a communication peer.
  if(esp_now_add_peer(&peerInfo)!=ESP_OK)
  {
    Serial.println("Peer Add Failed");

    while(1);
  }

  Serial.println("ESP NOW Ready");
}

void loop()
{
    // Read Euler Angles
    imu::Vector<3> euler =
    bno.getVector(Adafruit_BNO055::VECTOR_EULER);

    data.yaw   = euler.x();
    data.roll  = euler.y();
    data.pitch = euler.z();

    // Default Movement Command
    data.command = STOP;

    // Gesture Detection
    if(data.pitch > pitchThreshold)
    {
        data.command = FORWARD;
    }

    else if(data.pitch < -pitchThreshold)
    {
        data.command = BACKWARD;
    }

    else if(data.roll > rollThreshold)
    {
        data.command = RIGHT;
    }

    else if(data.roll < -rollThreshold)
    {
        data.command = LEFT;
    }

    // Dead Zone
    if(abs(data.pitch) < deadZone &&
       abs(data.roll)  < deadZone)
    {
        data.command = STOP;
    }

    // Display Sensor Values and Command
    Serial.print("Pitch : ");
    Serial.print(data.pitch);

    Serial.print(" Roll : ");
    Serial.print(data.roll);

    Serial.print(" Yaw : ");
    Serial.print(data.yaw);

    Serial.print(" Command : ");

    switch(data.command)
    {
        case FORWARD:
            Serial.println("FORWARD");
            break;

        case BACKWARD:
            Serial.println("BACKWARD");
            break;

        case LEFT:
            Serial.println("LEFT");
            break;

        case RIGHT:
            Serial.println("RIGHT");
            break;

        default:
            Serial.println("STOP");
            break;
    }
    // Send Data
    //Transmits the complete SensorData structure using ESP-NOW.
    esp_now_send(receiverMAC, //Destination ESP32 MAC address.
                 (uint8_t *)&data, //Converts the structure into a byte pointer for transmission.
                 sizeof(data)); //Specifies the total number of bytes to be transmitted.

    delay(100);
}