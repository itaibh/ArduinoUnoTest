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

inline bool operator==(const DeviceConfig& lhs, const DeviceConfig& rhs) {
    return (lhs.mac_address     == rhs.mac_address &&
            lhs.fan_speed       == rhs.fan_speed &&
            lhs.light_mode      == rhs.light_mode &&
            lhs.main_brightness == rhs.main_brightness &&
            lhs.main_warmness   == rhs.main_warmness &&
            lhs.ring_hue        == rhs.ring_hue &&
            lhs.ring_brightness == rhs.ring_brightness);
}

inline bool operator!=(const DeviceConfig& lhs, const DeviceConfig& rhs) {
    return !(lhs == rhs);
}

class StorageHandler : public IFanControllerListener, public ILightControllerListener{
public:
  StorageHandler(LightController* lc, FanController* fc);
  void store();
  void tryStore();
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

  DeviceConfig lastSavedDeviceConfig;
  long lastSaveTime;
  long lastChangeDetectedTime;
};
#endif  // STORAGE_HANDLER_H
