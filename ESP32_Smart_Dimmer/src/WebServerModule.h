// WebServerModule.h
#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <WebServer.h>
#include <SPIFFS.h> // Required for serving files from SPIFFS

class WebServerModule {
public:
    /** Constructor */
    WebServerModule();
    
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

    // Private helper methods
    void handleRoot();
    void handleControl();
    void handleNotFound();
    String getContentType(String filename);
};

#endif