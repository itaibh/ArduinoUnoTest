// WebServerModule.h
#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "BluetoothManager.h"
#include "LightController.h"
#include "FanController.h"
#include "StorageHandler.h"

class WebServerModule : public IBtDevicesListReadyListener {
public:
    /** Constructor */
    WebServerModule(StorageHandler* sh, BluetoothManager* bt, LightController* lc, FanController* fc);
    
    /**
     * @brief Initializes and starts the web server.
     * @return true if the server started successfully, false otherwise.
     */
    bool begin();

    /**
     * @brief Handles incoming HTTP client requests.
     * This should be called frequently in the main loop.
     */
    void handleClient();

    virtual void onDevicesListReady(std::map<String, BtDevice> devices);

private:
    WebServer _server; // Private instance of the WebServer
    StorageHandler* storageHandler;
    BluetoothManager* btManager;
    LightController* lightCtrl;
    FanController* fanCtrl;

    // Private helper methods
    void setupRoutes();
    void handleRoot();
    void handleControl();
    void handleFindDevices();
    void handleGetAllDevices();
    void handleNotFound();
    void handleAddDevice();
    void handleRemoveDevice();

    String getContentType(String filename);
};

#endif