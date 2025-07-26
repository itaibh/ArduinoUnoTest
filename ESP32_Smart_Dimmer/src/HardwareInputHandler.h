#ifndef HARDWARE_INPUT_HANDLER_H
#define HARDWARE_INPUT_HANDLER_H

#include <AiEsp32RotaryEncoder.h>
#include "LightController.h"
#include "FanController.h"

// ISR function must be in the global scope
void IRAM_ATTR readEncoderISR();

class HardwareInputHandler {
public:
  HardwareInputHandler(LightController* lc, FanController* fc,
               int clkPin, int dtPin, int swPin, int stepsPerNotch,
               int fanUpPin, int fanDownPin);
  void begin();
  void update();

  // Public method for ISR to call
  void handleEncoderISR();

private:
  LightController* lightCtrl;
  FanController* fanCtrl;
  AiEsp32RotaryEncoder rotaryEncoder;

  int fanUpPin, fanDownPin;

  long lastRotaryPosition = 0;

  // Click detection variables
  unsigned long lastDebounceTime = 0;
  unsigned long lastPressTime = 0;
  unsigned long lastLongPressActionTime = 0;
  //ButtonState lastButtonState = ButtonState::BUT_UP;
  bool currentEncoderButtonDown = false;
  bool lastEncoderButtonDown = false;
  bool isLongPress = false;
  int clickCount = 0;

  // Fan button state
  bool fanUpPressed = false;
  bool fanDownPressed = false;

  void pollRotaryEncoder();
  void pollEncoderSwitch();
  void pollFanButtons();
};

#endif  // HARDWARE_INPUT_HANDLER_H