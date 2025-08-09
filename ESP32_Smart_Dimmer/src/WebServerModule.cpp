#include "WebServerModule.h"
#include "Utils.h"
#include <SPIFFS.h>
#include <functional>

/**
 * Constructor: Initializes pointers to component classes.
 */
WebServerModule::WebServerModule(StorageHandler* sh, BluetoothManager* bt, LightController* lc, FanController* fc)
    : storageHandler(sh), btManager(bt), lightCtrl(lc), fanCtrl(fc) {
    // Constructor logic if needed
}

/**
 * Begin: Initializes the web server and its routes.
 */
bool WebServerModule::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return false;
    }
    Serial.println("SPIFFS mounted successfully");

    setupRoutes();

    _server.begin();
    Serial.println("HTTP server started");
    return true;
}

/**
 * Handle client: Must be called in the main loop().
 */
void WebServerModule::handleClient() {
    _server.handleClient();
}

/**
 * Sets up all HTTP endpoints for the server.
 */
void WebServerModule::setupRoutes() {
    // Root path handler
    _server.on("/", HTTP_GET, std::bind(&WebServerModule::handleRoot, this));
    
    // API endpoints corresponding to the Python mock server
    _server.on("/discover_devices", HTTP_GET, std::bind(&WebServerModule::handleFindDevices, this));
    _server.on("/get_all_devices", HTTP_GET, std::bind(&WebServerModule::handleGetAllDevices, this));
    _server.on("/add_device", HTTP_GET, std::bind(&WebServerModule::handleAddDevice, this));
    _server.on("/remove_device", HTTP_GET, std::bind(&WebServerModule::handleRemoveDevice, this));
    _server.on("/control", HTTP_GET, std::bind(&WebServerModule::handleControl, this));

    // Not found handler
    _server.onNotFound(std::bind(&WebServerModule::handleNotFound, this));
}

/**
 * Serves index.html as the root page.
 */
void WebServerModule::handleRoot() {
    String path = "/index.html";
    if (_server.hasArg("download")) {
        path = _server.arg("download");
    }
    String contentType = getContentType(path);
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        _server.streamFile(file, contentType);
        file.close();
    } else {
        handleNotFound();
    }
}

/**
 * Handles the '/discover_devices' endpoint.
 * Returns a JSON array of discovered Bluetooth devices.
 */
void WebServerModule::handleFindDevices() {
    log_i("Handling /discover_devices request - performing Bluetooth scan.");
    log_i("context address: %p", this);
    btManager->scanForDevices(&WebServerModule::_onDeviceListReady, this);
}

void WebServerModule::_onDeviceListReady(std::map<String, BtDevice> scannedDevices, void* context) {
    log_i("context: %p", context);
    // Cast the void* context pointer back to a WebServerModule instance pointer.
    WebServerModule* instance = static_cast<WebServerModule*>(context);
    
    // Now you can safely call the non-static member function on that instance.
    if (instance) {
        log_i("calling onDeviceListReady with %d devices", scannedDevices.size());
        instance->onDeviceListReady(scannedDevices);
    }
}

void WebServerModule::onDeviceListReady(std::map<String, BtDevice> scannedDevices) {
    if (storageHandler == nullptr) {
        log_e("storage handler is null");
        return;
    }
    const auto& configuredDevices = storageHandler->getAllManagedDevices();

    String jsonResponse = "[";
    bool firstDevice = true;
    for (auto const& pair : scannedDevices) {
        const String& mac = pair.first;
        const BtDevice& btDevice = pair.second;

        if (!firstDevice) {
            jsonResponse += ",";
        }
        firstDevice = false;
        String deviceName = sanitizeString(btDevice.name);

        jsonResponse += "{";
        jsonResponse += "\"name\":\"" + deviceName + "\",";
        jsonResponse += "\"mac_address\":\"" + btDevice.address + "\",";
        
        bool isConfigured = configuredDevices.count(mac);
        jsonResponse += "\"is_configured\":";
        jsonResponse += isConfigured ? "true" : "false";
        
        jsonResponse += "}";
    }
    jsonResponse += "]";

    _server.send(200, "application/json", jsonResponse);
    log_i("Sent /discover_devices response. Count: %d", scannedDevices.size());
}

/**
 * Handles the '/get_all_devices' endpoint.
 * Returns a JSON object of all configured devices.
 */
void WebServerModule::handleGetAllDevices() {
    Serial.println("Handling /get_all_devices request.");

    const auto& devicesMap = storageHandler->getAllManagedDevices();

    // Matching the Python mock server's output: a JSON object, not an array
    String jsonResponse = "{";
    bool firstEntry = true;
    for (auto const& pair : devicesMap) {
        const String& mac = pair.first;
        const DeviceConfig& config = pair.second;

        if (!firstEntry) {
            jsonResponse += ",";
        }
        firstEntry = false;

        jsonResponse += "\"" + mac + "\":{";
        jsonResponse += "\"mac_address\":\"" + config.mac_address + "\",";
        jsonResponse += "\"name\":\"" + config.name + "\",";
        jsonResponse += "\"fan_speed\":" + String(config.fan_speed) + ",";
        jsonResponse += "\"light_mode\":\"" + lightModeToString(config.light_mode) + "\",";
        jsonResponse += "\"main_brightness\":" + String(config.main_brightness) + ",";
        jsonResponse += "\"main_warmness\":" + String(config.main_warmness) + ",";
        jsonResponse += "\"ring_hue\":" + String(config.ring_hue) + ",";
        jsonResponse += "\"ring_brightness\":" + String(config.ring_brightness) + ",";
        jsonResponse += "\"is_on\":";
        jsonResponse += config.is_on ? "true" : "false";
        jsonResponse += "}";
    }
    jsonResponse += "}";

    _server.send(200, "application/json", jsonResponse);
    log_i("Sent /get_all_devices response. Count: %d\n", devicesMap.size());
}

