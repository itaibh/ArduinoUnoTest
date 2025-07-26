#ifndef IINPUT_HANDLER_H
#define IINPUT_HANDLER_H

#include <stdint.h>
#include "LightMode.h"

class IInputHandler {
public:
  virtual void onMainBrightnessChange(uint8_t main_brightness) = 0;
  virtual void onMainWarmnessChange(uint8_t main_warmness) = 0;
  virtual void onRingBrightnessChange(uint8_t ring_brightness) = 0;
  virtual void onRingHueChange(uint8_t ring_hue) = 0;
  virtual void onLightModeChange(LightMode light_mode) = 0;
  virtual void onFanSpeedChange(uint8_t fan_speed) = 0;

  virtual ~IInputHandler() = default;
};

#endif