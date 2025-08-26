#include "wifi_manager_web.h"
#include <WiFi.h>
#include <DNSServer.h>
#include "../database_manager/database_manager.h"

WiFiManagerWeb& WiFiManagerWeb::getInstance() {
    static WiFiManagerWeb instance;
    return instance;
}

WiFiManagerWeb::WiFiManagerWeb() : currentMode(WM_WIFI_MODE_DISCONNECTED), dnsServer(new DNSServer()) {}

WiFiManagerWeb::~WiFiManagerWeb() {
    WiFi.disconnect(true);
}

bool WiFiManagerWeb::connect() {
    Serial.println("Starting WiFi connection...");
    if (connectToSavedNetwork()) {
        return true;
    }

    Serial.println("Could not connect to saved network. Starting AP mode.");
    startAPMode();
    return false; // Indicates that we are in AP mode, not connected to WAN
}

bool WiFiManagerWeb::connectToSavedNetwork() {
    DatabaseManager& db = DatabaseManager::getInstance();
    APConfig apConfig = db.getAPConfig(); 

    if (!apConfig.enabled) {
        Serial.println("No saved & enabled WiFi credentials found.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(apConfig.ssid.c_str(), apConfig.password.c_str());

    Serial.print("Connecting to " + apConfig.ssid);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        Serial.print(".");
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        ipAddress = WiFi.localIP().toString();
        currentMode = WM_WIFI_MODE_STA;
        Serial.println("IP Address: " + ipAddress);
        return true;
    }

    Serial.println("\nConnection failed.");
    WiFi.disconnect();
    return false;
}

void WiFiManagerWeb::startAPMode() {
    DatabaseManager& db = DatabaseManager::getInstance();
    APConfig apConfig = db.getAPConfig();

    Serial.println("Starting Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apConfig.ssid.c_str(), apConfig.password.c_str(), apConfig.channel, 0, apConfig.maxConnections);
    
    ipAddress = WiFi.softAPIP().toString();
    currentMode = WM_WIFI_MODE_AP;

    // Start DNS server for captive portal
    dnsServer->start(53, "*", WiFi.softAPIP());

    Serial.println("AP SSID: " + apConfig.ssid);
    Serial.println("AP IP Address: " + ipAddress);
}

WiFiMode WiFiManagerWeb::getMode() {
    return currentMode;
}

String WiFiManagerWeb::getIPAddress() {
    return ipAddress;
}

void WiFiManagerWeb::loop() {
    if (currentMode == WM_WIFI_MODE_AP) {
        dnsServer->processNextRequest();
    }
}
