/**
 * @file log_manager.cpp
 * @brief 日志管理模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "log_manager.h"
#include "config_manager.h"
#include <Arduino.h>
#include <stdarg.h>

/**
 * @brief 构造函数
 */
LogManager::LogManager() : 
    currentLogLevel(LOG_LEVEL_INFO),
    timestampEnabled(true),
    moduleTagEnabled(true),
    initialized(false) {
}

/**
 * @brief 析构函数
 */
LogManager::~LogManager() {
    // 清理资源
}

/**
 * @brief 获取单例实例
 * @return LogManager& 单例引用
 */
LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

/**
 * @brief 初始化日志管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool LogManager::initialize() {
    if (initialized) {
        return true;
    }
    
    // 从配置管理器获取日志配置
    ConfigManager& configManager = ConfigManager::getInstance();
    SystemConfig sysConfig = configManager.getSystemConfig();
    
    // 设置日志级别
    setLogLevel(static_cast<LogLevel>(sysConfig.logLevel));
    
    // 启用时间戳和模块标识（根据调试模式）
    enableTimestamp(sysConfig.enableDebug);
    enableModuleTag(sysConfig.enableDebug);
    
    initialized = true;
    
    // 输出初始化信息
    logInfo(LOG_MODULE_SYSTEM, "日志管理器初始化完成");
    logInfo(LOG_MODULE_SYSTEM, "日志级别: " + getLevelName(currentLogLevel));
    
    return true;
}

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void LogManager::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

/**
 * @brief 获取日志级别
 * @return LogLevel 当前日志级别
 */
LogLevel LogManager::getLogLevel() {
    return currentLogLevel;
}

/**
 * @brief 启用/禁用时间戳
 * @param enable 是否启用时间戳
 */
void LogManager::enableTimestamp(bool enable) {
    timestampEnabled = enable;
}

/**
 * @brief 启用/禁用模块标识
 * @param enable 是否启用模块标识
 */
void LogManager::enableModuleTag(bool enable) {
    moduleTagEnabled = enable;
}

/**
 * @brief 记录错误日志
 * @param module 模块标识
 * @param message 日志消息
 */
void LogManager::logError(LogModule module, const String& message) {
    output(LOG_LEVEL_ERROR, module, message);
}

/**
 * @brief 记录警告日志
 * @param module 模块标识
 * @param message 日志消息
 */
void LogManager::logWarn(LogModule module, const String& message) {
    output(LOG_LEVEL_WARN, module, message);
}

/**
 * @brief 记录信息日志
 * @param module 模块标识
 * @param message 日志消息
 */
void LogManager::logInfo(LogModule module, const String& message) {
    output(LOG_LEVEL_INFO, module, message);
}

/**
 * @brief 记录调试日志
 * @param module 模块标识
 * @param message 日志消息
 */
void LogManager::logDebug(LogModule module, const String& message) {
    output(LOG_LEVEL_DEBUG, module, message);
}

/**
 * @brief 记录详细日志
 * @param module 模块标识
 * @param message 日志消息
 */
void LogManager::logVerbose(LogModule module, const String& message) {
    output(LOG_LEVEL_VERBOSE, module, message);
}

/**
 * @brief 记录格式化日志
 * @param level 日志级别
 * @param module 模块标识
 * @param format 格式字符串
 * @param ... 格式参数
 */
void LogManager::logf(LogLevel level, LogModule module, const char* format, ...) {
    if (!shouldLog(level)) {
        return;
    }
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    output(level, module, String(buffer));
}

/**
 * @brief 输出日志
 * @param level 日志级别
 * @param module 模块标识
 * @param message 日志消息
 */
void LogManager::output(LogLevel level, LogModule module, const String& message) {
    if (!shouldLog(level)) {
        return;
    }
    
    String logLine = "";
    
    // 添加时间戳
    if (timestampEnabled) {
        logLine += "[" + getTimestamp() + "] ";
    }
    
    // 添加日志级别
    logLine += "[" + getLevelName(level) + "] ";
    
    // 添加模块标识
    if (moduleTagEnabled) {
        logLine += "[" + getModuleName(module) + "] ";
    }
    
    // 添加消息内容
    logLine += message;
    
    // 输出到串口
    Serial.println(logLine);
}

/**
 * @brief 获取日志级别名称
 * @param level 日志级别
 * @return String 级别名称
 */
String LogManager::getLevelName(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_VERBOSE: return "VERBOSE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 获取模块名称
 * @param module 模块标识
 * @return String 模块名称
 */
String LogManager::getModuleName(LogModule module) {
    switch (module) {
        case LOG_MODULE_SYSTEM: return "SYS";
        case LOG_MODULE_GSM: return "GSM";
        case LOG_MODULE_SMS: return "SMS";
        case LOG_MODULE_PHONE: return "PHONE";
        case LOG_MODULE_UART: return "UART";
        case LOG_MODULE_CONFIG: return "CONFIG";
        case LOG_MODULE_TEST: return "TEST";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 获取当前时间戳
 * @return String 时间戳字符串
 */
String LogManager::getTimestamp() {
    unsigned long currentTime = millis();
    unsigned long seconds = currentTime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;
    
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu.%03lu", 
             hours, minutes, seconds, currentTime % 1000);
    
    return String(timeStr);
}

/**
 * @brief 检查日志级别是否应该输出
 * @param level 日志级别
 * @return true 应该输出
 * @return false 不应该输出
 */
bool LogManager::shouldLog(LogLevel level) {
    return level <= currentLogLevel;
}

/**
 * @brief 打印系统分隔线
 * @param title 标题（可选）
 */
void LogManager::printSeparator(const String& title) {
    String separator = "=";
    for (int i = 0; i < 50; i++) {
        separator += "=";
    }
    
    Serial.println(separator);
    
    if (title.length() > 0) {
        String centeredTitle = "=== " + title + " ===";
        Serial.println(centeredTitle);
        Serial.println(separator);
    }
}

/**
 * @brief 打印系统启动信息
 */
void LogManager::printStartupInfo() {
    printSeparator("ESP-SMS-Relay 系统启动");
    
    // 获取配置信息
    ConfigManager& configManager = ConfigManager::getInstance();
    SystemConfig sysConfig = configManager.getSystemConfig();
    
    logInfo(LOG_MODULE_SYSTEM, "设备名称: " + sysConfig.deviceName);
    logInfo(LOG_MODULE_SYSTEM, "固件版本: v1.0.0");
    logInfo(LOG_MODULE_SYSTEM, "编译时间: " + String(__DATE__) + " " + String(__TIME__));
    logInfo(LOG_MODULE_SYSTEM, "芯片型号: " + String(ESP.getChipModel()));
    logInfo(LOG_MODULE_SYSTEM, "CPU频率: " + String(ESP.getCpuFreqMHz()) + " MHz");
    logInfo(LOG_MODULE_SYSTEM, "Flash大小: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
    logInfo(LOG_MODULE_SYSTEM, "可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    logInfo(LOG_MODULE_SYSTEM, "调试模式: " + String(sysConfig.enableDebug ? "启用" : "禁用"));
    logInfo(LOG_MODULE_SYSTEM, "启动测试: " + String(sysConfig.runTestsOnStartup ? "启用" : "禁用"));
    
    printSeparator();
}