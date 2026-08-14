#include <CytronMD.h>

// --- Pin Definitions ---
// Front Left
#define FL_PWM 18
#define FL_DIR 19

// Front Right
#define FR_PWM 16
#define FR_DIR 17

// Rear Left
#define RL_PWM 22
#define RL_DIR 23

// Rear Right
#define RR_PWM 32
#define RR_DIR 33

// Initialize 4 Motor Drivers
CytronMD motorFL(PWM_DIR, FL_PWM, FL_DIR);
CytronMD motorFR(PWM_DIR, FR_PWM, FR_DIR);
CytronMD motorRL(PWM_DIR, RL_PWM, RL_DIR);
CytronMD motorRR(PWM_DIR, RR_PWM, RR_DIR);

const int SPEED = 150; // Test speed (0 - 255)

void setDrive(int fl, int fr, int rl, int rr) {
  motorFL.setSpeed(fl);
  motorFR.setSpeed(fr);
  motorRL.setSpeed(rl);
  motorRR.setSpeed(rr);
}

void stopCar() {
  setDrive(0, 0, 0, 0);
}

void setup() {
  // Wait 3 seconds before starting the motion test
  delay(3000); 
}

void loop() {
  // 1. Move FORWARD
  setDrive(SPEED, SPEED, SPEED, SPEED);
  delay(2000);
  stopCar();
  delay(1000);

  // 2. Move BACKWARD
  setDrive(-SPEED, -SPEED, -SPEED, -SPEED);
  delay(2000);
  stopCar();
  delay(1000);

  // 3. Turn LEFT (Tank Turn: Left side backward, Right side forward)
  setDrive(-SPEED, SPEED, -SPEED, SPEED);
  delay(1500);
  stopCar();
  delay(1000);

  // 4. Turn RIGHT (Tank Turn: Left side forward, Right side backward)
  setDrive(SPEED, -SPEED, SPEED, -SPEED);
  delay(1500);
  stopCar();
  delay(1000);

  // Pause for 5 seconds before repeating sequence
  delay(5000); 
}