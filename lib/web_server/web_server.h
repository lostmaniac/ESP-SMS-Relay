#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

class WebServer {
public:
    static WebServer& getInstance();

    void start();
    void stop();

private:
    WebServer();
    ~WebServer();
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    void setupRoutes();
    
    // Handlers
    static void handleRoot(class AsyncWebServerRequest *request);
    static void handleStyle(class AsyncWebServerRequest *request);
    static void handleScript(class AsyncWebServerRequest *request);
    static void handleGetRules(class AsyncWebServerRequest *request);
    static void handleAddRule(class AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    static void handleUpdateRule(class AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    static void handleDeleteRule(class AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    static void handleGetLogs(class AsyncWebServerRequest *request);
    static void handleReboot(class AsyncWebServerRequest *request);
    static void handleGetSmsHistory(class AsyncWebServerRequest *request);
    static void handleGetDocsGuide(class AsyncWebServerRequest *request);
    static void handleGetStaSettings(class AsyncWebServerRequest *request);
    static void handleUpdateStaSettings(class AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    static void handleNotFound(class AsyncWebServerRequest *request);
    static void handleAPRoot(class AsyncWebServerRequest *request);
    static void handleConnect(class AsyncWebServerRequest *request);
    static void handleGetPushChannels(class AsyncWebServerRequest *request);

    class AsyncWebServer* server;
};

#endif // WEB_SERVER_H