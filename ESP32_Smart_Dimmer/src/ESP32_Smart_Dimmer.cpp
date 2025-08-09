#include <Arduino.h>
#include <vector>
#include <string>
#include "WifiHandler.h"
#include "WebServerModule.h"
#include "BluetoothManager.h"
#include "LightController.h"
#include "FanController.h"
#include "HardwareInputHandler.h"
#include "StorageHandler.h"
#include "Utils.h"

// --- Pin Definitions ---
const int ROTARY_ENCODER_CLK_PIN = 18;
const int ROTARY_ENCODER_DT_PIN = 17;
const int ROTARY_ENCODER_SW_PIN = 16;
const int FAN_SPEED_UP_BTN_PIN = 22;
const int FAN_SPEED_DOWN_BTN_PIN = 23;
const int ROTARY_ENCODER_STEPS_PER_NOTCH = 4;

// --- Object Instantiation ---
// Create the core components, passing dependencies via constructors.
BluetoothManager *btManager = nullptr;
LightController *lightController = nullptr;
FanController *fanController = nullptr;
// HardwareInputHandler inputHandler(
//     &lightController,
//     &fanController,
//     ROTARY_ENCODER_CLK_PIN,
//     ROTARY_ENCODER_DT_PIN,
//     ROTARY_ENCODER_SW_PIN,
//     ROTARY_ENCODER_STEPS_PER_NOTCH,
//     FAN_SPEED_UP_BTN_PIN,
//     FAN_SPEED_DOWN_BTN_PIN);
StorageHandler *storageHandler = nullptr; 
WifiHandler *wifiHandler = nullptr;
WebServerModule *webServer = nullptr;

void listSpiffsFiles()
{
    Serial.println("\n--- Listing SPIFFS Files ---");
    File root = SPIFFS.open("/");
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    Serial.println("--- End of SPIFFS Listing ---\n");
}

void setup()
{
    Serial.begin(115200);
    log_i("ESP32 Smart Dimmer Prototype Starting...");

    btManager = new BluetoothManager("ESP32_Master_BT");
    btManager->begin();

    lightController = new LightController(btManager);
    fanController = new FanController(btManager);
    
    storageHandler = new StorageHandler(btManager, lightController, fanController);
    storageHandler->listNvsData();
    storageHandler->loadAllDeviceConfigs();

    wifiHandler = new WifiHandler();

    // 1. Connect to WiFi or start configuration portal
    log_i("Attempting WiFi connection or starting AP for configuration...");
    if (!wifiHandler->connect())
    { // No arguments needed here!
        log_w("WiFi connection failed or configuration timed out.");
        log_w("Please connect to AP: 'SmartLight_SETUP' to configure WiFi.");
        // If it's in AP mode, the web server won't start on your home network IP.
        // You might want a simplified web server for the AP mode to show status.
    }
    else
    {
        webServer = new WebServerModule(
            storageHandler,
            btManager,
            lightController,
            fanController);

        if (!webServer)
        { // Always check for failed allocation
            log_e("FATAL: Failed to allocate WebServerModule!");
            while (true)
                ; // Halt
        }

        // WiFi is connected in STA mode
        log_i("WiFi connected. Starting Web Server...");
        if (!webServer->begin())
        {
            log_e("Web server failed to start.");
        }
        else
        {
            //   listSpiffsFiles();
        }
    }
    delay(100); // Give it some time
    log_i("Setup complete.");
}

void loop()
{
    // Only handle web clients if we are actually connected to STA WiFi
    if (wifiHandler->isConnected())
    {
        webServer->handleClient();
    }
    else
    {
        // If not connected (e.g., in AP config mode), you could do other tasks
        // or just blink an LED to indicate awaiting config.
        // Serial.println("Waiting for WiFi configuration...");
        // You might want to blink an LED here to indicate AP mode
        delay(1000); // Simple delay to prevent hammering serial, remove for real-time
    }

    btManager->clearInputBuffer();
    delay(5); // Small delay for stability
}