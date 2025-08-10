#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <vector>
#include <map>
#include <Arduino.h>
#include <BluetoothSerial.h>
#include "DeviceConfig.h"
#include "CommandType.h"
#include "Utils.h"

// Define constants for received packet parsing
// These are based on your observed "BT Data Received" logs
const uint8_t RX_PACKET_HEADER_BYTE4 = 0x41;   // Identifies a received status packet
const uint8_t RX_PACKET_HEADER_BYTE5 = 0x81;   // Consistent second header byte for received packets
const uint8_t RX_PACKET_FUNCTION_FAN = 0x18;   // Byte 6 for fan/motor status
const uint8_t RX_PACKET_FUNCTION_LIGHT = 0x1C; // Byte 6 for light status (assuming, based on sent command)

// Indices for parsing received packets (0-indexed from start of 24-byte packet)
const int RX_PACKET_FUNCTION_BYTE_IDX = 6;
const int RX_PACKET_FAN_STATE_BYTE_IDX = 22; // For fan ON/OFF/Speed

struct BtDevice
{
    String name;
    String address;
};

class IBtDeviceConnectedListener
{
public:
    virtual void onDeviceConnected(String mac_address) = 0;
    virtual ~IBtDeviceConnectedListener() = default;
};

class IBtDisconnectedListener
{
public:
    virtual void onBtDisconnected() = 0;
    virtual ~IBtDisconnectedListener() = default;
};

class IBtDevicesListReadyListener
{
public:
    virtual void onDevicesListReady(std::map<String, BtDevice> devices) = 0;
    virtual ~IBtDevicesListReadyListener() = default;
};

class BluetoothManager
{
public:
    BluetoothManager(const char *deviceName);
    void begin();
    void clearInputBuffer();
    bool isConnected();
    void disconnect();
    void sendCommand(CommandType cmd, const uint8_t *payload, size_t payloadSize);
    bool sendConfigToDevice(const DeviceConfig &config);
    void registerDeviceConnectedListener(IBtDeviceConnectedListener *listener);
    void registerBtDisconnectedListener(IBtDisconnectedListener *listener);
    void registerDevicesListReadyListener(IBtDevicesListReadyListener *listener);
    bool waitForAck(const std::vector<CommandType> &expectedAckTypes, unsigned long timeout_ms);
    void scanForDevices();

private:
    BluetoothSerial SerialBT;
    const char *espDeviceName;
    bool deviceConnected = false;
    BTAddress connectedMacAddress;
    IBtDeviceConnectedListener *deviceConnectedListener = nullptr;
    IBtDevicesListReadyListener *devicesListReadyListener = nullptr;
    IBtDisconnectedListener *btDisconnectedListener = nullptr;
    long lastSendTime;
    bool waitingToScanForDevices = false;

    bool connectToDevice(const BTAddress &remoteAddress);
    void handleBtEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    void onDeviceDisconnected();

    static void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

    volatile bool _ackReceived = false;       // Flag set by callback when ACK is parsed
    volatile CommandType _ackType = CMD_NONE; // Type of command for which ACK was received
    volatile uint8_t _ackStateValue = 0;      // State value from the ACK (e.g., fan speed)

    // Static pointer to the instance to be used in the static callback
    static BluetoothManager *instance;
};

#endif // BLUETOOTH_MANAGER_H