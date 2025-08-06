/**
 * @file http_client_example.cpp
 * @brief HTTP客户端使用示例实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "http_client_example.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
HttpClientExample::HttpClientExample() : 
    httpClient(nullptr),
    debugMode(true) {
    httpClient = &HttpClient::getInstance();
}

/**
 * @brief 析构函数
 */
HttpClientExample::~HttpClientExample() {
    // 清理资源
}

/**
 * @brief 运行所有示例
 * @return true 所有示例执行成功
 * @return false 有示例执行失败
 */
bool HttpClientExample::runAllExamples() {
    Serial.println("\n=== HTTP客户端示例开始 ===");
    
    // 初始化HTTP客户端
    if (!httpClient->initialize()) {
        Serial.println("HTTP客户端初始化失败: " + httpClient->getLastError());
        return false;
    }
    
    httpClient->setDebugMode(debugMode);
    
    bool allSuccess = true;
    
    // 网络状态检查
    if (!networkStatusExample()) {
        allSuccess = false;
    }
    
    // 简单GET请求
    if (!simpleGetExample()) {
        allSuccess = false;
    }
    
    // 带请求头的GET请求
    if (!getWithHeadersExample()) {
        allSuccess = false;
    }
    
    // 简单POST请求
    if (!simplePostExample()) {
        allSuccess = false;
    }
    
    // JSON POST请求
    if (!jsonPostExample()) {
        allSuccess = false;
    }
    
    // HTTPS GET请求
    if (!httpsGetExample()) {
        allSuccess = false;
    }
    
    // HTTPS POST请求
    if (!httpsPostExample()) {
        allSuccess = false;
    }
    
    // 错误处理示例
    if (!errorHandlingExample()) {
        allSuccess = false;
    }
    
    // 自定义请求示例
    if (!customRequestExample()) {
        allSuccess = false;
    }
    
    Serial.println("\n=== HTTP客户端示例结束 ===");
    return allSuccess;
}

/**
 * @brief 简单GET请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::simpleGetExample() {
    printExampleTitle("简单GET请求示例");
    
    String url = "http://httpbin.org/get";
    HttpResponse response = httpClient->get(url);
    
    printResponse(response, "简单GET请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief 带请求头的GET请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::getWithHeadersExample() {
    printExampleTitle("带请求头的GET请求示例");
    
    String url = "http://httpbin.org/headers";
    
    // 设置自定义请求头
    std::map<String, String> headers;
    headers["User-Agent"] = "ESP32-HTTP-Client/1.0";
    headers["Accept"] = "application/json";
    headers["X-Custom-Header"] = "ESP32-Test";
    
    HttpResponse response = httpClient->get(url, headers);
    
    printResponse(response, "带请求头的GET请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief 简单POST请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::simplePostExample() {
    printExampleTitle("简单POST请求示例");
    
    String url = "http://httpbin.org/post";
    String data = "key1=value1&key2=value2";
    
    // 设置Content-Type
    std::map<String, String> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    
    HttpResponse response = httpClient->post(url, data, headers);
    
    printResponse(response, "简单POST请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief JSON POST请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::jsonPostExample() {
    printExampleTitle("JSON POST请求示例");
    
    String url = "http://httpbin.org/post";
    String jsonData = "{\"name\":\"ESP32\",\"type\":\"microcontroller\",\"status\":\"active\"}";
    
    // 设置JSON Content-Type
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    headers["Accept"] = "application/json";
    
    HttpResponse response = httpClient->post(url, jsonData, headers);
    
    printResponse(response, "JSON POST请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief HTTPS GET请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::httpsGetExample() {
    printExampleTitle("HTTPS GET请求示例");
    
    String url = "https://httpbin.org/get";
    HttpResponse response = httpClient->get(url);
    
    printResponse(response, "HTTPS GET请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief HTTPS POST请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::httpsPostExample() {
    printExampleTitle("HTTPS POST请求示例");
    
    String url = "https://httpbin.org/post";
    String data = "secure_data=encrypted_value";
    
    std::map<String, String> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    headers["User-Agent"] = "ESP32-HTTPS-Client/1.0";
    
    HttpResponse response = httpClient->post(url, data, headers);
    
    printResponse(response, "HTTPS POST请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief 错误处理示例
 * @return true 示例执行完成
 * @return false 示例执行失败
 */
bool HttpClientExample::errorHandlingExample() {
    printExampleTitle("错误处理示例");
    
    // 测试无效URL
    Serial.println("测试无效URL...");
    HttpResponse response1 = httpClient->get("invalid-url");
    Serial.println("无效URL错误: " + getErrorDescription(response1.error));
    
    // 测试不存在的域名
    Serial.println("\n测试不存在的域名...");
    HttpResponse response2 = httpClient->get("http://nonexistent-domain-12345.com");
    Serial.println("域名不存在错误: " + getErrorDescription(response2.error));
    
    // 测试404错误
    Serial.println("\n测试404错误...");
    HttpResponse response3 = httpClient->get("http://httpbin.org/status/404");
    Serial.println("404错误状态码: " + String(response3.statusCode));
    Serial.println("错误类型: " + getErrorDescription(response3.error));
    
    return true;
}

/**
 * @brief 自定义请求示例
 * @return true 请求成功
 * @return false 请求失败
 */
