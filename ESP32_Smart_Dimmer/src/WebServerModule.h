// WebServerModule.h
#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "LightController.h"
#include "FanController.h"
#include "StorageHandler.h"

class WebServerModule {
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

private:
    WebServer _server; // Private instance of the WebServer
    StorageHandler* storageHandler;
    BluetoothManager* btManager;
    LightController* lightCtrl;
    FanController* fanCtrl;

    // Private helper methods
    void handleRoot();
    void handleControl();
    void handleFindDevices();
    void handleGetAllDevices();
    void handleNotFound();
    String getContentType(String filename);
};

#endif