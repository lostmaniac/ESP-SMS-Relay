/**
 * @file log_manager.h
 * @brief 日志管理模块 - 统一管理系统日志输出
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 统一的日志输出接口
 * 2. 日志级别管理
 * 3. 日志格式化
 * 4. 日志过滤和控制
 */

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>

/**
 * @enum LogLevel
 * @brief 日志级别枚举
 */
enum LogLevel {
    LOG_LEVEL_NONE = 0,     ///< 无日志输出
    LOG_LEVEL_ERROR = 1,    ///< 错误级别
    LOG_LEVEL_WARN = 2,     ///< 警告级别
    LOG_LEVEL_INFO = 3,     ///< 信息级别
    LOG_LEVEL_DEBUG = 4,    ///< 调试级别
    LOG_LEVEL_VERBOSE = 5   ///< 详细级别
};

/**
 * @enum LogModule
 * @brief 日志模块枚举
 */
enum LogModule {
    LOG_MODULE_SYSTEM,      ///< 系统模块
    LOG_MODULE_GSM,         ///< GSM模块
    LOG_MODULE_SMS,         ///< 短信模块
    LOG_MODULE_PHONE,       ///< 电话模块
    LOG_MODULE_UART,        ///< 串口模块
    LOG_MODULE_CONFIG,      ///< 配置模块
    LOG_MODULE_TEST,        ///< 测试模块
    LOG_MODULE_UNKNOWN      ///< 未知模块
};

/**
 * @class LogManager
 * @brief 日志管理器类
 * 
 * 负责系统日志的统一管理和输出
 */
class LogManager {
public:
    /**
     * @brief 构造函数
     */
    LogManager();
    
    /**
     * @brief 析构函数
     */
    ~LogManager();
    
    /**
     * @brief 初始化日志管理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 设置日志级别
     * @param level 日志级别
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief 获取日志级别
     * @return LogLevel 当前日志级别
     */
    LogLevel getLogLevel();
    
    /**
     * @brief 启用/禁用时间戳
     * @param enable 是否启用时间戳
     */
    void enableTimestamp(bool enable);
    
    /**
     * @brief 启用/禁用模块标识
     * @param enable 是否启用模块标识
     */
    void enableModuleTag(bool enable);
    
    /**
     * @brief 记录错误日志
     * @param module 模块标识
     * @param message 日志消息
     */
    void logError(LogModule module, const String& message);
    
    /**
     * @brief 记录警告日志
     * @param module 模块标识
     * @param message 日志消息
     */
    void logWarn(LogModule module, const String& message);
    
    /**
     * @brief 记录信息日志
     * @param module 模块标识
     * @param message 日志消息
     */
    void logInfo(LogModule module, const String& message);
    
    /**
     * @brief 记录调试日志
     * @param module 模块标识
     * @param message 日志消息
     */
    void logDebug(LogModule module, const String& message);
    
    /**
     * @brief 记录详细日志
     * @param module 模块标识
     * @param message 日志消息
     */
    void logVerbose(LogModule module, const String& message);
    
    /**
     * @brief 记录格式化日志
     * @param level 日志级别
     * @param module 模块标识
     * @param format 格式字符串
     * @param ... 格式参数
     */
    void logf(LogLevel level, LogModule module, const char* format, ...);
    
    /**
     * @brief 获取单例实例
     * @return LogManager& 单例引用
     */
    static LogManager& getInstance();
    
    /**
     * @brief 打印系统分隔线
     * @param title 标题（可选）
     */
    void printSeparator(const String& title = "");
    
    /**
     * @brief 打印系统启动信息
     */
    void printStartupInfo();

private:
    LogLevel currentLogLevel;   ///< 当前日志级别
    bool timestampEnabled;      ///< 是否启用时间戳
    bool moduleTagEnabled;      ///< 是否启用模块标识
    bool initialized;           ///< 是否已初始化
    
    /**
     * @brief 输出日志
     * @param level 日志级别
     * @param module 模块标识
     * @param message 日志消息
     */
    void output(LogLevel level, LogModule module, const String& message);
    
    /**
     * @brief 获取日志级别名称
     * @param level 日志级别
     * @return String 级别名称
     */
    String getLevelName(LogLevel level);
    
    /**
     * @brief 获取模块名称
     * @param module 模块标识
     * @return String 模块名称
     */
    String getModuleName(LogModule module);
    
    /**
     * @brief 获取当前时间戳
     * @return String 时间戳字符串
     */
    String getTimestamp();
    
    /**
     * @brief 检查日志级别是否应该输出
     * @param level 日志级别
     * @return true 应该输出
     * @return false 不应该输出
     */
    bool shouldLog(LogLevel level);
};

// 便捷宏定义
#define LOG_ERROR(module, message) LogManager::getInstance().logError(module, message)
#define LOG_WARN(module, message) LogManager::getInstance().logWarn(module, message)
#define LOG_INFO(module, message) LogManager::getInstance().logInfo(module, message)
#define LOG_DEBUG(module, message) LogManager::getInstance().logDebug(module, message)
#define LOG_VERBOSE(module, message) LogManager::getInstance().logVerbose(module, message)

#define LOGF(level, module, format, ...) LogManager::getInstance().logf(level, module, format, ##__VA_ARGS__)

#endif // LOG_MANAGER_H