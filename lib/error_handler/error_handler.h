/**
 * @file error_handler.h
 * @brief 统一错误处理系统
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 提供统一的错误处理机制，包括错误代码定义、错误信息管理和异常处理
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "../../include/constants.h"

/**
 * @enum ErrorCode
 * @brief 系统错误代码枚举
 */
enum ErrorCode {
    // 成功
    ERR_SUCCESS = ERROR_CODE_SUCCESS,
    
    // 系统级错误 (1000-1099)
    ERR_INIT_FAILED = ERROR_CODE_INIT_FAILED,
    ERR_CONFIG_INVALID = ERROR_CODE_CONFIG_INVALID,
    ERR_MEMORY_INSUFFICIENT = ERROR_CODE_MEMORY_INSUFFICIENT,
    ERR_TIMEOUT = ERROR_CODE_TIMEOUT,
    ERR_INVALID_PARAMETER = ERROR_CODE_INVALID_PARAMETER,
    ERR_PERMISSION_DENIED = ERROR_CODE_PERMISSION_DENIED,
    
    // 网络错误 (1100-1199)
    ERR_NETWORK_FAILED = ERROR_CODE_NETWORK_FAILED,
    ERR_WIFI_CONNECTION_FAILED = 1101,
    ERR_HTTP_REQUEST_FAILED = 1102,
    ERR_DNS_RESOLUTION_FAILED = 1103,
    ERR_NETWORK_TIMEOUT = 1104,
    ERR_SSL_HANDSHAKE_FAILED = 1105,
    
    // 数据库错误 (1200-1299)
    ERR_DATABASE_ERROR = ERROR_CODE_DATABASE_ERROR,
    ERR_DB_CONNECTION_FAILED = 1201,
    ERR_DB_QUERY_FAILED = 1202,
    ERR_DB_TRANSACTION_FAILED = 1203,
    ERR_DB_SCHEMA_ERROR = 1204,
    ERR_DB_CORRUPTION = 1205,
    
    // SMS/GSM错误 (1300-1399)
    ERR_SMS_FAILED = ERROR_CODE_SMS_FAILED,
    ERR_GSM_INIT_FAILED = 1301,
    ERR_SIM_CARD_ERROR = 1302,
    ERR_NETWORK_REGISTRATION_FAILED = 1303,
    ERR_SMS_SEND_FAILED = 1304,
    ERR_SMS_RECEIVE_FAILED = 1305,
    ERR_AT_COMMAND_FAILED = 1306,
    
    // 推送错误 (1400-1499)
    ERR_PUSH_FAILED = ERROR_CODE_PUSH_FAILED,
    ERR_PUSH_CONFIG_INVALID = 1401,
    ERR_PUSH_AUTHENTICATION_FAILED = 1402,
    ERR_PUSH_RATE_LIMITED = 1403,
    ERR_PUSH_MESSAGE_TOO_LONG = 1404,
    
    // 文件系统错误 (1500-1599)
    ERR_FILESYSTEM_ERROR = 1500,
    ERR_FILE_NOT_FOUND = 1501,
    ERR_FILE_READ_FAILED = 1502,
    ERR_FILE_WRITE_FAILED = 1503,
    ERR_DISK_FULL = 1504,
    ERR_FILE_PERMISSION_DENIED = 1505,
    
    // 硬件错误 (1600-1699)
    ERR_HARDWARE_ERROR = 1600,
    ERR_SENSOR_READ_FAILED = 1601,
    ERR_GPIO_CONFIG_FAILED = 1602,
    ERR_UART_COMMUNICATION_FAILED = 1603,
    ERR_POWER_MANAGEMENT_FAILED = 1604
};

/**
 * @enum ErrorSeverity
 * @brief 错误严重程度
 */
enum ErrorSeverity {
    SEVERITY_INFO,      ///< 信息级别
    SEVERITY_WARNING,   ///< 警告级别
    SEVERITY_ERROR,     ///< 错误级别
    SEVERITY_CRITICAL   ///< 严重错误级别
};

/**
 * @struct ErrorInfo
 * @brief 错误信息结构体
 */
struct ErrorInfo {
    ErrorCode code;           ///< 错误代码
    ErrorSeverity severity;   ///< 错误严重程度
    String message;           ///< 错误消息
    String module;            ///< 发生错误的模块
    String function;          ///< 发生错误的函数
    unsigned long timestamp;  ///< 错误发生时间戳
    String details;           ///< 详细错误信息
};

/**
 * @typedef ErrorCallback
 * @brief 错误回调函数类型
 */
typedef std::function<void(const ErrorInfo&)> ErrorCallback;

/**
 * @class ErrorHandler
 * @brief 统一错误处理器
 * 
 * 提供统一的错误处理机制，包括错误记录、回调通知和错误恢复
 */
class ErrorHandler {
public:
    /**
     * @brief 获取单例实例
     * @return ErrorHandler& 单例引用
     */
    static ErrorHandler& getInstance();
    
    /**
     * @brief 初始化错误处理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 报告错误
     * @param code 错误代码
     * @param message 错误消息
     * @param module 模块名称
     * @param function 函数名称
     * @param details 详细信息
     */
    void reportError(ErrorCode code, const String& message, 
                    const String& module = "", const String& function = "",
                    const String& details = "");
    
