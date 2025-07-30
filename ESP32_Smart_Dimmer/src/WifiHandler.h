// WifiHandler.h
#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H
#define WOKWI

#include <WiFi.h>
#include <WiFiManager.h>

class WifiHandler {
public:
  /**
     * @brief Default constructor for WifiHandler with default AP name and password.
     */
  WifiHandler();
  /**
     * @brief Constructor for WifiHandler.
     * @param apName The name of the Access Point to create for configuration.
     * @param apPassword Optional password for the configuration Access Point. Use nullptr for no password.
     */
  WifiHandler(const char* apName, const char* apPassword = nullptr);

  /**
     * @brief Connects to WiFi, or starts a captive portal if credentials are not found/failed.
     * This method will block until a connection is established, or the configuration portal times out.
     * @return true if connected to STA mode, false if in AP mode (for configuration) or failed.
     */
  bool connect();

  /**
     * @brief Checks if WiFi is currently connected (in STA mode).
     * @return true if connected, false otherwise.
     */
  bool isConnected();

  /**
     * @brief Returns the local IP address if connected.
     * @return IPAddress object.
     */
  IPAddress getLocalIP();

private:
  WiFiManager _wifiManager;
  const char* _apName;
  const char* _apPassword;
};

#endif