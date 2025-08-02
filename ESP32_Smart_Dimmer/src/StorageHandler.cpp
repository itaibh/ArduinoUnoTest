#include "StorageHandler.h"
#include <Arduino.h> // Ensure Arduino core functions like millis() are available

// Use unsigned long for timestamps to avoid rollover issues after ~50 days
const unsigned long DEBOUNCE_DELAY_MS = 2000;    // Wait 2 seconds of inactivity before saving a connected device
const unsigned long MIN_SAVE_INTERVAL_MS = 1000; // Minimum 1 second between actual writes (if tryStore triggers)

// --- StorageHandler Constructor ---
StorageHandler::StorageHandler(BluetoothManager *bt, LightController *lc, FanController *fc)
    : btManager(bt), lightCtrl(lc), fanCtrl(fc), lastSaveTime(0), lastChangeDetectedTime(0)
{
    // Global preferences.begin() is typically done in main setup()
    bt->registerConnectionListener(this);
    lc->registerListener(this);
    fc->registerListener(this);
}

// --- Public Method: Load all device configurations from Preferences ---
void StorageHandler::loadAllDeviceConfigs()
{
    Serial.println("StorageHandler: Loading all device configurations from Preferences...");
    allManagedDevices.clear(); // Clear any existing in-memory data

    std::vector<String> macAddresses = _loadMacsFromMasterList();

    if (macAddresses.empty())
    {
        Serial.println("StorageHandler: No device MACs found in master list.");
        return;
    }

    for (const String &mac : macAddresses)
    {
        DeviceConfig config = _restoreSingleDevice(mac);
        if (!config.mac_address.isEmpty())
        { // Check if restore was successful (i.e., it found a MAC)
            allManagedDevices[mac] = config;
            Serial.printf("  Loaded config for %s: Mode=%s, Brightness=%d, IsOn=%d\n",
                          mac.c_str(), lightModeToString(config.light_mode).c_str(),
                          config.main_brightness, config.isOn);
        }
    }
    Serial.printf("StorageHandler: Finished loading %d devices into memory.\n", allManagedDevices.size());
}

// --- Public Method: Get a specific device's configuration ---
DeviceConfig StorageHandler::getDeviceConfig(String mac_address)
{
    if (allManagedDevices.count(mac_address))
    {
        return allManagedDevices[mac_address];
    }
    Serial.printf("StorageHandler: Device config not found for MAC: %s\n", mac_address.c_str());
    // Return a default/empty DeviceConfig if not found.
    // It's good practice to ensure this default is valid (e.g., all fields zeroed).
    return DeviceConfig(); // Assumes DeviceConfig has a default constructor that initializes fields well
}

// --- Public Method: Get all managed device configurations ---
std::map<String, DeviceConfig> StorageHandler::getAllManagedDevices()
{
    return allManagedDevices;
}

// --- Private Helper: Load master list of MAC addresses ---
std::vector<String> StorageHandler::_loadMacsFromMasterList()
{
    std::vector<String> macs;
    preferences.begin("device_list", true); // Open master list namespace (read-only)
    String macListStr = preferences.getString("macs", "");
    preferences.end(); // Close master list namespace

    if (macListStr.isEmpty())
        return macs;

    // Simple parser for comma-separated MAC addresses
    int firstComma = 0;
    int nextComma = -1;
    do
    {
        nextComma = macListStr.indexOf(',', firstComma);
        if (nextComma == -1)
        { // Last MAC address
            macs.push_back(macListStr.substring(firstComma));
        }
        else
        {
            macs.push_back(macListStr.substring(firstComma, nextComma));
        }
        firstComma = nextComma + 1;
    } while (nextComma != -1);

    return macs;
}

// --- Private Helper: Add MAC to master list (if not already present) ---
void StorageHandler::_addMacToMasterList(const String &mac_address)
{
    preferences.begin("device_list", false); // Open master list namespace (read-write)
    String macListStr = preferences.getString("macs", "");

    // FIX: Use indexOf instead of contains
    if (macListStr.indexOf(mac_address) == -1)
    { // Check if MAC is NOT in the list
        if (!macListStr.isEmpty())
        {
            macListStr += ","; // Add comma if not the first entry
        }
        macListStr += mac_address;
        preferences.putString("macs", macListStr);
        Serial.printf("StorageHandler: Added %s to master MAC list.\n", mac_address.c_str());
    }
    preferences.end(); // Close master list namespace
}

