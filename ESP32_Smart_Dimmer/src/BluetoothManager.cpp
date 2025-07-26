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

const int MIN_SEND_INTERVAL = 100; // give bluetooth time to digest...
const int MAX_PACKET_SIZE = 128;

BluetoothManager::BluetoothManager(uint8_t* targetAddress, const char* deviceName)
  : targetDeviceAddress(targetAddress), espDeviceName(deviceName) {
  instance = this;  // Set the static instance pointer
}

void BluetoothManager::registerConnectionListener(IBluetoothConnectionListener* listener) {
  connectionListener = listener; // Assign the listener to the private member
  Serial.println("BluetoothManager: Connection listener registered.");
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

  int diff = millis() - lastSendTime;
  if (diff < MIN_SEND_INTERVAL) {
    delay(MIN_SEND_INTERVAL - diff);
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

  lastSendTime = millis();

  // Print to Serial Monitor as hex string
  Serial.print("[millis: ");
  Serial.print(millis());
  Serial.print("] Sending packet (HEX): ");
  for (size_t i = 0; i < packetSize; i++) {
    // Use Serial.printf for formatted output (two digits, leading zero if needed)
    // or Serial.print(packetBuffer[i], HEX) and handle padding manually.
    // printf is generally cleaner for this.
    Serial.printf("%02X ", packetBuffer[i]);
  }
  Serial.println();  // Newline at the end for readability
}

// Member method to handle Bluetooth events
void BluetoothManager::handleBtEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  uint8_t receivedBytes[MAX_PACKET_SIZE];
  int bytesRead = 0;

  switch (event) {
    case ESP_SPP_OPEN_EVT:
      Serial.printf("[millis: %ld] Target device connected successfully!\n", millis());
      deviceConnected = true;
      if (connectionListener != nullptr) {
        char bda_str[18];
        uint8_t* bda = param->srv_open.rem_bda;  // Connected device's MAC address
        sprintf(bda_str, "%02X:%02X:%02X:%02X:%02X:%02X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        delay(200);
        connectionListener->onBluetoothConnected(String(bda_str));
      }
      break;
    case ESP_SPP_CLOSE_EVT:
      Serial.printf("[millis: %ld] Target device disconnected.\n", millis());
      // You might want an onBluetoothDisconnected callback here too
      deviceConnected = false;
      break;
    case ESP_SPP_DATA_IND_EVT:
    /*
      Serial.printf("[millis: %ld] BT Data Received (Len: %d, Handle: %d, MTU: %d): ",
                    millis(), param->data_ind.len, param->data_ind.handle, ESP_SPP_MAX_MTU);
      
      while (SerialBT.available() && bytesRead < MAX_PACKET_SIZE) {
        receivedBytes[bytesRead++] = SerialBT.read();
      } // Read and print all available bytes from the BluetoothSerial buffer
      
      // Print received data for debugging
      for (int i = 0; i < bytesRead; i++) {
        Serial.printf("%02X ", receivedBytes[i]);
      }
      Serial.println();
      
      // --- ACK Parsing Logic ---
      if (bytesRead == 24 &&
          receivedBytes[0] == 0x01 && receivedBytes[1] == 0xFE &&
          receivedBytes[4] == RX_PACKET_HEADER_BYTE4 && receivedBytes[5] == RX_PACKET_HEADER_BYTE5) { // Check for common status header

        uint8_t receivedFunction = receivedBytes[RX_PACKET_FUNCTION_BYTE_IDX]; // Byte 6: 0x18 for fan, 0x1C for light
        
        // --- Acknowledge Fan Commands ---
        if (receivedFunction == RX_PACKET_FUNCTION_FAN) {
            // For fan, we know byte 22 is the state (0x00 for off, 0x01 for low)
            uint8_t fanState = receivedBytes[RX_PACKET_FAN_STATE_BYTE_IDX];
            Serial.printf("[BT Manager] Received Fan Status: State=%02X\n", fanState);
            
            // If we are currently awaiting a FAN_SPEED ACK
            if (_ackType == CMD_FAN_SPEED) {
                // If the received state matches the state we just sent, consider it acknowledged.
                // You might need more sophisticated logic here if the fan has more states or needs to report errors.
                // For now, any fan status message after a fan command might be an ACK.
                _ackReceived = true;
                Serial.println("[BT Manager] Fan command acknowledged!");
            }
        }
        
        // --- Acknowledge Light Commands ---
        // You need to capture light status messages from the remote
        // similar to how you captured fan status messages.
        else if (receivedFunction == RX_PACKET_FUNCTION_LIGHT) { // Assuming 0x1C for light status
            Serial.println("[BT Manager] Received Light Status.");
            // Parse light status bytes here (e.g., brightness, color)
            
            if (_ackType == CMD_RGB) { // Assuming CMD_RGB covers all light settings
                _ackReceived = true;
                Serial.println("[BT Manager] Light command acknowledged!");
            }
        }
        
        // Else, it's an unhandled status message or from a different function
      }
*/
      break;
    default:
      break;
  }
}

void BluetoothManager::btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (instance) {
    instance->handleBtEvent(event, param);  // Forward to member method
  }
}

bool BluetoothManager::waitForAck(const std::vector<CommandType>& expectedAckTypes, unsigned long timeout_ms) {
  unsigned long startMillis = millis();
  
  // Clear the ACK flags at the start of waiting
  // This ensures we only acknowledge events that occur *during* this wait period.
  _ackReceived = false; 
  _ackType = CMD_NONE; 

  while ((millis() - startMillis < timeout_ms)) {
    // Check if an ACK was received AND its type is among the expected types
    if (_ackReceived && std::find(expectedAckTypes.begin(), expectedAckTypes.end(), _ackType) != expectedAckTypes.end()) {
        _ackReceived = false; // Reset for next command
        _ackType = CMD_NONE;  // Clear acknowledged type
        return true;          // Success: Expected ACK type received
    }
    
    // *** CRITICAL FIX: Replace delay(1) with vTaskDelay ***
    // Yield execution to allow other tasks (like BT event handler) to run
    vTaskDelay(pdMS_TO_TICKS(10)); // Yield for 10ms (adjust as needed, 1ms to 100ms)
  }
  
  // If loop finishes, it's a timeout or an unexpected ACK type arrived
  _ackReceived = false; // Reset flags for next operation
  _ackType = CMD_NONE;
  Serial.printf("[BluetoothManager] waitForAck timeout for expected types. Last ACK was %d\n", _ackType); // Debug timeout
  return false; // Timeout or incorrect ACK type received
}

