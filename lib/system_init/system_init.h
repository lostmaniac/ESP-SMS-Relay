/**
 * @file system_init.h
 * @brief 系统初始化模块 - 简化的系统启动管理
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 系统的统一初始化入口
 * 2. 调用模块管理器进行模块初始化
 * 3. 启动后台任务
 * 4. 系统状态监控
 */

#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <Arduino.h>

/**
 * @enum SystemStatus
 * @brief 系统状态枚举
 */
enum SystemStatus {
    SYSTEM_NOT_INITIALIZED,     ///< 系统未初始化
    SYSTEM_INITIALIZING,        ///< 系统正在初始化
    SYSTEM_READY,              ///< 系统就绪
    SYSTEM_ERROR,              ///< 系统错误
    SYSTEM_RUNNING             ///< 系统运行中
};

/**
 * @class SystemInit
 * @brief 系统初始化类
 * 
 * 负责系统的统一初始化和状态管理
 */
class SystemInit {
public:
    /**
     * @brief 构造函数
     */
    SystemInit();
    
    /**
     * @brief 析构函数
     */
    ~SystemInit();
    
    /**
     * @brief 初始化系统
     * @param runTests 是否运行测试
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize(bool runTests = true);
    
    /**
     * @brief 启动系统
     * @return true 启动成功
     * @return false 启动失败
     */
    bool start();
    
    /**
     * @brief 获取系统状态
     * @return SystemStatus 系统状态
     */
    SystemStatus getSystemStatus();
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 重启系统
     * @return true 重启成功
     * @return false 重启失败
     */
    bool restart();
    
    /**
     * @brief 获取单例实例
     * @return SystemInit& 单例引用
     */
    static SystemInit& getInstance();

private:
    SystemStatus systemStatus;      ///< 系统状态
    String lastError;              ///< 最后的错误信息
    bool initialized;              ///< 是否已初始化
    
    /**
     * @brief 设置系统状态
     * @param status 系统状态
     */
    void setSystemStatus(SystemStatus status);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
};

/**
 * @brief 系统初始化任务函数
 * @param pvParameters 任务参数
 */
void system_init_task(void *pvParameters);

#endif // SYSTEM_INIT_H