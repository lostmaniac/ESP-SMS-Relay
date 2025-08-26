/**
 * @file http_client.cpp
 * @brief HTTP客户端工具类实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "http_client.h"
#include "gsm_service.h"
#include "../../include/constants.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 * @param atHandler AT命令处理器引用
 * @param gsmService GSM服务引用
 */
HttpClient::HttpClient(AtCommandHandler& atHandler, GsmService& gsmService) 
    : atCommandHandler(atHandler), gsmService(gsmService), lastError(""), 
      debugMode(false), initialized(false), httpServiceActive(false), 
      defaultTimeout(DEFAULT_HTTP_TIMEOUT_MS), debugLog(""), maxLogSize(8192), 
      requestCount(0), lastLogTime(0) {
    // 构造函数实现
}

/**
 * @brief 析构函数
 */
HttpClient::~HttpClient() {
    if (httpServiceActive) {
        terminateHttpService();
    }
}

/**
 * @brief 获取单例实例
 * @return HttpClient& 单例引用
 */
HttpClient& HttpClient::getInstance() {
    static HttpClient* instance = nullptr;
    if (!instance) {
        AtCommandHandler& atHandler = AtCommandHandler::getInstance();
        GsmService& gsmSvc = GsmService::getInstance();
        instance = new HttpClient(atHandler, gsmSvc);
    }
    return *instance;
}

/**
 * @brief 初始化HTTP客户端
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool HttpClient::initialize() {
    if (initialized) {
        return true;
    }
    
    debugPrint("正在初始化HTTP客户端...");
    
    // 检查AT命令处理器是否已初始化
    if (!atCommandHandler.getLastError().isEmpty() && atCommandHandler.getLastError() != "") {
        // AT命令处理器可能有错误，但我们继续尝试
        debugPrint("警告: AT命令处理器可能存在问题: " + atCommandHandler.getLastError());
    }
    
    // 检查网络连接
    if (!isNetworkConnected()) {
        setError("网络未连接，请检查网络状态");
        return false;
    }
    
    // 检查并激活PDP上下文
    if (!isPdpContextActive()) {
        if (!activatePdpContext()) {
            setError("PDP上下文激活失败");
            return false;
        }
    }
    
    initialized = true;
    debugPrint("HTTP客户端初始化完成");
    return true;
}

/**
 * @brief 执行HTTP请求
 * @param request 请求参数
 * @return HttpResponse 响应结果
 */
