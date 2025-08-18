/**
 * @file http_diagnostics.cpp
 * @brief HTTP客户端诊断工具实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 提供HTTP客户端的诊断功能，帮助识别推送失败的根本原因
 */

#include "http_diagnostics.h"
#include "http_client.h"
#include "gsm_service.h"
#include "at_command_handler.h"
#include "../../include/constants.h"
#include <Arduino.h>

/**
 * @brief 获取单例实例
 * @return HttpDiagnostics& 单例引用
 */
HttpDiagnostics& HttpDiagnostics::getInstance() {
    static HttpDiagnostics instance;
    return instance;
}

/**
 * @brief 构造函数
 */
HttpDiagnostics::HttpDiagnostics() : debugMode(true) {
}

/**
 * @brief 析构函数
 */
HttpDiagnostics::~HttpDiagnostics() {
}

/**
 * @brief 执行完整的HTTP诊断
 * @return HttpDiagnosticResult 诊断结果
 */
HttpDiagnosticResult HttpDiagnostics::runFullDiagnostic() {
    HttpDiagnosticResult result;
    result.overallStatus = HTTP_DIAG_UNKNOWN;
    String errorDetails = "";
    
    debugPrint("=== 开始HTTP诊断 ===");
    
    // 1. 检查AT命令处理器状态
    result.atHandlerStatus = checkAtCommandHandler();
    debugPrint("AT命令处理器状态: " + getStatusString(result.atHandlerStatus));
    if (result.atHandlerStatus != HTTP_DIAG_OK) {
        errorDetails += "AT命令处理器异常; ";
    }
    
    // 2. 检查GSM模块状态
    result.gsmModuleStatus = checkGsmModule();
    debugPrint("GSM模块状态: " + getStatusString(result.gsmModuleStatus));
    if (result.gsmModuleStatus != HTTP_DIAG_OK) {
        errorDetails += "GSM模块异常; ";
    }
    
    // 3. 检查网络连接状态
    result.networkStatus = checkNetworkConnection();
    debugPrint("网络连接状态: " + getStatusString(result.networkStatus));
    if (result.networkStatus != HTTP_DIAG_OK) {
        errorDetails += "网络连接异常; ";
    }
    
    // 4. 检查PDP上下文状态
    result.pdpContextStatus = checkPdpContext();
    debugPrint("PDP上下文状态: " + getStatusString(result.pdpContextStatus));
    if (result.pdpContextStatus != HTTP_DIAG_OK) {
        errorDetails += "PDP上下文异常; ";
    }
    
    // 5. 检查HTTP服务状态
    result.httpServiceStatus = checkHttpService();
    debugPrint("HTTP服务状态: " + getStatusString(result.httpServiceStatus));
    if (result.httpServiceStatus != HTTP_DIAG_OK) {
        errorDetails += "HTTP服务异常; ";
    }
    
    // 6. 测试基本HTTP功能
    result.httpFunctionStatus = testHttpFunction();
    debugPrint("HTTP功能测试: " + getStatusString(result.httpFunctionStatus));
    if (result.httpFunctionStatus != HTTP_DIAG_OK) {
        errorDetails += "HTTP功能异常; ";
    }
    
    // 设置错误详情
    result.errorMessage = errorDetails;
    
    // 确定整体状态
    result.overallStatus = determineOverallStatus(result);
    
    debugPrint("=== HTTP诊断完成 ===");
    debugPrint("整体状态: " + getStatusString(result.overallStatus));
    
    return result;
}

/**
 * @brief 检查AT命令处理器状态
 * @return HttpDiagnosticStatus 诊断状态
 */
HttpDiagnosticStatus HttpDiagnostics::checkAtCommandHandler() {
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    
    // 测试基本AT命令
    AtResponse response = atHandler.sendCommand("AT", "OK", 3000);
    
    if (response.result == AT_RESULT_SUCCESS) {
        return HTTP_DIAG_OK;
    } else if (response.result == AT_RESULT_TIMEOUT) {
        return HTTP_DIAG_TIMEOUT;
    } else {
        return HTTP_DIAG_ERROR;
    }
}

/**
 * @brief 检查GSM模块状态
 * @return HttpDiagnosticStatus 诊断状态
 */
HttpDiagnosticStatus HttpDiagnostics::checkGsmModule() {
    GsmService& gsmService = GsmService::getInstance();
    
    if (!gsmService.isModuleOnline()) {
        return HTTP_DIAG_ERROR;
    }
    
    // 检查信号强度
    int signalStrength = gsmService.getSignalStrength();
    debugPrint("信号强度: " + String(signalStrength));
    
    if (signalStrength < 0) {
        return HTTP_DIAG_ERROR;
    } else if (signalStrength < 10) {
        return HTTP_DIAG_WARNING;
    } else {
        return HTTP_DIAG_OK;
    }
}

