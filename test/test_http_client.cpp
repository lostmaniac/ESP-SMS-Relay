/**
 * @file test_http_client.cpp
 * @brief HTTP客户端模块测试程序
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本文件包含HTTP客户端模块的单元测试和集成测试
 */

#include <Arduino.h>
#include "module_manager.h"
#include "http_client.h"

/**
 * @brief HTTP客户端基础功能测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool testHttpClientBasicFunctionality() {
    Serial.println("\n=== HTTP客户端基础功能测试 ===");
    
    // 获取HTTP客户端实例
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return false;
    }
    
    Serial.println("✅ HTTP客户端实例获取成功");
    
    // 测试错误字符串转换
    String errorStr = httpClient->getErrorString(HTTP_SUCCESS);
    if (errorStr.length() > 0) {
        Serial.println("✅ 错误字符串转换功能正常");
    } else {
        Serial.println("❌ 错误字符串转换功能异常");
        return false;
    }
    
    // 测试调试模式设置
    httpClient->setDebugMode(true);
    Serial.println("✅ 调试模式设置成功");
    
    httpClient->setDebugMode(false);
    Serial.println("✅ 调试模式关闭成功");
    
    Serial.println("✅ HTTP客户端基础功能测试通过");
    return true;
}

/**
 * @brief HTTP GET请求测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool testHttpGetRequest() {
    Serial.println("\n=== HTTP GET请求测试 ===");
    
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return false;
    }
    
    // 启用调试模式以查看详细信息
    httpClient->setDebugMode(true);
    
    // 创建GET请求
    HttpRequest request;
    request.method = HTTP_GET;
    request.url = "http://httpbin.org/get";
    request.headers.push_back({"User-Agent", "ESP32-Test/1.0"});
    request.headers.push_back({"Accept", "application/json"});
    
    Serial.println("发送HTTP GET请求...");
    HttpResponse response = httpClient->sendRequest(request);
    
    // 关闭调试模式
    httpClient->setDebugMode(false);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.printf("✅ GET请求成功! 状态码: %d\n", response.statusCode);
        Serial.printf("✅ 响应长度: %d字节\n", response.contentLength);
        
        if (response.statusCode == 200) {
            Serial.println("✅ HTTP状态码正确");
        } else {
            Serial.printf("⚠️  警告: HTTP状态码异常: %d\n", response.statusCode);
        }
        
        if (response.body.length() > 0) {
            Serial.println("✅ 响应内容不为空");
            Serial.println("响应内容预览:");
            Serial.println(response.body.substring(0, 200) + "...");
        } else {
            Serial.println("⚠️  警告: 响应内容为空");
        }
        
        return true;
    } else {
        Serial.printf("❌ GET请求失败: %s\n", httpClient->getErrorString(response.error).c_str());
        Serial.printf("❌ 详细错误: %s\n", httpClient->getLastError().c_str());
        return false;
    }
}

/**
 * @brief HTTP POST请求测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool testHttpPostRequest() {
    Serial.println("\n=== HTTP POST请求测试 ===");
    
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return false;
    }
    
    // 启用调试模式
    httpClient->setDebugMode(true);
    
    // 创建POST请求
    HttpRequest request;
    request.method = HTTP_POST;
    request.url = "http://httpbin.org/post";
    request.headers.push_back({"Content-Type", "application/json"});
    request.headers.push_back({"User-Agent", "ESP32-Test/1.0"});
    request.body = "{\"test\":\"HTTP POST from ESP32\",\"timestamp\":" + String(millis()) + "}";
    
    Serial.println("发送HTTP POST请求...");
    Serial.println("请求体: " + request.body);
    
    HttpResponse response = httpClient->sendRequest(request);
    
    // 关闭调试模式
    httpClient->setDebugMode(false);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.printf("✅ POST请求成功! 状态码: %d\n", response.statusCode);
        Serial.printf("✅ 响应长度: %d字节\n", response.contentLength);
        
        if (response.statusCode == 200) {
            Serial.println("✅ HTTP状态码正确");
        } else {
            Serial.printf("⚠️  警告: HTTP状态码异常: %d\n", response.statusCode);
        }
        
        if (response.body.length() > 0) {
            Serial.println("✅ 响应内容不为空");
            Serial.println("响应内容预览:");
            Serial.println(response.body.substring(0, 200) + "...");
        } else {
            Serial.println("⚠️  警告: 响应内容为空");
        }
        
        return true;
    } else {
        Serial.printf("❌ POST请求失败: %s\n", httpClient->getErrorString(response.error).c_str());
        Serial.printf("❌ 详细错误: %s\n", httpClient->getLastError().c_str());
        return false;
    }
}

/**
 * @brief HTTPS请求测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool testHttpsRequest() {
    Serial.println("\n=== HTTPS请求测试 ===");
    
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return false;
    }
    
    // 启用调试模式
    httpClient->setDebugMode(true);
    
    // 创建HTTPS GET请求
    HttpRequest request;
    request.method = HTTP_GET;
    request.url = "https://httpbin.org/get";  // 注意使用https://
    request.headers.push_back({"User-Agent", "ESP32-Test/1.0"});
    
    Serial.println("发送HTTPS GET请求...");
    HttpResponse response = httpClient->sendRequest(request);
    
    // 关闭调试模式
    httpClient->setDebugMode(false);
    
    if (response.error == HTTP_SUCCESS) {
        Serial.printf("✅ HTTPS请求成功! 状态码: %d\n", response.statusCode);
        Serial.printf("✅ 响应长度: %d字节\n", response.contentLength);
        return true;
    } else {
        Serial.printf("❌ HTTPS请求失败: %s\n", httpClient->getErrorString(response.error).c_str());
        Serial.printf("❌ 详细错误: %s\n", httpClient->getLastError().c_str());
        
        // HTTPS可能因为证书问题失败，这在某些GSM模块上是正常的
        Serial.println("ℹ️  注意: HTTPS失败可能是由于GSM模块的SSL证书验证问题");
        return false;
    }
}

/**
 * @brief 便捷方法测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool testConvenienceMethods() {
    Serial.println("\n=== 便捷方法测试 ===");
    
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return false;
    }
    
    bool allTestsPassed = true;
    
    // 测试便捷GET方法
    Serial.println("测试便捷GET方法...");
    std::vector<HttpHeader> headers = {
        {"User-Agent", "ESP32-Test/1.0"},
        {"Accept", "application/json"}
    };
    
    HttpResponse getResponse = httpClient->get("http://httpbin.org/get", headers);
    if (getResponse.error == HTTP_SUCCESS) {
        Serial.println("✅ 便捷GET方法测试通过");
    } else {
        Serial.println("❌ 便捷GET方法测试失败");
        allTestsPassed = false;
    }
    
    delay(2000); // 等待2秒
    
    // 测试便捷POST方法
    Serial.println("测试便捷POST方法...");
    std::vector<HttpHeader> postHeaders = {
        {"Content-Type", "application/json"},
        {"User-Agent", "ESP32-Test/1.0"}
    };
    
    String postBody = "{\"test\":\"convenience POST method\"}";
    HttpResponse postResponse = httpClient->post("http://httpbin.org/post", postBody, postHeaders);
    
    if (postResponse.error == HTTP_SUCCESS) {
        Serial.println("✅ 便捷POST方法测试通过");
    } else {
        Serial.println("❌ 便捷POST方法测试失败");
        allTestsPassed = false;
    }
    
    return allTestsPassed;
}

/**
 * @brief 错误处理测试
 * @return true 测试通过
 * @return false 测试失败
 */
