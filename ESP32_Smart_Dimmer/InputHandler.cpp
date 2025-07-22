#include "InputHandler.h"

// --- Global instance pointer for the ISR ---
InputHandler* globalInputHandler = nullptr;

// --- ISR Definition ---
void IRAM_ATTR readEncoderISR() {
  if (globalInputHandler) {
    globalInputHandler->handleEncoderISR();
  }
}

InputHandler::InputHandler(LightController* lc, FanController* fc,
                           int clkPin, int dtPin, int swPin, int stepsPerNotch,
                           int fanUpPin, int fanDownPin)
  : lightCtrl(lc), fanCtrl(fc),
    rotaryEncoder(clkPin, dtPin, swPin, stepsPerNotch),
    fanUpPin(fanUpPin), fanDownPin(fanDownPin) {
  globalInputHandler = this;
}

void InputHandler::begin() {
  // Initialize pins
  pinMode(fanUpPin, INPUT_PULLUP);
  pinMode(fanDownPin, INPUT_PULLUP);

  // Initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setAcceleration(250);
}

void InputHandler::update() {
  pollRotaryEncoder();
  pollEncoderSwitch();
  pollFanButtons();
}

void InputHandler::handleEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

void InputHandler::pollRotaryEncoder() {
  long newPosition = rotaryEncoder.readEncoder();
  if (newPosition != lastRotaryPosition) {
    Serial.printf("rotary position: %ld (last position: %ld)\n", newPosition, lastRotaryPosition);
    if (newPosition < lastRotaryPosition) {
      lightCtrl->increaseBrightness();
    } else {
      lightCtrl->decreaseBrightness();
    }
    lastRotaryPosition = newPosition;
  }
}

void InputHandler::pollEncoderSwitch() {
  const long debounceDelay = 50;
  const long longPressDelay = 750;
  const long doubleClickDelay = 500;
  const unsigned long longPressActionInterval = 100;

  //ButtonState buttonState = rotaryEncoder.readButtonState();
  bool isEncoderButtonDown = rotaryEncoder.isEncoderButtonDown();

  if (isEncoderButtonDown != lastEncoderButtonDown) {
    lastDebounceTime = millis();
    Serial.print("lastDebounceTime: ");
    Serial.println(lastDebounceTime);
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Serial.printf("isEncoderButtonDown: %s, lastEncoderButtonDown: %s\n",
    //   isEncoderButtonDown ? "true" : "false",
    //   lastEncoderButtonDown ? "true" : "false");
    if (isEncoderButtonDown != currentEncoderButtonDown) {
      currentEncoderButtonDown = isEncoderButtonDown;
      if (currentEncoderButtonDown) {  // PRESSED
        isLongPress = false;
        lastPressTime = millis();
        clickCount++;
        Serial.print("pressed. click count: ");
        Serial.println(clickCount);
      } else {  // RELEASED
        Serial.println("released");
        if (isLongPress) {
          isLongPress = false;
        }
      }
    }
  }
  lastEncoderButtonDown = isEncoderButtonDown;

  // Handle Long Press
  if (currentEncoderButtonDown && !isLongPress && (millis() - lastPressTime > longPressDelay)) {
    isLongPress = true;
    clickCount = 0;  // Cancel any pending clicks
    Serial.println("long press");
  }

  if (isLongPress) {
    if (millis() - lastLongPressActionTime > longPressActionInterval) {
      lastLongPressActionTime = millis();
      if (lightCtrl->getMode() == MAIN_LIGHT) {
        lightCtrl->changeWarmness();
      } else {
        lightCtrl->rotateHue();
      }
    }
  }

  // Handle Single/Double Click (after release)
  if (clickCount > 0 && !currentEncoderButtonDown && (millis() - lastPressTime) > doubleClickDelay) {
    if (clickCount == 1) {
      Serial.println("Single Click Detected!");
      lightCtrl->toggle();
    } else if (clickCount == 2) {
      Serial.println("Double Click Detected!");
      lightCtrl->switchMode();
    }
    clickCount = 0;
  }

  delay(10);
}

void InputHandler::pollFanButtons() {
  if (digitalRead(fanUpPin) == LOW) {
    if (!fanUpPressed) {
      fanUpPressed = true;
      fanCtrl->increaseSpeed();
    }
  } else {
    fanUpPressed = false;
  }

  if (digitalRead(fanDownPin) == LOW) {
    if (!fanDownPressed) {
      fanDownPressed = true;
      fanCtrl->decreaseSpeed();
    }
  } else {
    fanDownPressed = false;
  }
}