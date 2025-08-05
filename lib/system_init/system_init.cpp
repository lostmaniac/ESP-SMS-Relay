/**
 * @file system_init.cpp
 * @brief 系统初始化模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "system_init.h"
#include "module_manager.h"
#include "../uart_monitor/uart_monitor.h"
#include "config_manager.h"
#include "log_manager.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
SystemInit::SystemInit() : 
    systemStatus(SYSTEM_NOT_INITIALIZED),
    lastError(""),
    initialized(false) {
}

/**
 * @brief 析构函数
 */
SystemInit::~SystemInit() {
    // 清理资源
}

/**
 * @brief 获取单例实例
 * @return SystemInit& 单例引用
 */
SystemInit& SystemInit::getInstance() {
    static SystemInit instance;
    return instance;
}

/**
 * @brief 初始化系统
 * @param runTests 是否运行测试
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool SystemInit::initialize(bool runTests) {
    if (initialized) {
        return true;
    }
    
    // 首先初始化日志管理器
    LogManager& logManager = LogManager::getInstance();
    if (!logManager.initialize()) {
        Serial.println("日志管理器初始化失败");
        return false;
    }
    
    // 打印系统启动信息
    logManager.printStartupInfo();
    
    setSystemStatus(SYSTEM_INITIALIZING);
    LOG_INFO(LOG_MODULE_SYSTEM, "开始系统初始化");
    
    // 获取模块管理器实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    
    // 初始化所有模块
    LOG_INFO(LOG_MODULE_SYSTEM, "正在初始化系统模块...");
    if (!moduleManager.initializeAllModules()) {
        setError("模块初始化失败: " + moduleManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    // 检查所有模块是否就绪
    if (!moduleManager.areAllModulesReady()) {
        setError("部分模块未就绪");
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    LOG_INFO(LOG_MODULE_SYSTEM, "所有模块初始化完成");
    
    // 运行测试（可选）
    if (runTests) {
        LOG_INFO(LOG_MODULE_SYSTEM, "正在运行系统测试...");
        if (!moduleManager.runModuleTests()) {
            LOG_WARN(LOG_MODULE_SYSTEM, "部分测试未通过，但系统将继续运行");
        } else {
            LOG_INFO(LOG_MODULE_SYSTEM, "所有测试通过");
        }
    }
    
    setSystemStatus(SYSTEM_READY);
    initialized = true;
    
    logManager.printSeparator("系统初始化完成");
    return true;
}

/**
 * @brief 启动系统
 * @return true 启动成功
 * @return false 启动失败
 */
bool SystemInit::start() {
    if (!initialized) {
        setError("系统未初始化");
        return false;
    }
    
    if (systemStatus != SYSTEM_READY) {
        setError("系统状态不正确，无法启动");
        return false;
    }
    
    LOG_INFO(LOG_MODULE_SYSTEM, "正在启动系统服务...");
    
    // 获取模块管理器实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    
    // 启动后台任务
    if (!moduleManager.startBackgroundTasks()) {
        setError("启动后台任务失败: " + moduleManager.getLastError());
        setSystemStatus(SYSTEM_ERROR);
        return false;
    }
    
    setSystemStatus(SYSTEM_RUNNING);
    LogManager::getInstance().printSeparator("系统启动完成，开始运行");
    
    return true;
}

/**
 * @brief 获取系统状态
 * @return SystemStatus 系统状态
 */
SystemStatus SystemInit::getSystemStatus() {
    return systemStatus;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String SystemInit::getLastError() {
    return lastError;
}

/**
 * @brief 重启系统
 * @return true 重启成功
 * @return false 重启失败
 */
bool SystemInit::restart() {
    Serial.println("正在重启系统...");
    
    // 重置状态
    initialized = false;
    setSystemStatus(SYSTEM_NOT_INITIALIZED);
    
    // 等待一段时间
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // 重新初始化
    if (initialize()) {
        return start();
    }
    
    return false;
}

/**
 * @brief 设置系统状态
 * @param status 系统状态
 */
void SystemInit::setSystemStatus(SystemStatus status) {
    systemStatus = status;
    
    // 打印状态变化
    String statusName;
    switch (status) {
        case SYSTEM_NOT_INITIALIZED: statusName = "未初始化"; break;
        case SYSTEM_INITIALIZING: statusName = "初始化中"; break;
        case SYSTEM_READY: statusName = "就绪"; break;
        case SYSTEM_ERROR: statusName = "错误"; break;
        case SYSTEM_RUNNING: statusName = "运行中"; break;
        default: statusName = "未知"; break;
    }
    
    Serial.printf("系统状态: %s\n", statusName.c_str());
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void SystemInit::setError(const String& error) {
    lastError = error;
    LOG_ERROR(LOG_MODULE_SYSTEM, "系统错误: " + error);
}

/**
 * @brief 系统初始化任务函数
 * @param pvParameters 任务参数
 */
void system_init_task(void *pvParameters) {
    // 等待系统稳定
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    SystemInit& systemInit = SystemInit::getInstance();
    
    // 获取配置管理器以确定是否运行测试
    ConfigManager& configManager = ConfigManager::getInstance();
    SystemConfig sysConfig = configManager.getSystemConfig();
    
    // 初始化系统（根据配置决定是否运行测试）
    if (systemInit.initialize(sysConfig.runTestsOnStartup)) {
        // 启动系统
        if (systemInit.start()) {
            LOG_INFO(LOG_MODULE_SYSTEM, "系统启动成功，初始化任务完成");
        } else {
            LOG_ERROR(LOG_MODULE_SYSTEM, "系统启动失败: " + systemInit.getLastError());
        }
    } else {
        LOG_ERROR(LOG_MODULE_SYSTEM, "系统初始化失败: " + systemInit.getLastError());
    }
    
    // 删除此任务
    vTaskDelete(NULL);
}