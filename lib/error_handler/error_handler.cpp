/**
 * @file error_handler.cpp
 * @brief 统一错误处理系统实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 实现统一的错误处理机制
 */

#include "error_handler.h"
#include "../log_manager/log_manager.h"
#include "../../include/constants.h"
#include <Arduino.h>

/**
 * @brief 获取单例实例
 * @return ErrorHandler& 单例引用
 */
ErrorHandler& ErrorHandler::getInstance() {
    static ErrorHandler instance;
    return instance;
}

/**
 * @brief 构造函数
 */
ErrorHandler::ErrorHandler() 
    : initialized(false), maxErrorHistory(MAX_ERROR_HISTORY), errorLoggingEnabled(true),
      hasCriticalError(false), totalErrors(0), criticalErrors(0), warnings(0) {
    // 初始化最后错误信息
    lastError.code = ERR_SUCCESS;
    lastError.severity = SEVERITY_INFO;
    lastError.timestamp = 0;
}

/**
 * @brief 析构函数
 */
ErrorHandler::~ErrorHandler() {
    clearErrorHistory();
}

/**
 * @brief 初始化错误处理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ErrorHandler::initialize() {
    if (initialized) {
        return true;
    }
    
    // 预分配错误历史空间
    errorHistory.reserve(maxErrorHistory);
    
    // 清除所有状态
    clearErrorHistory();
    resetErrorState();
    
    initialized = true;
    
    // 记录初始化成功
    reportError(ERR_SUCCESS, "错误处理器初始化成功", "ErrorHandler", "initialize");
    
    return true;
}

/**
 * @brief 报告错误
 * @param code 错误代码
 * @param message 错误消息
 * @param module 模块名称
 * @param function 函数名称
 * @param details 详细信息
 */
void ErrorHandler::reportError(ErrorCode code, const String& message, 
                              const String& module, const String& function,
                              const String& details) {
    reportErrorInternal(SEVERITY_ERROR, code, message, module, function, details);
}

/**
 * @brief 报告警告
 * @param code 错误代码
 * @param message 警告消息
 * @param module 模块名称
 * @param function 函数名称
 */
void ErrorHandler::reportWarning(ErrorCode code, const String& message,
                                const String& module, const String& function) {
    reportErrorInternal(SEVERITY_WARNING, code, message, module, function, "");
}

/**
 * @brief 报告严重错误
 * @param code 错误代码
 * @param message 错误消息
 * @param module 模块名称
 * @param function 函数名称
 * @param details 详细信息
 */
void ErrorHandler::reportCriticalError(ErrorCode code, const String& message,
                                     const String& module, const String& function,
                                     const String& details) {
    reportErrorInternal(SEVERITY_CRITICAL, code, message, module, function, details);
}

/**
 * @brief 内部报告错误
 * @param severity 错误严重程度
 * @param code 错误代码
 * @param message 错误消息
 * @param module 模块名称
 * @param function 函数名称
 * @param details 详细信息
 */
void ErrorHandler::reportErrorInternal(ErrorSeverity severity, ErrorCode code, 
                                     const String& message, const String& module,
                                     const String& function, const String& details) {
    // 创建错误信息
    ErrorInfo errorInfo;
    errorInfo.code = code;
    errorInfo.severity = severity;
    errorInfo.message = message;
    errorInfo.module = module;
    errorInfo.function = function;
    errorInfo.timestamp = millis();
    errorInfo.details = details;
    
    // 更新统计信息
    totalErrors++;
    if (severity == SEVERITY_CRITICAL) {
        criticalErrors++;
        hasCriticalError = true;
    } else if (severity == SEVERITY_WARNING) {
        warnings++;
    }
    
    // 保存为最后错误
    lastError = errorInfo;
    
    // 添加到历史记录
    errorHistory.push_back(errorInfo);
    
    // 如果超过最大历史数量，删除最旧的记录
    if (errorHistory.size() > static_cast<size_t>(maxErrorHistory)) {
        errorHistory.erase(errorHistory.begin());
    }
    
    // 记录到日志
    if (errorLoggingEnabled) {
        logError(errorInfo);
    }
    
    // 通知回调
    notifyCallbacks(errorInfo);
}

/**
 * @brief 获取错误代码对应的消息
 * @param code 错误代码
 * @return String 错误消息
 */