HttpResponse HttpClient::request(const HttpRequest& request) {
    HttpResponse response;
    unsigned long startTime = millis();
    const int maxRetries = 2;  // 最大重试次数（针对网络连接问题）
    const unsigned long retryDelay = 2000;  // 重试间隔（毫秒）
    
    // 记录请求详情
    logRequestDetails(request);
    
    // 记录网络状态
    logNetworkStatus();
    
    // 验证URL
    if (!validateUrl(request.url)) {
        response.error = HTTP_ERROR_INVALID_URL;
        setError("无效的URL: " + request.url);
        response.duration = millis() - startTime;
        logResponseDetails(response);
        return response;
    }
    
    for (int attempt = 0; attempt <= maxRetries; attempt++) {
        if (attempt > 0) {
            debugPrint("HTTP请求重试第 " + String(attempt) + " 次");
            delay(retryDelay);
        }
        
        // 检查网络连接状态
        if (!isNetworkConnected()) {
            debugPrint("网络未连接，尝试等待网络恢复...");
            
            // 等待网络恢复，最多等待10秒
            unsigned long networkWaitStart = millis();
            while (!isNetworkConnected() && (millis() - networkWaitStart) < 10000) {
                delay(1000);
            }
            
            if (!isNetworkConnected()) {
                if (attempt < maxRetries) {
                    debugPrint("网络仍未连接，将在 " + String(retryDelay) + " 毫秒后重试");
                    continue;
                } else {
                    response.error = HTTP_ERROR_NETWORK;
                    setError("网络连接失败");
                    response.duration = millis() - startTime;
                    logResponseDetails(response);
                    return response;
                }
            }
        }
        
        // 检查PDP上下文状态
        if (!isPdpContextActive()) {
            debugPrint("PDP上下文未激活，尝试激活...");
            if (!activatePdpContext()) {
                if (attempt < maxRetries) {
                    debugPrint("PDP上下文激活失败，将重试");
                    continue;
                } else {
                    response.error = HTTP_ERROR_NETWORK;
                    setError("PDP上下文激活失败");
                    response.duration = millis() - startTime;
                    logResponseDetails(response);
                    return response;
                }
            }
        }
        
        // 检查初始化状态
        if (!initialized && !initialize()) {
            response.error = HTTP_ERROR_INIT;
            response.duration = millis() - startTime;
            logResponseDetails(response);
            return response;
        }
        
        // 初始化HTTP服务
        if (!initHttpService()) {
            if (attempt < maxRetries) {
                debugPrint("HTTP服务初始化失败，将重试");
                continue;
            } else {
                response.error = HTTP_ERROR_INIT;
                response.duration = millis() - startTime;
                logResponseDetails(response);
                return response;
            }
        }
        
        // 设置URL
        if (!setHttpParameter("URL", request.url)) {
            response.error = HTTP_ERROR_AT_COMMAND;
            terminateHttpService();
            
            if (attempt < maxRetries) {
                debugPrint("设置URL失败，将重试");
                continue;
            } else {
                response.duration = millis() - startTime;
                logResponseDetails(response);
                return response;
            }
        }
        
        // 设置请求头
        bool headerSetSuccess = true;
        for (const auto& header : request.headers) {
            String headerParam = header.first + ": " + header.second;
            if (!setHttpParameter("USERDATA", headerParam)) {
                debugPrint("设置请求头失败: " + headerParam);
                headerSetSuccess = false;
                break;
            }
        }
        
        if (!headerSetSuccess) {
            terminateHttpService();
            if (attempt < maxRetries) {
                debugPrint("设置请求头失败，将重试");
                continue;
            } else {
                response.error = HTTP_ERROR_AT_COMMAND;
                setError("设置请求头失败");
                return response;
            }
        }
        
        // 根据请求方法执行不同操作
        if (request.method == HTTP_CLIENT_POST || request.method == HTTP_CLIENT_PUT) {
            // POST/PUT请求需要先发送数据
            if (!request.body.isEmpty()) {
                if (!sendHttpData(request.body, request.timeout)) {
                    response.error = HTTP_ERROR_AT_COMMAND;
                    terminateHttpService();
                    
                    if (attempt < maxRetries) {
                        debugPrint("发送HTTP数据失败，将重试");
                        continue;
                    } else {
                        return response;
                    }
                }
            }
        }
        
        // 执行HTTP动作
        response = executeHttpAction(request.method, request.timeout);
        
        // 检查响应结果
        if (response.error == HTTP_SUCCESS) {
            // 如果请求成功，读取响应内容
            if (response.contentLength > 0) {
                response.body = readHttpResponse(0, response.contentLength);
            }
            
            // 终止HTTP服务
            terminateHttpService();
            
            response.duration = millis() - startTime;
            logResponseDetails(response);
            return response;
        } else if (response.error == HTTP_ERROR_TIMEOUT || response.error == HTTP_ERROR_AT_COMMAND) {
            // 超时或AT命令错误可以重试
            terminateHttpService();
            
            if (attempt < maxRetries) {
                debugPrint("HTTP请求失败（错误: " + String(response.error) + "），将重试");
                continue;
            }
        } else {
            // 其他错误不重试
            terminateHttpService();
            break;
        }
    }
    
    // 终止HTTP服务（确保清理）
    terminateHttpService();
    
    response.duration = millis() - startTime;
    logResponseDetails(response);
    return response;
}