bool HttpClientExample::customRequestExample() {
    printExampleTitle("自定义请求示例");
    
    // 创建自定义请求
    HttpRequest request;
    request.url = "http://httpbin.org/anything";
    request.method = HTTP_POST;
    request.protocol = HTTP_PROTOCOL;
    request.timeout = 15000; // 15秒超时
    
    // 设置自定义请求头
    request.headers["Authorization"] = "Bearer token123";
    request.headers["Content-Type"] = "application/json";
    request.headers["X-API-Version"] = "v1.0";
    
    // 设置请求体
    request.body = "{\"message\":\"Hello from ESP32\",\"timestamp\":" + String(millis()) + "}";
    
    HttpResponse response = httpClient->request(request);
    
    printResponse(response, "自定义请求");
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief 网络状态检查示例
 * @return true 检查完成
 * @return false 检查失败
 */
bool HttpClientExample::networkStatusExample() {
    printExampleTitle("网络状态检查示例");
    
    // 检查网络连接
    bool networkConnected = httpClient->isNetworkConnected();
    Serial.println("网络连接状态: " + String(networkConnected ? "已连接" : "未连接"));
    
    // 检查PDP上下文
    bool pdpActive = httpClient->isPdpContextActive();
    Serial.println("PDP上下文状态: " + String(pdpActive ? "已激活" : "未激活"));
    
    if (!networkConnected) {
        Serial.println("警告: 网络未连接，HTTP请求可能失败");
        return false;
    }
    
    if (!pdpActive) {
        Serial.println("尝试激活PDP上下文...");
        if (httpClient->activatePdpContext()) {
            Serial.println("PDP上下文激活成功");
        } else {
            Serial.println("PDP上下文激活失败: " + httpClient->getLastError());
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 打印HTTP响应信息
 * @param response HTTP响应
 * @param requestName 请求名称
 */
void HttpClientExample::printResponse(const HttpResponse& response, const String& requestName) {
    Serial.println("\n--- " + requestName + " 响应 ---");
    Serial.println("错误代码: " + getErrorDescription(response.error));
    Serial.println("状态码: " + String(response.statusCode));
    Serial.println("内容长度: " + String(response.contentLength));
    Serial.println("请求耗时: " + String(response.duration) + "ms");
    
    if (response.body.length() > 0) {
        Serial.println("响应内容 (前200字符):");
        String preview = response.body.substring(0, min(200, (int)response.body.length()));
        Serial.println(preview);
        if (response.body.length() > 200) {
            Serial.println("... (内容已截断)");
        }
    }
    
    printSeparator();
}

/**
 * @brief 获取错误描述
 * @param error 错误代码
 * @return String 错误描述
 */
String HttpClientExample::getErrorDescription(HttpClientError error) {
    switch (error) {
        case HTTP_SUCCESS: return "成功";
        case HTTP_ERROR_NETWORK: return "网络错误";
        case HTTP_ERROR_TIMEOUT: return "请求超时";
        case HTTP_ERROR_INIT: return "初始化失败";
        case HTTP_ERROR_INVALID_URL: return "无效URL";
        case HTTP_ERROR_SERVER: return "服务器错误";
        case HTTP_ERROR_AT_COMMAND: return "AT命令错误";
        default: return "未知错误";
    }
}

/**
 * @brief 打印示例标题
 * @param title 标题
 */
void HttpClientExample::printExampleTitle(const String& title) {
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println(title);
    Serial.println(String("=").substring(0, 50));
}

/**
 * @brief 打印分隔线
 */
void HttpClientExample::printSeparator() {
    Serial.println(String("-").substring(0, 30));
}

// 全局函数实现

/**
 * @brief 运行HTTP客户端示例
 * @return true 示例运行成功
 * @return false 示例运行失败
 */
bool runHttpClientExamples() {
    HttpClientExample example;
    return example.runAllExamples();
}

/**
 * @brief 测试HTTP GET请求
 * @param url 测试URL
 * @return true 测试成功
 * @return false 测试失败
 */
bool testHttpGet(const String& url) {
    HttpClient& client = HttpClient::getInstance();
    
    if (!client.initialize()) {
        Serial.println("HTTP客户端初始化失败");
        return false;
    }
    
    HttpResponse response = client.get(url);
    
    Serial.println("GET请求测试结果:");
    Serial.println("URL: " + url);
    Serial.println("状态码: " + String(response.statusCode));
    Serial.println("错误: " + String(response.error));
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief 测试HTTP POST请求
 * @param url 测试URL
 * @param data 发送数据
 * @return true 测试成功
 * @return false 测试失败
 */
bool testHttpPost(const String& url, const String& data) {
    HttpClient& client = HttpClient::getInstance();
    
    if (!client.initialize()) {
        Serial.println("HTTP客户端初始化失败");
        return false;
    }
    
    std::map<String, String> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    
    HttpResponse response = client.post(url, data, headers);
    
    Serial.println("POST请求测试结果:");
    Serial.println("URL: " + url);
    Serial.println("数据: " + data);
    Serial.println("状态码: " + String(response.statusCode));
    Serial.println("错误: " + String(response.error));
    
    return response.error == HTTP_SUCCESS;
}

/**
 * @brief 测试HTTPS请求
 * @param url 测试URL
 * @return true 测试成功
 * @return false 测试失败
 */
bool testHttpsRequest(const String& url) {
    HttpClient& client = HttpClient::getInstance();
    
    if (!client.initialize()) {
        Serial.println("HTTP客户端初始化失败");
        return false;
    }
    
    HttpResponse response = client.get(url);
    
    Serial.println("HTTPS请求测试结果:");
    Serial.println("URL: " + url);
    Serial.println("状态码: " + String(response.statusCode));
    Serial.println("错误: " + String(response.error));
    
    return response.error == HTTP_SUCCESS;
}