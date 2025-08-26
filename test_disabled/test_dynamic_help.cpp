/**
 * @file test_dynamic_help.cpp
 * @brief 动态帮助内容生成功能测试
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 测试TerminalManager的动态帮助内容生成功能
 */

#include <Arduino.h>
#include "../lib/terminal_manager/terminal_manager.h"
#include "../lib/push_manager/push_manager.h"

/**
 * @brief 测试动态帮助内容生成功能
 */
void testDynamicHelpGeneration() {
    Serial.println("\n=== 测试动态帮助内容生成功能 ===");
    
    // 初始化管理器
    TerminalManager& terminalManager = TerminalManager::getInstance();
    PushManager& pushManager = PushManager::getInstance();
    
    // 初始化
    if (!terminalManager.initialize()) {
        Serial.println("❌ TerminalManager初始化失败: " + terminalManager.getLastError());
        return;
    }
    
    if (!pushManager.initialize()) {
        Serial.println("❌ PushManager初始化失败: " + pushManager.getLastError());
        return;
    }
    
    Serial.println("✅ 管理器初始化成功");
    
    // 测试获取所有渠道帮助信息
    Serial.println("\n--- 测试获取所有渠道帮助信息 ---");
    std::vector<PushChannelHelp> helpList = pushManager.getAllChannelHelp();
    
    if (helpList.empty()) {
        Serial.println("⚠️  未找到可用的推送渠道帮助信息");
    } else {
        Serial.println("✅ 成功获取 " + String(helpList.size()) + " 个渠道的帮助信息:");
        for (const PushChannelHelp& help : helpList) {
            Serial.println("  - " + help.channelName + ": " + help.description);
        }
    }
    
    // 测试生成渠道帮助内容
    Serial.println("\n--- 测试生成渠道帮助内容 ---");
    String channelHelp = terminalManager.generateChannelHelp();
    Serial.println("生成的帮助内容:");
    Serial.println(channelHelp);
    
    // 测试生成渠道配置示例
    Serial.println("\n--- 测试生成渠道配置示例 ---");
    String channelExamples = terminalManager.generateChannelExamples();
    Serial.println("生成的配置示例:");
    Serial.println(channelExamples);
    
    // 测试完整的帮助命令
    Serial.println("\n--- 测试完整的帮助命令 ---");
    std::vector<String> helpArgs;
    terminalManager.executeHelpCommand(helpArgs);
    
    Serial.println("\n✅ 动态帮助内容生成功能测试完成");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("ESP-SMS-Relay 动态帮助内容生成测试");
    Serial.println("=====================================");
    
    testDynamicHelpGeneration();
}

void loop() {
    // 空循环
    delay(1000);
}