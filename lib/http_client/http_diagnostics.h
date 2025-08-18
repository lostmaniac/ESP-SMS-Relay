/**
 * @file http_diagnostics.h
 * @brief HTTP客户端诊断工具头文件
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 提供HTTP客户端的诊断功能，帮助识别推送失败的根本原因
 */

#ifndef HTTP_DIAGNOSTICS_H
#define HTTP_DIAGNOSTICS_H

#include <Arduino.h>
#include "at_command_handler.h"

/**
 * @enum HttpDiagnosticStatus
 * @brief HTTP诊断状态枚举
 */
enum HttpDiagnosticStatus {
    HTTP_DIAG_UNKNOWN = 0,    ///< 未知状态
    HTTP_DIAG_OK = 1,         ///< 正常
    HTTP_DIAG_WARNING = 2,    ///< 警告
    HTTP_DIAG_ERROR = 3,      ///< 错误
    HTTP_DIAG_TIMEOUT = 4     ///< 超时
};

/**
 * @struct HttpDiagnosticResult
 * @brief HTTP诊断结果结构体
 */
struct HttpDiagnosticResult {
    HttpDiagnosticStatus overallStatus;      ///< 整体状态
    HttpDiagnosticStatus atHandlerStatus;    ///< AT命令处理器状态
    HttpDiagnosticStatus gsmModuleStatus;    ///< GSM模块状态
    HttpDiagnosticStatus networkStatus;      ///< 网络连接状态
    HttpDiagnosticStatus pdpContextStatus;   ///< PDP上下文状态
    HttpDiagnosticStatus httpServiceStatus;  ///< HTTP服务状态
    HttpDiagnosticStatus httpFunctionStatus; ///< HTTP功能状态
    String errorMessage;                     ///< 错误详情信息
    
    /**
     * @brief 构造函数
     */
    HttpDiagnosticResult() {
        overallStatus = HTTP_DIAG_UNKNOWN;
        atHandlerStatus = HTTP_DIAG_UNKNOWN;
        gsmModuleStatus = HTTP_DIAG_UNKNOWN;
        networkStatus = HTTP_DIAG_UNKNOWN;
        pdpContextStatus = HTTP_DIAG_UNKNOWN;
        httpServiceStatus = HTTP_DIAG_UNKNOWN;
        httpFunctionStatus = HTTP_DIAG_UNKNOWN;
    }
};

/**
 * @class HttpDiagnostics
 * @brief HTTP诊断工具类
 * 
 * 提供全面的HTTP客户端诊断功能，帮助识别和解决推送失败问题
 */
class HttpDiagnostics {
public:
    /**
     * @brief 获取单例实例
     * @return HttpDiagnostics& 单例引用
     */
    static HttpDiagnostics& getInstance();
    
    /**
     * @brief 构造函数
     */
    HttpDiagnostics();
    
    /**
     * @brief 析构函数
     */
    ~HttpDiagnostics();
    
    /**
     * @brief 执行完整的HTTP诊断
     * @return HttpDiagnosticResult 诊断结果
     */
    HttpDiagnosticResult runFullDiagnostic();
    
    /**
     * @brief 生成诊断报告
     * @param result 诊断结果
     * @return String 诊断报告
     */
    String generateReport(const HttpDiagnosticResult& result);
    
    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试模式
     */
    void setDebugMode(bool enabled) { debugMode = enabled; }
    
private:
    bool debugMode;  ///< 调试模式标志
    
    /**
     * @brief 检查AT命令处理器状态
     * @return HttpDiagnosticStatus 诊断状态
     */
    HttpDiagnosticStatus checkAtCommandHandler();
    
    /**
     * @brief 检查GSM模块状态
     * @return HttpDiagnosticStatus 诊断状态
     */
    HttpDiagnosticStatus checkGsmModule();
    
    /**
     * @brief 检查网络连接状态
     * @return HttpDiagnosticStatus 诊断状态
     */
    HttpDiagnosticStatus checkNetworkConnection();
    
    /**
     * @brief 检查PDP上下文状态
     * @return HttpDiagnosticStatus 诊断状态
     */
    HttpDiagnosticStatus checkPdpContext();
    
    /**
     * @brief 检查HTTP服务状态
     * @return HttpDiagnosticStatus 诊断状态
     */
    HttpDiagnosticStatus checkHttpService();
    
    /**
     * @brief 测试HTTP功能
     * @return HttpDiagnosticStatus 诊断状态
     */
    HttpDiagnosticStatus testHttpFunction();
    
    /**
     * @brief 确定整体诊断状态
     * @param result 诊断结果
     * @return HttpDiagnosticStatus 整体状态
     */
    HttpDiagnosticStatus determineOverallStatus(const HttpDiagnosticResult& result);
    
    /**
     * @brief 获取状态字符串
     * @param status 诊断状态
     * @return String 状态描述
     */
    String getStatusString(HttpDiagnosticStatus status);
    
    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debugPrint(const String& message);
};

#endif // HTTP_DIAGNOSTICS_H