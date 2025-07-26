#include "WebServerModule.h"
#include <Arduino.h> // For Serial.print

// Note: If your stub methods are within a class, you would pass an instance
// of that class to WebServerModule during its initialization, and then
// call methods on that instance. For now, we assume global access via externs.

// Constructor - initializes the WebServer on port 80
WebServerModule::WebServerModule() : _server(80) {}

bool WebServerModule::begin() {
    // Initialize SPIFFS before serving files
    if (!SPIFFS.begin(true)) {
        Serial.println("WebServerModule: Error mounting SPIFFS! Web server cannot serve files.");
        return false;
    }
    Serial.println("WebServerModule: SPIFFS mounted successfully.");

    // Define web server routes
    _server.on("/", HTTP_GET, std::bind(&WebServerModule::handleRoot, this));
    _server.on("/control", HTTP_GET, std::bind(&WebServerModule::handleControl, this));
    _server.onNotFound(std::bind(&WebServerModule::handleNotFound, this));

    _server.begin();
    Serial.println("WebServerModule: HTTP server started on port 80.");
    return true;
}

void WebServerModule::handleClient() {
    _server.handleClient();
}

void WebServerModule::handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        Serial.println("WebServerModule: Failed to open index.html from SPIFFS.");
        _server.send(404, "text/plain", "File Not Found. Please upload index.html to SPIFFS.");
        return;
    }
    _server.streamFile(file, "text/html");
    file.close();
    Serial.println("WebServerModule: Served index.html");
}

void WebServerModule::handleControl() {
    String mode = _server.arg("mode");
    int brightness = _server.arg("bright").toInt();
    int warmness = _server.arg("warm").toInt();
    int rgbHue = _server.arg("hue").toInt();
    int rgbBrightness = _server.arg("rgbBright").toInt();
    int fanSpeed = _server.arg("fan").toInt();

    Serial.print("WebServerModule: Received Web Control: ");
    Serial.print("Mode="); Serial.print(mode);
    Serial.print(", Bright="); Serial.print(brightness);
    Serial.print(", Warm="); Serial.print(warmness);
    Serial.print(", Hue="); Serial.print(rgbHue);
    Serial.print(", RGBBright="); Serial.print(rgbBrightness);
    Serial.print(", Fan="); Serial.println(fanSpeed);

    // Call your existing stub methods
    // setLightMode(mode);
    // setBrightness(brightness);
    // setWarmness(warmness);
    // setRgbHue(rgbHue);
    // setRgbBrightness(rgbBrightness);
    // setFanSpeed(fanSpeed);

    _server.send(200, "text/plain", "OK"); // Send a simple "OK" back to the client
}

void WebServerModule::handleNotFound() {
    String path = _server.uri();
    Serial.print("WebServerModule: Requested file: ");
    Serial.println(path);

    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            String contentType = getContentType(path);
            _server.streamFile(file, contentType);
            file.close();
            Serial.print("WebServerModule: Served "); Serial.println(path);
            return;
        }
    }
    _server.send(404, "text/plain", "File Not Found");
    Serial.println("WebServerModule: File Not Found (404)");
}

String WebServerModule::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else return "application/octet-stream";
}