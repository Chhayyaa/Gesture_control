void executeCommand(GestureCommand cmd) {
  switch (cmd.direction) {
    case 0: stopMotors(); break;
    case 1:                 //Forward
      setMotor(LEFT_IN1, LEFT_IN2, LEFT_PWM, cmd.speed, true);
      setMotor(RIGHT_IN1, RIGHT_IN2, RIGHT_PWM, cmd.speed, true);
      break;
    case 2:                        //Backward
      setMotor(LEFT_IN1, LEFT_IN2, LEFT_PWM, cmd.speed, false);
      setMotor(RIGHT_IN1, RIGHT_IN2, RIGHT_PWM, cmd.speed, false);
      break;
    case 3:  // LEFT turn — right wheel dominant and speed will decrease
      setMotor(LEFT_IN1, LEFT_IN2, LEFT_PWM, cmd.speed * 0.4, false);
      setMotor(RIGHT_IN1, RIGHT_IN2, RIGHT_PWM, cmd.speed, true);
      break;
    case 4:  // RIGHT turn — left wheel dominant
      setMotor(LEFT_IN1, LEFT_IN2, LEFT_PWM, cmd.speed, true);
      setMotor(RIGHT_IN1, RIGHT_IN2, RIGHT_PWM, cmd.speed * 0.4, false);
      break;
  }
}