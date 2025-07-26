#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "BluetoothManager.h"

class IFanControllerListener {
public:
  virtual void onFanControllerChange(int fan_speed) = 0;
};


class FanController {
public:
  FanController(BluetoothManager* bt);
  void setSpeed(int speed);
  void increaseSpeed();
  void decreaseSpeed();
  void registerListener(IFanControllerListener* listener);
private:
  BluetoothManager* btManager;
  int currentSpeed = 0;  // 0=Off, 1=Low, 2=Medium, 3=High
  IFanControllerListener* listener;
};

#endif  // FAN_CONTROLLER_H