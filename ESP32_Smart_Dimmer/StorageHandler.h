#ifndef STORAGE_HANDLER_H
#define STORAGE_HANDLER_H
#include <Preferences.h>
#include "LightController.h"
#include "FanController.h"

struct DeviceConfig {
  String mac_address;
  uint8_t fan_speed;
  uint8_t light_mode;
  uint8_t main_brightness;
  uint8_t main_warmness;
  uint8_t ring_hue;
  uint8_t ring_brightness;
};

class StorageHandler : public IFanControllerListener, public ILightControllerListener{
public:
  StorageHandler(LightController* lc, FanController* fc);
  void store();
  void restore(String mac_address);
  DeviceConfig getDeviceConfig() {
    return deviceConfig;
  }

  void onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue);
  void onFanControllerChange(int fan_speed);

private:
  LightController* lightCtrl;
  FanController* fanCtrl;

  Preferences preferences;
  DeviceConfig deviceConfig;
};
#endif  // STORAGE_HANDLER_H
