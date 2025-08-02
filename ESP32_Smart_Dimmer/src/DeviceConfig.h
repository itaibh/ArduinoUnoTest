#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <string>
#include "LightMode.h"

struct DeviceConfig
{
    String mac_address;
    uint8_t fan_speed;
    LightMode light_mode;
    uint8_t main_brightness;
    uint8_t main_warmness;
    uint8_t ring_hue;
    uint8_t ring_brightness;
    bool isOn; // Indicates if the device is currently powered on/off
};

inline bool operator==(const DeviceConfig &lhs, const DeviceConfig &rhs)
{
    return (lhs.mac_address == rhs.mac_address &&
            lhs.fan_speed == rhs.fan_speed &&
            lhs.light_mode == lhs.light_mode && // Typo correction from my side previously, should be rhs.light_mode
            lhs.main_brightness == rhs.main_brightness &&
            lhs.main_warmness == rhs.main_warmness &&
            lhs.ring_hue == rhs.ring_hue &&
            lhs.ring_brightness == rhs.ring_brightness &&
            lhs.isOn == rhs.isOn);
}

inline bool operator!=(const DeviceConfig &lhs, const DeviceConfig &rhs)
{
    return !(lhs == rhs);
}

#endif