/**
 * @brief 执行POST请求
 * @param url 请求URL
 * @param body 请求体
 * @param headers 请求头（可选）
 * @param timeout 超时时间（可选）
 * @return HttpResponse 响应结果
 */
HttpResponse HttpClient::post(const String& url, 
                             const String& body,
                             const std::map<String, String>& headers,
                             unsigned long timeout) {
    HttpRequest request;
    request.url = url;
    request.method = HTTP_CLIENT_POST;
    request.protocol = detectProtocol(url);
    request.body = body;
    request.headers = headers;
    request.timeout = timeout;
    
    return this->request(request);
}

/**
 * @brief 检查网络连接状态
 * @return true 网络已连接
 * @return false 网络未连接
 */
bool HttpClient::isNetworkConnected() {
    GsmService& gsmService = GsmService::getInstance();
    GsmNetworkStatus status = gsmService.getNetworkStatus();
    
    return (status == GSM_NETWORK_REGISTERED_HOME || 
            status == GSM_NETWORK_REGISTERED_ROAMING);
}

/**
 * @brief 检查PDP上下文状态
 * @return true PDP上下文已激活
 * @return false PDP上下文未激活
 */
bool HttpClient::isPdpContextActive() {
    AtResponse response = atCommandHandler.sendCommandWithFullResponse("AT+CGACT?", DEFAULT_AT_COMMAND_TIMEOUT_MS);
    
    if (response.result == AT_RESULT_SUCCESS) {
        // 检查响应中是否包含激活的PDP上下文
        return response.response.indexOf("+CGACT: 1,1") != -1;
    }
    
    return false;
}

/**
 * @brief 配置APN
 * @param apn APN名称
 * @param username 用户名（可选）
 * @param password 密码（可选）
 * @return true 配置成功
 * @return false 配置失败
 */
bool HttpClient::configureApn(const String& apn, const String& username, const String& password) {
    debugPrint("正在配置APN: " + apn);
    
    // 配置PDP上下文
    String command = "AT+CGDCONT=1,\"IP\",\"" + apn + "\"";
    AtResponse response = atCommandHandler.sendCommand(command, "OK", DEFAULT_HTTP_TIMEOUT_MS);
    
    if (response.result != AT_RESULT_SUCCESS) {
        setError("配置PDP上下文失败: " + response.response);
        return false;
    }
    
    // 如果提供了用户名和密码，配置认证
    if (username.length() > 0 || password.length() > 0) {
        String authCommand = "AT+CGAUTH=1,1,\"" + username + "\",\"" + password + "\"";
        response = atCommandHandler.sendCommand(authCommand, "OK", DEFAULT_HTTP_TIMEOUT_MS);
        
        if (response.result != AT_RESULT_SUCCESS) {
            debugPrint("警告: 配置认证失败，但继续执行: " + response.response);
        } else {
            debugPrint("APN认证配置成功");
        }
    }
    
    debugPrint("APN配置成功: " + apn);
    return true;
}

/**
 * @brief 激活PDP上下文
 * @return true 激活成功
 * @return false 激活失败
 */
bool HttpClient::activatePdpContext() {
    debugPrint("正在激活PDP上下文...");
    
    // 激活PDP上下文
    AtResponse response = atCommandHandler.sendCommand("AT+CGACT=1,1", "OK", DEFAULT_HTTP_TIMEOUT_MS);
    
    if (response.result == AT_RESULT_SUCCESS) {
        debugPrint("PDP上下文激活成功");
        return true;
    }
    
    setError("PDP上下文激活失败: " + response.response);
    return false;
}

/**
 * @brief 配置APN并激活PDP上下文
 * @param apn APN名称
 * @param username 用户名（可选）
 * @param password 密码（可选）
 * @return true 配置和激活成功
 * @return false 配置或激活失败
 */
