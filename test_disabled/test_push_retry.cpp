/**
 * @file test_push_retry.cpp
 * @brief 推送重试机制测试
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 测试推送管理器的重试机制和垃圾回收功能
 */

#include <Arduino.h>
#include "../lib/push_manager/push_manager.h"
#include "../lib/push_manager/push_channel_registry.h"
#include "../lib/push_manager/push_channel_base.h"
#include "../include/constants.h"

/**
 * @class MockFailingChannel
 * @brief 模拟失败的推送渠道，用于测试重试机制
 */
class MockFailingChannel : public PushChannelBase {
public:
    MockFailingChannel() : failCount(0), maxFails(2) {}
    
    String getChannelName() const override {
        return "mock_failing";
    }
    
    String getChannelDescription() const override {
        return "模拟失败的推送渠道，用于测试重试机制";
    }
    
    PushResult push(const String& config, const PushContext& context) override {
        failCount++;
        
        if (debugMode) {
            Serial.println("MockFailingChannel::push - 尝试 " + String(failCount));
        }
        
        // 前maxFails次失败，之后成功
        if (failCount <= maxFails) {
            setError("模拟推送失败 (尝试 " + String(failCount) + ")");
            return PUSH_FAILED;
        } else {
            if (debugMode) {
                Serial.println("MockFailingChannel::push - 成功 (尝试 " + String(failCount) + ")");
            }
            return PUSH_SUCCESS;
        }
    }
    
    PushChannelExample getConfigExample() const override {
        PushChannelExample example;
        example.configExample = "{\"test\": \"mock\"}";
        example.usage = "测试用模拟渠道";
        return example;
    }
    
    PushChannelHelp getHelp() const override {
        PushChannelHelp help;
        help.channelName = "mock_failing";
        help.description = "模拟失败的推送渠道";
        help.configFields = "无需配置";
        help.ruleExample = "测试规则";
        help.troubleshooting = "这是测试渠道";
        return help;
    }
    
    String getCliDemo() const override {
        return "mock_failing 测试命令";
    }
    
    void resetFailCount() {
        failCount = 0;
    }
    
    void setMaxFails(int max) {
        maxFails = max;
    }
    
    int getFailCount() const {
        return failCount;
    }
    
private:
    int failCount;  ///< 失败次数计数
    int maxFails;   ///< 最大失败次数
};

/**
 * @class MockAlwaysFailChannel
 * @brief 模拟总是失败的推送渠道，用于测试重试上限
 */
class MockAlwaysFailChannel : public PushChannelBase {
public:
    MockAlwaysFailChannel() : attemptCount(0) {}
    
    String getChannelName() const override {
        return "mock_always_fail";
    }
    
    String getChannelDescription() const override {
        return "模拟总是失败的推送渠道，用于测试重试上限";
    }
    
    PushResult push(const String& config, const PushContext& context) override {
        attemptCount++;
        
        if (debugMode) {
            Serial.println("MockAlwaysFailChannel::push - 尝试 " + String(attemptCount) + " (总是失败)");
        }
        
        setError("模拟推送总是失败 (尝试 " + String(attemptCount) + ")");
        return PUSH_FAILED;
    }
    
    PushChannelExample getConfigExample() const override {
        PushChannelExample example;
        example.configExample = "{\"test\": \"always_fail\"}";
        example.usage = "测试用总是失败的渠道";
        return example;
    }
    
    PushChannelHelp getHelp() const override {
        PushChannelHelp help;
        help.channelName = "mock_always_fail";
        help.description = "模拟总是失败的推送渠道";
        help.configFields = "无需配置";
        help.ruleExample = "测试规则";
        help.troubleshooting = "这是测试渠道，总是失败";
        return help;
    }
    
    String getCliDemo() const override {
        return "mock_always_fail 测试命令";
    }
    
    void resetAttemptCount() {
        attemptCount = 0;
    }
    
    int getAttemptCount() const {
        return attemptCount;
    }
    
private:
    int attemptCount;  ///< 尝试次数计数
};

// 全局测试渠道实例
static MockFailingChannel* g_mockFailingChannel = nullptr;
static MockAlwaysFailChannel* g_mockAlwaysFailChannel = nullptr;

/**
 * @brief 注册测试用的模拟推送渠道
 */
