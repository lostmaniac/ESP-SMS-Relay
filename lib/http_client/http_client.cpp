/**
 * @file http_client.cpp
 * @brief HTTP客户端工具类实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "http_client.h"
#include "gsm_service.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 * @param atHandler AT命令处理器引用
 * @param gsmService GSM服务引用
 */
HttpClient::HttpClient(AtCommandHandler& atHandler, GsmService& gsmService) 
    : atCommandHandler(atHandler), gsmService(gsmService), lastError(""), 
      debugMode(false), initialized(false), httpServiceActive(false), 
      defaultTimeout(30000) {
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
    
    // 验证URL
    if (!validateUrl(request.url)) {
        response.error = HTTP_ERROR_INVALID_URL;
        setError("无效的URL: " + request.url);
        return response;
    }
    
    // 检查初始化状态
    if (!initialized && !initialize()) {
        response.error = HTTP_ERROR_INIT;
        return response;
    }
    
    // 初始化HTTP服务
    if (!initHttpService()) {
        response.error = HTTP_ERROR_INIT;
        return response;
    }
    
    // 设置URL
    if (!setHttpParameter("URL", request.url)) {
        response.error = HTTP_ERROR_AT_COMMAND;
        terminateHttpService();
        return response;
    }
    
    // 设置请求头
    for (const auto& header : request.headers) {
        String headerParam = header.first + ": " + header.second;
        if (!setHttpParameter("USERDATA", headerParam)) {
            debugPrint("设置请求头失败: " + headerParam);
        }
    }
    
    // 根据请求方法执行不同操作
    if (request.method == HTTP_CLIENT_POST || request.method == HTTP_CLIENT_PUT) {
        // POST/PUT请求需要先发送数据
        if (!request.body.isEmpty()) {
            if (!sendHttpData(request.body, request.timeout)) {
                response.error = HTTP_ERROR_AT_COMMAND;
                terminateHttpService();
                return response;
            }
        }
    }
    
    // 执行HTTP动作
    response = executeHttpAction(request.method, request.timeout);
    
    // 如果请求成功，读取响应内容
    if (response.error == HTTP_SUCCESS && response.contentLength > 0) {
        response.body = readHttpResponse(0, response.contentLength);
    }
    
    // 终止HTTP服务
    terminateHttpService();
    
    response.duration = millis() - startTime;
    return response;
}

/**
 * @brief 执行GET请求
 * @param url 请求URL
 * @param headers 请求头（可选）
 * @param timeout 超时时间（可选）
 * @return HttpResponse 响应结果
 */
HttpResponse HttpClient::get(const String& url, 
                            const std::map<String, String>& headers,
                            unsigned long timeout) {
    HttpRequest request;
    request.url = url;
    request.method = HTTP_CLIENT_GET;
    request.protocol = detectProtocol(url);
    request.headers = headers;
    request.timeout = timeout;
    
    return this->request(request);
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
    AtResponse response = atCommandHandler.sendCommandWithFullResponse("AT+CGACT?", 5000);
    
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
    AtResponse response = atCommandHandler.sendCommand(command, "OK", 10000);
    
    if (response.result != AT_RESULT_SUCCESS) {
        setError("配置PDP上下文失败: " + response.response);
        return false;
    }
    
    // 如果提供了用户名和密码，配置认证
    if (username.length() > 0 || password.length() > 0) {
        String authCommand = "AT+CGAUTH=1,1,\"" + username + "\",\"" + password + "\"";
        response = atCommandHandler.sendCommand(authCommand, "OK", 10000);
        
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
    AtResponse response = atCommandHandler.sendCommand("AT+CGACT=1,1", "OK", 30000);
    
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
    
    AtResponse response = atCommandHandler.sendCommand("AT+HTTPINIT", "OK", 10000);
    
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
    
    AtResponse response = atCommandHandler.sendCommand("AT+HTTPTERM", "OK", 10000);
    
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
    
    AtResponse response = atCommandHandler.sendCommand(command, "OK", 5000);
    
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
    
    String command = "AT+HTTPACTION=" + String((int)method);
    
    debugPrint("执行HTTP动作: " + getMethodString(method));
    
    // 发送HTTP动作命令
    AtResponse atResponse = atCommandHandler.sendCommand(command, "OK", 5000);
    
    if (atResponse.result != AT_RESULT_SUCCESS) {
        response.error = HTTP_ERROR_AT_COMMAND;
        setError("HTTP动作命令失败: " + atResponse.response);
        return response;
    }
    
    // 等待HTTP动作完成响应
    atResponse = atCommandHandler.waitForResponse("+HTTPACTION:", timeout);
    
    if (atResponse.result == AT_RESULT_SUCCESS) {
        if (parseHttpActionResponse(atResponse.response, response)) {
            debugPrint("HTTP请求完成，状态码: " + String(response.statusCode));
        } else {
            response.error = HTTP_ERROR_SERVER;
            setError("解析HTTP响应失败: " + atResponse.response);
        }
    } else {
        response.error = HTTP_ERROR_TIMEOUT;
        setError("HTTP请求超时");
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
    String command = "AT+HTTPDATA=" + String(data.length()) + "," + String(timeout);
    
    debugPrint("准备发送HTTP数据，长度: " + String(data.length()));
    
    // 发送数据长度命令
    AtResponse response = atCommandHandler.sendCommand(command, "DOWNLOAD", 5000);
    
    if (response.result != AT_RESULT_SUCCESS) {
        setError("HTTP数据准备失败: " + response.response);
        return false;
    }
    
    // 发送实际数据
    response = atCommandHandler.sendRawData(data, timeout);
    
    if (response.result == AT_RESULT_SUCCESS && response.response.indexOf("OK") != -1) {
        debugPrint("HTTP数据发送成功");
        return true;
    }
    
    setError("HTTP数据发送失败: " + response.response);
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
    
    AtResponse response = atCommandHandler.sendCommandWithFullResponse(command, 10000);
    
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