bool HttpClient::configureAndActivateApn(const String& apn, const String& username, const String& password) {
    // 先配置APN
    if (!configureApn(apn, username, password)) {
        return false;
    }
    
    // 再激活PDP上下文
    return activatePdpContext();
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String HttpClient::getLastError() {
    return lastError;
}

/**
 * @brief 设置调试模式
 * @param enabled 是否启用调试
 */
void HttpClient::setDebugMode(bool enabled) {
    debugMode = enabled;
    atCommandHandler.setDebugMode(enabled);
}

/**
 * @brief 设置默认超时时间
 * @param timeout 超时时间(ms)
 */
void HttpClient::setDefaultTimeout(unsigned long timeout) {
    defaultTimeout = timeout;
}

/**
 * @brief 初始化HTTP服务
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool HttpClient::initHttpService() {
    if (httpServiceActive) {
        return true;
    }
    
    debugPrint("初始化HTTP服务...");
    
    unsigned long cmdStartTime = millis();
    AtResponse response = atCommandHandler.sendCommand("AT+HTTPINIT", "OK", DEFAULT_HTTP_TIMEOUT_MS);
    logAtCommandDetails("AT+HTTPINIT", response.response, millis() - cmdStartTime);
    
    if (response.result == AT_RESULT_SUCCESS) {
        httpServiceActive = true;
        debugPrint("HTTP服务初始化成功");
        return true;
    }
    
    setError("HTTP服务初始化失败: " + response.response);
    return false;
}

/**
 * @brief 终止HTTP服务
 * @return true 终止成功
 * @return false 终止失败
 */
bool HttpClient::terminateHttpService() {
    if (!httpServiceActive) {
        return true;
    }
    
    debugPrint("终止HTTP服务...");
    
    unsigned long cmdStartTime = millis();
    AtResponse response = atCommandHandler.sendCommand("AT+HTTPTERM", "OK", DEFAULT_HTTP_TIMEOUT_MS);
    logAtCommandDetails("AT+HTTPTERM", response.response, millis() - cmdStartTime);
    
    httpServiceActive = false;
    
    if (response.result == AT_RESULT_SUCCESS) {
        debugPrint("HTTP服务终止成功");
        return true;
    }
    
    debugPrint("HTTP服务终止失败: " + response.response);
    return false;
}

/**
 * @brief 设置HTTP参数
 * @param parameter 参数名
 * @param value 参数值
 * @return true 设置成功
 * @return false 设置失败
 */
bool HttpClient::setHttpParameter(const String& parameter, const String& value) {
    String command = "AT+HTTPPARA=\"" + parameter + "\",\"" + value + "\"";
    
    unsigned long cmdStartTime = millis();
    AtResponse response = atCommandHandler.sendCommand(command, "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS);
    logAtCommandDetails(command, response.response, millis() - cmdStartTime);
    
    if (response.result == AT_RESULT_SUCCESS) {
        debugPrint("设置HTTP参数成功: " + parameter + " = " + value);
        return true;
    }
    
    setError("设置HTTP参数失败: " + parameter + ", 响应: " + response.response);
    return false;
}

/**
 * @brief 执行HTTP动作
 * @param method HTTP方法
 * @param timeout 超时时间
 * @return HttpResponse 响应结果
 */
HttpResponse HttpClient::executeHttpAction(HttpClientMethod method, unsigned long timeout) {
    HttpResponse response;
    const int maxRetries = 3;  // 最大重试次数
    const unsigned long retryDelay = 1000;  // 重试间隔（毫秒）
    
    String command = "AT+HTTPACTION=" + String((int)method);
    
    for (int attempt = 0; attempt <= maxRetries; attempt++) {
        if (attempt > 0) {
            debugPrint("HTTP动作重试第 " + String(attempt) + " 次");
            delay(retryDelay);
            
            // 重试前重新初始化HTTP服务
            terminateHttpService();
            delay(500);
            if (!initHttpService()) {
                debugPrint("重试时HTTP服务初始化失败");
                continue;
            }
        }
        
        debugPrint("执行HTTP动作: " + getMethodString(method) + (attempt > 0 ? " (重试 " + String(attempt) + ")" : ""));
        
        // 发送HTTP动作命令
        unsigned long cmdStartTime = millis();
        AtResponse atResponse = atCommandHandler.sendCommand(command, "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS);
        logAtCommandDetails(command, atResponse.response, millis() - cmdStartTime);
        
        if (atResponse.result != AT_RESULT_SUCCESS) {
            response.error = HTTP_ERROR_AT_COMMAND;
            setError("HTTP动作命令失败: " + atResponse.response);
            
            // 如果是最后一次尝试，返回错误
            if (attempt == maxRetries) {
                return response;
            }
            continue;
        }
        
        // 等待HTTP动作完成响应
        cmdStartTime = millis();
        atResponse = atCommandHandler.waitForResponse("+HTTPACTION:", timeout);
        logAtCommandDetails("WAIT +HTTPACTION:", atResponse.response, millis() - cmdStartTime);
        
        if (atResponse.result == AT_RESULT_SUCCESS) {
            if (parseHttpActionResponse(atResponse.response, response)) {
                debugPrint("HTTP请求完成，状态码: " + String(response.statusCode));
                
                // 检查HTTP状态码，某些错误状态码需要重试
                if (response.statusCode >= 200 && response.statusCode < 300) {
                    // 成功状态码，直接返回
                    return response;
                } else if (response.statusCode >= 500 || response.statusCode == 408 || response.statusCode == 429) {
                    // 服务器错误、请求超时或请求过多，可以重试
                    if (attempt < maxRetries) {
                        debugPrint("HTTP状态码 " + String(response.statusCode) + " 需要重试");
                        continue;
                    }
                }
                
                // 其他状态码或已达到最大重试次数，返回响应
                return response;
            } else {
                response.error = HTTP_ERROR_SERVER;
                setError("解析HTTP响应失败: " + atResponse.response);
                
                // 解析失败可以重试
                if (attempt < maxRetries) {
                    continue;
                }
            }
        } else {
            response.error = HTTP_ERROR_TIMEOUT;
            setError("HTTP请求超时");
            
            // 超时可以重试
            if (attempt < maxRetries) {
                continue;
            }
        }
    }
    
    return response;
}

/**
 * @brief 发送HTTP数据（用于POST请求）
 * @param data 要发送的数据
 * @param timeout 超时时间
 * @return true 发送成功
 * @return false 发送失败
 */
bool HttpClient::sendHttpData(const String& data, unsigned long timeout) {
    const int MAX_RETRY_COUNT = 3;
    const int httpRetryDelay = 1000;  // HTTP数据发送重试间隔（毫秒）
    
    String command = "AT+HTTPDATA=" + String(data.length()) + "," + String(timeout);
    
    debugPrint("准备发送HTTP数据，长度: " + String(data.length()));
    
    for (int attempt = 1; attempt <= MAX_RETRY_COUNT; attempt++) {
        debugPrint("HTTP数据发送尝试 " + String(attempt) + "/" + String(MAX_RETRY_COUNT));
        
        // 发送数据长度命令
        unsigned long cmdStartTime = millis();
        AtResponse response = atCommandHandler.sendCommand(command, "DOWNLOAD", DEFAULT_AT_COMMAND_TIMEOUT_MS);
        logAtCommandDetails(command, response.response, millis() - cmdStartTime);
        
        if (response.result != AT_RESULT_SUCCESS) {
            String errorMsg = "HTTP数据准备失败 (尝试 " + String(attempt) + "): " + command + " -> " + response.response;
            debugPrint(errorMsg);
            
            if (attempt < MAX_RETRY_COUNT) {
                debugPrint("等待 " + String(httpRetryDelay) + "ms 后重试...");
                delay(httpRetryDelay);
                
                // 尝试重新初始化HTTP服务
                terminateHttpService();
                delay(500);
                if (!initHttpService()) {
                    debugPrint("重新初始化HTTP服务失败");
                    continue;
                }
            } else {
                setError(errorMsg);
                return false;
            }
            continue;
        }
        
        // 发送实际数据
        debugPrint("开始发送HTTP数据内容...");
        cmdStartTime = millis();
        response = atCommandHandler.sendRawData(data, timeout);
        logAtCommandDetails("[RAW DATA: " + String(data.length()) + " bytes]", response.response, millis() - cmdStartTime);
        
        if (response.result == AT_RESULT_SUCCESS && response.response.indexOf("OK") != -1) {
            debugPrint("HTTP数据发送成功 (尝试 " + String(attempt) + ")");
            return true;
        }
        
        String errorMsg = "HTTP数据发送失败 (尝试 " + String(attempt) + "): " + response.response;
        debugPrint(errorMsg);
        
        if (attempt < MAX_RETRY_COUNT) {
            debugPrint("等待 " + String(httpRetryDelay) + "ms 后重试...");
            delay(httpRetryDelay);
        } else {
            setError(errorMsg);
            return false;
        }
    }
    
    return false;
}

/**
 * @brief 读取HTTP响应
 * @param startPos 开始位置
 * @param length 读取长度
 * @return String 响应内容
 */
String HttpClient::readHttpResponse(int startPos, int length) {
    String command = "AT+HTTPREAD=" + String(startPos) + "," + String(length);
    
    debugPrint("读取HTTP响应，起始位置: " + String(startPos) + ", 长度: " + String(length));
    
    AtResponse response = atCommandHandler.sendCommandWithFullResponse(command, DEFAULT_HTTP_TIMEOUT_MS);
    
    if (response.result == AT_RESULT_SUCCESS) {
        // 解析响应内容
        int readIndex = response.response.indexOf("+HTTPREAD:");
        if (readIndex != -1) {
            int contentStart = response.response.indexOf('\n', readIndex);
            if (contentStart != -1) {
                contentStart++; // 跳过换行符
                int contentEnd = response.response.lastIndexOf("\n+HTTPREAD: 0");
                if (contentEnd == -1) {
                    contentEnd = response.response.length();
                }
                
                String content = response.response.substring(contentStart, contentEnd);
                debugPrint("成功读取HTTP响应内容");
                return content;
            }
        }
    }
    
    setError("读取HTTP响应失败: " + response.response);
    return "";
}

/**
 * @brief 解析HTTP动作响应
 * @param response AT+HTTPACTION的响应
 * @param httpResponse HTTP响应结构体
 * @return true 解析成功
 * @return false 解析失败
 */
bool HttpClient::parseHttpActionResponse(const String& response, HttpResponse& httpResponse) {
    // 解析响应格式: +HTTPACTION: <method>,<errcode>,<datalen>
    int actionIndex = response.indexOf("+HTTPACTION:");
    if (actionIndex == -1) {
        return false;
    }
    
    int firstComma = response.indexOf(',', actionIndex);
    int secondComma = response.indexOf(',', firstComma + 1);
    
    if (firstComma == -1 || secondComma == -1) {
        return false;
    }
    
    // 提取状态码
    String statusStr = response.substring(firstComma + 1, secondComma);
    statusStr.trim();
    httpResponse.statusCode = statusStr.toInt();
    
    // 提取数据长度
    String lengthStr = response.substring(secondComma + 1);
    lengthStr.trim();
    httpResponse.contentLength = lengthStr.toInt();
    
    // 判断请求是否成功
    if (httpResponse.statusCode == 200) {
        httpResponse.error = HTTP_SUCCESS;
    } else if (httpResponse.statusCode >= 400) {
        httpResponse.error = HTTP_ERROR_SERVER;
    } else {
        httpResponse.error = HTTP_ERROR_NETWORK;
    }
    
    return true;
}

/**
 * @brief 检测URL协议类型
 * @param url URL字符串
 * @return HttpProtocol 协议类型
 */
HttpProtocol HttpClient::detectProtocol(const String& url) {
    if (url.startsWith("https://")) {
        return HTTPS_PROTOCOL;
    }
    return HTTP_PROTOCOL;
}

/**
 * @brief 验证URL格式
 * @param url URL字符串
 * @return true URL有效
 * @return false URL无效
 */
bool HttpClient::validateUrl(const String& url) {
    if (url.length() == 0) {
        return false;
    }
    
    return (url.startsWith("http://") || url.startsWith("https://"));
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void HttpClient::setError(const String& error) {
    lastError = error;
    if (debugMode) {
        Serial.printf("HTTP客户端错误: %s\n", error.c_str());
    }
}

/**
 * @brief 打印调试信息
 * @param message 调试信息
 */
void HttpClient::debugPrint(const String& message) {
    if (debugMode) {
        Serial.printf("[HTTP] %s\n", message.c_str());
    }
}

/**
 * @brief 获取HTTP方法字符串
 * @param method HTTP方法枚举
 * @return String 方法字符串
 */
String HttpClient::getMethodString(HttpClientMethod method) {
    switch (method) {
        case HTTP_CLIENT_GET: return "GET";
    case HTTP_CLIENT_POST: return "POST";
    case HTTP_CLIENT_PUT: return "PUT";
    case HTTP_CLIENT_DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 获取详细的调试日志
 * @return String 调试日志内容
 */
String HttpClient::getDebugLog() {
    return debugLog;
}

/**
 * @brief 清空调试日志
 */
void HttpClient::clearDebugLog() {
    debugLog = "";
    lastLogTime = millis();
    debugPrint("调试日志已清空");
}

/**
 * @brief 记录请求详细信息
 * @param request HTTP请求对象
 */
void HttpClient::logRequestDetails(const HttpRequest& request) {
    if (!debugMode) return;
    
    requestCount++;
    String logEntry = "\n[" + String(millis()) + "] === HTTP请求详情 #" + String(requestCount) + " ===\n";
    logEntry += "URL: " + request.url + "\n";
    logEntry += "方法: " + getMethodString(request.method) + "\n";
    logEntry += "协议: " + String(request.protocol == HTTP_PROTOCOL ? "HTTP" : "HTTPS") + "\n";
    logEntry += "超时: " + String(request.timeout) + "ms\n";
    
    // 记录请求头
    if (!request.headers.empty()) {
        logEntry += "请求头:\n";
        for (const auto& header : request.headers) {
            logEntry += "  " + header.first + ": " + header.second + "\n";
        }
    }
    
    // 记录请求体（仅显示前200字符）
    if (request.body.length() > 0) {
        String bodyPreview = request.body;
        if (bodyPreview.length() > 200) {
            bodyPreview = bodyPreview.substring(0, 200) + "...";
        }
        logEntry += "请求体: " + bodyPreview + "\n";
        logEntry += "请求体大小: " + String(request.body.length()) + " bytes\n";
    }
    
    appendToDebugLog(logEntry);
}

/**
 * @brief 记录响应详细信息
 * @param response HTTP响应对象
 */
void HttpClient::logResponseDetails(const HttpResponse& response) {
    if (!debugMode) return;
    
    String logEntry = "\n[" + String(millis()) + "] === HTTP响应详情 #" + String(requestCount) + " ===\n";
    logEntry += "错误代码: " + String(response.error) + "\n";
    logEntry += "状态码: " + String(response.statusCode) + "\n";
    logEntry += "耗时: " + String(response.duration) + "ms\n";
    logEntry += "内容长度: " + String(response.contentLength) + " bytes\n";
    
    // 记录响应头
    if (!response.headers.empty()) {
        logEntry += "响应头:\n";
        for (const auto& header : response.headers) {
            logEntry += "  " + header.first + ": " + header.second + "\n";
        }
    }
    
    // 记录响应体（仅显示前500字符）
    if (response.body.length() > 0) {
        String bodyPreview = response.body;
        if (bodyPreview.length() > 500) {
            bodyPreview = bodyPreview.substring(0, 500) + "...";
        }
        logEntry += "响应体: " + bodyPreview + "\n";
    }
    
    // 错误分析
    if (response.error != HTTP_SUCCESS) {
        logEntry += "错误分析: ";
        switch (response.error) {
            case HTTP_ERROR_NETWORK:
                logEntry += "网络连接错误，检查网络状态\n";
                break;
            case HTTP_ERROR_TIMEOUT:
                logEntry += "请求超时，考虑增加超时时间\n";
                break;
            case HTTP_ERROR_INIT:
                logEntry += "HTTP服务初始化失败\n";
                break;
            case HTTP_ERROR_INVALID_URL:
                logEntry += "URL格式无效\n";
                break;
            case HTTP_ERROR_SERVER:
                logEntry += "服务器错误，状态码: " + String(response.statusCode) + "\n";
                break;
            case HTTP_ERROR_AT_COMMAND:
                logEntry += "AT命令执行失败\n";
                break;
            default:
                logEntry += "未知错误\n";
                break;
        }
    }
    
    appendToDebugLog(logEntry);
}

/**
 * @brief 记录网络状态信息
 */
void HttpClient::logNetworkStatus() {
    if (!debugMode) return;
    
    String logEntry = "\n[" + String(millis()) + "] === 网络状态检查 ===\n";
    
    // 检查网络连接
    bool networkConnected = isNetworkConnected();
    logEntry += "网络连接: " + String(networkConnected ? "已连接" : "未连接") + "\n";
    
    // 检查PDP上下文
    bool pdpActive = isPdpContextActive();
    logEntry += "PDP上下文: " + String(pdpActive ? "已激活" : "未激活") + "\n";
    
    // 检查HTTP服务状态
    logEntry += "HTTP服务: " + String(httpServiceActive ? "已激活" : "未激活") + "\n";
    
    appendToDebugLog(logEntry);
}

/**
 * @brief 记录AT命令执行详情
 * @param command AT命令
 * @param response AT响应
 * @param duration 执行时长
 */
void HttpClient::logAtCommandDetails(const String& command, const String& response, unsigned long duration) {
    if (!debugMode) return;
    
    String logEntry = "\n[" + String(millis()) + "] === AT命令执行 ===\n";
    logEntry += "命令: " + command + "\n";
    logEntry += "耗时: " + String(duration) + "ms\n";
    
    // 响应内容（限制长度）
    String responsePreview = response;
    if (responsePreview.length() > 300) {
        responsePreview = responsePreview.substring(0, 300) + "...";
    }
    logEntry += "响应: " + responsePreview + "\n";
    
    // 响应分析
    if (response.indexOf("OK") != -1) {
        logEntry += "状态: 成功\n";
    } else if (response.indexOf("ERROR") != -1) {
        logEntry += "状态: 错误\n";
        if (response.indexOf("+CME ERROR") != -1) {
            logEntry += "类型: GSM模块错误\n";
        } else if (response.indexOf("+CMS ERROR") != -1) {
            logEntry += "类型: SMS错误\n";
        }
    } else if (response.length() == 0) {
        logEntry += "状态: 超时\n";
    } else {
        logEntry += "状态: 未知响应\n";
    }
    
    appendToDebugLog(logEntry);
}

/**
 * @brief 向调试日志追加内容
 * @param logEntry 日志条目
 */
void HttpClient::appendToDebugLog(const String& logEntry) {
    // 检查日志大小，如果超过限制则清理旧日志
    if (debugLog.length() + logEntry.length() > maxLogSize) {
        // 保留最后一半的日志
        int keepSize = maxLogSize / 2;
        if (debugLog.length() > keepSize) {
            debugLog = debugLog.substring(debugLog.length() - keepSize);
            debugLog = "\n[日志已截断]\n" + debugLog;
        }
    }
    
    debugLog += logEntry;
    lastLogTime = millis();
}