void registerMockChannels() {
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    
    // 创建模拟渠道实例
    g_mockFailingChannel = new MockFailingChannel();
    g_mockAlwaysFailChannel = new MockAlwaysFailChannel();
    
    // 注册到注册器
    registry.registerChannel("mock_failing", []() -> std::unique_ptr<PushChannelBase> {
        return std::make_unique<MockFailingChannel>(*g_mockFailingChannel);
    });
    
    registry.registerChannel("mock_always_fail", []() -> std::unique_ptr<PushChannelBase> {
        return std::make_unique<MockAlwaysFailChannel>(*g_mockAlwaysFailChannel);
    });
    
    Serial.println("✅ 模拟推送渠道注册完成");
}

/**
 * @brief 清理测试用的模拟推送渠道
 */
void cleanupMockChannels() {
    if (g_mockFailingChannel) {
        delete g_mockFailingChannel;
        g_mockFailingChannel = nullptr;
    }
    
    if (g_mockAlwaysFailChannel) {
        delete g_mockAlwaysFailChannel;
        g_mockAlwaysFailChannel = nullptr;
    }
    
    Serial.println("✅ 模拟推送渠道清理完成");
}

/**
 * @brief 测试推送重试机制 - 最终成功的情况
 * @return bool 测试是否通过
 */
bool testPushRetrySuccess() {
    Serial.println("\n=== 测试推送重试机制 - 最终成功 ===");
    
    PushManager& pushManager = PushManager::getInstance();
    pushManager.setDebugMode(true);
    
    // 重置失败计数
    if (g_mockFailingChannel) {
        g_mockFailingChannel->resetFailCount();
        g_mockFailingChannel->setMaxFails(2); // 前2次失败，第3次成功
    }
    
    // 创建测试上下文
    PushContext context;
    context.sender = "测试发送方";
    context.content = "测试重试机制的消息内容";
    context.timestamp = "240101120000";
    context.smsRecordId = -1;
    
    // 执行推送
    unsigned long startTime = millis();
    PushResult result = pushManager.pushToChannel("mock_failing", "{\"test\": \"retry\"}", context);
    unsigned long endTime = millis();
    
    Serial.println("推送结果: " + String(result == PUSH_SUCCESS ? "成功" : "失败"));
    Serial.println("耗时: " + String(endTime - startTime) + "ms");
    
    if (result != PUSH_SUCCESS) {
        Serial.println("❌ 推送应该最终成功，但结果为失败");
        Serial.println("错误信息: " + pushManager.getLastError());
        return false;
    }
    
    // 验证重试次数
    if (g_mockFailingChannel && g_mockFailingChannel->getFailCount() != 3) {
        Serial.println("❌ 预期重试3次，实际重试 " + String(g_mockFailingChannel->getFailCount()) + " 次");
        return false;
    }
    
    // 验证耗时（应该包含重试延迟）
    unsigned long expectedMinTime = 2 * PUSH_RETRY_DELAY_MS; // 2次重试延迟
    if (endTime - startTime < expectedMinTime) {
        Serial.println("❌ 耗时过短，可能没有正确执行重试延迟");
        return false;
    }
    
    Serial.println("✅ 推送重试机制测试通过 - 最终成功");
    return true;
}

/**
 * @brief 测试推送重试机制 - 最终失败的情况
 * @return bool 测试是否通过
 */
bool testPushRetryFailure() {
    Serial.println("\n=== 测试推送重试机制 - 最终失败 ===");
    
    PushManager& pushManager = PushManager::getInstance();
    pushManager.setDebugMode(true);
    
    // 重置尝试计数
    if (g_mockAlwaysFailChannel) {
        g_mockAlwaysFailChannel->resetAttemptCount();
    }
    
    // 创建测试上下文
    PushContext context;
    context.sender = "测试发送方";
    context.content = "测试重试上限的消息内容";
    context.timestamp = "240101120000";
    context.smsRecordId = -1;
    
    // 执行推送
    unsigned long startTime = millis();
    PushResult result = pushManager.pushToChannel("mock_always_fail", "{\"test\": \"always_fail\"}", context);
    unsigned long endTime = millis();
    
    Serial.println("推送结果: " + String(result == PUSH_SUCCESS ? "成功" : "失败"));
    Serial.println("耗时: " + String(endTime - startTime) + "ms");
    
    if (result == PUSH_SUCCESS) {
        Serial.println("❌ 推送应该最终失败，但结果为成功");
        return false;
    }
    
    // 验证重试次数
    if (g_mockAlwaysFailChannel && g_mockAlwaysFailChannel->getAttemptCount() != MAX_PUSH_RETRY_COUNT) {
        Serial.println("❌ 预期重试 " + String(MAX_PUSH_RETRY_COUNT) + " 次，实际重试 " + String(g_mockAlwaysFailChannel->getAttemptCount()) + " 次");
        return false;
    }
    
    // 验证错误信息包含重试信息
    String lastError = pushManager.getLastError();
    if (lastError.indexOf(String(MAX_PUSH_RETRY_COUNT) + "次重试后") == -1) {
        Serial.println("❌ 错误信息应该包含重试次数信息");
        Serial.println("实际错误信息: " + lastError);
        return false;
    }
    
    // 验证耗时（应该包含重试延迟）
    unsigned long expectedMinTime = (MAX_PUSH_RETRY_COUNT - 1) * PUSH_RETRY_DELAY_MS;
    if (endTime - startTime < expectedMinTime) {
        Serial.println("❌ 耗时过短，可能没有正确执行重试延迟");
        return false;
    }
    
    Serial.println("✅ 推送重试机制测试通过 - 最终失败");
    return true;
}

