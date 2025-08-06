/**
 * @file web_server.cpp
 * @brief Web服务器模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "web_server.h"

// 静态实例
static WebServerManager* webServerInstance = nullptr;

/**
 * @brief 构造函数
 */
WebServerManager::WebServerManager() {
    server = nullptr;
    status = WEB_SERVER_NOT_STARTED;
    debugMode = false;
    initialized = false;
    startTime = 0;
    
    // 初始化统计信息
    stats.uptime = 0;
    stats.totalRequests = 0;
    stats.activeConnections = 0;
    stats.lastRequestTime = 0;
    stats.lastRequestPath = "";
}

/**
 * @brief 析构函数
 */
WebServerManager::~WebServerManager() {
    if (server) {
        stop();
        delete server;
        server = nullptr;
    }
}

/**
 * @brief 获取单例实例
 */
WebServerManager& WebServerManager::getInstance() {
    if (webServerInstance == nullptr) {
        webServerInstance = new WebServerManager();
    }
    return *webServerInstance;
}

/**
 * @brief 获取默认配置
 */
WebServerConfig WebServerManager::getDefaultConfig() {
    WebServerConfig defaultConfig;
    defaultConfig.port = 80;
    defaultConfig.enableCORS = true;
    defaultConfig.enableAuth = false;
    defaultConfig.authUsername = "admin";
    defaultConfig.authPassword = "admin123";
    defaultConfig.maxConnections = 4;
    return defaultConfig;
}

/**
 * @brief 初始化Web服务器
 */
bool WebServerManager::initialize(const WebServerConfig& config) {
    debugPrint("初始化Web服务器...");
    
    if (initialized) {
        debugPrint("Web服务器已初始化");
        return true;
    }
    
    this->config = config;
    
    // 创建服务器实例
    if (server) {
        delete server;
    }
    server = new WebServer(config.port);
    
    if (!server) {
        setError("创建Web服务器实例失败");
        return false;
    }
    
    // 设置路由
    setupRoutes();
    
    initialized = true;
    debugPrint("Web服务器初始化完成，端口: " + String(config.port));
    
    return true;
}

/**
 * @brief 启动Web服务器
 */
bool WebServerManager::start() {
    if (!initialized) {
        setError("Web服务器未初始化");
        return false;
    }
    
    if (status == WEB_SERVER_RUNNING) {
        debugPrint("Web服务器已在运行");
        return true;
    }
    
    debugPrint("启动Web服务器...");
    status = WEB_SERVER_STARTING;
    
    // 检查WiFi是否可用
    WiFiManager& wifiMgr = WiFiManager::getInstance();
    if (!wifiMgr.isAPActive()) {
        setError("WiFi热点未激活，无法启动Web服务器");
        status = WEB_SERVER_ERROR;
        return false;
    }
    
    // 启动服务器
    server->begin();
    
    startTime = millis();
    status = WEB_SERVER_RUNNING;
    
    String serverURL = getServerURL();
    debugPrint("Web服务器启动成功: " + serverURL);
    
    return true;
}

/**
 * @brief 停止Web服务器
 */
bool WebServerManager::stop() {
    if (status == WEB_SERVER_NOT_STARTED || status == WEB_SERVER_STOPPED) {
        debugPrint("Web服务器未运行");
        return true;
    }
    
    debugPrint("停止Web服务器");
    
    if (server) {
        server->stop();
    }
    
    status = WEB_SERVER_STOPPED;
    startTime = 0;
    
    debugPrint("Web服务器已停止");
    return true;
}

/**
 * @brief 重启Web服务器
 */
bool WebServerManager::restart() {
    debugPrint("重启Web服务器");
    
    if (!stop()) {
        return false;
    }
    
    delay(1000); // 等待完全停止
    
    return start();
}

/**
 * @brief 处理客户端请求
 */
void WebServerManager::handleClient() {
    if (status == WEB_SERVER_RUNNING && server) {
        server->handleClient();
        updateStats();
    }
}

/**
 * @brief 获取服务器状态
 */
WebServerStatus WebServerManager::getStatus() {
    return status;
}

/**
 * @brief 获取服务器统计信息
 */
WebServerStats WebServerManager::getStats() {
    updateStats();
    return stats;
}

/**
 * @brief 获取服务器配置
 */
WebServerConfig WebServerManager::getConfig() {
    return config;
}

/**
 * @brief 更新服务器配置
 */
bool WebServerManager::updateConfig(const WebServerConfig& newConfig) {
    debugPrint("更新Web服务器配置");
    
    bool wasRunning = (status == WEB_SERVER_RUNNING);
    
    if (wasRunning) {
        stop();
    }
    
    config = newConfig;
    
    // 重新初始化
    initialized = false;
    if (!initialize(config)) {
        return false;
    }
    
    if (wasRunning) {
        return start();
    }
    
    return true;
}

