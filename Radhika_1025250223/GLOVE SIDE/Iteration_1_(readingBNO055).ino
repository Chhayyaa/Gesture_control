// Iteration 1 (To check whether the ESP32 can detect the BNO055)
#include <Wire.h>        // Handle I2C protocol between the BNO055 and ESP32 using SDA and SCL lines
#include <Adafruit_Sensor.h>  //It defines a common interface (such as the sensors_event_t structure)
#include <Adafruit_BNO055.h>  //read orientation data using functions like bno.getEvent()
#include <utility/imumaths.h>
// I2C communication
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

void setupIMU() {
  Wire.begin(SDA_PIN, SCL_PIN);   // Take data from pin 21 and 22
  Wire.setClock(400000);          // Fast mode 400kHz — better response time

  if (!bno.begin()) {
    Serial.println("BNO055 not detected");    // May be due to the issue in wiring
    while (1);
  }
  bno.setExtCrystalUse(true);     // use onboard crystal to get the better accuracy
  Serial.println("BNO055 Ready");
}