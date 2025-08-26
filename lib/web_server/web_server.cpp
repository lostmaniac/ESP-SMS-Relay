#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include "../database_manager/database_manager.h"
#include <ArduinoJson.h>
#include "html.h"
#include "css.h"
#include "js.h"
#include "ap_html.h"
#include "docs_guide.h"
#include "../wifi_manager_web/wifi_manager_web.h"
#include "../push_manager/push_channel_registry.h"

// --- Singleton Instance ---
WebServer& WebServer::getInstance() {
    static WebServer instance;
    return instance;
}

// --- Constructor & Destructor ---
WebServer::WebServer() : server(new AsyncWebServer(80)) {}

WebServer::~WebServer() {
    delete server;
}

// --- Public Methods ---
void WebServer::start() {
    setupRoutes();
    server->begin();
}

void WebServer::stop() {
    server->end();
}

// --- Route Setup ---
void WebServer::setupRoutes() {
    // Static content
    server->on("/", HTTP_GET, WebServer::handleRoot);
    server->on("/style.css", HTTP_GET, WebServer::handleStyle);
    server->on("/script.js", HTTP_GET, WebServer::handleScript);

    // API routes
    server->on("/api/rules", HTTP_GET, WebServer::handleGetRules);
    server->on("/api/sms_history", HTTP_GET, WebServer::handleGetSmsHistory);
    server->on("/api/docs/forward_rules", HTTP_GET, WebServer::handleGetDocsGuide);
    server->on("/api/logs", HTTP_GET, WebServer::handleGetLogs);
    server->on("/api/reboot", HTTP_POST, WebServer::handleReboot);
    server->on("/api/push_channels", HTTP_GET, WebServer::handleGetPushChannels);
    server->on("/api/wifi/ap_settings", HTTP_GET, WebServer::handleGetAPSettings);

    // API routes with body parsers
    server->on("/api/rules", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleAddRule);
    server->on("/api/rules/update", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleUpdateRule);
    server->on("/api/rules/delete", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleDeleteRule);
    server->on("/api/wifi/ap_settings", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleUpdateAPSettings);

    // AP Mode routes
    server->on("/connect", HTTP_POST, WebServer::handleConnect);
    
    // Not found
    server->onNotFound(WebServer::handleNotFound);
}

// --- Handlers Implementation ---
void WebServer::handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", HTML_CONTENT);
}

void WebServer::handleStyle(AsyncWebServerRequest *request) {
    request->send_P(200, "text/css", CSS_CONTENT);
}

void WebServer::handleScript(AsyncWebServerRequest *request) {
    request->send_P(200, "application/javascript", JS_CONTENT);
}

void WebServer::handleGetDocsGuide(AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain; charset=utf-8", DOCS_GUIDE_CONTENT);
}

