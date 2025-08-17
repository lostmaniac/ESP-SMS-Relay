/**
 * @file module_manager.cpp
 * @brief 模块管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "module_manager.h"
#include "gsm_service.h"
#include "phone_caller.h"
#include "../uart_monitor/uart_monitor.h"
#include "at_command_handler.h"
#include "http_client.h"
#include "config_manager.h"
#include "config.h"
#include "../sms_handler/sms_handler.h"
#include "log_manager.h"
#include "../../include/constants.h"
#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

// 前向声明以避免重复包含pdulib.h
class SmsSender;

// 外部串口声明
extern HardwareSerial simSerial;

// 全局模块实例
static PhoneCaller* g_phone_caller = nullptr;
static AtCommandHandler* g_at_command_handler = nullptr;
static HttpClient* g_http_client = nullptr;

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
    if (g_phone_caller) {
        delete g_phone_caller;
        g_phone_caller = nullptr;
    }
    if (g_at_command_handler) {
        delete g_at_command_handler;
        g_at_command_handler = nullptr;
    }
    if (g_http_client) {
        delete g_http_client;
        g_http_client = nullptr;
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
    // 开始模块管理器初始化
    
    // 首先初始化配置管理器
    ConfigManager& configManager = ConfigManager::getInstance();
    if (!configManager.initialize()) {
        setError("配置管理器初始化失败: " + configManager.getLastError());
        return false;
    }
    
    // 打印当前配置
    configManager.printConfig();
    
    // 按依赖顺序初始化模块
    if (!initializeModule(MODULE_AT_COMMAND)) {
        setError("AT命令处理模块初始化失败");
        return false;
    }
    
    if (!initializeModule(MODULE_GSM_BASIC)) {
        setError("GSM基础模块初始化失败");
        return false;
    }
    
    if (!initializeModule(MODULE_HTTP_CLIENT)) {
        setError("HTTP客户端模块初始化失败");
        return false;
    }
    
    if (!initializeModule(MODULE_PHONE_CALLER)) {
        setError("电话拨打模块初始化失败");
        return false;
    }
    
    initialized = true;
    // 模块管理器初始化完成
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
        case MODULE_AT_COMMAND:
            result = initAtCommandModule();
            break;
        case MODULE_GSM_BASIC:
            result = initGsmBasicModule();
            break;
        case MODULE_HTTP_CLIENT:
            result = initHttpClientModule();
            break;
        case MODULE_PHONE_CALLER:
            result = initPhoneCallerModule();
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
 * @brief 初始化AT命令处理模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initAtCommandModule() {
    // 初始化AT命令处理模块
    
    // 创建AT命令处理器实例
    if (!g_at_command_handler) {
        g_at_command_handler = new AtCommandHandler(simSerial);
        if (!g_at_command_handler) {
            setError("无法创建AT命令处理器实例");
            return false;
        }
    }
    
    // 初始化AT命令处理器
    if (!g_at_command_handler->initialize()) {
        setError("AT命令处理器初始化失败: " + g_at_command_handler->getLastError());
        return false;
    }
    
    return true;
}

/**
 * @brief 初始化GSM基础模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initGsmBasicModule() {
    // 初始化GSM基础模块
    
    GsmService& gsmService = GsmService::getInstance();
    
    if (!gsmService.initialize()) {
        setError("GSM服务初始化失败: " + gsmService.getLastError());
        return false;
    }
    
    // GSM服务初始化成功后，尝试同步网络时间
    LOG_INFO(LOG_MODULE_SYSTEM, "正在同步网络时间...");
    unsigned long networkTimestamp = gsmService.getUnixTimestamp();
    if (networkTimestamp > 0) {
        // 设置系统时间（ESP32使用settimeofday函数）
        struct timeval tv;
        tv.tv_sec = networkTimestamp;
        tv.tv_usec = 0;
        
        if (settimeofday(&tv, NULL) == 0) {
            LOG_INFO(LOG_MODULE_SYSTEM, "网络时间同步成功，时间戳: " + String(networkTimestamp));
            
            // 验证时间设置
            time_t now;
            time(&now);
            struct tm* timeinfo = localtime(&now);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
            LOG_INFO(LOG_MODULE_SYSTEM, "当前系统时间: " + String(timeStr));
        } else {
            LOG_WARN(LOG_MODULE_SYSTEM, "设置系统时间失败");
        }
    } else {
        LOG_WARN(LOG_MODULE_SYSTEM, "获取网络时间失败，将使用系统默认时间");
    }
    
    return true;
}

/**
 * @brief 初始化HTTP客户端模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initHttpClientModule() {
    // 初始化HTTP客户端模块
    
    // 检查依赖模块是否已初始化
    if (!g_at_command_handler) {
        setError("HTTP客户端模块依赖AT命令处理模块，但该模块未初始化");
        return false;
    }
    
    // 创建HTTP客户端实例
    if (!g_http_client) {
        g_http_client = new HttpClient(*g_at_command_handler, GsmService::getInstance());
        if (!g_http_client) {
            setError("无法创建HTTP客户端实例");
            return false;
        }
    }
    
    // 初始化HTTP客户端
    if (!g_http_client->initialize()) {
        setError("HTTP客户端初始化失败: " + g_http_client->getLastError());
        return false;
    }
    
    return true;
}



/**
 * @brief 初始化电话拨打模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initPhoneCallerModule() {
    // 初始化电话拨打模块
    
    // 创建电话拨打器实例
    if (!g_phone_caller) {
        g_phone_caller = new PhoneCaller();
        if (!g_phone_caller) {
            setError("无法创建电话拨打器实例");
            return false;
        }
    }
    
    return true;
}



/**
 * @brief 初始化串口监听模块
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ModuleManager::initUartMonitorModule() {
    // 初始化串口监听模块
    
    // 串口监听模块不需要特殊初始化
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
    // 模块功能测试已移除
    return true;
}

/**
 * @brief 启动后台任务
 * @return true 启动成功
 * @return false 启动失败
 */
bool ModuleManager::startBackgroundTasks() {
    // 启动后台任务
    
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
}

// 全局访问函数



/**
 * @brief 获取电话拨打器实例
 * @return PhoneCaller* 电话拨打器指针，如果未初始化则返回nullptr
 */
PhoneCaller* getPhoneCaller() {
    return g_phone_caller;
}



/**
 * @brief 获取AT命令处理器实例
 * @return AtCommandHandler* AT命令处理器指针，如果未初始化则返回nullptr
 */
AtCommandHandler* getAtCommandHandler() {
    return g_at_command_handler;
}

/**
 * @brief 获取HTTP客户端实例
 * @return HttpClient* HTTP客户端指针，如果未初始化则返回nullptr
 */
HttpClient* getHttpClient() {
    return g_http_client;
}