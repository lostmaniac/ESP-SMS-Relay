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
#include "../filesystem_manager/filesystem_manager.h"
#include "../gsm_service/gsm_service.h"
#include "../task_scheduler/task_scheduler.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_heap_caps.h>

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
    // API routes with body parsers - longest paths first
    server->on("/api/docs/forward_rules", HTTP_GET, WebServer::handleGetDocsGuide);
    server->on("/api/database/execute", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleExecuteSQL);
    server->on("/api/wifi/ap_settings", HTTP_GET, WebServer::handleGetAPSettings);
    server->on("/api/wifi/ap_settings", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleUpdateAPSettings);
    server->on("/api/rules/update", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleUpdateRule);
    server->on("/api/rules/delete", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleDeleteRule);
    
    // API routes - medium length paths
    server->on("/api/push_channels", HTTP_GET, WebServer::handleGetPushChannels);
    server->on("/api/system_status", HTTP_GET, WebServer::handleGetSystemStatus);
    server->on("/api/sms_history", HTTP_GET, WebServer::handleGetSmsHistory);
    server->on("/api/rules", HTTP_GET, WebServer::handleGetRules);
    server->on("/api/rules", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, WebServer::handleAddRule);
    server->on("/api/reboot", HTTP_POST, WebServer::handleReboot);
    server->on("/api/logs", HTTP_GET, WebServer::handleGetLogs);
    
    // Static content - shorter paths
    server->on("/style.css", HTTP_GET, WebServer::handleStyle);
    server->on("/script.js", HTTP_GET, WebServer::handleScript);
    server->on("/connect", HTTP_POST, WebServer::handleConnect);
    server->on("/", HTTP_GET, WebServer::handleRoot);
    
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
            // 使用任务调度器延迟重启，避免阻塞
            TaskScheduler& scheduler = TaskScheduler::getInstance();
            scheduler.addOnceTask("reboot_task", 1000, []() {
                ESP.restart();
            });
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

        // 参数校验
        if (!doc.containsKey("id")) {
            request->send(400, "text/plain", "Missing required field: id");
            return;
        }
        
        int ruleId = doc["id"].as<int>();
        
        // 校验规则ID的有效性
        if (ruleId <= 0) {
            request->send(400, "text/plain", "Invalid rule ID: must be positive integer");
            return;
        }
        
        DatabaseManager& dbManager = DatabaseManager::getInstance();
        
        // 使用带事务保护的删除方法
        if (dbManager.deleteForwardRuleWithTransaction(ruleId)) {
            request->send(200, "text/plain", "Rule deleted successfully");
        } else {
            String errorMsg = "Failed to delete rule: " + dbManager.getLastError();
            request->send(500, "text/plain", errorMsg);
        }
    }
}

void WebServer::handleGetLogs(AsyncWebServerRequest *request) {
    // This should be implemented to fetch logs from LogManager
    request->send(200, "application/json", "[]");
}

void WebServer::handleReboot(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Rebooting...");
    // 使用任务调度器延迟重启，避免阻塞
    TaskScheduler& scheduler = TaskScheduler::getInstance();
    scheduler.addOnceTask("reboot_task", 1000, []() {
        ESP.restart();
    });
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



void WebServer::handleExecuteSQL(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data, len);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
            return;
        }

        // 参数校验
        if (!doc.containsKey("sql")) {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing required field: sql\"}");
            return;
        }
        
        String sqlCommand = doc["sql"].as<String>();
        
        // 基本的SQL注入防护 - 检查危险关键字
        String sqlLower = sqlCommand;
        sqlLower.toLowerCase();
        
        // 禁止的危险操作
        if (sqlLower.indexOf("drop table") >= 0 || 
            sqlLower.indexOf("drop database") >= 0 ||
            sqlLower.indexOf("truncate") >= 0 ||
            sqlLower.indexOf("delete from") >= 0 ||
            sqlLower.indexOf("alter table") >= 0) {
            request->send(403, "application/json", "{\"success\":false,\"error\":\"Dangerous SQL operations are not allowed\"}");
            return;
        }
        
        // 记录执行开始时间
        unsigned long startTime = millis();
        
        DatabaseManager& dbManager = DatabaseManager::getInstance();
        
        JsonDocument responseDoc;
        
        // 判断是否为查询语句
        if (sqlLower.startsWith("select") || sqlLower.startsWith("pragma")) {
            // 执行查询
            std::vector<std::map<String, String>> results = dbManager.executeQuery(sqlCommand);
            
            if (results.empty() && !dbManager.getLastError().isEmpty()) {
                // 查询出错
                responseDoc["success"] = false;
                responseDoc["error"] = dbManager.getLastError();
            } else {
                // 查询成功
                responseDoc["success"] = true;
                responseDoc["type"] = "query";
                responseDoc["rowCount"] = results.size();
                
                JsonArray dataArray = responseDoc["data"].to<JsonArray>();
                for (const auto& row : results) {
                    JsonObject rowObj = dataArray.add<JsonObject>();
                    for (const auto& pair : row) {
                        rowObj[pair.first] = pair.second;
                    }
                }
            }
        } else {
            // 执行非查询语句（INSERT, UPDATE等）
            bool success = dbManager.executeSQL(sqlCommand);
            
            if (success) {
                responseDoc["success"] = true;
                responseDoc["type"] = "execute";
                responseDoc["message"] = "SQL executed successfully";
                // 注意：SQLite不直接提供affected rows，这里简化处理
                responseDoc["affectedRows"] = "N/A";
            } else {
                responseDoc["success"] = false;
                responseDoc["error"] = dbManager.getLastError();
            }
        }
        
        // 计算执行耗时
        unsigned long executionTime = millis() - startTime;
        responseDoc["executionTime"] = executionTime;
        
        String response;
        serializeJson(responseDoc, response);
        request->send(200, "application/json", response);
    }
}

void WebServer::handleGetSystemStatus(AsyncWebServerRequest *request) {
    JsonDocument doc;
    
    // Flash存储信息
    FilesystemManager& fsManager = FilesystemManager::getInstance();
    FilesystemInfo fsInfo = fsManager.getFilesystemInfo();
    
    doc["flash"]["total"] = fsInfo.totalBytes;
    doc["flash"]["used"] = fsInfo.usedBytes;
    doc["flash"]["free"] = fsInfo.freeBytes;
    doc["flash"]["usage_percent"] = fsInfo.usagePercent;
    doc["flash"]["mounted"] = fsInfo.mounted;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}