// --- Private Helper: Restore (load) a single device's config from Preferences ---
DeviceConfig StorageHandler::_restoreSingleDevice(String mac_address)
{
    DeviceConfig config;
    config.mac_address = mac_address; // Always set MAC for the config being restored

    String macNoColons = mac_address;
    macNoColons.replace(":", "");
    // Use the full MAC address for namespace uniqueness
    String prefNS = "devcfg_" + macNoColons;

    preferences.begin(prefNS.c_str(), true); // Open device namespace (read-only)

    // Check if namespace has data (e.g., if "fan_speed" key exists)
    if (preferences.isKey("fan_speed"))
    { // Check for a key to confirm data exists for this device
        config.fan_speed = preferences.getUChar("fan_speed", 0);
        config.light_mode = static_cast<LightMode>(preferences.getInt("light_mode", LightMode::MAIN_LIGHT));
        config.main_brightness = preferences.getUChar("main_brightness", 0);
        config.main_warmness = preferences.getUChar("main_warmness", 0);
        config.ring_hue = preferences.getUChar("ring_hue", 0);
        config.ring_brightness = preferences.getUChar("ring_brightness", 0);
        config.isOn = preferences.getBool("is_on", false); // Read isOn
        Serial.printf("StorageHandler: Restored config for %s from NVS.\n", mac_address.c_str());
    }
    else
    {
        // If no existing config, initialize with defaults
        Serial.printf("StorageHandler: No existing config for %s, initializing with defaults.\n", mac_address.c_str());
        config.fan_speed = 0;
        config.light_mode = LightMode::MAIN_LIGHT;
        config.main_brightness = 0;
        config.main_warmness = 0;
        config.ring_hue = 0;
        config.ring_brightness = 0;
        config.isOn = false; // Default new devices to off
        // This default config will be added to allManagedDevices and then saved by saveSpecificDeviceConfig
        // when a device is first seen/connected.
    }
    preferences.end(); // Close device namespace
    return config;
}

// --- Public Method: Save a specific device's configuration to Preferences ---
void StorageHandler::saveSpecificDeviceConfig(const DeviceConfig &config)
{
    Serial.printf("StorageHandler: Saving config for %s to Preferences...\n", config.mac_address.c_str());

    String macNoColons = config.mac_address;
    macNoColons.replace(":", "");
    String prefNS = "devcfg_" + macNoColons; // Using full MAC for namespace uniqueness

    preferences.begin(prefNS.c_str(), false); // Open device namespace (read-write)

    // Write all current values for the config
    preferences.putUChar("fan_speed", config.fan_speed);
    preferences.putInt("light_mode", static_cast<int>(config.light_mode));
    preferences.putUChar("main_brightness", config.main_brightness);
    preferences.putUChar("main_warmness", config.main_warmness);
    preferences.putUChar("ring_hue", config.ring_hue);
    preferences.putUChar("ring_brightness", config.ring_brightness);
    preferences.putBool("is_on", config.isOn); // Save isOn

    preferences.end(); // Close device namespace

    _addMacToMasterList(config.mac_address); // Ensure MAC is in the master list

    // If the saved config is for the currently connected device, update its 'lastSavedDeviceConfig'
    if (config.mac_address == currentConnectedMac)
    {
        lastSavedDeviceConfig = config; // Update the last saved state for the connected device
        lastSaveTime = millis();        // Record save time
    }

    Serial.printf("StorageHandler: Saved %s config: Mode=%s, Brightness=%d, Fan=%d, IsOn=%d\n",
                  config.mac_address.c_str(), lightModeToString(config.light_mode).c_str(),
                  config.main_brightness, config.fan_speed, config.isOn);
}

