// if no packet is recieved till the 300 miliseconds the car will stop automatically
void safetyCheck() {
  if (millis() - lastPacketTime > 300) {
    receivedCmd.direction = 0;
    receivedCmd.speed = 0;
  }
}