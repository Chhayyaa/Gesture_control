// Iteration 2 (using BNO055 data for the basic movements command)


struct GestureCommand {
  // use unsigned 8-bit  integer as it takes 1 byte so to save the memory as int takes the 4 bytes
  uint8_t direction;   // 0=STOP, 1=FORWARD, 2=BACKWARD, 3=LEFT, 4=RIGHT
  uint8_t speed;        // 0-255 PWM value
};

GestureCommand analyseGesture(float roll, float pitch) {
  GestureCommand cmd;
  const float DEADZONE = 8.0;      // it is threshold value 
  const float MAX_ANGLE = 45.0;    // full tilt = full speed

  if (abs(pitch) < DEADZONE && abs(roll) < DEADZONE) {
    cmd.direction = 0;   // STOP (can say value between -8 to 8 will have no impact)
    cmd.speed = 0;
    return cmd;
  }

  // Pitch-forward/backward, Roll-left/right
  if (abs(pitch) > abs(roll)) {
    cmd.direction = (pitch > 0) ? 1 : 2;   // Forward : Backward
    cmd.speed = map(constrain(abs(pitch), DEADZONE, MAX_ANGLE),
                     DEADZONE, MAX_ANGLE, 100, 255);
  } else {
    cmd.direction = (roll > 0) ? 4 : 3;    // Right : Left
    cmd.speed = map(constrain(abs(roll), DEADZONE, MAX_ANGLE),
                     DEADZONE, MAX_ANGLE, 100, 255);
  }
  return cmd;  
}
// Now we got the cmd (final command) which will be returned by the ESP32
