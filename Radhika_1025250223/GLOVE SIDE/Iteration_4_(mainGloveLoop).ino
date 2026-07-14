void setup() {
  Serial.begin(115200);
  setupIMU();
  setupESPNOW();
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);  // Iteration 1 will wrk here to get the data input via BNO055
  // To get the values of roll and pitch
  float roll  = event.orientation.y;   
  float pitch = event.orientation.z;

  GestureCommand cmd = analyseGesture(roll, pitch); // Iteration 2 will work here like logic is applied to get the direction and speed
  sendCommand(cmd); // Iteration 3 will work here to send the command to car side 

  delay(50);   // for the smooth control
}


//-----------------------------------------------------------------//


// To get the car MaC adress
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);   // Station mode set karo (ESP-NOW jis mode mein chalega)
  delay(1000);

  Serial.print("Car ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}
