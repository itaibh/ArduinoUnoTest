#include "BluetoothManager.h"

// Initialize static instance pointer
BluetoothManager* BluetoothManager::instance = nullptr;

// --- Constants for BT Commands ---
const uint8_t BT_PREFIX[] = { 0x01, 0xfe, 0x00, 0x00, 0x51, 0x81, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d };
const uint8_t BT_SUFFIX[] = { 0x0e, 0x00 };
const uint8_t ON_OFF_DATA_PREFIX[] = { 0x07, 0x01, 0x03, 0x01 };
const uint8_t INTENSITY_DATA_PREFIX[] = { 0x07, 0x01, 0x03, 0x02 };
const uint8_t WARMNESS_DATA_PREFIX[] = { 0x07, 0x01, 0x03, 0x03 };
const uint8_t RGB_DATA_PREFIX[] = { 0x0A, 0x02, 0x03, 0x0C };
const uint8_t FAN_SPEED_DATA_PREFIX[] = { 0x07, 0x0e, 0x03, 0x03 };


BluetoothManager::BluetoothManager(uint8_t* targetAddress, char* deviceName)
  : targetDeviceAddress(targetAddress), espDeviceName(deviceName) {
  instance = this;  // Set the static instance pointer
}

void BluetoothManager::begin() {
  SerialBT.begin(espDeviceName, true);  // Master mode
  SerialBT.register_callback(btCallback);
}

void BluetoothManager::update() {
  if (!deviceConnected) {
    connectToServer();
  }
}

bool BluetoothManager::isConnected() {
  return deviceConnected;
}

void BluetoothManager::connectToServer() {
  Serial.println("Attempting to connect to target device...");
  if (!SerialBT.connect(targetDeviceAddress)) {
    Serial.println("Failed to initiate connection. Retrying in 5s...");
    delay(5000);
  }
}

void BluetoothManager::sendCommand(CommandType cmd, const uint8_t* payload, size_t payloadSize) {
  if (!deviceConnected) {
    Serial.println("Cannot send command: Not connected.");
    return;
  }

  const uint8_t* cmdPrefix;
  size_t cmdPrefixSize;
  uint8_t finalPacketByte6 = 0x18;  // Default value

  switch (cmd) {
    case CMD_LIGHT_ON_OFF:
      cmdPrefix = ON_OFF_DATA_PREFIX;
      cmdPrefixSize = sizeof(ON_OFF_DATA_PREFIX);
      break;
    case CMD_LIGHT_INTENSITY:
      cmdPrefix = INTENSITY_DATA_PREFIX;
      cmdPrefixSize = sizeof(INTENSITY_DATA_PREFIX);
      break;
    case CMD_LIGHT_WARMNESS:
      cmdPrefix = WARMNESS_DATA_PREFIX;
      cmdPrefixSize = sizeof(WARMNESS_DATA_PREFIX);
      break;
    case CMD_RGB:
      cmdPrefix = RGB_DATA_PREFIX;
      cmdPrefixSize = sizeof(RGB_DATA_PREFIX);
      finalPacketByte6 = 0x1c;  // Special value for RGB
      break;
    case CMD_FAN_SPEED:
      cmdPrefix = FAN_SPEED_DATA_PREFIX;
      cmdPrefixSize = sizeof(FAN_SPEED_DATA_PREFIX);
      break;
    default:
      Serial.println("Unknown command type.");
      return;
  }

  const int MAX_PACKET_SIZE = 128;
  uint8_t packetBuffer[MAX_PACKET_SIZE];
  size_t packetSize = 0;

  memcpy(packetBuffer, BT_PREFIX, sizeof(BT_PREFIX));
  packetSize += sizeof(BT_PREFIX);

  packetBuffer[6] = finalPacketByte6;  // Set byte 6 based on command

  memcpy(&packetBuffer[packetSize], cmdPrefix, cmdPrefixSize);
  packetSize += cmdPrefixSize;

  memcpy(&packetBuffer[packetSize], payload, payloadSize);
  packetSize += payloadSize;

  memcpy(&packetBuffer[packetSize], BT_SUFFIX, sizeof(BT_SUFFIX));
  packetSize += sizeof(BT_SUFFIX);

  SerialBT.write(packetBuffer, packetSize);
  SerialBT.flush();
}

void BluetoothManager::btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (instance == nullptr) return;  // Guard against null instance

  if (event == ESP_SPP_OPEN_EVT) {
    Serial.println("Target device connected successfully!");
    instance->deviceConnected = true;
  } else if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("Target device disconnected.");
    instance->deviceConnected = false;
  }
}