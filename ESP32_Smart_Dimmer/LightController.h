#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include "BluetoothManager.h"

enum LightMode {
  MAIN_LIGHT,
  RGB_RING
};

class LightController {
public:
  LightController(BluetoothManager* bt);
  void turnOn();
  void turnOff();
  void toggle();
  void setBrightness(int brightness, bool forceUpdate = false);
  void increaseBrightness();
  void decreaseBrightness();
  void changeWarmness();
  void rotateHue();
  void switchMode();
  LightMode getMode();

private:
  BluetoothManager* btManager;
  bool isOn = false;
  int brightness = 128;  // 0-255
  int warmness = 0;      // 0-250 (0xFA)
  int hue = 0;           // 0-99
  LightMode currentMode = MAIN_LIGHT;
  int warmnessStep = 10;

  void sendState();
  void sendRGBState();
  static void hslToRgb(float h, float s, float l, int* r, int* g, int* b);
  static float hueToRgb(float p, float q, float t);
};

#endif  // LIGHT_CONTROLLER_H