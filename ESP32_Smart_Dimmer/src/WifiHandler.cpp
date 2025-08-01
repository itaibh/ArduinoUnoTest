// WifiHandler.cpp
#include "WifiHandler.h"
#include <Arduino.h> // For Serial.print, delay

const char *WIFI_CONFIG_AP_NAME = "SmartLight_SETUP";
const char *WIFI_CONFIG_AP_PASSWORD = nullptr; // Or "yourAPpassword" (e.g., "smartlight123")

WifiHandler::WifiHandler()
    : _apName(WIFI_CONFIG_AP_NAME), _apPassword(WIFI_CONFIG_AP_PASSWORD)
{
#ifndef WOKWI
    _wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout for configuration
#endif
}

// Constructor: Initializes the WiFiManager and stores the AP credentials
WifiHandler::WifiHandler(const char *apName, const char *apPassword)
    : _apName(apName), _apPassword(apPassword)
{
#ifndef WOKWI
    // WiFiManager configuration options can be set here if needed
    // For example, setting a timeout for the configuration portal
    _wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout for configuration
                                              // _wifiManager.setDebugOutput(true); // Uncomment for more detailed debug messages from WiFiManager
#endif
}

bool WifiHandler::connect()
{
#ifndef WOKWI
    Serial.print("WifiHandler: Attempting to connect to saved WiFi or start AP: ");
    Serial.println(_apName);
    Serial.flush();
    // This is the main call. It will:
    // 1. Try to connect to previously saved WiFi credentials.
    // 2. If it fails (no credentials, wrong password, or network not found),
    //    it will start an Access Point (AP) named `_apName`.
    // 3. It then acts as a captive portal, allowing a user to configure WiFi via a web browser.
    // The call `autoConnect()` is blocking until connection or timeout.
    if (!_wifiManager.autoConnect(_apName, _apPassword))
    {
        Serial.println("WifiHandler: Failed to connect and AP config timed out/failed.");
        // We are likely still in AP mode here, or failed completely.
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("WifiHandler: Despite autoConnect returning false, we seem to be connected (unexpected).");
            return true; // Should not happen often if autoConnect works as expected
        }
        Serial.println("WifiHandler: Remaining in AP mode for configuration.");
        return false; // Still in AP mode, or no connection
    }
    Serial.println("WifiHandler: Successfully connected to WiFi (STA mode).");
    Serial.print("WifiHandler: Local IP: ");
    Serial.println(WiFi.localIP());
    return true; // Successfully connected to STA mode
#else
    const char *ssid = "Wokwi-GUEST"; // Wokwi's default WiFi
    const char *password = "";

    Serial.println("WOKWI MODE: Connecting with hard-coded credentials.");
    WiFi.begin(ssid, password);

    int retries = 20; // 10 seconds timeout
    while (WiFi.status() != WL_CONNECTED && retries-- > 0)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    else
    {
        Serial.println("\nWiFi connection failed.");
        return false;
    }

#endif
}

bool WifiHandler::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WifiHandler::getLocalIP()
{
    return WiFi.localIP();
}