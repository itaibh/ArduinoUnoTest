#include <vector>
#include <string>
#include "WifiHandler.h"
#include "WebServerModule.h"
#include "BluetoothManager.h"
#include "LightController.h"
#include "FanController.h"
#include "HardwareInputHandler.h"
#include "StorageHandler.h"

// --- Pin Definitions ---
const int ROTARY_ENCODER_CLK_PIN = 18;
const int ROTARY_ENCODER_DT_PIN = 17;
const int ROTARY_ENCODER_SW_PIN = 16;
const int FAN_SPEED_UP_BTN_PIN = 22;
const int FAN_SPEED_DOWN_BTN_PIN = 23;
const int ROTARY_ENCODER_STEPS_PER_NOTCH = 4;

// --- Bluetooth Target ---
uint8_t targetDeviceAddress[6] = { 0xC9, 0xA3, 0x05, 0x36, 0xC4, 0x72 };

// --- Object Instantiation ---
// Create the core components, passing dependencies via constructors.
BluetoothManager btManager(targetDeviceAddress, "ESP32_Master_BT");
LightController lightController(&btManager);
FanController fanController(&btManager);
HardwareInputHandler inputHandler(
  &lightController,
  &fanController,
  ROTARY_ENCODER_CLK_PIN,
  ROTARY_ENCODER_DT_PIN,
  ROTARY_ENCODER_SW_PIN,
  ROTARY_ENCODER_STEPS_PER_NOTCH,
  FAN_SPEED_UP_BTN_PIN,
  FAN_SPEED_DOWN_BTN_PIN);
StorageHandler storageHandler(
  &btManager,
  &lightController,
  &fanController);
WifiHandler wifiHandler;
WebServerModule webServer(
  &lightController,
  &fanController);

void listSpiffsFiles() {
    Serial.println("\n--- Listing SPIFFS Files ---");
    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    Serial.println("--- End of SPIFFS Listing ---\n");
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Smart Dimmer Prototype Starting...");
  
  // 1. Connect to WiFi or start configuration portal
  Serial.println("Attempting WiFi connection or starting AP for configuration...");
  if (!wifiHandler.connect()) {  // No arguments needed here!
    Serial.println("WiFi connection failed or configuration timed out.");
    Serial.println("Please connect to AP: 'SmartLight_SETUP' to configure WiFi.");
    // If it's in AP mode, the web server won't start on your home network IP.
    // You might want a simplified web server for the AP mode to show status.
  } else {
    // WiFi is connected in STA mode
    Serial.println("WiFi connected. Starting Web Server...");
    if (!webServer.begin()) {
      Serial.println("Web server failed to start.");
    } else {
      listSpiffsFiles();
    }
  }

  // Initialize all components
  btManager.begin();
//   inputHandler.begin();

  Serial.println("Setup complete.");
}

void loop() {

  // Only handle web clients if we are actually connected to STA WiFi
  if (wifiHandler.isConnected()) {
    webServer.handleClient();
  } else {
    // If not connected (e.g., in AP config mode), you could do other tasks
    // or just blink an LED to indicate awaiting config.
    // Serial.println("Waiting for WiFi configuration...");
    // You might want to blink an LED here to indicate AP mode
    delay(1000);  // Simple delay to prevent hammering serial, remove for real-time
  }

  // Update all components in the main loop.
  // Each component handles its own timing and state.
  btManager.update();
  inputHandler.update();
  storageHandler.tryStore();

  delay(5);  // Small delay for stability
}