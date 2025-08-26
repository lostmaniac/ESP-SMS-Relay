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

bool WiFiManagerWeb::startAP() {
    Serial.println("Starting WiFi Access Point...");
    configureAPMode();
    return true; // AP mode started successfully
}

void WiFiManagerWeb::configureAPMode() {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    Serial.println("[WiFiManager] Reading AP config from database...");
    APConfig apConfig = db.getAPConfig();
    
    Serial.println("[WiFiManager] AP Configuration loaded:");
    Serial.println("  SSID: " + apConfig.ssid);
    Serial.println("  Password: " + String(apConfig.password.length() > 0 ? "[SET]" : "[EMPTY]"));
    Serial.println("  Channel: " + String(apConfig.channel));
    Serial.println("  Max Connections: " + String(apConfig.maxConnections));
    Serial.println("  Enabled: " + String(apConfig.enabled));

    Serial.println("[WiFiManager] Starting Access Point...");
    WiFi.mode(WIFI_AP);
    
    bool apResult = WiFi.softAP(apConfig.ssid.c_str(), apConfig.password.c_str(), apConfig.channel, 0, apConfig.maxConnections);
    Serial.println("[WiFiManager] WiFi.softAP() result: " + String(apResult ? "SUCCESS" : "FAILED"));
    
    ipAddress = WiFi.softAPIP().toString();
    currentMode = WM_WIFI_MODE_AP;

    // Start DNS server for captive portal
    dnsServer->start(53, "*", WiFi.softAPIP());

    Serial.println("[WiFiManager] AP Started:");
    Serial.println("  SSID: " + apConfig.ssid);
    Serial.println("  IP Address: " + ipAddress);
    
    // Verify the actual AP configuration
    Serial.println("[WiFiManager] Verifying actual AP settings:");
    Serial.println("  Actual SSID: " + WiFi.softAPSSID());
    Serial.println("  Actual IP: " + WiFi.softAPIP().toString());
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
