#include "StorageHandler.h"

const long MIN_SAVE_INTERVAL = 1000; // at least a second

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
}

void StorageHandler::onFanControllerChange(int fan_speed) {
  Serial.println("onFanControllerChange");
  deviceConfig.fan_speed = fan_speed;
}

void StorageHandler::tryStore() {
  if (lastSavedDeviceConfig != deviceConfig && lastSaveTime - millis() > MIN_SAVE_INTERVAL) {
    store();
  }
}