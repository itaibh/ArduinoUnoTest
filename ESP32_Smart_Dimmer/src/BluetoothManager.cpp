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
    lastSendTime = millis() - MIN_SEND_INTERVAL;
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
    log_i("Disconnecting");
    if (!SerialBT.disconnect())
    {
        log_w("Failed to disconnect.");
    }
}

// Connects to a specific device
void BluetoothManager::connectToDevice(const BTAddress &remoteAddress)
{
    log_i("remoteAddress: %s (current connected device: %s (%s))", 
        remoteAddress.toString(true).c_str(),
        connectedMacAddress.toString(true).c_str(),
        deviceConnected ? "connected" : "not connecetd");

    if (deviceConnected)
    {
        if (connectedMacAddress.equals(remoteAddress))
        {
            log_i("Already connected to this device.");
            return;
        }
        disconnect();
    }

    SerialBT.connect(remoteAddress);
}

bool BluetoothManager::sendConfigToDevice(const DeviceConfig &config)
{
    log_i("deviceConnected: %s , connectedMacAddress: %s",
          deviceConnected ? "true" : "false", connectedMacAddress.toString(true).c_str());

    BTAddress address(config.mac_address);
    if (!deviceConnected || !connectedMacAddress.equals(address))
    {
        log_i("need to switch device");
        awaitingDeviceConfig = config;
        waitingToSendCommand = true;
        // Automatically try to connect if not connected to the right device
        connectToDevice(config.mac_address);
        return false;
    }
    else {
        log_i("correct device connected");
    }
    waitingToSendCommand = false;
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
    log_i("enter");
    if (!deviceConnected)
    {
        log_w("Cannot send command: Not connected.");
        return;
    }

    int time = millis();
    int diff = time - lastSendTime;
    if (diff < MIN_SEND_INTERVAL)
    {
        log_i("waiting %d ms (%d - (%d - %d))", MIN_SEND_INTERVAL - diff, MIN_SEND_INTERVAL, time, lastSendTime);
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

void printStatus(esp_spp_status_t status)
{
    switch (status)
    {
    case ESP_SPP_SUCCESS:
        log_i("ESP_SPP_SUCCESS");
        break;
    case ESP_SPP_FAILURE:
        log_i("ESP_SPP_FAILURE");
        break;
    case ESP_SPP_BUSY:
        log_i("ESP_SPP_BUSY");
        break;
    case ESP_SPP_NO_DATA:
        log_i("ESP_SPP_NO_DATA");
        break;
    case ESP_SPP_NO_RESOURCE:
        log_i("ESP_SPP_NO_RESOURCE");
        break;
    case ESP_SPP_NEED_INIT:
        log_i("ESP_SPP_NEED_INIT");
        break;
    case ESP_SPP_NEED_DEINIT:
        log_i("ESP_SPP_NEED_DEINIT");
        break;
    case ESP_SPP_NO_CONNECTION:
        log_i("ESP_SPP_NO_CONNECTION");
        break;
    case ESP_SPP_NO_SERVER:
        log_i("ESP_SPP_NO_SERVER");
        break;
    default:
        log_i("Unknown status");
        break;
    }
}

// Member method to handle Bluetooth events
void BluetoothManager::handleBtEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    uint8_t receivedBytes[MAX_PACKET_SIZE];
    int bytesRead = 0;

    log_i("%d", event);
    switch (event)
    {

    case ESP_SPP_INIT_EVT:
        log_i("ESP_SPP_INIT_EVT");
        printStatus(param->init.status);
        break;
    case ESP_SPP_UNINIT_EVT:
        log_i("ESP_SPP_UNINIT_EVT");
        printStatus(param->uninit.status);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        log_i("ESP_SPP_DISCOVERY_COMP_EVT");
        printStatus(param->disc_comp.status);
        break;
    case ESP_SPP_OPEN_EVT:
        log_i("ESP_SPP_OPEN_EVT");
        {
            printStatus(param->open.status);
            BTAddress mac(param->open.rem_bda);
            log_i("Target device connected successfully. mac: %s", mac.toString(true).c_str());
            deviceConnected = true;
            connectedMacAddress = mac;
            if (deviceConnectedListener != nullptr)
            {
                deviceConnectedListener->onDeviceConnected(mac.toString(true));
            }
        }
        break;
    case ESP_SPP_CLOSE_EVT:
        log_i("ESP_SPP_CLOSE_EVT");
        {
            printStatus(param->close.status);
            log_i("Target device disconnected.");
            // You might want an onBluetoothDisconnected callback here too
            deviceConnected = false;
            connectedMacAddress = BTAddress();
            if (btDisconnectedListener)
            {
                btDisconnectedListener->onBtDisconnected();
            }
            onDeviceDisconnected();
        }
        break;

    case ESP_SPP_START_EVT:
        log_i("ESP_SPP_START_EVT");
        printStatus(param->start.status);
        break;
    case ESP_SPP_CL_INIT_EVT:
        log_i("ESP_SPP_CL_INIT_EVT");
        printStatus(param->cl_init.status);
        break;
    case ESP_SPP_DATA_IND_EVT:
        log_i("ESP_SPP_DATA_IND_EVT");
        printStatus(param->data_ind.status);
        break;
    case ESP_SPP_CONG_EVT:
        log_i("ESP_SPP_CONG_EVT");
        printStatus(param->cong.status);
        break;
    case ESP_SPP_WRITE_EVT:
        log_i("ESP_SPP_WRITE_EVT");
        printStatus(param->write.status);
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        log_i("ESP_SPP_SRV_OPEN_EVT");
        printStatus(param->srv_open.status);
        break;
    case ESP_SPP_SRV_STOP_EVT:
        log_i("ESP_SPP_SRV_STOP_EVT");
        printStatus(param->srv_stop.status);
        break;
    case ESP_SPP_VFS_REGISTER_EVT:
        log_i("ESP_SPP_VFS_REGISTER_EVT");
        printStatus(param->vfs_register.status);
        break;
    case ESP_SPP_VFS_UNREGISTER_EVT:
        log_i("ESP_SPP_VFS_UNREGISTER_EVT");
        printStatus(param->vfs_unregister.status);
        break;
    default:
        log_i("Unknown SPP event: %d", event);
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

void BluetoothManager::onDeviceConnected(const BTAddress &mac)
{
    if (waitingToSendCommand)
    {
        log_i("calling sendConfigToDevice");
        sendConfigToDevice(awaitingDeviceConfig);
    }
}

void BluetoothManager::onDeviceDisconnected()
{
    if (waitingToScanForDevices)
    {
        log_i("calling scanForDevices");
        delay(500);
        scanForDevices();
    }
}

void BluetoothManager::scanForDevices()
{
    log_i("deviceConnected: %s, connectedDevice: %s",
        deviceConnected ? "true" : "false",
        connectedMacAddress.toString(true).c_str());

    if (deviceConnected)
    {
        waitingToScanForDevices = true;
        disconnect();
        return;
    }
    
    waitingToScanForDevices = false;

    log_i("------------------------------------");
    log_i("Scanning for devices (10 seconds)...");

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
}