    /**
     * @brief 报告警告
     * @param code 错误代码
     * @param message 警告消息
     * @param module 模块名称
     * @param function 函数名称
     */
    void reportWarning(ErrorCode code, const String& message,
                      const String& module = "", const String& function = "");
    
    /**
     * @brief 报告严重错误
     * @param code 错误代码
     * @param message 错误消息
     * @param module 模块名称
     * @param function 函数名称
     * @param details 详细信息
     */
    void reportCriticalError(ErrorCode code, const String& message,
                           const String& module = "", const String& function = "",
                           const String& details = "");
    
    /**
     * @brief 获取错误代码对应的消息
     * @param code 错误代码
     * @return String 错误消息
     */
    String getErrorMessage(ErrorCode code);
    
    /**
     * @brief 获取最后一个错误
     * @return ErrorInfo 最后一个错误信息
     */
    ErrorInfo getLastError();
    
    /**
     * @brief 获取错误历史
     * @param maxCount 最大返回数量
     * @return std::vector<ErrorInfo> 错误历史列表
     */
    std::vector<ErrorInfo> getErrorHistory(int maxCount = 10);
    
    /**
     * @brief 清除错误历史
     */
    void clearErrorHistory();
    
    /**
     * @brief 注册错误回调
     * @param callback 回调函数
     */
    void registerErrorCallback(ErrorCallback callback);
    
    /**
     * @brief 设置最大错误历史数量
     * @param maxCount 最大数量
     */
    void setMaxErrorHistory(int maxCount);
    
    /**
     * @brief 启用/禁用错误日志
     * @param enable 是否启用
     */
    void enableErrorLogging(bool enable);
    
    /**
     * @brief 获取错误统计信息
     * @return String 统计信息
     */
    String getErrorStatistics();
    
    /**
     * @brief 检查是否有严重错误
     * @return true 有严重错误
     * @return false 无严重错误
     */
    bool hasCriticalErrors();
    
    /**
     * @brief 重置错误状态
     */
    void resetErrorState();
    
private:
    /**
     * @brief 构造函数
     */
    ErrorHandler();
    
    /**
     * @brief 析构函数
     */
    ~ErrorHandler();
    
    /**
     * @brief 禁止拷贝构造
     */
    ErrorHandler(const ErrorHandler&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    
    /**
     * @brief 内部报告错误
     * @param severity 错误严重程度
     * @param code 错误代码
     * @param message 错误消息
     * @param module 模块名称
     * @param function 函数名称
     * @param details 详细信息
     */
    void reportErrorInternal(ErrorSeverity severity, ErrorCode code, 
                           const String& message, const String& module,
                           const String& function, const String& details);
    
    /**
     * @brief 通知错误回调
     * @param errorInfo 错误信息
     */
    void notifyCallbacks(const ErrorInfo& errorInfo);
    
    /**
     * @brief 记录错误到日志
     * @param errorInfo 错误信息
     */
    void logError(const ErrorInfo& errorInfo);
    
    /**
     * @brief 获取严重程度字符串
     * @param severity 严重程度
     * @return String 严重程度字符串
     */
    String getSeverityString(ErrorSeverity severity);
    
private:
    bool initialized;                           ///< 是否已初始化
    std::vector<ErrorInfo> errorHistory;       ///< 错误历史
    std::vector<ErrorCallback> callbacks;      ///< 错误回调列表
    int maxErrorHistory;                        ///< 最大错误历史数量
    bool errorLoggingEnabled;                   ///< 是否启用错误日志
    ErrorInfo lastError;                        ///< 最后一个错误
    bool hasCriticalError;                      ///< 是否有严重错误
    
    // 错误统计
    int totalErrors;                            ///< 总错误数
    int criticalErrors;                         ///< 严重错误数
    int warnings;                               ///< 警告数
};

// ==================== 便利宏定义 ====================

/**
 * @brief 报告错误的便利宏
 */
#define REPORT_ERROR(code, message) \
    ErrorHandler::getInstance().reportError(code, message, __FILE__, __FUNCTION__)

/**
 * @brief 报告带详细信息的错误的便利宏
 */
#define REPORT_ERROR_DETAILED(code, message, details) \
    ErrorHandler::getInstance().reportError(code, message, __FILE__, __FUNCTION__, details)

/**
 * @brief 报告警告的便利宏
 */
#define REPORT_WARNING(code, message) \
    ErrorHandler::getInstance().reportWarning(code, message, __FILE__, __FUNCTION__)

/**
 * @brief 报告严重错误的便利宏
 */
#define REPORT_CRITICAL_ERROR(code, message) \
    ErrorHandler::getInstance().reportCriticalError(code, message, __FILE__, __FUNCTION__)

/**
 * @brief 检查条件并报告错误的便利宏
 */
#define CHECK_AND_REPORT_ERROR(condition, code, message) \
    do { \
        if (!(condition)) { \
            REPORT_ERROR(code, message); \
            return false; \
        } \
    } while(0)

/**
 * @brief 检查指针并报告错误的便利宏
 */
#define CHECK_POINTER_AND_REPORT(ptr, code, message) \
    CHECK_AND_REPORT_ERROR((ptr) != nullptr, code, message)

#endif // ERROR_HANDLER_H