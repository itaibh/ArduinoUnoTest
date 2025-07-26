#include "StorageHandler.h"

// Use unsigned long for timestamps to avoid rollover issues after ~50 days
const unsigned long DEBOUNCE_DELAY_MS = 2000; // Wait 2 seconds of inactivity before saving
const unsigned long MIN_SAVE_INTERVAL_MS = 1000; // Minimum 1 second between actual writes (if you want this too)

StorageHandler::StorageHandler(BluetoothManager* bt, LightController* lc, FanController* fc)
  : btManager(bt), lightCtrl(lc), fanCtrl(fc) {
  bt->registerConnectionListener(this);
  lc->registerListener(this);
  fc->registerListener(this);
}

void StorageHandler::onBluetoothConnected(String mac_address) {
  Serial.printf("StorageHandler: Bluetooth connected to MAC: %s. Restoring configuration.\n", mac_address.c_str());
  restore(mac_address); // Restore config from NVS

  // 1. Send Light command and wait for ACK
  lightCtrl->setAll(deviceConfig.light_mode, deviceConfig.main_brightness, deviceConfig.main_warmness, deviceConfig.ring_brightness, deviceConfig.ring_hue);
  Serial.println("[Debug] Light command sent. Waiting for acknowledgment...");
  // if (btManager->waitForAck({CMD_LIGHT_INTENSITY, CMD_RGB}, 6000)) { // Wait up to 6 seconds for light ACK
  //   Serial.println("[Debug] Light command acknowledged.");
  // } else {
  //   Serial.println("[Debug] Light command ACK timeout.");
  // }
  // 2. Send Fan command and wait for ACK
  delay(500);
  fanCtrl->setSpeed(deviceConfig.fan_speed);
  Serial.println("[Debug] Fan command sent. Waiting for acknowledgment...");
  // if (btManager->waitForAck({CMD_FAN_SPEED}, 6000)) { // Wait up to 6 seconds for fan ACK
  //   Serial.println("[Debug] Fan command acknowledged. Sending next command.");
  // } else {
  //   Serial.println("[Debug] Fan command ACK timeout. Proceeding anyway, but might be issues.");
  // }

}

void StorageHandler::_putUCharIfChanged(const char* key, uint8_t newValue, uint8_t oldValue, bool& didWriteFlag) {
  if (newValue != oldValue) {
    preferences.putUChar(key, newValue);
    // Removed verbose print, only log that something was saved
    Serial.printf("  Saved %s: %d\n", key, newValue); // Less verbose now
    didWriteFlag = true; // Set the flag indicating a write occurred
  }
}

void StorageHandler::store() {
  Serial.println("storing...");
  
  String macNoColons = deviceConfig.mac_address;
  macNoColons.replace(":", "");
  String prefNS = "devcfg_" + macNoColons.substring(macNoColons.length() - 4);
  
  preferences.begin(prefNS.c_str(), false);
  preferences.begin(prefNS.c_str(), false);

  bool anyValueActuallyWrittenToFlash = false; // Flag to track if any put operation happened

  // Use the helper method for each uint8_t member
  _putUCharIfChanged("fan_speed", deviceConfig.fan_speed, lastSavedDeviceConfig.fan_speed, anyValueActuallyWrittenToFlash);
  _putUCharIfChanged("light_mode", deviceConfig.light_mode, lastSavedDeviceConfig.light_mode, anyValueActuallyWrittenToFlash);
  _putUCharIfChanged("main_brightness", deviceConfig.main_brightness, lastSavedDeviceConfig.main_brightness, anyValueActuallyWrittenToFlash);
  _putUCharIfChanged("main_warmness", deviceConfig.main_warmness, lastSavedDeviceConfig.main_warmness, anyValueActuallyWrittenToFlash);
  _putUCharIfChanged("ring_brightness", deviceConfig.ring_brightness, lastSavedDeviceConfig.ring_brightness, anyValueActuallyWrittenToFlash);
  _putUCharIfChanged("ring_hue", deviceConfig.ring_hue, lastSavedDeviceConfig.ring_hue, anyValueActuallyWrittenToFlash);
  
  preferences.end(); // Close preferences to ensure data is written to flash

  if (anyValueActuallyWrittenToFlash) {
    lastSavedDeviceConfig = deviceConfig; // Update the entire struct to the newly saved state
    lastSaveTime = millis();              // Record when this actual save completed
    Serial.println("Config values written to flash successfully.");
  } else {
    // This branch implies that while configIsDirty might have been true,
    // all changes that occurred were effectively reverted before store() was called,
    // or the state became identical to lastSavedDeviceConfig by coincidence.
    Serial.println("No values needed saving, config already up-to-date in NVS.");
  }
}

void StorageHandler::restore(String mac_address) {
  String macNoColons = mac_address;
  macNoColons.replace(":", "");
  String prefNS = "devcfg_" + macNoColons.substring(8);
  preferences.begin(prefNS.c_str(), false);
  deviceConfig.mac_address = mac_address;
  deviceConfig.fan_speed = preferences.getUChar("fan_speed");
  deviceConfig.light_mode = (LightMode)preferences.getUChar("light_mode");
  deviceConfig.main_brightness = preferences.getUChar("main_brightness");
  deviceConfig.main_warmness = preferences.getUChar("main_warmness");
  deviceConfig.ring_brightness = preferences.getUChar("ring_brightness");
  deviceConfig.ring_hue = preferences.getUChar("ring_hue");
  preferences.end();
  lastSavedDeviceConfig = deviceConfig;
  lastSaveTime = millis();
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
  if (btManager && !btManager->isConnected()) return;
  if (lastSavedDeviceConfig != deviceConfig &&
      (millis() - lastChangeDetectedTime) > DEBOUNCE_DELAY_MS &&
      (millis() - lastSaveTime) > MIN_SAVE_INTERVAL_MS) {
    store();
  }
}