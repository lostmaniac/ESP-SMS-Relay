#ifndef WIFI_MANAGER_WEB_H
#define WIFI_MANAGER_WEB_H

#include <Arduino.h>

class DNSServer;

enum WiFiMode {
    WM_WIFI_MODE_AP,
    WM_WIFI_MODE_STA,
    WM_WIFI_MODE_DISCONNECTED
};

class WiFiManagerWeb {
public:
    static WiFiManagerWeb& getInstance();

    bool connect();
    WiFiMode getMode();
    String getIPAddress();
    void loop();

private:
    DNSServer* dnsServer;
    WiFiManagerWeb();
    ~WiFiManagerWeb();
    WiFiManagerWeb(const WiFiManagerWeb&) = delete;
    WiFiManagerWeb& operator=(const WiFiManagerWeb&) = delete;

    bool connectToSavedNetwork();
    void startAPMode();

    WiFiMode currentMode;
    String ipAddress;
};

#endif // WIFI_MANAGER_WEB_H