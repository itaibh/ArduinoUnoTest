#include "StorageHandler.h"

// Use unsigned long for timestamps to avoid rollover issues after ~50 days
const unsigned long DEBOUNCE_DELAY_MS = 2000; // Wait 2 seconds of inactivity before saving
const unsigned long MIN_SAVE_INTERVAL_MS = 1000; // Minimum 1 second between actual writes (if you want this too)

StorageHandler::StorageHandler(LightController* lc, FanController* fc)
  : lightCtrl(lc), fanCtrl(fc) {
  lc->registerListener(this);
  fc->registerListener(this);
}

void StorageHandler::store() {
  Serial.println("storing... Done! (STUB PRINT)");
  lastSavedDeviceConfig = deviceConfig;
  lastSaveTime = millis();
}

void StorageHandler::restore(String mac_address) {
  String macNoColons = mac_address;
  macNoColons.replace(":", "");
  String prefNS = "devcfg_" + macNoColons.substring(8);
  preferences.begin(prefNS.c_str(), false);
  deviceConfig.mac_address = mac_address;
  deviceConfig.fan_speed = preferences.getUChar("fan_speed");
  deviceConfig.light_mode = preferences.getUChar("light_mode");
  deviceConfig.main_brightness = preferences.getUChar("main_brightness");
  deviceConfig.main_warmness = preferences.getUChar("main_warmness");
  deviceConfig.ring_brightness = preferences.getUChar("ring_brightness");
  deviceConfig.ring_hue = preferences.getUChar("ring_hue");
  preferences.end();
}

void StorageHandler::onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue) {
  Serial.println("onLightControllerChange");
  deviceConfig.light_mode = light_mode;
  deviceConfig.main_brightness = main_brightness;
  deviceConfig.main_warmness = main_warmness;
  deviceConfig.ring_brightness = ring_brightness;
  deviceConfig.ring_hue = ring_hue;
  if (lastSavedDeviceConfig != deviceConfig) {
    lastChangeDetectedTime = millis(); 
  }
}

void StorageHandler::onFanControllerChange(int fan_speed) {
  Serial.println("onFanControllerChange");
  deviceConfig.fan_speed = fan_speed;
  if (lastSavedDeviceConfig != deviceConfig) {
    lastChangeDetectedTime = millis(); 
  }
}

void StorageHandler::tryStore() {
  if (lastSavedDeviceConfig != deviceConfig &&
      (millis() - lastChangeDetectedTime) > DEBOUNCE_DELAY_MS &&
      (millis() - lastSaveTime) > MIN_SAVE_INTERVAL_MS) {
    store();
  }
}