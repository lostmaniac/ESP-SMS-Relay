/**
 * @file wifi_manager.cpp
 * @brief WiFi管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "wifi_manager.h"

// 静态实例
static WiFiManager* wifiManagerInstance = nullptr;

/**
 * @brief 构造函数
 */
WiFiManager::WiFiManager() {
    status = WIFI_MANAGER_NOT_STARTED;
    debugMode = false;
    initialized = false;
    apStartTime = 0;
    lastStatusCheck = 0;
    
    // 初始化连接信息
    connectionInfo.connectedClients = 0;
    connectionInfo.apIP = "";
    connectionInfo.apMAC = "";
    connectionInfo.uptime = 0;
    connectionInfo.isActive = false;
    
    // 初始化配置
    currentConfig.ssid = "";
    currentConfig.password = "";
    currentConfig.enabled = false;
    currentConfig.channel = 1;
    currentConfig.maxConnections = 4;
}

/**
 * @brief 析构函数
 */
WiFiManager::~WiFiManager() {
    if (isAPActive()) {
        stopAP();
    }
}

/**
 * @brief 获取单例实例
 */
WiFiManager& WiFiManager::getInstance() {
    if (wifiManagerInstance == nullptr) {
        wifiManagerInstance = new WiFiManager();
    }
    return *wifiManagerInstance;
}

/**
 * @brief 初始化WiFi管理器
 */
bool WiFiManager::initialize() {
    debugPrint("初始化WiFi管理器...");
    
    status = WIFI_MANAGER_INITIALIZING;
    
    // 设置WiFi模式为AP模式
    WiFi.mode(WIFI_AP);
    
    // 从数据库加载配置
    if (!loadAPConfigFromDatabase()) {
        setError("从数据库加载AP配置失败");
        status = WIFI_MANAGER_ERROR;
        return false;
    }
    
    initialized = true;
    status = WIFI_MANAGER_NOT_STARTED;
    debugPrint("WiFi管理器初始化完成");
    
    return true;
}

/**
 * @brief 启动WiFi热点
 */
bool WiFiManager::startAP() {
    if (!initialized) {
        setError("WiFi管理器未初始化");
        return false;
    }
    
    if (!currentConfig.enabled) {
        setError("AP配置未启用");
        return false;
    }
    
    debugPrint("启动WiFi热点: " + currentConfig.ssid);
    status = WIFI_MANAGER_AP_STARTING;
    
    // 应用AP配置
    if (!applyAPConfig(currentConfig)) {
        status = WIFI_MANAGER_ERROR;
        return false;
    }
    
    // 等待AP启动
    unsigned long startTime = millis();
    while (WiFi.softAPgetStationNum() == 0 && millis() - startTime < 5000) {
        delay(100);
    }
    
    // 检查AP是否成功启动
    if (WiFi.softAPIP().toString() == "0.0.0.0") {
        setError("WiFi热点启动失败");
        status = WIFI_MANAGER_ERROR;
        return false;
    }
    
    apStartTime = millis();
    status = WIFI_MANAGER_AP_ACTIVE;
    updateConnectionInfo();
    
    debugPrint("WiFi热点启动成功，IP: " + WiFi.softAPIP().toString());
    return true;
}

/**
 * @brief 停止WiFi热点
 */
bool WiFiManager::stopAP() {
    debugPrint("停止WiFi热点");
    
    bool result = WiFi.softAPdisconnect(true);
    
    if (result) {
        status = WIFI_MANAGER_NOT_STARTED;
        connectionInfo.isActive = false;
        connectionInfo.connectedClients = 0;
        apStartTime = 0;
        debugPrint("WiFi热点已停止");
    } else {
        setError("停止WiFi热点失败");
    }
    
    return result;
}

/**
 * @brief 重启WiFi热点
 */
bool WiFiManager::restartAP() {
    debugPrint("重启WiFi热点");
    
    if (isAPActive()) {
        if (!stopAP()) {
            return false;
        }
        delay(1000); // 等待完全停止
    }
    
    return startAP();
}

/**
 * @brief 更新WiFi配置
 */
bool WiFiManager::updateAPConfig(const APConfig& config) {
    debugPrint("更新AP配置");
    
    // 验证配置
    if (!validateAPConfig(config)) {
        setError("AP配置无效");
        return false;
    }
    
    // 更新数据库
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.updateAPConfig(config)) {
        setError("更新数据库AP配置失败: " + db.getLastError());
        return false;
    }
    
    // 更新当前配置
    currentConfig = config;
    
    // 如果AP正在运行，重启以应用新配置
    if (isAPActive() && config.enabled) {
        return restartAP();
    } else if (!config.enabled && isAPActive()) {
        return stopAP();
    } else if (config.enabled && !isAPActive()) {
        return startAP();
    }
    
    return true;
}