// --- Listener: On Bluetooth Connected ---
void StorageHandler::onBluetoothConnected(String mac_address)
{
    Serial.printf("StorageHandler: Bluetooth connected to MAC: %s.\n", mac_address.c_str());
    currentConnectedMac = mac_address; // Store the currently connected MAC

    // Get (or create default) the DeviceConfig for this MAC address
    DeviceConfig configForConnectedDevice = _restoreSingleDevice(mac_address);

    // Update the in-memory map
    allManagedDevices[mac_address] = configForConnectedDevice;

    // Set the lastSavedDeviceConfig for the currently connected device for debounce logic
    lastSavedDeviceConfig = configForConnectedDevice;

    // Apply the restored/default config to controllers
    lightCtrl->setAll(configForConnectedDevice.light_mode,
                      configForConnectedDevice.main_brightness,
                      configForConnectedDevice.main_warmness,
                      configForConnectedDevice.ring_brightness,
                      configForConnectedDevice.ring_hue);
    Serial.println("[Debug] Light command sent to connected device.");

    delay(500); // Small delay before fan command
    fanCtrl->setSpeed(configForConnectedDevice.fan_speed);
    Serial.println("[Debug] Fan command sent to connected device.");

    // Reset change detection for the connected device as we just applied its config
    lastChangeDetectedTime = millis();
    lastSaveTime = millis(); // Mark as just saved/applied
}

// --- Listener: On Light Controller Change ---
void StorageHandler::onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue)
{
    if (currentConnectedMac.isEmpty() || !allManagedDevices.count(currentConnectedMac))
    {
        Serial.println("StorageHandler: Light change detected, but no device connected or managed for updates.");
        return;
    }
    Serial.printf("StorageHandler: Light change detected for %s. Updating in-memory config.\n", currentConnectedMac.c_str());

    DeviceConfig &currentConfig = allManagedDevices[currentConnectedMac]; // Get reference to modify

    // Update the in-memory config for the connected device
    currentConfig.light_mode = light_mode;
    currentConfig.main_brightness = main_brightness;
    currentConfig.main_warmness = main_warmness;
    currentConfig.ring_hue = ring_hue;
    currentConfig.ring_brightness = ring_brightness;
    // Note: isOn state - if your LightController knows if it's truly off (e.g., brightness=0 from web)
    // you might update currentConfig.isOn here or in a separate listener for power state.
    // For now, it remains as set by _restoreSingleDevice or saveSpecificDeviceConfig.

    lastChangeDetectedTime = millis(); // Mark that a change occurred for debounce
}

// --- Listener: On Fan Controller Change ---
void StorageHandler::onFanControllerChange(int fan_speed)
{
    if (currentConnectedMac.isEmpty() || !allManagedDevices.count(currentConnectedMac))
    {
        Serial.println("StorageHandler: Fan change detected, but no device connected or managed for updates.");
        return;
    }
    Serial.printf("StorageHandler: Fan change detected for %s. Updating in-memory config.\n", currentConnectedMac.c_str());

    DeviceConfig &currentConfig = allManagedDevices[currentConnectedMac]; // Get reference to modify
    currentConfig.fan_speed = fan_speed;

    lastChangeDetectedTime = millis(); // Mark that a change occurred for debounce
}

// --- Public Method: tryStore (Debounced save for connected device) ---
void StorageHandler::tryStore()
{
    // Only attempt to store if there's a connected device whose state we manage
    if (currentConnectedMac.isEmpty() || !allManagedDevices.count(currentConnectedMac))
    {
        return;
    }

    DeviceConfig &currentConfig = allManagedDevices[currentConnectedMac];

    // Check if the current in-memory config is different from the last saved one for this device
    // AND if debounce/min interval conditions are met.
    if (currentConfig != lastSavedDeviceConfig &&
        (millis() - lastChangeDetectedTime) > DEBOUNCE_DELAY_MS &&
        (millis() - lastSaveTime) > MIN_SAVE_INTERVAL_MS)
    {
        // Save the current in-memory config for the connected device
        saveSpecificDeviceConfig(currentConfig);
        // The `saveSpecificDeviceConfig` method will now also update `lastSavedDeviceConfig`
        // and `lastSaveTime` if the saved config is for the `currentConnectedMac`.
    }
}