/**
 * @brief 检查网络连接状态
 * @return HttpDiagnosticStatus 诊断状态
 */
HttpDiagnosticStatus HttpDiagnostics::checkNetworkConnection() {
    GsmService& gsmService = GsmService::getInstance();
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    
    // 检查网络注册状态
    GsmNetworkStatus networkStatus = gsmService.getNetworkStatus();
    debugPrint("网络注册状态: " + String(networkStatus));
    
    // 检查信号强度
    int signalStrength = gsmService.getSignalStrength();
    debugPrint("信号强度: " + String(signalStrength) + " dBm");
    
    // 检查运营商信息
    AtResponse operatorResponse = atHandler.sendCommand("AT+COPS?", "OK", 3000);
    if (operatorResponse.result == AT_RESULT_SUCCESS) {
        debugPrint("运营商信息: " + operatorResponse.response);
    }
    
    // 检查APN配置
    AtResponse apnResponse = atHandler.sendCommand("AT+CGDCONT?", "OK", 3000);
    if (apnResponse.result == AT_RESULT_SUCCESS) {
        debugPrint("APN配置: " + apnResponse.response);
    }
    
    switch (networkStatus) {
        case GSM_NETWORK_REGISTERED_HOME:
        case GSM_NETWORK_REGISTERED_ROAMING:
            if (signalStrength < -100) {
                debugPrint("警告: 信号强度较弱");
                return HTTP_DIAG_WARNING;
            }
            return HTTP_DIAG_OK;
        case GSM_NETWORK_SEARCHING:
            debugPrint("网络搜索中...");
            return HTTP_DIAG_WARNING;
        case GSM_NETWORK_NOT_REGISTERED:
            debugPrint("错误: 网络未注册");
            return HTTP_DIAG_ERROR;
        case GSM_NETWORK_REGISTRATION_DENIED:
            debugPrint("错误: 网络注册被拒绝");
            return HTTP_DIAG_ERROR;
        default:
            debugPrint("错误: 未知网络状态");
            return HTTP_DIAG_ERROR;
    }
}

/**
 * @brief 检查PDP上下文状态
 * @return HttpDiagnosticStatus 诊断状态
 */
HttpDiagnosticStatus HttpDiagnostics::checkPdpContext() {
    HttpClient& httpClient = HttpClient::getInstance();
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    
    // 检查当前PDP上下文状态
    AtResponse pdpStatusResponse = atHandler.sendCommand("AT+CGACT?", "OK", 3000);
    if (pdpStatusResponse.result == AT_RESULT_SUCCESS) {
        debugPrint("PDP上下文状态: " + pdpStatusResponse.response);
    }
    
    // 检查PDP上下文配置
    AtResponse pdpConfigResponse = atHandler.sendCommand("AT+CGDCONT?", "OK", 3000);
    if (pdpConfigResponse.result == AT_RESULT_SUCCESS) {
        debugPrint("PDP上下文配置: " + pdpConfigResponse.response);
    }
    
    if (httpClient.isPdpContextActive()) {
        debugPrint("PDP上下文已激活");
        
        // 检查IP地址分配
        AtResponse ipResponse = atHandler.sendCommand("AT+CGPADDR=1", "OK", 3000);
        if (ipResponse.result == AT_RESULT_SUCCESS) {
            debugPrint("IP地址信息: " + ipResponse.response);
            if (ipResponse.response.indexOf("0.0.0.0") != -1) {
                debugPrint("警告: 未获取到有效IP地址");
                return HTTP_DIAG_WARNING;
            }
        }
        
        return HTTP_DIAG_OK;
    } else {
        debugPrint("PDP上下文未激活，尝试激活...");
        
        // 尝试激活PDP上下文
        if (httpClient.activatePdpContext()) {
            debugPrint("PDP上下文激活成功");
            
            // 等待IP地址分配
            delay(2000);
            AtResponse ipResponse = atHandler.sendCommand("AT+CGPADDR=1", "OK", 3000);
            if (ipResponse.result == AT_RESULT_SUCCESS) {
                debugPrint("激活后IP地址: " + ipResponse.response);
            }
            
            return HTTP_DIAG_OK;
        } else {
            debugPrint("错误: PDP上下文激活失败");
            return HTTP_DIAG_ERROR;
        }
    }
}

/**
 * @brief 检查HTTP服务状态
 * @return HttpDiagnosticStatus 诊断状态
 */