/**
 * @brief 获取当前WiFi状态
 */
WiFiManagerStatus WiFiManager::getStatus() {
    return status;
}

/**
 * @brief 获取WiFi连接信息
 */
WiFiConnectionInfo WiFiManager::getConnectionInfo() {
    updateConnectionInfo();
    return connectionInfo;
}

/**
 * @brief 获取当前AP配置
 */
APConfig WiFiManager::getCurrentConfig() {
    return currentConfig;
}

/**
 * @brief 检查AP是否激活
 */
bool WiFiManager::isAPActive() {
    return (status == WIFI_MANAGER_AP_ACTIVE) && (WiFi.softAPIP().toString() != "0.0.0.0");
}

/**
 * @brief 获取已连接的客户端数量
 */
int WiFiManager::getConnectedClientsCount() {
    if (isAPActive()) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

/**
 * @brief 获取最后的错误信息
 */
String WiFiManager::getLastError() {
    return lastError;
}

/**
 * @brief 设置调试模式
 */
void WiFiManager::setDebugMode(bool enabled) {
    debugMode = enabled;
}

/**
 * @brief 处理WiFi事件
 */
void WiFiManager::handleEvents() {
    unsigned long currentTime = millis();
    
    // 每5秒检查一次状态
    if (currentTime - lastStatusCheck > 5000) {
        lastStatusCheck = currentTime;
        
        if (isAPActive()) {
            updateConnectionInfo();
        }
    }
}

/**
 * @brief 从数据库加载AP配置
 */
bool WiFiManager::loadAPConfigFromDatabase() {
    debugPrint("从数据库加载AP配置");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.isReady()) {
        setError("数据库未就绪");
        return false;
    }
    
    currentConfig = db.getAPConfig();
    
    if (currentConfig.ssid.isEmpty()) {
        setError("数据库中未找到有效的AP配置");
        return false;
    }
    
    debugPrint("AP配置加载成功: " + currentConfig.ssid);
    return true;
}

/**
 * @brief 应用AP配置
 */
bool WiFiManager::applyAPConfig(const APConfig& config) {
    debugPrint("应用AP配置: " + config.ssid);
    
    // 验证配置
    if (!validateAPConfig(config)) {
        setError("AP配置无效");
        return false;
    }
    
    bool result;
    
    // 根据是否有密码选择不同的启动方式
    if (config.password.isEmpty() || config.password.length() < 8) {
        // 开放网络
        result = WiFi.softAP(config.ssid.c_str(), nullptr, config.channel, 0, config.maxConnections);
        debugPrint("启动开放WiFi热点");
    } else {
        // 加密网络
        result = WiFi.softAP(config.ssid.c_str(), config.password.c_str(), config.channel, 0, config.maxConnections);
        debugPrint("启动加密WiFi热点");
    }
    
    if (!result) {
        setError("WiFi.softAP()调用失败");
        return false;
    }
    
    // 等待IP地址分配
    delay(1000);
    
    return true;
}

/**
 * @brief 更新连接信息
 */
void WiFiManager::updateConnectionInfo() {
    if (isAPActive()) {
        connectionInfo.isActive = true;
        connectionInfo.connectedClients = WiFi.softAPgetStationNum();
        connectionInfo.apIP = WiFi.softAPIP().toString();
        connectionInfo.apMAC = WiFi.softAPmacAddress();
        connectionInfo.uptime = apStartTime > 0 ? millis() - apStartTime : 0;
    } else {
        connectionInfo.isActive = false;
        connectionInfo.connectedClients = 0;
        connectionInfo.apIP = "";
        connectionInfo.apMAC = "";
        connectionInfo.uptime = 0;
    }
}

/**
 * @brief 设置错误信息
 */
void WiFiManager::setError(const String& error) {
    lastError = error;
    debugPrint("错误: " + error);
}

/**
 * @brief 打印调试信息
 */
void WiFiManager::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[WiFiManager] " + message);
    }
}

/**
 * @brief 验证AP配置
 */
bool WiFiManager::validateAPConfig(const APConfig& config) {
    // 检查SSID
    if (config.ssid.isEmpty() || config.ssid.length() > 32) {
        setError("SSID无效（长度应为1-32字符）");
        return false;
    }
    
    // 检查密码（如果设置了密码）
    if (!config.password.isEmpty() && config.password.length() < 8) {
        setError("密码长度至少需要8个字符");
        return false;
    }
    
    // 检查信道
    if (config.channel < 1 || config.channel > 13) {
        setError("WiFi信道无效（应为1-13）");
        return false;
    }
    
    // 检查最大连接数
    if (config.maxConnections < 1 || config.maxConnections > 8) {
        setError("最大连接数无效（应为1-8）");
        return false;
    }
    
    return true;
}