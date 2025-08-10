#include "BluetoothManager.h"

// Initialize static instance pointer
BluetoothManager *BluetoothManager::instance = nullptr;

// --- Constants for BT Commands ---
const uint8_t BT_PREFIX[] = {0x01, 0xfe, 0x00, 0x00, 0x51, 0x81, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d};
const uint8_t BT_SUFFIX[] = {0x0e, 0x00};
const uint8_t ON_OFF_DATA_PREFIX[] = {0x07, 0x01, 0x03, 0x01};
const uint8_t INTENSITY_DATA_PREFIX[] = {0x07, 0x01, 0x03, 0x02};
const uint8_t WARMNESS_DATA_PREFIX[] = {0x07, 0x01, 0x03, 0x03};
const uint8_t RGB_DATA_PREFIX[] = {0x0A, 0x02, 0x03, 0x0C};
const uint8_t FAN_SPEED_DATA_PREFIX[] = {0x07, 0x0e, 0x03, 0x03};

const int MIN_SEND_INTERVAL = 100; // give bluetooth time to digest...
const int MAX_PACKET_SIZE = 128;

BluetoothManager::BluetoothManager(const char *deviceName)
    : espDeviceName(deviceName)
{
    instance = this; // Set the static instance pointer
}

void BluetoothManager::registerDeviceConnectedListener(IBtDeviceConnectedListener *listener)
{
    deviceConnectedListener = listener; // Assign the listener to the private member
    log_i("Device connected listener registered.");
}

void BluetoothManager::registerBtDisconnectedListener(IBtDisconnectedListener *listener)
{
    btDisconnectedListener = listener;
    log_i("Bluetooth disconected listener registered.");
}

void BluetoothManager::registerDevicesListReadyListener(IBtDevicesListReadyListener *listener)
{
    devicesListReadyListener = listener;
    log_i("Devices list ready listener registered.");
}

void BluetoothManager::begin()
{
    SerialBT.begin(espDeviceName, true); // Master mode
    SerialBT.register_callback(btCallback);
}

bool BluetoothManager::isConnected()
{
    return deviceConnected;
}

void BluetoothManager::disconnect()
{
    if (!SerialBT.disconnect())
    {
        log_w("Failed to disconnect.");
    }
}

// Connects to a specific device
bool BluetoothManager::connectToDevice(const BTAddress &remoteAddress)
{
    log_i("remoteAddress: %s", remoteAddress.toString(true).c_str());
    if (deviceConnected)
    {
        if (connectedMacAddress.equals(remoteAddress))
        {
            log_i("Already connected to this device.");
            return true;
        }
        disconnect();
    }

    if (SerialBT.connect(remoteAddress))
    {
        deviceConnected = true;
        connectedMacAddress = remoteAddress;
        if (deviceConnectedListener)
        {
            deviceConnectedListener->onDeviceConnected(connectedMacAddress.toString(true));
        }
        log_i("Successfully connected to: %s", remoteAddress.toString(true).c_str());
        return true;
    }
    else
    {
        log_w("Failed to connect to: %s", remoteAddress.toString(true).c_str());
        return false;
    }
}

bool BluetoothManager::sendConfigToDevice(const DeviceConfig &config)
{
    log_i("deviceConnected: %s , connectedMacAddress: %s",
          deviceConnected ? "true" : "false", connectedMacAddress.toString(true).c_str());

    BTAddress address(config.mac_address);
    if (!deviceConnected || !connectedMacAddress.equals(address))
    {
        // Automatically try to connect if not connected to the right device
        if (!connectToDevice(config.mac_address))
        {
            log_w("Failed to connect for sending command.");
            return false;
        }
    }

    // Now call your existing sendCommand with the new parameters
    uint8_t payload[4]; // Max payload size for your commands

    // Light ON/OFF
    payload[0] = config.is_on ? 0x01 : 0x00;
    sendCommand(CMD_LIGHT_ON_OFF, payload, 1);

    // Fan Speed
    payload[0] = config.fan_speed;
    sendCommand(CMD_FAN_SPEED, payload, 1);

    // Light Intensity
    payload[0] = config.main_brightness;
    sendCommand(CMD_LIGHT_INTENSITY, payload, 1);

    // Warmness
    payload[0] = config.main_warmness;
    sendCommand(CMD_LIGHT_WARMNESS, payload, 1);

    // RGB (if applicable)
    if (config.light_mode == LightMode::RGB_RING)
    {
        payload[0] = (config.ring_hue >> 8) & 0xFF;
        payload[1] = config.ring_hue & 0xFF;
        payload[2] = config.ring_brightness;
        sendCommand(CMD_RGB, payload, 3);
    }
    // Note: You may need more logic here for other light modes

    return true;
}

