/**
 * @file module_manager.cpp
 * @brief 模块管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "module_manager.h"
#include "gsm_service.h"
#include "sms_sender.h"
#include "phone_caller.h"
#include "../uart_monitor/uart_monitor.h"
#include "test_manager.h"
#include "config_manager.h"
#include <Arduino.h>

// 前向声明SmsHandler以避免重复包含pdulib.h
class SmsHandler;

// 外部串口对象
extern HardwareSerial simSerial;

// 全局模块实例
static SmsSender* g_sms_sender = nullptr;
static PhoneCaller* g_phone_caller = nullptr;
static SmsHandler* g_sms_handler = nullptr;

/**
 * @brief 构造函数
 */
ModuleManager::ModuleManager() : initialized(false) {
    // 初始化所有模块状态为未初始化
    for (int i = 0; i < MODULE_COUNT; i++) {
        moduleStatuses[i] = MODULE_NOT_INITIALIZED;
    }
    lastError = "";
}

/**
 * @brief 析构函数
 */
ModuleManager::~ModuleManager() {
    // 清理资源
    if (g_sms_sender) {
        delete g_sms_sender;
        g_sms_sender = nullptr;
    }
    if (g_phone_caller) {
        delete g_phone_caller;
        g_phone_caller = nullptr;
    }
    if (g_sms_handler) {
        delete g_sms_handler;
        g_sms_handler = nullptr;
    }
}

/**
 * @brief 获取单例实例
 * @return ModuleManager& 单例引用
 */
ModuleManager& ModuleManager::getInstance() {
    static ModuleManager instance;
    return instance;
}

// 静态函数已移至GSM服务模块

/**
 * @brief 初始化所有模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initializeAllModules() {
    Serial.println("=== 开始模块管理器初始化 ===");
    
    // 首先初始化配置管理器
    ConfigManager& configManager = ConfigManager::getInstance();
    if (!configManager.initialize()) {
        setError("配置管理器初始化失败: " + configManager.getLastError());
        return false;
    }
    
    // 打印当前配置
    configManager.printConfig();
    
    // 按依赖顺序初始化模块
    if (!initializeModule(MODULE_GSM_BASIC)) {
        setError("GSM基础模块初始化失败");
        return false;
    }
    
    if (!initializeModule(MODULE_SMS_SENDER)) {
        setError("短信发送模块初始化失败");
        return false;
    }
    
    if (!initializeModule(MODULE_PHONE_CALLER)) {
        setError("电话拨打模块初始化失败");
        return false;
    }
    
    if (!initializeModule(MODULE_SMS_HANDLER)) {
        setError("短信处理模块初始化失败");
        return false;
    }
    
    initialized = true;
    Serial.println("=== 模块管理器初始化完成 ===");
    return true;
}

/**
 * @brief 初始化指定模块
 * @param moduleType 模块类型
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initializeModule(ModuleType moduleType) {
    if (moduleStatuses[moduleType] == MODULE_READY) {
        return true; // 已经初始化
    }
    
    setModuleStatus(moduleType, MODULE_INITIALIZING);
    
    bool result = false;
    switch (moduleType) {
        case MODULE_GSM_BASIC:
            result = initGsmBasicModule();
            break;
        case MODULE_SMS_SENDER:
            result = initSmsSenderModule();
            break;
        case MODULE_PHONE_CALLER:
            result = initPhoneCallerModule();
            break;
        case MODULE_SMS_HANDLER:
            result = initSmsHandlerModule();
            break;
        case MODULE_UART_MONITOR:
            result = initUartMonitorModule();
            break;
        default:
            setError("未知的模块类型");
            result = false;
            break;
    }
    
    if (result) {
        setModuleStatus(moduleType, MODULE_READY);
    } else {
        setModuleStatus(moduleType, MODULE_ERROR);
    }
    
    return result;
}

/**
 * @brief 初始化GSM基础模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initGsmBasicModule() {
    Serial.println("正在初始化GSM基础模块...");
    
    GsmService& gsmService = GsmService::getInstance();
    
    if (!gsmService.initialize()) {
        setError("GSM服务初始化失败: " + gsmService.getLastError());
        return false;
    }
    
    Serial.println("GSM基础模块初始化完成。");
    return true;
}

/**
 * @brief 初始化短信发送模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initSmsSenderModule() {
    Serial.println("正在初始化短信发送模块...");
    
    // 获取GSM服务实例
    GsmService& gsmService = GsmService::getInstance();
    
    // 获取短信中心号码
    String scaAddress = gsmService.getSmsCenterNumber();
    if (scaAddress.length() == 0) {
        setError("无法获取短信中心号码");
        return false;
    }
    
    // 创建短信发送器实例
    if (!g_sms_sender) {
        g_sms_sender = new SmsSender(200); // 200字节缓冲区
        if (!g_sms_sender) {
            setError("无法创建短信发送器实例");
            return false;
        }
    }
    
    // 初始化短信发送器
    if (!g_sms_sender->initialize(scaAddress)) {
        setError("短信发送器初始化失败: " + g_sms_sender->getLastError());
        return false;
    }
    
    Serial.println("短信发送模块初始化完成。");
    return true;
}

/**
 * @brief 初始化电话拨打模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initPhoneCallerModule() {
    Serial.println("正在初始化电话拨打模块...");
    
    // 创建电话拨打器实例
    if (!g_phone_caller) {
        g_phone_caller = new PhoneCaller();
        if (!g_phone_caller) {
            setError("无法创建电话拨打器实例");
            return false;
        }
    }
    
    Serial.println("电话拨打模块初始化完成。");
    return true;
}

/**
 * @brief 初始化短信处理模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initSmsHandlerModule() {
    Serial.println("正在初始化短信处理模块...");
    
    // 暂时跳过SmsHandler初始化以避免头文件冲突
    // TODO: 重构SmsHandler以避免与SmsSender的pdulib冲突
    Serial.println("短信处理模块初始化暂时跳过（避免头文件冲突）");
    
    Serial.println("短信处理模块初始化完成。");
    return true;
}

/**
 * @brief 初始化串口监听模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initUartMonitorModule() {
    Serial.println("正在初始化串口监听模块...");
    
    // 串口监听模块不需要特殊初始化
    Serial.println("串口监听模块初始化完成。");
    return true;
}

/**
 * @brief 获取模块状态
 * @param moduleType 模块类型
 * @return ModuleStatus 模块状态
 */