bool testErrorHandling() {
    Serial.println("\n=== 错误处理测试 ===");
    
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return false;
    }
    
    // 测试无效URL
    Serial.println("测试无效URL处理...");
    HttpRequest invalidRequest;
    invalidRequest.method = HTTP_GET;
    invalidRequest.url = "invalid-url";  // 无效URL
    
    HttpResponse response = httpClient->sendRequest(invalidRequest);
    if (response.error != HTTP_SUCCESS) {
        Serial.println("✅ 无效URL错误处理正确");
        Serial.printf("✅ 错误类型: %s\n", httpClient->getErrorString(response.error).c_str());
    } else {
        Serial.println("❌ 无效URL应该返回错误");
        return false;
    }
    
    // 测试不存在的域名
    Serial.println("测试不存在域名处理...");
    HttpRequest nonExistentRequest;
    nonExistentRequest.method = HTTP_GET;
    nonExistentRequest.url = "http://this-domain-does-not-exist-12345.com";
    
    HttpResponse nonExistentResponse = httpClient->sendRequest(nonExistentRequest);
    if (nonExistentResponse.error != HTTP_SUCCESS) {
        Serial.println("✅ 不存在域名错误处理正确");
        Serial.printf("✅ 错误类型: %s\n", httpClient->getErrorString(nonExistentResponse.error).c_str());
    } else {
        Serial.println("⚠️  警告: 不存在的域名请求意外成功");
    }
    
    Serial.println("✅ 错误处理测试完成");
    return true;
}

