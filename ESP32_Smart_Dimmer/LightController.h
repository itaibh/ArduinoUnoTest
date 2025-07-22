#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include "BluetoothManager.h"

enum LightMode {
  MAIN_LIGHT,
  RGB_RING
};

class ILightControllerListener {
  public:
  virtual void onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue) = 0;
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
  void registerListener(ILightControllerListener* listener);

private:
  BluetoothManager* btManager;
  bool isOn = false;
  int brightnessMain = 16;  // 1-16
  int brightnessRing = 128;  // 0-255
  int warmness = 0;      // 0-250 (0xFA)
  int hue = 0;           // 0-99
  LightMode currentMode = MAIN_LIGHT;
  int warmnessStep = 10;

  int* brightness = &brightnessMain;  // current light mode brightness
  int minIntensity = 1;  // current light mode min intensity
  int maxIntensity = 16; // current light mode max intensity

  void sendState();
  void sendRGBState();
  static void hslToRgb(float h, float s, float l, int* r, int* g, int* b);
  static float hueToRgb(float p, float q, float t);
  
  ILightControllerListener* listener;
  void invokeCallback();
};

#endif  // LIGHT_CONTROLLER_H