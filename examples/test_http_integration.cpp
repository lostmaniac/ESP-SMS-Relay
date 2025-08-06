/**
 * @file test_http_integration.cpp
 * @brief HTTP客户端集成测试示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件演示如何在主程序中集成和使用HTTP客户端模块
 */

#include <Arduino.h>
#include "module_manager.h"
#include "http_client.h"
#include "at_command_handler.h"
#include "gsm_service.h"

/**
 * @brief HTTP客户端集成测试函数
 * 
 * 演示如何通过模块管理器使用HTTP客户端
 */
void testHttpClientIntegration() {
    Serial.println("\n=== HTTP客户端集成测试 ===");
    
    // 获取模块管理器实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    
    // 初始化所有模块
    Serial.println("初始化所有模块...");
    if (!moduleManager.initializeAllModules()) {
        Serial.println("模块初始化失败: " + moduleManager.getLastError());
        return;
    }
    
    // 检查模块状态
    if (moduleManager.getModuleStatus(MODULE_HTTP_CLIENT) != MODULE_READY) {
        Serial.println("HTTP客户端模块未就绪");
        return;
    }
    
    // 获取HTTP客户端实例
    HttpClient* httpClient = moduleManager.getHttpClient();
    if (!httpClient) {
        Serial.println("无法获取HTTP客户端实例");
        return;
    }
    
    // 启用调试模式
    httpClient->setDebugMode(true);
    
    Serial.println("HTTP客户端模块集成测试完成");
}

/**
 * @brief 执行简单的HTTP GET请求测试
 */
void testSimpleHttpGet() {
    Serial.println("\n=== 简单HTTP GET请求测试 ===");
    
    // 获取HTTP客户端实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    HttpClient* httpClient = moduleManager.getHttpClient();
    
    if (!httpClient) {
        Serial.println("HTTP客户端未初始化");
        return;
    }
    
    // 执行GET请求
    String url = "http://httpbin.org/get";
    Serial.println("发送GET请求到: " + url);
    
    HttpResponse response = httpClient->get(url);
    
    // 检查响应
    if (response.success) {
        Serial.println("GET请求成功!");
        Serial.println("状态码: " + String(response.statusCode));
        Serial.println("响应长度: " + String(response.body.length()));
        
        // 显示部分响应内容
        if (response.body.length() > 0) {
            Serial.println("响应内容(前100字符):");
            Serial.println(response.body.substring(0, min(100, (int)response.body.length())));
        }
    } else {
        Serial.println("GET请求失败: " + response.errorMessage);
    }
}

/**
 * @brief 执行HTTP POST请求测试
 */
void testSimpleHttpPost() {
    Serial.println("\n=== 简单HTTP POST请求测试 ===");
    
    // 获取HTTP客户端实例
    ModuleManager& moduleManager = ModuleManager::getInstance();
    HttpClient* httpClient = moduleManager.getHttpClient();
    
    if (!httpClient) {
        Serial.println("HTTP客户端未初始化");
        return;
    }
    
    // 准备POST数据
    String url = "http://httpbin.org/post";
    String postData = "{\"message\":\"Hello from ESP32\",\"timestamp\":" + String(millis()) + "}";
    
    Serial.println("发送POST请求到: " + url);
    Serial.println("POST数据: " + postData);
    
    // 执行POST请求
    HttpResponse response = httpClient->post(url, postData);
    
    // 检查响应
    if (response.success) {
        Serial.println("POST请求成功!");
        Serial.println("状态码: " + String(response.statusCode));
        Serial.println("响应长度: " + String(response.body.length()));
        
        // 显示部分响应内容
        if (response.body.length() > 0) {
            Serial.println("响应内容(前100字符):");
            Serial.println(response.body.substring(0, min(100, (int)response.body.length())));
        }
    } else {
        Serial.println("POST请求失败: " + response.errorMessage);
    }
}

/**
 * @brief 主测试函数
 * 
 * 按顺序执行所有HTTP客户端测试
 */
void runHttpClientTests() {
    Serial.println("\n========================================");
    Serial.println("      HTTP客户端集成测试开始");
    Serial.println("========================================");
    
    // 等待系统稳定
    delay(2000);
    
    // 1. 集成测试
    testHttpClientIntegration();
    delay(1000);
    
    // 2. GET请求测试
    testSimpleHttpGet();
    delay(2000);
    
    // 3. POST请求测试
    testSimpleHttpPost();
    delay(2000);
    
    Serial.println("\n========================================");
    Serial.println("      HTTP客户端集成测试完成");
    Serial.println("========================================");
}

/**
 * @brief 示例主函数
 * 
 * 注意：这个函数不会被自动调用，需要在主程序中手动调用
 */
void httpClientIntegrationExample() {
    // 运行HTTP客户端测试
    runHttpClientTests();
    
    // 可以在这里添加更多测试...
}