#include "LightController.h"
#include <Arduino.h>

const int LIGHT_BRIGHTNESS_STEP = 1;
const uint8_t MIN_INTENSITY_MAIN = 0x01;
const uint8_t MAX_INTENSITY_MAIN = 0x10;
const uint8_t MIN_INTENSITY_RING = 0x00;
const uint8_t MAX_INTENSITY_RING = 0xFF;
const uint8_t MIN_WARMNESS = 0x00;
const uint8_t MAX_WARMNESS = 0xFA;

LightController::LightController(BluetoothManager* bt)
  : btManager(bt) {}

LightMode LightController::getMode() {
  return currentMode;
}

void LightController::turnOn() {
  if (!isOn) {
    isOn = true;
    Serial.println("Light ON");
    uint8_t payload[] = { 0x01 };  // ON
    btManager->sendCommand(CMD_LIGHT_ON_OFF, payload, sizeof(payload));
  }
}

void LightController::turnOff() {
  if (isOn) {
    isOn = false;
    Serial.println("Light OFF");
    uint8_t payload[] = { 0x02 };  // OFF
    btManager->sendCommand(CMD_LIGHT_ON_OFF, payload, sizeof(payload));
  }
}

void LightController::toggle() {
  isOn = !isOn;
  Serial.printf("Light Toggled: %s\n", isOn ? "ON" : "OFF");
  uint8_t payload[] = { isOn ? (uint8_t)0x01 : (uint8_t)0x02 };
  btManager->sendCommand(CMD_LIGHT_ON_OFF, payload, sizeof(payload));
}

void LightController::setBrightness(int newBrightness, bool forceUpdate) {
  int oldBrightness = currentMode == MAIN_LIGHT ? brightnessMain : brightnessRing;
  *brightness = constrain(newBrightness, minIntensity, maxIntensity);
  if ((*brightness) != oldBrightness || forceUpdate) {
    Serial.printf("Brightness set to: %d\n", *brightness);
    sendState();
  }
}

void LightController::increaseBrightness() {
  if (!isOn) return;
  setBrightness((*brightness) + LIGHT_BRIGHTNESS_STEP, false);
}

void LightController::decreaseBrightness() {
  if (!isOn) return;
  setBrightness((*brightness) - LIGHT_BRIGHTNESS_STEP, false);
}

void LightController::changeWarmness() {
  if (!isOn || currentMode != MAIN_LIGHT) return;
  warmness += warmnessStep;
  if (warmness >= MAX_WARMNESS) {
    warmness = MAX_WARMNESS;
    warmnessStep = -warmnessStep;
  } else if (warmness <= MIN_WARMNESS) {
    warmness = MIN_WARMNESS;
    warmnessStep = -warmnessStep;
  }
  Serial.printf("Warmness changed to: %d\n", warmness);
  sendState();
}

void LightController::rotateHue() {
  if (!isOn || currentMode != RGB_RING) return;
  hue = (hue + 1) % 100;
  sendState();
}

void LightController::switchMode() {
  if (currentMode == MAIN_LIGHT) {
    currentMode = RGB_RING;
    minIntensity = MIN_INTENSITY_RING;
    maxIntensity = MAX_INTENSITY_RING;
    brightness = &brightnessRing;
  } else {
    currentMode = MAIN_LIGHT;
    minIntensity = MIN_INTENSITY_MAIN;
    maxIntensity = MAX_INTENSITY_MAIN;
    brightness = &brightnessMain;
  }
  Serial.printf("Mode switched to: %s\n", (currentMode == MAIN_LIGHT) ? "Main Light" : "RGB Ring");
  // Resend state to apply current settings to the new mode
  sendState();
  delay(10);  // Send twice for reliability with some devices
  sendState();
}

void LightController::sendState() {
  if (!btManager->isConnected()) return;

  if (currentMode == MAIN_LIGHT) {
    uint8_t intensityPayload[] = { (uint8_t)constrain(brightnessMain, MIN_INTENSITY_MAIN, MAX_INTENSITY_MAIN) };
    btManager->sendCommand(CMD_LIGHT_INTENSITY, intensityPayload, sizeof(intensityPayload));

    uint8_t warmnessPayload[] = { (uint8_t)constrain(warmness, MIN_WARMNESS, MAX_WARMNESS) };
    btManager->sendCommand(CMD_LIGHT_WARMNESS, warmnessPayload, sizeof(warmnessPayload));
  } else {
    sendRGBState();
  }
}

void LightController::sendRGBState() {
  int r, g, b;
  hslToRgb((float)hue / 100.0, 1.0, (float)brightnessRing / 255.0, &r, &g, &b);
  Serial.printf("Ring RGB: %d, %d, %d (hue: %d, brightness: %d)\n", r, g, b, hue, brightnessRing);

  uint8_t payload[] = {
    (uint8_t)brightnessRing,
    (uint8_t)r,
    (uint8_t)g,
    (uint8_t)b
  };
  btManager->sendCommand(CMD_RGB, payload, sizeof(payload));
}

// HSL to RGB conversion functions (kept as static private methods)
void LightController::hslToRgb(float h, float s, float l, int* r, int* g, int* b) {
  if (s == 0.0) {
    *r = *g = *b = (int)(l * 255.0);
  } else {
    float q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    float p = 2.0 * l - q;
    *r = (int)(255 * hueToRgb(p, q, h + 1.0 / 3.0));
    *g = (int)(255 * hueToRgb(p, q, h));
    *b = (int)(255 * hueToRgb(p, q, h - 1.0 / 3.0));
  }
}

float LightController::hueToRgb(float p, float q, float t) {
  if (t < 0.0) t += 1;
  if (t > 1.0) t -= 1;
  if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
  if (t < 1.0 / 2.0) return q;
  if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
  return p;
}