/**
 * @brief 检查服务器是否运行
 */
bool WebServerManager::isRunning() {
    return status == WEB_SERVER_RUNNING;
}

/**
 * @brief 获取服务器URL
 */
String WebServerManager::getServerURL() {
    WiFiManager& wifiMgr = WiFiManager::getInstance();
    WiFiConnectionInfo connInfo = wifiMgr.getConnectionInfo();
    
    if (connInfo.isActive && !connInfo.apIP.isEmpty()) {
        return "http://" + connInfo.apIP + ":" + String(config.port);
    }
    
    return "http://192.168.4.1:" + String(config.port);
}

/**
 * @brief 获取最后的错误信息
 */
String WebServerManager::getLastError() {
    return lastError;
}

/**
 * @brief 设置调试模式
 */
void WebServerManager::setDebugMode(bool enabled) {
    debugMode = enabled;
}

/**
 * @brief 设置路由处理器
 */
void WebServerManager::setupRoutes() {
    if (!server) return;
    
    // 主页
    server->on("/", [this]() { handleRoot(); });
    
    // API接口
    server->on("/api", [this]() { handleAPI(); });
    server->on("/api/status", [this]() { handleSystemStatus(); });
    server->on("/api/wifi", [this]() { handleWiFiConfig(); });
    server->on("/api/database", [this]() { handleDatabaseStatus(); });
    
    // 404处理
    server->onNotFound([this]() { handleNotFound(); });
    
    debugPrint("路由设置完成");
}

/**
 * @brief 处理根路径请求
 */
void WebServerManager::handleRoot() {
    logRequest("/");
    
    if (config.enableAuth && !checkAuthentication()) {
        server->requestAuthentication();
        return;
    }
    
    if (config.enableCORS) {
        addCORSHeaders();
    }
    
    String html = generateHomePage();
    sendHTMLResponse(html);
}

/**
 * @brief 处理API请求
 */
void WebServerManager::handleAPI() {
    logRequest("/api");
    
    if (config.enableCORS) {
        addCORSHeaders();
    }
    
    String json = "{\"message\":\"ESP-SMS-Relay API\",\"version\":\"1.0\",\"endpoints\":[\"/api/status\",\"/api/wifi\",\"/api/database\"]}";
    sendJSONResponse(json);
}

/**
 * @brief 处理系统状态请求
 */
void WebServerManager::handleSystemStatus() {
    logRequest("/api/status");
    
    if (config.enableCORS) {
        addCORSHeaders();
    }
    
    String json = generateSystemStatusJSON();
    sendJSONResponse(json);
}

/**
 * @brief 处理WiFi配置请求
 */
void WebServerManager::handleWiFiConfig() {
    logRequest("/api/wifi");
    
    if (config.enableCORS) {
        addCORSHeaders();
    }
    
    WiFiManager& wifiMgr = WiFiManager::getInstance();
    WiFiConnectionInfo connInfo = wifiMgr.getConnectionInfo();
    APConfig apConfig = wifiMgr.getCurrentConfig();
    
    String json = "{";
    json += "\"status\":\"" + String(wifiMgr.isAPActive() ? "active" : "inactive") + "\",";
    json += "\"ssid\":\"" + apConfig.ssid + "\",";
    json += "\"ip\":\"" + connInfo.apIP + "\",";
    json += "\"clients\":" + String(connInfo.connectedClients) + ",";
    json += "\"uptime\":" + String(connInfo.uptime);
    json += "}";
    
    sendJSONResponse(json);
}

/**
 * @brief 处理数据库状态请求
 */
