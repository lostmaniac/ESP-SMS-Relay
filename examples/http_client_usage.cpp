/**
 * @file http_client_usage.cpp
 * @brief HTTP客户端使用示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本文件展示了如何在主程序中使用HTTP客户端模块进行HTTP/HTTPS请求
 */

#include <Arduino.h>
#include "module_manager.h"
#include "http_client.h"
#include "http_client_example.h"

/**
 * @brief HTTP客户端使用示例函数
 * @details 展示如何使用HTTP客户端进行各种类型的请求
 */
void httpClientUsageExample() {
    Serial.println("\n=== HTTP客户端使用示例 ===");
    
    // 获取HTTP客户端实例
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("错误: HTTP客户端未初始化");
        return;
    }
    
    // 创建示例管理器
    HttpClientExample example(*httpClient);
    
    // 初始化示例
    if (!example.initialize()) {
        Serial.println("错误: HTTP客户端示例初始化失败");
        return;
    }
    
    // 检查网络状态
    Serial.println("\n--- 检查网络状态 ---");
    if (!example.checkNetworkStatus()) {
        Serial.println("警告: 网络状态检查失败，但继续执行示例");
    }
    
    // 执行简单GET请求示例
    Serial.println("\n--- 简单GET请求示例 ---");
    example.simpleGetRequest();
    
    delay(2000); // 等待2秒
    
    // 执行带请求头的GET请求示例
    Serial.println("\n--- 带请求头的GET请求示例 ---");
    example.getRequestWithHeaders();
    
    delay(2000); // 等待2秒
    
    // 执行简单POST请求示例
    Serial.println("\n--- 简单POST请求示例 ---");
    example.simplePostRequest();
    
    delay(2000); // 等待2秒
    
    // 执行JSON POST请求示例
    Serial.println("\n--- JSON POST请求示例 ---");
    example.jsonPostRequest();
    
    delay(2000); // 等待2秒
    
    // 执行HTTPS GET请求示例
    Serial.println("\n--- HTTPS GET请求示例 ---");
    example.httpsGetRequest();
    
    delay(2000); // 等待2秒
    
    // 执行HTTPS POST请求示例
    Serial.println("\n--- HTTPS POST请求示例 ---");
    example.httpsPostRequest();
    
    delay(2000); // 等待2秒
    
    // 执行错误处理示例
    Serial.println("\n--- 错误处理示例 ---");
    example.errorHandlingExample();
    
    Serial.println("\n=== HTTP客户端使用示例完成 ===");
}

/**
 * @brief 自定义HTTP请求示例
 * @details 展示如何创建自定义的HTTP请求
 */
void customHttpRequestExample() {
    Serial.println("\n=== 自定义HTTP请求示例 ===");
    
    // 获取HTTP客户端实例
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("错误: HTTP客户端未初始化");
        return;
    }
    
    // 创建自定义GET请求
    HttpRequest getRequest;
    getRequest.method = HTTP_GET;
    getRequest.url = "http://httpbin.org/get";
    getRequest.headers.push_back({"User-Agent", "ESP32-SMS-Relay/1.0"});
    getRequest.headers.push_back({"Accept", "application/json"});
    
    Serial.println("发送自定义GET请求...");
    HttpResponse getResponse = httpClient->sendRequest(getRequest);
    
    if (getResponse.error == HTTP_SUCCESS) {
        Serial.printf("GET请求成功! 状态码: %d\n", getResponse.statusCode);
        Serial.printf("响应长度: %d字节\n", getResponse.body.length());
        Serial.println("响应内容:");
        Serial.println(getResponse.body);
    } else {
        Serial.printf("GET请求失败: %s\n", httpClient->getErrorString(getResponse.error).c_str());
    }
    
    delay(3000); // 等待3秒
    
    // 创建自定义POST请求
    HttpRequest postRequest;
    postRequest.method = HTTP_POST;
    postRequest.url = "http://httpbin.org/post";
    postRequest.headers.push_back({"Content-Type", "application/json"});
    postRequest.headers.push_back({"User-Agent", "ESP32-SMS-Relay/1.0"});
    postRequest.body = "{\"message\":\"Hello from ESP32\",\"timestamp\":" + String(millis()) + "}";
    
    Serial.println("\n发送自定义POST请求...");
    HttpResponse postResponse = httpClient->sendRequest(postRequest);
    
    if (postResponse.error == HTTP_SUCCESS) {
        Serial.printf("POST请求成功! 状态码: %d\n", postResponse.statusCode);
        Serial.printf("响应长度: %d字节\n", postResponse.body.length());
        Serial.println("响应内容:");
        Serial.println(postResponse.body);
    } else {
        Serial.printf("POST请求失败: %s\n", httpClient->getErrorString(postResponse.error).c_str());
    }
    
    Serial.println("\n=== 自定义HTTP请求示例完成 ===");
}

/**
 * @brief 在主程序中集成HTTP客户端的示例
 * @details 展示如何在setup()和loop()函数中使用HTTP客户端
 */
void integrateHttpClientInMain() {
    Serial.println("\n=== 主程序集成示例 ===");
    Serial.println("以下代码可以添加到您的main.cpp文件中:");
    Serial.println();
    
    Serial.println("// 在setup()函数中:");
    Serial.println("void setup() {");
    Serial.println("    Serial.begin(115200);");
    Serial.println("    ");
    Serial.println("    // 初始化模块管理器");
    Serial.println("    ModuleManager& moduleManager = ModuleManager::getInstance();");
    Serial.println("    if (!moduleManager.initializeAllModules()) {");
    Serial.println("        Serial.println(\"模块初始化失败: \" + moduleManager.getLastError());");
    Serial.println("        return;");
    Serial.println("    }");
    Serial.println("    ");
    Serial.println("    // 运行HTTP客户端示例");
    Serial.println("    httpClientUsageExample();");
    Serial.println("}");
    Serial.println();
    
    Serial.println("// 在loop()函数中:");
    Serial.println("void loop() {");
    Serial.println("    // 定期发送HTTP请求的示例");
    Serial.println("    static unsigned long lastHttpRequest = 0;");
    Serial.println("    const unsigned long HTTP_INTERVAL = 60000; // 60秒");
    Serial.println("    ");
    Serial.println("    if (millis() - lastHttpRequest >= HTTP_INTERVAL) {");
    Serial.println("        HttpClient* httpClient = getHttpClient();");
    Serial.println("        if (httpClient) {");
    Serial.println("            HttpRequest request;");
    Serial.println("            request.method = HTTP_GET;");
    Serial.println("            request.url = \"http://api.example.com/status\";");
    Serial.println("            ");
    Serial.println("            HttpResponse response = httpClient->sendRequest(request);");
    Serial.println("            if (response.error == HTTP_SUCCESS) {");
    Serial.println("                Serial.println(\"定期HTTP请求成功\");");
    Serial.println("            }");
    Serial.println("        }");
    Serial.println("        lastHttpRequest = millis();");
    Serial.println("    }");
    Serial.println("    ");
    Serial.println("    delay(1000); // 等待1秒");
    Serial.println("}");
    
    Serial.println("\n=== 主程序集成示例完成 ===");
}