void BluetoothManager::sendCommand(CommandType cmd, const uint8_t *payload, size_t payloadSize)
{
    if (!deviceConnected)
    {
        log_w("Cannot send command: Not connected.");
        return;
    }

    int diff = millis() - lastSendTime;
    if (diff < MIN_SEND_INTERVAL)
    {
        delay(MIN_SEND_INTERVAL - diff);
    }
    const uint8_t *cmdPrefix;
    size_t cmdPrefixSize;
    uint8_t finalPacketByte6 = 0x18; // Default value

    String commandTypeName = commandTypeToString(cmd);
    log_i("command: %s", commandTypeName.c_str());

    switch (cmd)
    {
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
        finalPacketByte6 = 0x1c; // Special value for RGB
        break;
    case CMD_FAN_SPEED:
        cmdPrefix = FAN_SPEED_DATA_PREFIX;
        cmdPrefixSize = sizeof(FAN_SPEED_DATA_PREFIX);
        break;
    default:
        log_w("Unknown command type.");
        return;
    }

    uint8_t packetBuffer[MAX_PACKET_SIZE];
    size_t packetSize = 0;

    memcpy(packetBuffer, BT_PREFIX, sizeof(BT_PREFIX));
    packetSize += sizeof(BT_PREFIX);

    packetBuffer[6] = finalPacketByte6; // Set byte 6 based on command

    memcpy(&packetBuffer[packetSize], cmdPrefix, cmdPrefixSize);
    packetSize += cmdPrefixSize;

    memcpy(&packetBuffer[packetSize], payload, payloadSize);
    packetSize += payloadSize;

    memcpy(&packetBuffer[packetSize], BT_SUFFIX, sizeof(BT_SUFFIX));
    packetSize += sizeof(BT_SUFFIX);

    log_i("create packet (size: %d)", packetSize);
    String str = "Sending packet (HEX): ";
    for (size_t i = 0; i < packetSize; i++)
    {
        str += String(packetBuffer[i], HEX);
    }
    log_i("%s", str.c_str());

    SerialBT.write(packetBuffer, packetSize);
    SerialBT.flush();

    lastSendTime = millis();
}

// Member method to handle Bluetooth events
void BluetoothManager::handleBtEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    uint8_t receivedBytes[MAX_PACKET_SIZE];
    int bytesRead = 0;
    BTAddress mac(param->srv_open.rem_bda);

    switch (event)
    {
    case ESP_SPP_OPEN_EVT:
        log_i("Target device connected successfully. mac: %s", mac.toString(true).c_str());
        deviceConnected = true;
        // if (connectionListener != nullptr)
        // {
        //     char bda_str[18];
        //     uint8_t *bda = param->srv_open.rem_bda; // Connected device's MAC address
        //     sprintf(bda_str, "%02X:%02X:%02X:%02X:%02X:%02X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        //     delay(200);
        //     connectionListener->onBluetoothConnected(String(bda_str));
        // }
        break;
    case ESP_SPP_CLOSE_EVT:
        log_i("Target device disconnected. mac: %s", mac.toString(true).c_str());
        // You might want an onBluetoothDisconnected callback here too
        deviceConnected = false;

        if (btDisconnectedListener)
        {
            btDisconnectedListener->onBtDisconnected();
        }
        onDeviceDisconnected();
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

void BluetoothManager::btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    if (instance)
    {
        instance->handleBtEvent(event, param); // Forward to member method
    }
}

bool BluetoothManager::waitForAck(const std::vector<CommandType> &expectedAckTypes, unsigned long timeout_ms)
{
    unsigned long startMillis = millis();

    // Clear the ACK flags at the start of waiting
    // This ensures we only acknowledge events that occur *during* this wait period.
    _ackReceived = false;
    _ackType = CMD_NONE;

    while ((millis() - startMillis < timeout_ms))
    {
        // Check if an ACK was received AND its type is among the expected types
        if (_ackReceived && std::find(expectedAckTypes.begin(), expectedAckTypes.end(), _ackType) != expectedAckTypes.end())
        {
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
    return false;                                                                                           // Timeout or incorrect ACK type received
}

void BluetoothManager::clearInputBuffer()
{
    while (SerialBT.available())
    {
        SerialBT.read();
    }
}

void BluetoothManager::onDeviceDisconnected()
{
    if (waitingToScanForDevices)
    {
        log_i("calling scanForDevices");
        scanForDevices();
    }
}

void BluetoothManager::scanForDevices()
{
    if (deviceConnected)
    {
        waitingToScanForDevices = true;
        disconnect();
        return;
    }

    Serial.println("------------------------------------");
    Serial.println("Scanning for devices (10 seconds)...");

    BTScanResults *scanResults = SerialBT.discover(10000);

    std::map<String, BtDevice> *foundDevices = new std::map<String, BtDevice>();
    if (scanResults != nullptr && scanResults->getCount() > 0)
    {
        log_i("Found %d devices:\n", scanResults->getCount());
        for (int i = 0; i < scanResults->getCount(); i++)
        {
            BTAdvertisedDevice *device_result = scanResults->getDevice(i);
            String name = device_result->getName().c_str();
            BTAddress address = device_result->getAddress();
            String mac = address.toString(true);
            log_i("  - Found Device: %s, Address: %s\n", name.c_str(), mac.c_str());
            if (mac.startsWith("C9:A3:05"))
            {
                (*foundDevices)[mac] = {name, mac};
            }
        }

        if (devicesListReadyListener)
        {
            log_i("calling callback with %d devices", foundDevices->size());
            // Call the method using the pointer.
            devicesListReadyListener->onDevicesListReady(*foundDevices);
            delete foundDevices;
        }
    }
    else
    {
        log_i("No classic Bluetooth devices found.");
    }
    waitingToScanForDevices = false;
}