void WebServerManager::handleDatabaseStatus() {
    logRequest("/api/database");
    
    if (config.enableCORS) {
        addCORSHeaders();
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    DatabaseInfo dbInfo = db.getDatabaseInfo();
    
    String json = "{";
    json += "\"ready\":" + String(db.isReady() ? "true" : "false") + ",";
    json += "\"path\":\"" + dbInfo.dbPath + "\",";
    json += "\"size\":" + String(dbInfo.dbSize) + ",";
    json += "\"tables\":" + String(dbInfo.tableCount) + ",";
    json += "\"records\":" + String(dbInfo.recordCount);
    json += "}";
    
    sendJSONResponse(json);
}

/**
 * @brief 处理404错误
 */
void WebServerManager::handleNotFound() {
    logRequest(server->uri());
    
    if (config.enableCORS) {
        addCORSHeaders();
    }
    
    sendErrorResponse("页面未找到", 404);
}

/**
 * @brief 发送JSON响应
 */
void WebServerManager::sendJSONResponse(const String& json, int httpCode) {
    server->send(httpCode, "application/json; charset=utf-8", json);
}

/**
 * @brief 发送HTML响应
 */
void WebServerManager::sendHTMLResponse(const String& html, int httpCode) {
    server->send(httpCode, "text/html; charset=utf-8", html);
}

/**
 * @brief 发送错误响应
 */
void WebServerManager::sendErrorResponse(const String& message, int httpCode) {
    String json = "{\"error\":\"" + message + "\"}";
    sendJSONResponse(json, httpCode);
}

/**
 * @brief 检查认证
 */
bool WebServerManager::checkAuthentication() {
    if (!config.enableAuth) {
        return true;
    }
    
    return server->authenticate(config.authUsername.c_str(), config.authPassword.c_str());
}

/**
 * @brief 添加CORS头
 */
void WebServerManager::addCORSHeaders() {
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

/**
 * @brief 记录请求
 */
void WebServerManager::logRequest(const String& path) {
    stats.totalRequests++;
    stats.lastRequestTime = millis();
    stats.lastRequestPath = path;
    
    debugPrint("请求: " + path);
}

/**
 * @brief 生成主页HTML
 */
String WebServerManager::generateHomePage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>ESP-SMS-Relay 管理界面</title>";
    html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5}";
    html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center}h2{color:#666;border-bottom:2px solid #eee;padding-bottom:10px}";
    html += ".status{padding:10px;margin:10px 0;border-radius:4px}.status.ok{background:#d4edda;color:#155724}";
    html += ".status.error{background:#f8d7da;color:#721c24}.info{background:#e3f2fd;padding:15px;border-radius:4px;margin:10px 0}";
    html += "button{background:#007bff;color:white;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;margin:5px}";
    html += "button:hover{background:#0056b3}</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>ESP-SMS-Relay 系统</h1>";
    html += "<div class='info'>欢迎访问ESP-SMS-Relay管理界面。这是一个基于ESP32的短信中继系统。</div>";
    
    // 系统状态
    html += "<h2>系统状态</h2>";
    html += "<div class='status ok'>Web服务器运行正常</div>";
    
    // WiFi状态
    WiFiManager& wifiMgr = WiFiManager::getInstance();
    if (wifiMgr.isAPActive()) {
        WiFiConnectionInfo connInfo = wifiMgr.getConnectionInfo();
        html += "<div class='status ok'>WiFi热点已激活 - IP: " + connInfo.apIP + "</div>";
        html += "<div class='info'>已连接设备: " + String(connInfo.connectedClients) + " 台</div>";
    } else {
        html += "<div class='status error'>WiFi热点未激活</div>";
    }
    
    // 数据库状态
    DatabaseManager& db = DatabaseManager::getInstance();
    if (db.isReady()) {
        html += "<div class='status ok'>数据库连接正常</div>";
    } else {
        html += "<div class='status error'>数据库连接异常</div>";
    }
    
    // API链接
    html += "<h2>API接口</h2>";
    html += "<button onclick='location.href=\"/api/status\"'>系统状态</button>";
    html += "<button onclick='location.href=\"/api/wifi\"'>WiFi信息</button>";
    html += "<button onclick='location.href=\"/api/database\"'>数据库信息</button>";
    
    html += "<div class='info' style='margin-top:20px'>";
    html += "<strong>注意:</strong> 这是一个基础的管理界面。更多功能正在开发中。";
    html += "</div>";
    
    html += "</div></body></html>";
    
    return html;
}

/**
 * @brief 生成系统状态JSON
 */
String WebServerManager::generateSystemStatusJSON() {
    WiFiManager& wifiMgr = WiFiManager::getInstance();
    DatabaseManager& db = DatabaseManager::getInstance();
    
    String json = "{";
    json += "\"system\":{";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\"";
    json += "},";
    
    json += "\"wifi\":{";
    json += "\"ap_active\":" + String(wifiMgr.isAPActive() ? "true" : "false") + ",";
    json += "\"clients\":" + String(wifiMgr.getConnectedClientsCount());
    json += "},";
    
    json += "\"database\":{";
    json += "\"ready\":" + String(db.isReady() ? "true" : "false");
    json += "},";
    
    json += "\"web_server\":{";
    json += "\"status\":\"running\",";
    json += "\"requests\":" + String(stats.totalRequests) + ",";
    json += "\"uptime\":" + String(startTime > 0 ? millis() - startTime : 0);
    json += "}";
    
    json += "}";
    
    return json;
}

/**
 * @brief 设置错误信息
 */
void WebServerManager::setError(const String& error) {
    lastError = error;
    debugPrint("错误: " + error);
}

/**
 * @brief 打印调试信息
 */
void WebServerManager::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[WebServer] " + message);
    }
}

/**
 * @brief 更新统计信息
 */
void WebServerManager::updateStats() {
    if (startTime > 0) {
        stats.uptime = millis() - startTime;
    }
    
    // 这里可以添加更多统计信息的更新逻辑
    stats.activeConnections = 1; // 简化处理
}