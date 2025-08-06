/**
 * @file config_manager.cpp
 * @brief 配置管理模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "config_manager.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
ConfigManager::ConfigManager() : initialized(false), lastError("") {
    setDefaultConfig();
}

/**
 * @brief 析构函数
 */
ConfigManager::~ConfigManager() {
    if (initialized) {
        preferences.end();
    }
}

/**
 * @brief 获取单例实例
 * @return ConfigManager& 单例引用
 */
ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

/**
 * @brief 初始化配置管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ConfigManager::initialize() {
    if (initialized) {
        return true;
    }
    
    // 初始化NVS
    if (!preferences.begin("esp-sms-relay", false)) {
        setError("无法初始化NVS存储");
        return false;
    }
    
    initialized = true;
    
    // 加载配置
    if (!loadConfig()) {
        Serial.println("警告: 加载配置失败，使用默认配置");
        setDefaultConfig();
        saveConfig(); // 保存默认配置
    }
    
    // 验证配置
    if (!validateConfig()) {
        Serial.println("警告: 配置验证失败，重置为默认配置");
        resetToDefaults();
    }
    
    Serial.println("配置管理器初始化完成");
    return true;
}

/**
 * @brief 加载配置
 * @return true 加载成功
 * @return false 加载失败
 */
bool ConfigManager::loadConfig() {
    if (!initialized) {
        setError("配置管理器未初始化");
        return false;
    }
    
    try {
        // 加载串口配置
        uartConfig.baudRate = preferences.getInt("uart_baud", 115200);
        uartConfig.rxPin = preferences.getInt("uart_rx", 16);
        uartConfig.txPin = preferences.getInt("uart_tx", 17);
        uartConfig.serialNumber = preferences.getInt("uart_num", 2);
        uartConfig.timeout = preferences.getULong("uart_timeout", 5000);
        
        // 加载短信配置
        smsConfig.smsCenterNumber = preferences.getString("sms_center", "+8613010200500");
        smsConfig.testPhoneNumber = preferences.getString("test_phone", "10086");
        smsConfig.maxRetries = preferences.getInt("sms_retries", 3);
        smsConfig.sendTimeout = preferences.getULong("sms_timeout", 30000);
        smsConfig.enableNotification = preferences.getBool("sms_notify", true);
        
        // 加载GSM配置
        gsmConfig.initTimeout = preferences.getULong("gsm_init_timeout", 30000);
        gsmConfig.commandTimeout = preferences.getULong("gsm_cmd_timeout", 5000);
        gsmConfig.maxInitRetries = preferences.getInt("gsm_retries", 3);
        gsmConfig.signalThreshold = preferences.getInt("gsm_signal", 10);
        gsmConfig.autoReconnect = preferences.getBool("gsm_reconnect", true);
        
        // 加载系统配置
        systemConfig.enableDebug = preferences.getBool("sys_debug", true);
        systemConfig.runTestsOnStartup = preferences.getBool("sys_test", true);
        systemConfig.watchdogTimeout = preferences.getULong("sys_watchdog", 60000);
        systemConfig.logLevel = preferences.getInt("sys_log_level", 3);
        systemConfig.deviceName = preferences.getString("sys_name", "ESP-SMS-Relay");
        
        Serial.println("配置加载完成");
        return true;
    } catch (...) {
        setError("加载配置时发生异常");
        return false;
    }
}

/**
 * @brief 保存配置
 * @return true 保存成功
 * @return false 保存失败
 */
bool ConfigManager::saveConfig() {
    if (!initialized) {
        setError("配置管理器未初始化");
        return false;
    }
    
    try {
        // 保存串口配置
        preferences.putInt("uart_baud", uartConfig.baudRate);
        preferences.putInt("uart_rx", uartConfig.rxPin);
        preferences.putInt("uart_tx", uartConfig.txPin);
        preferences.putInt("uart_num", uartConfig.serialNumber);
        preferences.putULong("uart_timeout", uartConfig.timeout);
        
        // 保存短信配置
        preferences.putString("sms_center", smsConfig.smsCenterNumber);
        preferences.putString("test_phone", smsConfig.testPhoneNumber);
        preferences.putInt("sms_retries", smsConfig.maxRetries);
        preferences.putULong("sms_timeout", smsConfig.sendTimeout);
        preferences.putBool("sms_notify", smsConfig.enableNotification);
        
        // 保存GSM配置
        preferences.putULong("gsm_init_timeout", gsmConfig.initTimeout);
        preferences.putULong("gsm_cmd_timeout", gsmConfig.commandTimeout);
        preferences.putInt("gsm_retries", gsmConfig.maxInitRetries);
        preferences.putInt("gsm_signal", gsmConfig.signalThreshold);
        preferences.putBool("gsm_reconnect", gsmConfig.autoReconnect);
        
        // 保存系统配置
        preferences.putBool("sys_debug", systemConfig.enableDebug);
        preferences.putBool("sys_test", systemConfig.runTestsOnStartup);
        preferences.putULong("sys_watchdog", systemConfig.watchdogTimeout);
        preferences.putInt("sys_log_level", systemConfig.logLevel);
        preferences.putString("sys_name", systemConfig.deviceName);
        
        Serial.println("配置保存完成");
        return true;
    } catch (...) {
        setError("保存配置时发生异常");
        return false;
    }
}

/**
 * @brief 重置为默认配置
 * @return true 重置成功
 * @return false 重置失败
 */
