#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "BluetoothManager.h"

class FanController {
public:
  FanController(BluetoothManager* bt);
  void setSpeed(int speed);
  void increaseSpeed();
  void decreaseSpeed();

private:
  BluetoothManager* btManager;
  int currentSpeed = 0;  // 0=Off, 1=Low, 2=Medium, 3=High
};

#endif  // FAN_CONTROLLER_H