void WebServer::handleGetPushChannels(AsyncWebServerRequest *request) {
    std::vector<String> channels = PushChannelRegistry::getInstance().getAvailableChannels();
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (const String& channel : channels) {
        array.add(channel);
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleGetRules(AsyncWebServerRequest *request) {
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    std::vector<ForwardRule> rules = dbManager.getAllForwardRules();

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& rule : rules) {
        JsonObject ruleObj = array.add<JsonObject>();
        ruleObj["id"] = rule.id;
        ruleObj["rule_name"] = rule.ruleName;
        ruleObj["source_number"] = rule.sourceNumber;
        ruleObj["keywords"] = rule.keywords;
        ruleObj["push_type"] = rule.pushType;
        ruleObj["push_config"] = rule.pushConfig;
        ruleObj["enabled"] = rule.enabled;
        ruleObj["is_default_forward"] = rule.isDefaultForward;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleGetSmsHistory(AsyncWebServerRequest *request) {
    int page = 1;
    int limit = 20; // Default limit, matches JS

    if (request->hasParam("page")) {
        page = request->getParam("page")->value().toInt();
    }
    if (request->hasParam("limit")) {
        limit = request->getParam("limit")->value().toInt();
    }

    int offset = (page - 1) * limit;

    DatabaseManager& dbManager = DatabaseManager::getInstance();
    std::vector<SMSRecord> records = dbManager.getSMSRecords(limit, offset);
    int totalRecords = dbManager.getSMSRecordCount();

    JsonDocument doc;
    doc["total"] = totalRecords;
    JsonArray recordsArray = doc["records"].to<JsonArray>();

    for (const auto& record : records) {
        JsonObject recordObj = recordsArray.add<JsonObject>();
        recordObj["id"] = record.id;
        recordObj["from"] = record.fromNumber;
        recordObj["content"] = record.content;
        recordObj["received_at"] = record.receivedAt;
        recordObj["status"] = record.status;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleGetAPSettings(AsyncWebServerRequest *request) {
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    APConfig config = dbManager.getAPConfig();
    
    JsonDocument doc;
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["channel"] = config.channel;
    doc["maxConnections"] = config.maxConnections;
    doc["enabled"] = config.enabled;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleUpdateAPSettings(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data, len);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            return;
        }

        String ssid = doc["ssid"];
        String password = doc["password"];
        int channel = doc["channel"].as<int>();
        int maxConnections = doc["maxConnections"].as<int>();
        bool enabled = doc["enabled"];

        if (password.length() > 0 && password.length() < 8) {
            request->send(400, "text/plain", "Password must be at least 8 characters long");
            return;
        }

        if (channel < 1 || channel > 13) {
            request->send(400, "text/plain", "Channel must be between 1 and 13");
            return;
        }

        if (maxConnections < 1 || maxConnections > 8) {
            request->send(400, "text/plain", "Max connections must be between 1 and 8");
            return;
        }

        DatabaseManager& dbManager = DatabaseManager::getInstance();
        APConfig config = dbManager.getAPConfig(); // Get existing AP config to preserve other fields
        
        Serial.println("[WebServer] Updating AP settings:");
        Serial.println("  SSID: " + ssid);
        Serial.println(String("  Password: ") + (password.length() > 0 ? "[SET]" : "[EMPTY]"));
        Serial.println("  Channel: " + String(channel));
        Serial.println("  Max Connections: " + String(maxConnections));
        Serial.println("  Enabled: " + String(enabled));
        
        config.ssid = ssid;
        config.password = password;
        config.channel = channel;
        config.maxConnections = maxConnections;
        config.enabled = enabled;

        Serial.println("[WebServer] Calling updateAPConfig...");
        bool updateResult = dbManager.updateAPConfig(config);
        Serial.println("[WebServer] Update result: " + String(updateResult));
        
        if (updateResult) {
            Serial.println("[WebServer] Settings saved successfully. Rebooting...");
            request->send(200, "text/plain", "Settings saved. Rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("[WebServer] Failed to save settings to database");
            Serial.println("[WebServer] Last error: " + dbManager.getLastError());
            request->send(500, "text/plain", "Failed to save settings: " + dbManager.getLastError());
        }
    }
}

void WebServer::handleAddRule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) { // Process only the first chunk for simplicity
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data, len);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            return;
        }

        ForwardRule rule;
        rule.ruleName = doc["rule_name"].as<String>();
        rule.sourceNumber = doc["source_number"].as<String>();
        rule.keywords = doc["keywords"].as<String>();
        rule.pushType = doc["push_type"].as<String>();
        rule.pushConfig = doc["push_config"].as<String>();
        rule.enabled = doc["enabled"].as<bool>();
        rule.isDefaultForward = doc["is_default_forward"].as<bool>();

        if (DatabaseManager::getInstance().addForwardRule(rule) != -1) {
            request->send(200, "text/plain", "OK");
        } else {
            request->send(500, "text/plain", "Failed to add rule");
        }
    }
}

void WebServer::handleUpdateRule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data, len);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            return;
        }

        ForwardRule rule;
        rule.id = doc["id"].as<int>();
        rule.ruleName = doc["rule_name"].as<String>();
        rule.sourceNumber = doc["source_number"].as<String>();
        rule.keywords = doc["keywords"].as<String>();
        rule.pushType = doc["push_type"].as<String>();
        rule.pushConfig = doc["push_config"].as<String>();
        rule.enabled = doc["enabled"].as<bool>();
        rule.isDefaultForward = doc["is_default_forward"].as<bool>();

        if (DatabaseManager::getInstance().updateForwardRule(rule)) {
            request->send(200, "text/plain", "OK");
        } else {
            request->send(500, "text/plain", "Failed to update rule");
        }
    }
}

void WebServer::handleDeleteRule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data, len);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            return;
        }

        int ruleId = doc["id"].as<int>();
        if (DatabaseManager::getInstance().deleteForwardRule(ruleId)) {
            request->send(200, "text/plain", "OK");
        } else {
            request->send(500, "text/plain", "Failed to delete rule");
        }
    }
}

void WebServer::handleGetLogs(AsyncWebServerRequest *request) {
    // This should be implemented to fetch logs from LogManager
    request->send(200, "application/json", "[]");
}

void WebServer::handleReboot(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Rebooting...");
    delay(1000);
    ESP.restart();
}

void WebServer::handleNotFound(AsyncWebServerRequest *request) {
    if (WiFi.getMode() == WIFI_AP) {
        request->send_P(200, "text/html", AP_HTML_CONTENT);
    } else {
        request->send(404, "text/plain", "Not found");
    }
}

void WebServer::handleAPRoot(AsyncWebServerRequest *request) {
    // This handler is now effectively replaced by the onNotFound logic for AP mode
    request->send_P(200, "text/html", AP_HTML_CONTENT);
}

void WebServer::handleConnect(AsyncWebServerRequest *request) {
    // This should be implemented to handle WiFi connection
    request->send(200, "text/plain", "Connecting...");
}