String ErrorHandler::getErrorMessage(ErrorCode code) {
    switch (code) {
        case ERR_SUCCESS:
            return "操作成功";
            
        // 系统级错误
        case ERR_INIT_FAILED:
            return "初始化失败";
        case ERR_CONFIG_INVALID:
            return "配置无效";
        case ERR_MEMORY_INSUFFICIENT:
            return "内存不足";
        case ERR_TIMEOUT:
            return "操作超时";
        case ERR_INVALID_PARAMETER:
            return "参数无效";
        case ERR_PERMISSION_DENIED:
            return "权限被拒绝";
            
        // 网络错误
        case ERR_NETWORK_FAILED:
            return "网络连接失败";
        case ERR_WIFI_CONNECTION_FAILED:
            return "WiFi连接失败";
        case ERR_HTTP_REQUEST_FAILED:
            return "HTTP请求失败";
        case ERR_DNS_RESOLUTION_FAILED:
            return "DNS解析失败";
        case ERR_NETWORK_TIMEOUT:
            return "网络超时";
        case ERR_SSL_HANDSHAKE_FAILED:
            return "SSL握手失败";
            
        // 数据库错误
        case ERR_DATABASE_ERROR:
            return "数据库错误";
        case ERR_DB_CONNECTION_FAILED:
            return "数据库连接失败";
        case ERR_DB_QUERY_FAILED:
            return "数据库查询失败";
        case ERR_DB_TRANSACTION_FAILED:
            return "数据库事务失败";
        case ERR_DB_SCHEMA_ERROR:
            return "数据库架构错误";
        case ERR_DB_CORRUPTION:
            return "数据库损坏";
            
        // SMS/GSM错误
        case ERR_SMS_FAILED:
            return "短信操作失败";
        case ERR_GSM_INIT_FAILED:
            return "GSM模块初始化失败";
        case ERR_SIM_CARD_ERROR:
            return "SIM卡错误";
        case ERR_NETWORK_REGISTRATION_FAILED:
            return "网络注册失败";
        case ERR_SMS_SEND_FAILED:
            return "短信发送失败";
        case ERR_SMS_RECEIVE_FAILED:
            return "短信接收失败";
        case ERR_AT_COMMAND_FAILED:
            return "AT命令执行失败";
            
        // 推送错误
        case ERR_PUSH_FAILED:
            return "推送失败";
        case ERR_PUSH_CONFIG_INVALID:
            return "推送配置无效";
        case ERR_PUSH_AUTHENTICATION_FAILED:
            return "推送认证失败";
        case ERR_PUSH_RATE_LIMITED:
            return "推送频率受限";
        case ERR_PUSH_MESSAGE_TOO_LONG:
            return "推送消息过长";
            
        // 文件系统错误
        case ERR_FILESYSTEM_ERROR:
            return "文件系统错误";
        case ERR_FILE_NOT_FOUND:
            return "文件未找到";
        case ERR_FILE_READ_FAILED:
            return "文件读取失败";
        case ERR_FILE_WRITE_FAILED:
            return "文件写入失败";
        case ERR_DISK_FULL:
            return "磁盘空间不足";
        case ERR_FILE_PERMISSION_DENIED:
            return "文件权限被拒绝";
            
        // 硬件错误
        case ERR_HARDWARE_ERROR:
            return "硬件错误";
        case ERR_SENSOR_READ_FAILED:
            return "传感器读取失败";
        case ERR_GPIO_CONFIG_FAILED:
            return "GPIO配置失败";
        case ERR_UART_COMMUNICATION_FAILED:
            return "UART通信失败";
        case ERR_POWER_MANAGEMENT_FAILED:
            return "电源管理失败";
            
        default:
            return "未知错误 (" + String(static_cast<int>(code)) + ")";
    }
}

/**
 * @brief 获取最后一个错误
 * @return ErrorInfo 最后一个错误信息
 */
ErrorInfo ErrorHandler::getLastError() {
    return lastError;
}

/**
 * @brief 获取错误历史
 * @param maxCount 最大返回数量
 * @return std::vector<ErrorInfo> 错误历史列表
 */
std::vector<ErrorInfo> ErrorHandler::getErrorHistory(int maxCount) {
    if (maxCount <= 0 || maxCount >= static_cast<int>(errorHistory.size())) {
        return errorHistory;
    }
    
    // 返回最新的maxCount个错误
    std::vector<ErrorInfo> result;
    int startIndex = errorHistory.size() - maxCount;
    for (int i = startIndex; i < static_cast<int>(errorHistory.size()); i++) {
        result.push_back(errorHistory[i]);
    }
    
    return result;
}

