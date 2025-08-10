#include "StorageHandler.h"
#include "Utils.h"
#include <Arduino.h> // Ensure Arduino core functions like millis() are available
#include <nvs.h>
#include <nvs_flash.h>

// Use unsigned long for timestamps to avoid rollover issues after ~50 days
const unsigned long DEBOUNCE_DELAY_MS = 2000;    // Wait 2 seconds of inactivity before saving a connected device
const unsigned long MIN_SAVE_INTERVAL_MS = 1000; // Minimum 1 second between actual writes (if tryStore triggers)

// --- StorageHandler Constructor ---
StorageHandler::StorageHandler(BluetoothManager *bt, LightController *lc, FanController *fc)
    : btManager(bt), lightCtrl(lc), fanCtrl(fc), lastSaveTime(0), lastChangeDetectedTime(0)
{
    // Global preferences.begin() is typically done in main setup()
    bt->registerDeviceConnectedListener(this);
    lc->registerListener(this);
    fc->registerListener(this);
}

// --- Public Method: Load all device configurations from Preferences ---
void StorageHandler::loadAllDeviceConfigs()
{
    log_i("Loading all device configurations from Preferences...");
    allManagedDevices.clear(); // Clear any existing in-memory data

    std::vector<String>* macAddresses = _loadMacsFromMasterList();

    if (macAddresses->empty())
    {
        log_i("No device MACs found in master list.");
        return;
    }

    for (const String &mac : *macAddresses)
    {
        DeviceConfig config = _restoreSingleDevice(mac);
        if (!config.mac_address.isEmpty())
        { // Check if restore was successful (i.e., it found a MAC)
            allManagedDevices[mac] = config;
            log_i("  Loaded config for %s: Mode=%s, Brightness=%d, IsOn=%d",
                          mac.c_str(), lightModeToString(config.light_mode).c_str(),
                          config.main_brightness, config.is_on);
        }
    }
    log_i("Finished loading %d devices into memory.", allManagedDevices.size());
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

/**
 * Loads the master list of MAC addresses from NVS.
 */
std::vector<String>* StorageHandler::_loadMacsFromMasterList()
{
    std::vector<String>* macs = new std::vector<String>();
    preferences.begin("master_list", true);
    String macsString = preferences.getString("mac_addresses", "");
    preferences.end();

    int lastIndex = -1;
    while (macsString.length() > 0)
    {
        int commaIndex = macsString.indexOf(',');
        if (commaIndex == -1)
        {
            break;
        }
        String mac = macsString.substring(0, commaIndex);
        macs->push_back(mac);
        Serial.println(mac);
        macsString.remove(0, commaIndex + 1);
        log_i("loaded %s", mac.c_str());
    }
    return macs;
}

/**
 * Loads the master list of MAC addresses from NVS.
 */
void StorageHandler::_addMacToMasterList(const String &mac_address)
{
    String mac_addr = mac_address;
    mac_addr.toUpperCase();
    std::vector<String>* macs = _loadMacsFromMasterList();
    bool found = false;
    for (const String &mac : *macs)
    {
        if (mac.equalsIgnoreCase(mac_addr))
        {
            found = true;
            log_i("mac already in master list: %s", mac_addr.c_str());
            break;
        }
    }
    if (!found)
    {
        macs->push_back(mac_addr);
        String macsString = "";
        for (const String &mac : *macs)
        {
            macsString += mac + ",";
        }
        log_i("added mac to master list: %s (storing mac_addresses = %s)", mac_addr.c_str(), macsString.c_str());
        preferences.begin("master_list", false);
        preferences.putString("mac_addresses", macsString);
        preferences.end();
    }
    delete macs;
}

/**
 * Removes a MAC address from the master list.
 */
void StorageHandler::_removeMacFromMasterList(const String &mac_address)
{
    String mac_addr = mac_address;
    std::vector<String>* macs = _loadMacsFromMasterList();
    for (size_t i = 0; i < macs->size(); ++i)
    {
        if ((*macs)[i].equalsIgnoreCase(mac_addr))
        {
            macs->erase(macs->begin() + i);
            break;
        }
    }
    String macsString = "";
    for (const String &mac : *macs)
    {
        macsString += mac + ",";
    }
    log_i("removed mac from master list: %s (storing mac_addresses = %s)", mac_addr.c_str(), macsString.c_str());
    preferences.begin("master_list", false);
    preferences.putString("mac_addresses", macsString);
    preferences.end();
    delete macs;
}

// --- Private Helper: Restore (load) a single device's config from Preferences ---
DeviceConfig StorageHandler::_restoreSingleDevice(String mac_address)
{
    DeviceConfig config;
    mac_address.toUpperCase();
    config.mac_address = mac_address; // Always set MAC for the config being restored

    // Use the full MAC address for namespace uniqueness
    String prefNS = getDeviceNamespace(mac_address);
    log_i("Restoring config for %s from NVS (namespace: %s)...", mac_address.c_str(), prefNS.c_str());
    preferences.begin(prefNS.c_str(), true); // Open device namespace (read-only)

    // Check if namespace has data (e.g., if "fan_speed" key exists)
    if (preferences.isKey("fan_speed"))
    { // Check for a key to confirm data exists for this device
        config.name = preferences.getString("name", "Unnamed");
        config.fan_speed = preferences.getUChar("fan_speed", 0);
        config.light_mode = static_cast<LightMode>(preferences.getInt("light_mode", LightMode::MAIN_LIGHT));
        config.main_brightness = preferences.getUChar("main_brightness", 0);
        config.main_warmness = preferences.getUChar("main_warmness", 0);
        config.ring_hue = preferences.getUChar("ring_hue", 0);
        config.ring_brightness = preferences.getUChar("ring_brightness", 0);
        config.is_on = preferences.getBool("is_on", false); // Read isOn
        log_i("Restored config for %s from NVS.", mac_address.c_str());
    }
    else
    {
        // If no existing config, initialize with defaults
        log_i("No existing config for %s, initializing with defaults.", mac_address.c_str());
        config.name = "Unnamed";
        config.fan_speed = 0;
        config.light_mode = LightMode::MAIN_LIGHT;
        config.main_brightness = 8;
        config.main_warmness = 140;
        config.ring_hue = 0;
        config.ring_brightness = 160;
        config.is_on = false; // Default new devices to off
        // This default config will be added to allManagedDevices and then saved by saveSpecificDeviceConfig
        // when a device is first seen/connected.
    }
    preferences.end(); // Close device namespace
    return config;
}

// --- Public Method: Save a specific device's configuration to Preferences ---
void StorageHandler::saveSpecificDeviceConfig(const DeviceConfig &config)
{
    DeviceConfig conf = config; // Use local variable to avoid memory issues
    String prefNS = getDeviceNamespace(conf.mac_address);
    log_i("StorageHandler: Saving config for %s to Preferences (namespace: %s)...\n", conf.mac_address.c_str(), prefNS);

    preferences.begin(prefNS.c_str(), false); // Open device namespace (read-write)

    // Write all current values for the config
    preferences.putString("name", conf.name);
    preferences.putUChar("fan_speed", conf.fan_speed);
    preferences.putInt("light_mode", static_cast<int>(conf.light_mode));
    preferences.putUChar("main_brightness", conf.main_brightness);
    preferences.putUChar("main_warmness", conf.main_warmness);
    preferences.putUChar("ring_hue", conf.ring_hue);
    preferences.putUChar("ring_brightness", conf.ring_brightness);
    preferences.putBool("is_on", conf.is_on); // Save isOn

    preferences.end(); // Close device namespace

    _addMacToMasterList(conf.mac_address); // Ensure MAC is in the master list

    // If the saved config is for the currently connected device, update its 'lastSavedDeviceConfig'
    if (conf.mac_address.equalsIgnoreCase(currentConnectedMac))
    {
        lastSavedDeviceConfig = conf; // Update the last saved state for the connected device
        lastSaveTime = millis();        // Record save time
    }

    allManagedDevices[conf.mac_address] = conf;

    Serial.printf("StorageHandler: Saved %s config: Mode=%s, Brightness=%d, Fan=%d, IsOn=%d\n",
                  conf.mac_address.c_str(), lightModeToString(conf.light_mode).c_str(),
                  conf.main_brightness, conf.fan_speed, conf.is_on);
}

/**
 * @brief Loads a specific device's configuration into the provided struct.
 */
bool StorageHandler::loadSpecificDeviceConfig(const String& mac_address, DeviceConfig& config) {
    if (allManagedDevices.count(mac_address) > 0) {
        config = allManagedDevices.at(mac_address);
        return true;
    }
    return false;
}
/**
 * Checks if a device is configured.
 */
bool StorageHandler::isDeviceConfigured(const String &mac_address)
{
    return allManagedDevices.count(mac_address) > 0;
}

/**
 * Deletes a device's config from both RAM and NVS.
 */
bool StorageHandler::deleteDeviceConfig(const String &mac_address)
{
    if (isDeviceConfigured(mac_address))
    {
        allManagedDevices.erase(mac_address);  // Erase from RAM
        _removeMacFromMasterList(mac_address); // Remove from master list in NVS

        // Erase the device's config from NVS
        preferences.begin(String("device_" + mac_address).c_str(), false);
        preferences.clear();
        preferences.end();
        log_i("Removed device %s from NVS.", mac_address.c_str());
        return true;
    }
    return false;
}

// --- Listener: On Bluetooth Connected ---
void StorageHandler::onDeviceConnected(String mac_address)
{
    currentConnectedMac = mac_address; // Store the currently connected MAC
    log_i("Bluetooth connected to MAC: %s.", mac_address.c_str());
    std::vector<String>* macAddresses = _loadMacsFromMasterList();
    if (std::find(macAddresses->begin(), macAddresses->end(), mac_address) == macAddresses->end()){
        log_w("mac address not in valid addresses list:");
        for (size_t i = 0; i < macAddresses->size(); ++i) {
            log_i("  %s", (*macAddresses)[i].c_str());
        }
        return;
    }

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
    log_d("Light command sent to connected device.");

    delay(500); // Small delay before fan command
    fanCtrl->setSpeed(configForConnectedDevice.fan_speed);
    log_d("Fan command sent to connected device.");

    // Reset change detection for the connected device as we just applied its config
    lastChangeDetectedTime = millis();
    lastSaveTime = millis(); // Mark as just saved/applied
}

// --- Listener: On Light Controller Change ---
void StorageHandler::onLightControllerChange(LightMode light_mode, int main_brightness, int main_warmness, int ring_brightness, int ring_hue)
{
    if (currentConnectedMac.isEmpty() || !allManagedDevices.count(currentConnectedMac))
    {
        log_w("Light change detected, but no device connected or managed for updates.");
        return;
    }
    log_i("Light change detected for %s. Updating in-memory config.", currentConnectedMac.c_str());

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
        log_w("StorageHandler: Fan change detected, but no device connected or managed for updates.");
        return;
    }
    log_i("StorageHandler: Fan change detected for %s. Updating in-memory config.\n", currentConnectedMac.c_str());

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

void StorageHandler::listNvsData(){
    nvs_flash_init();

    // Iterator for namespaces
    nvs_iterator_t namespace_it = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY);
    
    while (namespace_it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(namespace_it, &info);
        Serial.printf("Namespace: %s\n", info.namespace_name);

        // Separate iterator for keys within the current namespace
        nvs_iterator_t key_it = nvs_entry_find("nvs", info.namespace_name, NVS_TYPE_ANY);

        // while (key_it != NULL) {
        //     nvs_entry_info_t key_info;
        //     nvs_entry_info(key_it, &key_info);
        //     Serial.printf("  Key: %s, Type: %d\n", key_info.key, key_info.type);
        //     key_it = nvs_entry_next(key_it);
        // }

        namespace_it = nvs_entry_next(namespace_it);
    }
}