/**
 * Handles the '/add_device?name=<name>&address=<mac>' endpoint.
 */
void WebServerModule::handleAddDevice() {
    String name = _server.arg("name");
    String address = _server.arg("address");

    log_i("Handling /add_device request. Name: %s, Address: %s\n", name.c_str(), address.c_str());

    if (name.length() > 0 && address.length() > 0) {
        if (!storageHandler->isDeviceConfigured(address)) {
            DeviceConfig newConfig;
            newConfig.mac_address = address;
            newConfig.name = name;
            newConfig.fan_speed = 0;
            newConfig.light_mode = LightMode::MAIN_LIGHT;
            newConfig.main_brightness = 8;
            newConfig.main_warmness = 150;
            newConfig.ring_hue = 0;
            newConfig.ring_brightness = 0;
            newConfig.is_on = false;

            storageHandler->saveSpecificDeviceConfig(newConfig);
            _server.send(200, "text/plain", "OK");
            log_i("Device %s (%s) added successfully.\n", name.c_str(), address.c_str());
        } else {
            _server.send(409, "text/plain", "Error: Device already configured.");
            log_e("Error: Device %s already configured.\n", address.c_str());
        }
    } else {
        _server.send(400, "text/plain", "Error: Missing 'name' or 'address' parameters.");
        log_e("Error: Missing 'name' or 'address' parameters.");
    }
}

/**
 * Handles the '/remove_device?address=<mac>' endpoint.
 */
void WebServerModule::handleRemoveDevice() {
    String address = _server.arg("address");

    log_i("Handling /remove_device request for address: %s\n", address.c_str());

    if (address.length() > 0) {
        if (storageHandler->deleteDeviceConfig(address)) {
            _server.send(200, "text/plain", "OK");
            log_i("Device %s removed successfully.\n", address.c_str());
        } else {
            _server.send(404, "text/plain", "Error: Device not found.");
            log_e("Error: Device %s not found in storage.\n", address.c_str());
        }
    } else {
        _server.send(400, "text/plain", "Error: Missing 'address' parameter.");
        log_e("Error: Missing 'address' parameter.");
    }
}

/**
 * Handles the '/control?address=<mac>&<params>...' endpoint.
 */
void WebServerModule::handleControl() {
    String address = _server.arg("address");
    
    if (address.length() == 0) {
        _server.send(400, "text/plain", "Error: Missing 'address' parameter.");
        return;
    }

    log_i("Handling /control request for address: %s", address.c_str());

    // Retrieve the device's current state from StorageHandler
    DeviceConfig currentConfig;
    if (!storageHandler->loadSpecificDeviceConfig(address, currentConfig)) {
        _server.send(404, "text/plain", "Error: Device not found.");
        return;
    }

    // Update the device's state based on query parameters
    if (_server.hasArg("fan")) {
        currentConfig.fan_speed = _server.arg("fan").toInt();
        fanCtrl->setSpeed(currentConfig.fan_speed);
    }
    if (_server.hasArg("bright")) {
        currentConfig.main_brightness = _server.arg("bright").toInt();
    }
    if (_server.hasArg("warm")) {
        currentConfig.main_warmness = _server.arg("warm").toInt();
    }
    if (_server.hasArg("hue")) {
        currentConfig.ring_hue = _server.arg("hue").toInt();
    }
    if (_server.hasArg("rgbValue")) {
        currentConfig.ring_brightness = _server.arg("rgbValue").toInt();
    }
    if (_server.hasArg("mode")) {
        String lightModeArg = _server.arg("mode");
        if (lightModeArg == "off") {
            currentConfig.is_on = false;
        }
        else if (lightModeArg == "main") {
            currentConfig.is_on = true;
            currentConfig.light_mode = LightMode::MAIN_LIGHT;
        } else if (lightModeArg == "rgb") {
            currentConfig.is_on = true;
            currentConfig.light_mode = LightMode::RGB_RING;
        }
        else {
            Serial.printf("light mode not supported: %s\n", lightModeArg);
        }
    }

    // Now, send the control commands via Bluetooth
    if (btManager->sendConfigToDevice(currentConfig)) {
        // If the command was sent successfully, save the new state to storage
        storageHandler->saveSpecificDeviceConfig(currentConfig);
        _server.send(200, "text/plain", "OK");
        Serial.printf("Control commands sent and state saved for %s.\n", address.c_str());
    } else {
        _server.send(503, "text/plain", "Error: Could not send command via Bluetooth. Is the device connected?");
        Serial.printf("Error: Failed to send BT command to %s.\n", address.c_str());
    }
}

/**
 * Handles 404 (Not Found) errors.
 */
void WebServerModule::handleNotFound() {
    // Check if the requested file exists in SPIFFS and serve it
    String path = _server.uri();
    if (SPIFFS.exists(path)) {
        String contentType = getContentType(path);
        File file = SPIFFS.open(path, "r");
        _server.streamFile(file, contentType);
        file.close();
        return;
    }
    // If the file doesn't exist, send a 404
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += _server.uri();
    message += "\nMethod: ";
    message += (_server.method() == HTTP_GET) ? "GET" : "POST";
    _server.send(404, "text/plain", message);
}


/**
 * Helper to determine content type from file extension.
 */
String WebServerModule::getContentType(String filename) {
    if (_server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    return "text/plain";
}