/**
 * @brief 运行所有HTTP客户端测试
 * @return true 所有测试通过
 * @return false 存在测试失败
 */
bool runAllHttpClientTests() {
    Serial.println("\n\n🚀 开始HTTP客户端模块测试");
    Serial.println("===========================================");
    
    bool allTestsPassed = true;
    int totalTests = 0;
    int passedTests = 0;
    
    // 基础功能测试
    totalTests++;
    if (testHttpClientBasicFunctionality()) {
        passedTests++;
    } else {
        allTestsPassed = false;
    }
    
    delay(2000);
    
    // HTTP GET请求测试
    totalTests++;
    if (testHttpGetRequest()) {
        passedTests++;
    } else {
        allTestsPassed = false;
    }
    
    delay(3000);
    
    // HTTP POST请求测试
    totalTests++;
    if (testHttpPostRequest()) {
        passedTests++;
    } else {
        allTestsPassed = false;
    }
    
    delay(3000);
    
    // HTTPS请求测试
    totalTests++;
    if (testHttpsRequest()) {
        passedTests++;
    } else {
        // HTTPS失败不算作致命错误
        Serial.println("ℹ️  HTTPS测试失败，但这可能是正常的");
    }
    
    delay(3000);
    
    // 便捷方法测试
    totalTests++;
    if (testConvenienceMethods()) {
        passedTests++;
    } else {
        allTestsPassed = false;
    }
    
    delay(2000);
    
    // 错误处理测试
    totalTests++;
    if (testErrorHandling()) {
        passedTests++;
    } else {
        allTestsPassed = false;
    }
    
    // 输出测试结果
    Serial.println("\n===========================================");
    Serial.printf("📊 测试结果: %d/%d 通过\n", passedTests, totalTests);
    
    if (allTestsPassed) {
        Serial.println("🎉 所有HTTP客户端测试通过!");
    } else {
        Serial.println("❌ 部分HTTP客户端测试失败");
    }
    
    Serial.println("===========================================");
    return allTestsPassed;
}

/**
 * @brief HTTP客户端示例演示
 */
void runHttpClientExamples() {
    Serial.println("\n\n🎯 HTTP客户端示例演示");
    Serial.println("===========================================");
    
    HttpClient* httpClient = getHttpClient();
    if (!httpClient) {
        Serial.println("❌ 错误: HTTP客户端未初始化");
        return;
    }
    
    // 创建示例管理器
    HttpClientExample example(*httpClient);
    
    // 初始化示例
    if (!example.initialize()) {
        Serial.println("❌ 错误: HTTP客户端示例初始化失败");
        return;
    }
    
    // 运行各种示例
    Serial.println("\n--- 网络状态检查示例 ---");
    example.checkNetworkStatus();
    
    delay(2000);
    
    Serial.println("\n--- 简单GET请求示例 ---");
    example.simpleGetRequest();
    
    delay(3000);
    
    Serial.println("\n--- 带请求头的GET请求示例 ---");
    example.getRequestWithHeaders();
    
    delay(3000);
    
    Serial.println("\n--- 简单POST请求示例 ---");
    example.simplePostRequest();
    
    delay(3000);
    
    Serial.println("\n--- JSON POST请求示例 ---");
    example.jsonPostRequest();
    
    delay(3000);
    
    Serial.println("\n--- 错误处理示例 ---");
    example.errorHandlingExample();
    
    Serial.println("\n🎯 HTTP客户端示例演示完成");
    Serial.println("===========================================");
}

/**
 * @brief 主测试函数
 * @details 在主程序中调用此函数来运行HTTP客户端测试
 */
void testHttpClientModule() {
    Serial.println("\n\n🔧 HTTP客户端模块完整测试");
    Serial.println("=============================================");
    
    // 检查模块管理器是否已初始化
    ModuleManager& moduleManager = ModuleManager::getInstance();
    if (!moduleManager.areAllModulesReady()) {
        Serial.println("❌ 错误: 模块管理器未完全初始化");
        Serial.println("请确保在调用此函数前已初始化所有模块");
        return;
    }
    
    // 运行单元测试
    bool testsPass = runAllHttpClientTests();
    
    delay(5000);
    
    // 运行示例演示
    runHttpClientExamples();
    
    // 最终结果
    Serial.println("\n=============================================");
    if (testsPass) {
        Serial.println("🎉 HTTP客户端模块测试完成 - 所有测试通过!");
    } else {
        Serial.println("⚠️  HTTP客户端模块测试完成 - 部分测试失败");
    }
    Serial.println("=============================================");
}