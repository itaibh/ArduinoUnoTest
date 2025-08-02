#include "WebServerModule.h"
#include "Utils.h"
#include <Arduino.h> // For Serial.print

// Note: If your stub methods are within a class, you would pass an instance
// of that class to WebServerModule during its initialization, and then
// call methods on that instance. For now, we assume global access via externs.

// Constructor - initializes the WebServer on port 80
WebServerModule::WebServerModule(StorageHandler* sh, BluetoothManager *bt, LightController *lc, FanController *fc)
    : _server(80), storageHandler(sh), btManager(bt), lightCtrl(lc), fanCtrl(fc) {}

bool WebServerModule::begin()
{
    // Initialize SPIFFS before serving files
    if (!SPIFFS.begin(true))
    {
        Serial.println("WebServerModule: Error mounting SPIFFS! Web server cannot serve files.");
        return false;
    }
    Serial.println("WebServerModule: SPIFFS mounted successfully.");

    // Define web server routes
    _server.on("/", HTTP_GET, std::bind(&WebServerModule::handleRoot, this));
    _server.on("/control", HTTP_GET, std::bind(&WebServerModule::handleControl, this));
    _server.on("/discover_devices", HTTP_GET, std::bind(&WebServerModule::handleFindDevices, this));
    _server.onNotFound(std::bind(&WebServerModule::handleNotFound, this));

    _server.begin();
    Serial.println("WebServerModule: HTTP server started on port 80.");
    return true;
}

void WebServerModule::handleClient()
{
    _server.handleClient();
}

void WebServerModule::handleRoot()
{
    File file = SPIFFS.open("/index.html", "r");
    if (!file)
    {
        Serial.println("WebServerModule: Failed to open index.html from SPIFFS.");
        _server.send(404, "text/plain", "File Not Found. Please upload index.html to SPIFFS.");
        return;
    }
    _server.streamFile(file, "text/html");
    file.close();
    Serial.println("WebServerModule: Served index.html");
}

void WebServerModule::handleControl()
{
    String mode = _server.arg("mode");
    int brightness = _server.arg("bright").toInt();
    int warmness = _server.arg("warm").toInt();
    int rgbHue = _server.arg("hue").toInt();
    int rgbBrightness = _server.arg("rgbBright").toInt();
    int fanSpeed = _server.arg("fan").toInt();

    Serial.print("WebServerModule: Received Web Control: ");
    Serial.print("Mode=");
    Serial.print(mode);
    Serial.print(", Bright=");
    Serial.print(brightness);
    Serial.print(", Warm=");
    Serial.print(warmness);
    Serial.print(", Hue=");
    Serial.print(rgbHue);
    Serial.print(", RGBBright=");
    Serial.print(rgbBrightness);
    Serial.print(", Fan=");
    Serial.println(fanSpeed);

    // Call your existing stub methods
    if (mode == "off")
    {
        lightCtrl->turnOff();
    }
    else
    {
        LightMode lightMode = mode == "main" ? LightMode::MAIN_LIGHT : LightMode::RGB_RING;
        lightCtrl->setAll(lightMode, brightness, warmness, rgbBrightness, rgbHue);
    }
    fanCtrl->setSpeed(fanSpeed);

    _server.send(200, "text/plain", "OK"); // Send a simple "OK" back to the client
}

void WebServerModule::handleFindDevices()
{
    std::map<String, BtDevice> devices = btManager->scanForDevices();

    String jsonResponse = "[";
    bool firstDevice = true;
    for (auto const &pair : devices)
    { // Use 'pair' for C++11 compatibility
        const String &mac = pair.first;
        const BtDevice &config = pair.second;

        if (!firstDevice)
        {
            jsonResponse += ",";
        }
        firstDevice = false;

        jsonResponse += "{";
        jsonResponse += "\"address\":\"" + config.address + "\",";
        jsonResponse += "\"name\":" + config.name;
        jsonResponse += "}";
    }
    jsonResponse += "]";

    Serial.printf("Sent /discover_devices response (Manual JSON): %s\n", jsonResponse.c_str());
    _server.sendHeader("Access-Control-Allow-Origin", "*"); // IMPORTANT: Enable CORS
    _server.send(200, "application/json", jsonResponse);
}

void WebServerModule::handleGetAllDevices()
{
    // std::map<String, BtDevice> devices = btManager->scanForDevices();
    std::map<String, DeviceConfig> devices = storageHandler->getAllManagedDevices();

    String jsonResponse = "[";
    bool firstDevice = true;
    for (auto const &pair : devices)
    { // Use 'pair' for C++11 compatibility
        const String &mac = pair.first;
        const DeviceConfig &config = pair.second;

        if (!firstDevice)
        {
            jsonResponse += ",";
        }
        firstDevice = false;

        jsonResponse += "{";
        jsonResponse += "\"mac_address\":\"" + config.mac_address + "\",";
        jsonResponse += "\"fan_speed\":" + String(config.fan_speed) + ",";
        jsonResponse += "\"light_mode\":\"" + lightModeToString(config.light_mode) + "\",";
        jsonResponse += "\"main_brightness\":" + String(config.main_brightness) + ",";
        jsonResponse += "\"main_warmness\":" + String(config.main_warmness) + ",";
        jsonResponse += "\"ring_hue\":" + String(config.ring_hue) + ",";
        jsonResponse += "\"ring_brightness\":" + String(config.ring_brightness) + ",";
        jsonResponse += "\"is_on\":";
        jsonResponse += (config.isOn ? "true" : "false"); // Convert bool to "true" or "false"

        // Optional: Add connected status if currentConnectedMac is accessible
        // bool isConnected = (mac == storageHandler.currentConnectedMac);
        // jsonResponse += ",\"connected\":" + (isConnected ? "true" : "false");

        jsonResponse += "}";
    }
    jsonResponse += "]";

    Serial.printf("Sent /discover_devices response (Manual JSON): %s\n", jsonResponse.c_str());
    _server.sendHeader("Access-Control-Allow-Origin", "*"); // IMPORTANT: Enable CORS
    _server.send(200, "application/json", jsonResponse);
}

void WebServerModule::handleNotFound()
{
    String path = _server.uri();
    Serial.print("WebServerModule: Requested file: ");
    Serial.println(path);

    if (SPIFFS.exists(path))
    {
        File file = SPIFFS.open(path, "r");
        if (file)
        {
            String contentType = getContentType(path);
            _server.streamFile(file, contentType);
            file.close();
            Serial.print("WebServerModule: Served ");
            Serial.println(path);
            return;
        }
    }
    _server.send(404, "text/plain", "File Not Found");
    Serial.println("WebServerModule: File Not Found (404)");
}

String WebServerModule::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".json"))
        return "application/json";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else
        return "application/octet-stream";
}