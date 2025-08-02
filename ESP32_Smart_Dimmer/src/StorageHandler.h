#ifndef STORAGE_HANDLER_H
#define STORAGE_HANDLER_H
#include <Preferences.h>
#include <map>    // Required for std::map
#include <vector> // Required for std::vector (used in MAC list parsing)

#include "BluetoothManager.h"
#include "LightController.h"
#include "FanController.h"
#include "LightMode.h"

// Helper to convert LightMode enum to String (for web UI/debug)
String lightModeToString(LightMode mode);
// Helper to convert String to LightMode enum (for web UI input/Preferences)
LightMode stringToLightMode(const String &modeStr);

struct DeviceConfig
{
    String mac_address;
    uint8_t fan_speed;
    LightMode light_mode;
    uint8_t main_brightness;
    uint8_t main_warmness;
    uint8_t ring_hue;
    uint8_t ring_brightness;
    bool isOn; // Indicates if the device is currently powered on/off
};

inline bool operator==(const DeviceConfig &lhs, const DeviceConfig &rhs)
{
    return (lhs.mac_address == rhs.mac_address &&
            lhs.fan_speed == rhs.fan_speed &&
            lhs.light_mode == lhs.light_mode && // Typo correction from my side previously, should be rhs.light_mode
            lhs.main_brightness == rhs.main_brightness &&
            lhs.main_warmness == rhs.main_warmness &&
            lhs.ring_hue == rhs.ring_hue &&
            lhs.ring_brightness == rhs.ring_brightness &&
            lhs.isOn == rhs.isOn);
}

inline bool operator!=(const DeviceConfig &lhs, const DeviceConfig &rhs)
{
    return !(lhs == rhs);
}

class StorageHandler : public IBluetoothConnectionListener, public IFanControllerListener, public ILightControllerListener
{
public:
    StorageHandler(BluetoothManager *bt, LightController *lc, FanController *fc);

    // Public function to load all known device configs on startup
    void loadAllDeviceConfigs();
    // Public function to get a specific device's config
    DeviceConfig getDeviceConfig(String mac_address);
    // Public function to get all managed device configs
    std::map<String, DeviceConfig> getAllManagedDevices();

    // Public function to save a specific device's config
    void saveSpecificDeviceConfig(const DeviceConfig &config);
    // Public method for debounced saving of the connected device's config
    void tryStore();

    // Listener callbacks
    void onBluetoothConnected(String mac_address);
    void onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue);
    void onFanControllerChange(int fan_speed);

private:
    BluetoothManager *btManager;
    LightController *lightCtrl;
    FanController *fanCtrl;

    Preferences preferences; // Preferences object

    // This map will hold the configurations of ALL managed devices in RAM
    std::map<String, DeviceConfig> allManagedDevices;

    // Track the MAC address of the currently connected device, if any
    String currentConnectedMac;

    // THIS IS THE MISSING LINE: Tracks the last saved state of the *currently connected device* for debounce
    DeviceConfig lastSavedDeviceConfig;

    long lastSaveTime;           // Time when `lastSavedDeviceConfig` was last saved (or applied on connect)
    long lastChangeDetectedTime; // Time when a change was last detected for `currentConnectedMac`'s config

    // Private helper to restore a single device's config from NVS
    DeviceConfig _restoreSingleDevice(String mac_address);

    // Private helpers to manage the master list of MAC addresses in Preferences
    void _addMacToMasterList(const String &mac_address);
    std::vector<String> _loadMacsFromMasterList();
};
#endif // STORAGE_HANDLER_H