/**
 * @brief 清除错误历史
 */
void ErrorHandler::clearErrorHistory() {
    errorHistory.clear();
}

/**
 * @brief 注册错误回调
 * @param callback 回调函数
 */
void ErrorHandler::registerErrorCallback(ErrorCallback callback) {
    if (callback) {
        callbacks.push_back(callback);
    }
}

/**
 * @brief 设置最大错误历史数量
 * @param maxCount 最大数量
 */
void ErrorHandler::setMaxErrorHistory(int maxCount) {
    if (maxCount > 0) {
        this->maxErrorHistory = maxCount;
        
        // 如果当前历史超过新的限制，删除旧记录
        while (errorHistory.size() > static_cast<size_t>(maxCount)) {
            errorHistory.erase(errorHistory.begin());
        }
    }
}

/**
 * @brief 启用/禁用错误日志
 * @param enable 是否启用
 */
void ErrorHandler::enableErrorLogging(bool enable) {
    errorLoggingEnabled = enable;
}

/**
 * @brief 获取错误统计信息
 * @return String 统计信息
 */
String ErrorHandler::getErrorStatistics() {
    String stats = "错误统计:\n";
    stats += "总错误数: " + String(totalErrors) + "\n";
    stats += "严重错误数: " + String(criticalErrors) + "\n";
    stats += "警告数: " + String(warnings) + "\n";
    stats += "历史记录数: " + String(errorHistory.size()) + "\n";
    stats += "是否有严重错误: " + String(hasCriticalError ? "是" : "否");
    
    return stats;
}

/**
 * @brief 检查是否有严重错误
 * @return true 有严重错误
 * @return false 无严重错误
 */
bool ErrorHandler::hasCriticalErrors() {
    return hasCriticalError;
}

/**
 * @brief 重置错误状态
 */
void ErrorHandler::resetErrorState() {
    hasCriticalError = false;
    totalErrors = 0;
    criticalErrors = 0;
    warnings = 0;
    
    lastError.code = ERR_SUCCESS;
    lastError.severity = SEVERITY_INFO;
    lastError.timestamp = millis();
    lastError.message = "状态已重置";
    lastError.module = "ErrorHandler";
    lastError.function = "resetErrorState";
    lastError.details = "";
}

/**
 * @brief 通知错误回调
 * @param errorInfo 错误信息
 */
void ErrorHandler::notifyCallbacks(const ErrorInfo& errorInfo) {
    for (auto& callback : callbacks) {
        try {
            callback(errorInfo);
        } catch (...) {
            // 忽略回调中的异常，避免递归错误
        }
    }
}

/**
 * @brief 记录错误到日志
 * @param errorInfo 错误信息
 */
void ErrorHandler::logError(const ErrorInfo& errorInfo) {
    LogManager& logger = LogManager::getInstance();
    
    String logMessage = "[" + getSeverityString(errorInfo.severity) + "] ";
    logMessage += "错误代码: " + String(static_cast<int>(errorInfo.code)) + ", ";
    logMessage += "消息: " + errorInfo.message;
    
    if (!errorInfo.module.isEmpty()) {
        logMessage += ", 模块: " + errorInfo.module;
    }
    
    if (!errorInfo.function.isEmpty()) {
        logMessage += ", 函数: " + errorInfo.function;
    }
    
    if (!errorInfo.details.isEmpty()) {
        logMessage += ", 详情: " + errorInfo.details;
    }
    
    // 根据严重程度选择日志级别
    switch (errorInfo.severity) {
        case SEVERITY_INFO:
            logger.logInfo(LOG_MODULE_SYSTEM, logMessage);
            break;
        case SEVERITY_WARNING:
            logger.logWarn(LOG_MODULE_SYSTEM, logMessage);
            break;
        case SEVERITY_ERROR:
        case SEVERITY_CRITICAL:
            logger.logError(LOG_MODULE_SYSTEM, logMessage);
            break;
    }
}

/**
 * @brief 获取严重程度字符串
 * @param severity 严重程度
 * @return String 严重程度字符串
 */
String ErrorHandler::getSeverityString(ErrorSeverity severity) {
    switch (severity) {
        case SEVERITY_INFO:
            return "信息";
        case SEVERITY_WARNING:
            return "警告";
        case SEVERITY_ERROR:
            return "错误";
        case SEVERITY_CRITICAL:
            return "严重";
        default:
            return "未知";
    }
}