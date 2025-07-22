#include "FanController.h"
#include <Arduino.h>

const uint8_t MIN_FAN_SPEED = 0;
const uint8_t MAX_FAN_SPEED = 3;

FanController::FanController(BluetoothManager* bt)
  : btManager(bt) {}

void FanController::setSpeed(int speed) {
  int newSpeed = constrain(speed, MIN_FAN_SPEED, MAX_FAN_SPEED);
  if (newSpeed != currentSpeed) {
    currentSpeed = newSpeed;
    String speedText;
    switch (currentSpeed) {
      case 0: speedText = "Off"; break;
      case 1: speedText = "Low"; break;
      case 2: speedText = "Medium"; break;
      case 3: speedText = "High"; break;
    }
    Serial.printf("Fan Speed set to: %s (%d)\n", speedText.c_str(), currentSpeed);

    uint8_t payload[] = { (uint8_t)currentSpeed };
    btManager->sendCommand(CMD_FAN_SPEED, payload, sizeof(payload));
  }
}

void FanController::increaseSpeed() {
  setSpeed(currentSpeed + 1);
}

void FanController::decreaseSpeed() {
  setSpeed(currentSpeed - 1);
}