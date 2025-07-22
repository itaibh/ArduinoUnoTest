#include "StorageHandler.h"

StorageHandler::StorageHandler(LightController* lc, FanController* fc)
: lightCtrl(lc), fanCtrl(fc) {
}

void StorageHandler::store() {
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
  deviceConfig.ring_hue = preferences.getUChar("ring_hue");
  deviceConfig.ring_brightness = preferences.getUChar("ring_brightness");
  preferences.end();
}

void StorageHandler::onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue) {
  Serial.println("onLightControllerChange");
}

void StorageHandler::onFanControllerChange(int fan_speed) {
  Serial.println("onFanControllerChange");
}
