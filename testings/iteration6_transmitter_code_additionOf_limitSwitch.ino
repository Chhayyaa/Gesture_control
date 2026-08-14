#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>


Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);


#define LIMIT_SWITCH 25


float pitch;
float roll;
float yaw;



void setup() 
{
  Serial.begin(115200);

  Wire.begin();


  if(!bno.begin())
  {
    Serial.println("BNO055 is not detected");
    while(1);
  }


  bno.setExtCrystalUse(true);


  pinMode(LIMIT_SWITCH, INPUT_PULLUP);


  Serial.println("System Ready");

}



void loop() 
{

  sensors_event_t event;

  bno.getEvent(&event);



 

  pitch = event.orientation.z;
  roll  = event.orientation.y;
  yaw   = event.orientation.x;



  if(digitalRead(LIMIT_SWITCH) == HIGH)
  {

    Serial.println("EMERGENCY STOP");



    pitch = pitch * 0.90;
    roll  = roll * 0.90;
    yaw   = yaw * 0.90;





    if(abs(pitch) < 0.1)
      pitch = 0;


    if(abs(roll) < 0.1)
      roll = 0;


    if(abs(yaw) < 0.1)
      yaw = 0;

  }

  else
  {
    Serial.println("Normal Operation");
  }



  Serial.print("Pitch : ");
  Serial.println(pitch);


  Serial.print("Roll : ");
  Serial.println(roll);


  Serial.print("Yaw : ");
  Serial.println(yaw);


  Serial.println("----------------");


  delay(100);

}
code for limit switch just need to add in transmitter code