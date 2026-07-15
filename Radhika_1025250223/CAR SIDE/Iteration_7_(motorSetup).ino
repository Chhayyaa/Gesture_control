// Motor Driver Pins considering L298N
// IN for the direction control and PWM for the speed control 
#define LEFT_IN1   25      
#define LEFT_IN2   26
#define LEFT_PWM   27             
#define RIGHT_IN1  32
#define RIGHT_IN2  33
#define RIGHT_PWM  14
void setupMotors() {
  // set the each direction control pin to the output mode
  pinMode(LEFT_IN1, OUTPUT);  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT); pinMode(RIGHT_IN2, OUTPUT);
  ledcAttach(LEFT_PWM, 5000, 8);   // 5kHz, 8-bit resolution
  ledcAttach(RIGHT_PWM, 5000, 8);
}
// here in1 are the direction pins
void setMotor(int in1, int in2, int pwmPin, int speed, bool forward) {
  digitalWrite(in1, forward ? HIGH : LOW);
  digitalWrite(in2, forward ? LOW  : HIGH);
  ledcWrite(pwmPin, speed);
}
void stopMotors() {
  ledcWrite(LEFT_PWM, 0);
  ledcWrite(RIGHT_PWM, 0);
}