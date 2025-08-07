/**
 * @file main_integration_example.cpp
 * @brief 主程序集成推送管理器的示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本文件展示如何在主程序中集成新的推送管理器模块
 * 替代原有的硬编码转发逻辑
 */

#include <Arduino.h>
#include <WiFi.h>
#include "../lib/database_manager/database_manager.h"
#include "../lib/push_manager/push_manager.h"
#include "../lib/sms_handler/sms_handler.h"
#include "../lib/module_manager/module_manager.h"

// WiFi配置（示例）
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASSWORD = "Your_WiFi_Password";

// 全局对象实例
DatabaseManager* dbManager = nullptr;
PushManager* pushManager = nullptr;
SmsHandler* smsHandler = nullptr;
ModuleManager* moduleManager = nullptr;

/**
 * @brief 初始化WiFi连接
 * @return true 连接成功
 * @return false 连接失败
 */
bool initializeWiFi() {
    Serial.println("\n========== 初始化WiFi连接 ==========");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    const int maxAttempts = 20; // 最多尝试20次，每次500ms
    
    Serial.print("正在连接WiFi");
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✅ WiFi连接成功");
        Serial.printf("📶 SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("🌐 IP地址: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("📡 信号强度: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        Serial.println();
        Serial.println("❌ WiFi连接失败");
        Serial.printf("⚠️ 状态码: %d\n", WiFi.status());
        return false;
    }
}

/**
 * @brief 初始化数据库管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeDatabaseManager() {
    Serial.println("\n========== 初始化数据库管理器 ==========");
    
    dbManager = &DatabaseManager::getInstance();
    
    if (!dbManager->initialize()) {
        Serial.println("❌ 数据库初始化失败: " + dbManager->getLastError());
        return false;
    }
    
    Serial.println("✅ 数据库管理器初始化成功");
    
    // 显示数据库状态
    DatabaseInfo dbInfo = dbManager->getDatabaseInfo();
    Serial.printf("📊 数据库版本: %s\n", dbInfo.version.c_str());
    Serial.printf("📋 短信记录数: %d\n", dbInfo.smsCount);
    Serial.printf("🔄 转发规则数: %d\n", dbInfo.forwardRuleCount);
    
    return true;
}

/**
 * @brief 初始化推送管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializePushManager() {
    Serial.println("\n========== 初始化推送管理器 ==========");
    
    pushManager = &PushManager::getInstance();
    pushManager->setDebugMode(true); // 开启调试模式
    
    if (!pushManager->initialize()) {
        Serial.println("❌ 推送管理器初始化失败: " + pushManager->getLastError());
        return false;
    }
    
    Serial.println("✅ 推送管理器初始化成功");
    
    // 显示转发规则状态
    std::vector<ForwardRule> rules = dbManager->getAllForwardRules();
    int enabledCount = 0;
    
    for (const auto& rule : rules) {
        if (rule.enabled) {
            enabledCount++;
            Serial.printf("✅ 启用规则: %s\n", rule.ruleName.c_str());
        }
    }
    
    Serial.printf("📊 转发规则状态: %d/%d 已启用\n", enabledCount, (int)rules.size());
    
    if (enabledCount == 0) {
        Serial.println("⚠️ 没有启用的转发规则，短信将不会被转发");
    }
    
    return true;
}

/**
 * @brief 初始化短信处理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeSmsHandler() {
    Serial.println("\n========== 初始化短信处理器 ==========");
    
    smsHandler = new SmsHandler();
    
    if (!smsHandler) {
        Serial.println("❌ 短信处理器创建失败");
        return false;
    }
    
    Serial.println("✅ 短信处理器初始化成功");
    return true;
}

/**
 * @brief 初始化模块管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeModuleManager() {
    Serial.println("\n========== 初始化模块管理器 ==========");
    
    moduleManager = &ModuleManager::getInstance();
    
    if (!moduleManager->initialize()) {
        Serial.println("❌ 模块管理器初始化失败");
        return false;
    }
    
    Serial.println("✅ 模块管理器初始化成功");
    return true;
}

/**
 * @brief 测试推送功能
 * @return true 测试成功
 * @return false 测试失败
 */
bool testPushFunction() {
    Serial.println("\n========== 测试推送功能 ==========");
    
    if (!pushManager) {
        Serial.println("❌ 推送管理器未初始化");
        return false;
    }
    
    // 创建测试短信上下文
    PushContext testContext;
    testContext.sender = "10086";
    testContext.content = "这是一条测试短信，用于验证转发功能是否正常工作。";
    testContext.timestamp = "2024-01-01 12:00:00";
    testContext.smsId = "test_001";
    
    Serial.println("📤 发送测试短信转发...");
    Serial.printf("📞 发送方: %s\n", testContext.sender.c_str());
    Serial.printf("📄 内容: %s\n", testContext.content.c_str());
    
    PushResult result = pushManager->processSmsForward(testContext);
    
    switch (result) {
        case PushResult::SUCCESS:
            Serial.println("✅ 测试转发成功");
            return true;
            
        case PushResult::NO_MATCHING_RULES:
            Serial.println("ℹ️ 没有匹配的转发规则");
            return true; // 这也算正常情况
            
        case PushResult::RULES_DISABLED:
            Serial.println("⚠️ 转发规则已禁用");
            return true; // 这也算正常情况
            
        case PushResult::CONFIG_ERROR:
            Serial.println("❌ 转发配置错误: " + pushManager->getLastError());
            return false;
            
        case PushResult::NETWORK_ERROR:
            Serial.println("❌ 网络错误: " + pushManager->getLastError());
            return false;
            
        case PushResult::PUSH_FAILED:
            Serial.println("❌ 推送失败: " + pushManager->getLastError());
            return false;
            
        default:
            Serial.println("❌ 未知错误");
            return false;
    }
}

/**
 * @brief 处理接收到的短信数据
 * @param smsData 短信原始数据
 */
void processSmsData(const String& smsData) {
    if (!smsHandler) {
        Serial.println("❌ 短信处理器未初始化");
        return;
    }
    
    Serial.println("\n📨 处理新短信数据:");
    Serial.println(smsData);
    
    // 使用短信处理器处理数据
    // 注意：这里假设smsHandler有processLine方法
    // 实际使用时需要根据smsHandler的具体接口调整
    smsHandler->processLine(smsData);
}

/**
 * @brief 显示系统状态
 */
void showSystemStatus() {
    Serial.println("\n========== 系统状态 ==========");
    
    // WiFi状态
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("📶 WiFi: 已连接 (%s)\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("📶 WiFi: 未连接");
    }
    
    // 数据库状态
    if (dbManager && dbManager->isConnected()) {
        DatabaseInfo dbInfo = dbManager->getDatabaseInfo();
        Serial.printf("💾 数据库: 已连接 (短信:%d, 规则:%d)\n", 
                     dbInfo.smsCount, dbInfo.forwardRuleCount);
    } else {
        Serial.println("💾 数据库: 未连接");
    }
    
    // 推送管理器状态
    if (pushManager) {
        std::vector<ForwardRule> rules = dbManager->getAllForwardRules();
        int enabledCount = 0;
        for (const auto& rule : rules) {
            if (rule.enabled) enabledCount++;
        }
        Serial.printf("🔄 推送管理器: 正常 (启用规则:%d/%d)\n", 
                     enabledCount, (int)rules.size());
    } else {
        Serial.println("🔄 推送管理器: 未初始化");
    }
    
    // 内存状态
    Serial.printf("💾 可用内存: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("⏱️ 运行时间: %lu ms\n", millis());
    
    Serial.println("================================\n");
}

/**
 * @brief 清理资源
 */
void cleanup() {
    Serial.println("\n========== 清理资源 ==========");
    
    if (smsHandler) {
        delete smsHandler;
        smsHandler = nullptr;
        Serial.println("✅ 短信处理器已清理");
    }
    
    if (dbManager) {
        dbManager->close();
        Serial.println("✅ 数据库连接已关闭");
    }
    
    WiFi.disconnect();
    Serial.println("✅ WiFi连接已断开");
    
    Serial.println("🏁 资源清理完成");
}

/**
 * @brief 主程序初始化
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n🚀 ESP-SMS-Relay 启动中...");
    Serial.println("📱 集成推送管理器版本");
    
    bool initSuccess = true;
    
    // 1. 初始化WiFi
    if (!initializeWiFi()) {
        Serial.println("❌ WiFi初始化失败，某些功能可能无法使用");
        // WiFi失败不影响其他模块初始化
    }
    
    // 2. 初始化数据库管理器
    if (!initializeDatabaseManager()) {
        Serial.println("❌ 数据库初始化失败，系统无法正常工作");
        initSuccess = false;
    }
    
    // 3. 初始化推送管理器
    if (initSuccess && !initializePushManager()) {
        Serial.println("❌ 推送管理器初始化失败，转发功能将不可用");
        // 推送管理器失败不影响短信接收
    }
    
    // 4. 初始化短信处理器
    if (initSuccess && !initializeSmsHandler()) {
        Serial.println("❌ 短信处理器初始化失败，系统无法处理短信");
        initSuccess = false;
    }
    
    // 5. 初始化模块管理器
    if (initSuccess && !initializeModuleManager()) {
        Serial.println("❌ 模块管理器初始化失败，某些功能可能无法使用");
        // 模块管理器失败不影响核心功能
    }
    
    if (initSuccess) {
        Serial.println("\n🎉 系统初始化完成！");
        
        // 测试推送功能
        if (WiFi.status() == WL_CONNECTED && pushManager) {
            testPushFunction();
        }
        
        // 显示系统状态
        showSystemStatus();
        
        Serial.println("📡 等待短信数据...");
    } else {
        Serial.println("\n💥 系统初始化失败，请检查配置和连接");
        cleanup();
    }
}

/**
 * @brief 主循环
 */
void loop() {
    static unsigned long lastStatusCheck = 0;
    static unsigned long lastMemoryCheck = 0;
    const unsigned long STATUS_CHECK_INTERVAL = 60000; // 60秒
    const unsigned long MEMORY_CHECK_INTERVAL = 30000; // 30秒
    
    unsigned long currentTime = millis();
    
    // 检查串口数据（模拟短信数据接收）
    if (Serial.available()) {
        String smsData = Serial.readStringUntil('\n');
        smsData.trim();
        
        if (smsData.length() > 0) {
            processSmsData(smsData);
        }
    }
    
    // 定期显示系统状态
    if (currentTime - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        showSystemStatus();
        lastStatusCheck = currentTime;
    }
    
    // 定期检查内存使用情况
    if (currentTime - lastMemoryCheck >= MEMORY_CHECK_INTERVAL) {
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 10000) { // 少于10KB时警告
            Serial.printf("⚠️ 内存不足警告: %d bytes\n", freeHeap);
        }
        lastMemoryCheck = currentTime;
    }
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long lastReconnectAttempt = 0;
        if (currentTime - lastReconnectAttempt >= 30000) { // 30秒重连一次
            Serial.println("🔄 尝试重新连接WiFi...");
            WiFi.reconnect();
            lastReconnectAttempt = currentTime;
        }
    }
    
    // 短暂延时，避免CPU占用过高
    delay(100);
}

/**
 * @brief 程序结束时的清理工作
 */
void __attribute__((destructor)) programCleanup() {
    cleanup();
}