HttpDiagnosticStatus HttpDiagnostics::checkHttpService() {
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    
    // 先尝试终止可能存在的HTTP服务
    atHandler.sendCommand("AT+HTTPTERM", "OK", 3000);
    delay(1000);
    
    // 尝试初始化HTTP服务
    AtResponse response = atHandler.sendCommand("AT+HTTPINIT", "OK", 5000);
    
    if (response.result == AT_RESULT_SUCCESS) {
        // 测试HTTP参数设置
        AtResponse paramResponse = atHandler.sendCommand(
            "AT+HTTPPARA=\"URL\",\"http://httpbin.org/get\"", "OK", 3000);
        
        if (paramResponse.result == AT_RESULT_SUCCESS) {
            return HTTP_DIAG_OK;
        } else {
            return HTTP_DIAG_WARNING;
        }
    } else {
        return HTTP_DIAG_ERROR;
    }
}

/**
 * @brief 测试HTTP功能
 * @return HttpDiagnosticStatus 诊断状态
 */
HttpDiagnosticStatus HttpDiagnostics::testHttpFunction() {
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    
    // 测试简单的HTTP数据准备命令
    String testData = "{\"test\":\"data\"}";
    String command = "AT+HTTPDATA=" + String(testData.length()) + ",10000";
    
    debugPrint("测试HTTP数据命令: " + command);
    
    AtResponse response = atHandler.sendCommand(command, "DOWNLOAD", 10000);
    
    if (response.result == AT_RESULT_SUCCESS) {
        // 发送测试数据
        AtResponse dataResponse = atHandler.sendRawData(testData, 5000);
        if (dataResponse.result == AT_RESULT_SUCCESS) {
            return HTTP_DIAG_OK;
        } else {
            debugPrint("HTTP数据发送失败: " + dataResponse.response);
            return HTTP_DIAG_ERROR;
        }
    } else {
        debugPrint("HTTP数据准备失败: " + response.response);
        return HTTP_DIAG_ERROR;
    }
}

/**
 * @brief 确定整体诊断状态
 * @param result 诊断结果
 * @return HttpDiagnosticStatus 整体状态
 */
HttpDiagnosticStatus HttpDiagnostics::determineOverallStatus(const HttpDiagnosticResult& result) {
    // 如果任何关键组件出错，整体状态为错误
    if (result.atHandlerStatus == HTTP_DIAG_ERROR ||
        result.gsmModuleStatus == HTTP_DIAG_ERROR ||
        result.networkStatus == HTTP_DIAG_ERROR ||
        result.pdpContextStatus == HTTP_DIAG_ERROR ||
        result.httpServiceStatus == HTTP_DIAG_ERROR) {
        return HTTP_DIAG_ERROR;
    }
    
    // 如果有警告，整体状态为警告
    if (result.atHandlerStatus == HTTP_DIAG_WARNING ||
        result.gsmModuleStatus == HTTP_DIAG_WARNING ||
        result.networkStatus == HTTP_DIAG_WARNING ||
        result.pdpContextStatus == HTTP_DIAG_WARNING ||
        result.httpServiceStatus == HTTP_DIAG_WARNING) {
        return HTTP_DIAG_WARNING;
    }
    
    // HTTP功能测试结果决定最终状态
    return result.httpFunctionStatus;
}

/**
 * @brief 获取状态字符串
 * @param status 诊断状态
 * @return String 状态描述
 */
String HttpDiagnostics::getStatusString(HttpDiagnosticStatus status) {
    switch (status) {
        case HTTP_DIAG_OK:
            return "正常";
        case HTTP_DIAG_WARNING:
            return "警告";
        case HTTP_DIAG_ERROR:
            return "错误";
        case HTTP_DIAG_TIMEOUT:
            return "超时";
        case HTTP_DIAG_UNKNOWN:
        default:
            return "未知";
    }
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void HttpDiagnostics::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[HttpDiagnostics] " + message);
    }
}

/**
 * @brief 生成诊断报告
 * @param result 诊断结果
 * @return String 诊断报告
 */
String HttpDiagnostics::generateReport(const HttpDiagnosticResult& result) {
    String report = "\n=== HTTP诊断报告 ===\n";
    report += "AT命令处理器: " + getStatusString(result.atHandlerStatus) + "\n";
    report += "GSM模块: " + getStatusString(result.gsmModuleStatus) + "\n";
    report += "网络连接: " + getStatusString(result.networkStatus) + "\n";
    report += "PDP上下文: " + getStatusString(result.pdpContextStatus) + "\n";
    report += "HTTP服务: " + getStatusString(result.httpServiceStatus) + "\n";
    report += "HTTP功能: " + getStatusString(result.httpFunctionStatus) + "\n";
    report += "整体状态: " + getStatusString(result.overallStatus) + "\n";
    report += "==================\n";
    
    return report;
}