/**
 * @brief 测试内存垃圾回收
 * @return bool 测试是否通过
 */
bool testMemoryGarbageCollection() {
    Serial.println("\n=== 测试内存垃圾回收 ===");
    
    // 获取初始内存状态
    size_t initialFreeHeap = ESP.getFreeHeap();
    Serial.println("初始可用堆内存: " + String(initialFreeHeap) + " bytes");
    
    PushManager& pushManager = PushManager::getInstance();
    pushManager.setDebugMode(false); // 关闭调试输出以减少干扰
    
    // 执行多次推送操作，测试内存是否正确释放
    for (int i = 0; i < 10; i++) {
        PushContext context;
        context.sender = "测试发送方" + String(i);
        context.content = "测试垃圾回收的消息内容 " + String(i);
        context.timestamp = "240101120000";
        context.smsRecordId = -1;
        
        // 使用总是失败的渠道，确保会进行重试和垃圾回收
        if (g_mockAlwaysFailChannel) {
            g_mockAlwaysFailChannel->resetAttemptCount();
        }
        
        PushResult result = pushManager.pushToChannel("mock_always_fail", "{\"test\": \"gc_test\"}", context);
        
        // 每次操作后检查内存
        size_t currentFreeHeap = ESP.getFreeHeap();
        Serial.println("第 " + String(i + 1) + " 次操作后可用堆内存: " + String(currentFreeHeap) + " bytes");
    }
    
    // 强制垃圾回收
    delay(100);
    
    // 获取最终内存状态
    size_t finalFreeHeap = ESP.getFreeHeap();
    Serial.println("最终可用堆内存: " + String(finalFreeHeap) + " bytes");
    
    // 计算内存差异
    int memoryDiff = (int)initialFreeHeap - (int)finalFreeHeap;
    Serial.println("内存差异: " + String(memoryDiff) + " bytes");
    
    // 允许一定的内存差异（由于系统开销），但不应该有大量内存泄漏
    if (memoryDiff > 5000) { // 允许5KB的差异
        Serial.println("❌ 可能存在内存泄漏，内存差异过大: " + String(memoryDiff) + " bytes");
        return false;
    }
    
    Serial.println("✅ 内存垃圾回收测试通过");
    return true;
}

/**
 * @brief 运行所有推送重试机制测试
 * @return bool 所有测试是否通过
 */
bool runPushRetryTests() {
    Serial.println("\n========== 推送重试机制测试开始 ==========");
    
    // 初始化推送管理器
    PushManager& pushManager = PushManager::getInstance();
    if (!pushManager.initialize()) {
        Serial.println("❌ 推送管理器初始化失败");
        return false;
    }
    
    // 注册测试用的模拟渠道
    registerMockChannels();
    
    bool allTestsPassed = true;
    
    // 运行各项测试
    allTestsPassed &= testPushRetrySuccess();
    allTestsPassed &= testPushRetryFailure();
    allTestsPassed &= testMemoryGarbageCollection();
    
    // 清理测试资源
    cleanupMockChannels();
    
    if (allTestsPassed) {
        Serial.println("\n✅ 所有推送重试机制测试通过");
    } else {
        Serial.println("\n❌ 部分推送重试机制测试失败");
    }
    
    Serial.println("========== 推送重试机制测试结束 ==========\n");
    
    return allTestsPassed;
}

/**
 * @brief 演示推送重试机制
 */
void demoPushRetry() {
    Serial.println("\n========== 推送重试机制演示 ==========");
    
    // 运行测试
    runPushRetryTests();
    
    Serial.println("========== 推送重试机制演示结束 ==========\n");
}