bool ConfigManager::resetToDefaults() {
    if (!initialized) {
        setError("配置管理器未初始化");
        return false;
    }
    
    // 清除所有存储的配置
    preferences.clear();
    
    // 设置默认配置
    setDefaultConfig();
    
    // 保存默认配置
    if (!saveConfig()) {
        setError("保存默认配置失败");
        return false;
    }
    
    Serial.println("配置已重置为默认值");
    return true;
}

/**
 * @brief 验证配置
 * @return true 配置有效
 * @return false 配置无效
 */
bool ConfigManager::validateConfig() {
    // 验证串口配置
    if (uartConfig.baudRate <= 0 || uartConfig.rxPin < 0 || uartConfig.txPin < 0) {
        setError("串口配置无效");
        return false;
    }
    
    // 验证短信配置
    if (smsConfig.smsCenterNumber.length() == 0 || smsConfig.maxRetries <= 0) {
        setError("短信配置无效");
        return false;
    }
    
    // 验证GSM配置
    if (gsmConfig.initTimeout <= 0 || gsmConfig.commandTimeout <= 0) {
        setError("GSM配置无效");
        return false;
    }
    
    // 验证系统配置
    if (systemConfig.watchdogTimeout <= 0 || systemConfig.deviceName.length() == 0) {
        setError("系统配置无效");
        return false;
    }
    
    return true;
}

/**
 * @brief 设置默认配置
 */
void ConfigManager::setDefaultConfig() {
    // 串口默认配置
    uartConfig.baudRate = 115200;
    uartConfig.rxPin = 16;
    uartConfig.txPin = 17;
    uartConfig.serialNumber = 2;
    uartConfig.timeout = 5000;
    
    // 短信默认配置
    smsConfig.smsCenterNumber = "+8613010200500";
    smsConfig.testPhoneNumber = "10086";
    smsConfig.maxRetries = 3;
    smsConfig.sendTimeout = 30000;
    smsConfig.enableNotification = true;
    
    // GSM默认配置
    gsmConfig.initTimeout = 30000;
    gsmConfig.commandTimeout = 5000;
    gsmConfig.maxInitRetries = 3;
    gsmConfig.signalThreshold = 10;
    gsmConfig.autoReconnect = true;
    
    // 系统默认配置
    systemConfig.enableDebug = true;
    systemConfig.runTestsOnStartup = true;
    systemConfig.watchdogTimeout = 60000;
    systemConfig.logLevel = 2;
    systemConfig.deviceName = "ESP-SMS-Relay";
}

// 配置获取方法
UartConfig ConfigManager::getUartConfig() { return uartConfig; }
SmsConfig ConfigManager::getSmsConfig() { return smsConfig; }
GsmConfig ConfigManager::getGsmConfig() { return gsmConfig; }
SystemConfig ConfigManager::getSystemConfig() { return systemConfig; }

// 配置设置方法
void ConfigManager::setUartConfig(const UartConfig& config) { uartConfig = config; }
void ConfigManager::setSmsConfig(const SmsConfig& config) { smsConfig = config; }
void ConfigManager::setGsmConfig(const GsmConfig& config) { gsmConfig = config; }
void ConfigManager::setSystemConfig(const SystemConfig& config) { systemConfig = config; }

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String ConfigManager::getLastError() {
    return lastError;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void ConfigManager::setError(const String& error) {
    lastError = error;
    Serial.printf("配置管理器错误: %s\n", error.c_str());
}

/**
 * @brief 打印当前配置
 */
void ConfigManager::printConfig() {
    Serial.println("\n=== 当前系统配置 ===");
    
    Serial.println("[串口配置]");
    Serial.printf("  波特率: %d\n", uartConfig.baudRate);
    Serial.printf("  RX引脚: %d\n", uartConfig.rxPin);
    Serial.printf("  TX引脚: %d\n", uartConfig.txPin);
    Serial.printf("  串口号: %d\n", uartConfig.serialNumber);
    Serial.printf("  超时时间: %lu ms\n", uartConfig.timeout);
    
    Serial.println("[短信配置]");
    Serial.printf("  短信中心: %s\n", smsConfig.smsCenterNumber.c_str());
    Serial.printf("  测试号码: %s\n", smsConfig.testPhoneNumber.c_str());
    Serial.printf("  最大重试: %d\n", smsConfig.maxRetries);
    Serial.printf("  发送超时: %lu ms\n", smsConfig.sendTimeout);
    Serial.printf("  新短信通知: %s\n", smsConfig.enableNotification ? "启用" : "禁用");
    
    Serial.println("[GSM配置]");
    Serial.printf("  初始化超时: %lu ms\n", gsmConfig.initTimeout);
    Serial.printf("  命令超时: %lu ms\n", gsmConfig.commandTimeout);
    Serial.printf("  最大重试: %d\n", gsmConfig.maxInitRetries);
    Serial.printf("  信号阈值: %d\n", gsmConfig.signalThreshold);
    Serial.printf("  自动重连: %s\n", gsmConfig.autoReconnect ? "启用" : "禁用");
    
    Serial.println("[系统配置]");
    Serial.printf("  调试模式: %s\n", systemConfig.enableDebug ? "启用" : "禁用");
    Serial.printf("  启动测试: %s\n", systemConfig.runTestsOnStartup ? "启用" : "禁用");
    Serial.printf("  看门狗超时: %lu ms\n", systemConfig.watchdogTimeout);
    Serial.printf("  日志级别: %d\n", systemConfig.logLevel);
    Serial.printf("  设备名称: %s\n", systemConfig.deviceName.c_str());
    
    Serial.println("=== 配置信息结束 ===\n");
}