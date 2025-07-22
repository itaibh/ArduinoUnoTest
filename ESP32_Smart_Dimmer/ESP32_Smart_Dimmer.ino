#include "BluetoothManager.h"
#include "LightController.h"
#include "FanController.h"
#include "InputHandler.h"
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
InputHandler inputHandler(
  &lightController,
  &fanController,
  ROTARY_ENCODER_CLK_PIN,
  ROTARY_ENCODER_DT_PIN,
  ROTARY_ENCODER_SW_PIN,
  ROTARY_ENCODER_STEPS_PER_NOTCH,
  FAN_SPEED_UP_BTN_PIN,
  FAN_SPEED_DOWN_BTN_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Smart Dimmer Prototype Starting...");

  // Initialize all components
  btManager.begin();
  inputHandler.begin();

  // Set initial device states
  lightController.setBrightness(120, true);  // force update
  lightController.turnOn();
  fanController.setSpeed(1);

  Serial.println("Setup complete.");
}

void loop() {
  // Update all components in the main loop.
  // Each component handles its own timing and state.
  btManager.update();
  inputHandler.update();

  delay(5);  // Small delay for stability
}