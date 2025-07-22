#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <BluetoothSerial.h>

// Define command types for clarity
enum CommandType {
  CMD_LIGHT_ON_OFF,
  CMD_LIGHT_INTENSITY,
  CMD_LIGHT_WARMNESS,
  CMD_RGB,
  CMD_FAN_SPEED
};

class BluetoothManager {
public:
  BluetoothManager(uint8_t* targetAddress, char* deviceName);
  void begin();
  void update();  // Handles connection logic
  bool isConnected();
  void sendCommand(CommandType cmd, const uint8_t* payload, size_t payloadSize);

private:
  BluetoothSerial SerialBT;
  BTAddress targetDeviceAddress;
  const char* espDeviceName;
  bool deviceConnected = false;

  void connectToServer();
  static void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param);

  // Static pointer to the instance to be used in the static callback
  static BluetoothManager* instance;
};

#endif  // BLUETOOTH_MANAGER_H