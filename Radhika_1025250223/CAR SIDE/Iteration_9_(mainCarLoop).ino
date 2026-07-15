void setup() {
  Serial.begin(115200);
  setupMotors();   //Iteration 7 will work here
  setupESPNOWReceiver(); //Iteration 5
}
void loop() {
  safetyCheck(); // Iteration 6
  GestureCommand cmd = receivedCmd; 
  executeCommand(cmd); // Iteration 8
  delay(20);
}