ModuleStatus ModuleManager::getModuleStatus(ModuleType moduleType) {
    if (moduleType >= MODULE_COUNT) {
        return MODULE_ERROR;
    }
    return moduleStatuses[moduleType];
}

/**
 * @brief 检查所有模块是否就绪
 * @return true 所有模块就绪
 * @return false 存在未就绪模块
 */
bool ModuleManager::areAllModulesReady() {
    for (int i = 0; i < MODULE_COUNT - 1; i++) { // 排除UART_MONITOR，它在后台启动
        if (moduleStatuses[i] != MODULE_READY) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String ModuleManager::getLastError() {
    return lastError;
}

/**
 * @brief 运行模块测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool ModuleManager::runModuleTests() {
    Serial.println("=== 开始模块功能测试 ===");
    
    TestManager& testManager = TestManager::getInstance();
    
    // 初始化测试管理器
    if (!testManager.initialize()) {
        setError("测试管理器初始化失败: " + testManager.getLastError());
        return false;
    }
    
    // 运行所有测试
    bool allTestsPassed = testManager.runAllTests();
    
    Serial.println("\n=== 模块功能测试完成 ===");
    return allTestsPassed;
}

/**
 * @brief 启动后台任务
 * @return true 启动成功
 * @return false 启动失败
 */
bool ModuleManager::startBackgroundTasks() {
    Serial.println("正在启动后台任务...");
    
    // 启动串口监听任务
    if (initializeModule(MODULE_UART_MONITOR)) {
        xTaskCreate(
            uart_monitor_task,   // Task function
            "UartMonitorTask",   // Task name
            10000,               // Stack size (bytes)
            NULL,                // Parameter
            1,                   // Priority
            NULL                 // Task handle
        );
        Serial.println("串口监听任务已启动。");
        return true;
    } else {
        setError("启动串口监听任务失败");
        return false;
    }
}

/**
 * @brief 设置模块状态
 * @param moduleType 模块类型
 * @param status 模块状态
 */
void ModuleManager::setModuleStatus(ModuleType moduleType, ModuleStatus status) {
    if (moduleType < MODULE_COUNT) {
        moduleStatuses[moduleType] = status;
    }
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void ModuleManager::setError(const String& error) {
    lastError = error;
    Serial.printf("模块管理器错误: %s\n", error.c_str());
}

// 全局访问函数

/**
 * @brief 获取短信发送器实例
 * @return SmsSender* 短信发送器指针，如果未初始化则返回nullptr
 */
SmsSender* getSmsSender() {
    return g_sms_sender;
}

/**
 * @brief 获取电话拨打器实例
 * @return PhoneCaller* 电话拨打器指针，如果未初始化则返回nullptr
 */
PhoneCaller* getPhoneCaller() {
    return g_phone_caller;
}

/**
 * @brief 获取短信处理器实例
 * @return SmsHandler* 短信处理器指针，如果未初始化则返回nullptr
 */
SmsHandler* getSmsHandler() {
    return g_sms_handler;
}