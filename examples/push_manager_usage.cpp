/**
 * @file push_manager_usage.cpp
 * @brief 推送管理器使用示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本示例展示如何使用推送管理器进行短信转发配置和测试
 */

#include <Arduino.h>
#include "../lib/push_manager/push_manager.h"
#include "../lib/database_manager/database_manager.h"

/**
 * @brief 创建企业微信转发规则示例
 */
void createWechatForwardRule() {
    Serial.println("\n=== 创建企业微信转发规则 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 创建企业微信转发规则
    ForwardRule wechatRule;
    wechatRule.ruleName = "企业微信默认转发";
    wechatRule.sourceNumber = ""; // 空表示匹配所有号码
    wechatRule.keywords = ""; // 空表示匹配所有内容
    wechatRule.pushType = "wechat";
    wechatRule.pushConfig = R"({
        "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY_HERE",
        "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
    })";
    wechatRule.enabled = true;
    wechatRule.isDefaultForward = true; // 设为默认转发
    
    int ruleId = db.addForwardRule(wechatRule);
    if (ruleId > 0) {
        Serial.printf("✅ 企业微信转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ 企业微信转发规则创建失败: " + db.getLastError());
    }
}

/**
 * @brief 创建钉钉转发规则示例
 */
void createDingTalkForwardRule() {
    Serial.println("\n=== 创建钉钉转发规则 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 创建钉钉转发规则（仅转发包含"重要"关键词的短信）
    ForwardRule dingRule;
    dingRule.ruleName = "钉钉重要消息转发";
    dingRule.sourceNumber = ""; // 匹配所有号码
    dingRule.keywords = "重要,紧急,警告"; // 包含这些关键词才转发
    dingRule.pushType = "dingtalk";
    dingRule.pushConfig = R"({
        "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN_HERE",
        "template": "🚨 重要短信通知\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
    })";
    dingRule.enabled = true;
    dingRule.isDefaultForward = false;
    
    int ruleId = db.addForwardRule(dingRule);
    if (ruleId > 0) {
        Serial.printf("✅ 钉钉转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ 钉钉转发规则创建失败: " + db.getLastError());
    }
}

/**
 * @brief 创建Webhook转发规则示例
 */
void createWebhookForwardRule() {
    Serial.println("\n=== 创建Webhook转发规则 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 创建Webhook转发规则（仅转发特定号码的短信）
    ForwardRule webhookRule;
    webhookRule.ruleName = "银行短信Webhook转发";
    webhookRule.sourceNumber = "95588,95533,95599"; // 仅转发这些银行号码的短信
    webhookRule.keywords = ""; // 匹配所有内容
    webhookRule.pushType = "webhook";
    webhookRule.pushConfig = R"({
        "webhook_url": "https://your-server.com/api/sms-webhook",
        "method": "POST",
        "content_type": "application/json",
        "headers": "Authorization:Bearer YOUR_TOKEN,X-Source:ESP-SMS-Relay",
        "body_template": "{\"type\":\"sms\",\"from\":\"{sender}\",\"content\":\"{content}\",\"timestamp\":\"{timestamp}\",\"sms_id\":\"{sms_id}\"}"
    })";
    webhookRule.enabled = true;
    webhookRule.isDefaultForward = false;
    
    int ruleId = db.addForwardRule(webhookRule);
    if (ruleId > 0) {
        Serial.printf("✅ Webhook转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ Webhook转发规则创建失败: " + db.getLastError());
    }
}

/**
 * @brief 测试推送配置
 */
void testPushConfigurations() {
    Serial.println("\n=== 测试推送配置 ===");
    
    PushManager& pushManager = PushManager::getInstance();
    pushManager.setDebugMode(true);
    
    if (!pushManager.initialize()) {
        Serial.println("❌ 推送管理器初始化失败: " + pushManager.getLastError());
        return;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    
    for (const auto& rule : rules) {
        if (rule.enabled) {
            Serial.printf("\n测试规则: %s (ID: %d)\n", rule.ruleName.c_str(), rule.id);
            
            PushResult result = pushManager.testPushConfig(rule.id, "这是一条测试消息，用于验证转发配置是否正常工作。");
            
            switch (result) {
                case PUSH_SUCCESS:
                    Serial.println("✅ 测试成功");
                    break;
                case PUSH_CONFIG_ERROR:
                    Serial.println("❌ 配置错误: " + pushManager.getLastError());
                    break;
                case PUSH_NETWORK_ERROR:
                    Serial.println("❌ 网络错误: " + pushManager.getLastError());
                    break;
                default:
                    Serial.println("❌ 测试失败: " + pushManager.getLastError());
                    break;
            }
        }
    }
}

/**
 * @brief 模拟短信转发测试
 */
void simulateSmsForward() {
    Serial.println("\n=== 模拟短信转发测试 ===");
    
    PushManager& pushManager = PushManager::getInstance();
    pushManager.setDebugMode(true);
    
    if (!pushManager.initialize()) {
        Serial.println("❌ 推送管理器初始化失败: " + pushManager.getLastError());
        return;
    }
    
    // 模拟不同类型的短信
    struct TestSms {
        String sender;
        String content;
        String description;
    };
    
    TestSms testMessages[] = {
        {"10086", "您的话费余额为100元", "普通短信"},
        {"95588", "您的账户发生一笔重要交易", "银行重要短信"},
        {"12306", "您的火车票预订成功", "普通通知短信"},
        {"10010", "紧急通知：您的套餐即将到期", "包含关键词的短信"}
    };
    
    for (int i = 0; i < 4; i++) {
        Serial.printf("\n测试短信 %d: %s\n", i + 1, testMessages[i].description.c_str());
        Serial.printf("发送方: %s\n", testMessages[i].sender.c_str());
        Serial.printf("内容: %s\n", testMessages[i].content.c_str());
        
        PushContext context;
        context.sender = testMessages[i].sender;
        context.content = testMessages[i].content;
        context.timestamp = "241201120000"; // 2024-12-01 12:00:00
        context.smsRecordId = i + 1;
        
        PushResult result = pushManager.processSmsForward(context);
        
        switch (result) {
            case PUSH_SUCCESS:
                Serial.println("✅ 转发成功");
                break;
            case PUSH_NO_RULE:
                Serial.println("ℹ️ 没有匹配的转发规则");
                break;
            case PUSH_RULE_DISABLED:
                Serial.println("ℹ️ 转发规则已禁用");
                break;
            default:
                Serial.println("❌ 转发失败: " + pushManager.getLastError());
                break;
        }
        
        delay(1000); // 间隔1秒
    }
}

/**
 * @brief 显示所有转发规则
 */
void listForwardRules() {
    Serial.println("\n=== 当前转发规则列表 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    
    if (rules.empty()) {
        Serial.println("没有配置任何转发规则");
        return;
    }
    
    for (const auto& rule : rules) {
        Serial.printf("\n规则ID: %d\n", rule.id);
        Serial.printf("规则名称: %s\n", rule.ruleName.c_str());
        Serial.printf("号码匹配: %s\n", rule.sourceNumber.isEmpty() ? "所有号码" : rule.sourceNumber.c_str());
        Serial.printf("关键词: %s\n", rule.keywords.isEmpty() ? "所有内容" : rule.keywords.c_str());
        Serial.printf("推送类型: %s\n", rule.pushType.c_str());
        Serial.printf("状态: %s\n", rule.enabled ? "启用" : "禁用");
        Serial.printf("默认转发: %s\n", rule.isDefaultForward ? "是" : "否");
        Serial.println("配置: " + rule.pushConfig);
        Serial.println("---");
    }
}

/**
 * @brief 推送管理器使用示例主函数
 */
void demonstratePushManager() {
    Serial.println("\n========== 推送管理器使用示例 ==========");
    
    // 初始化数据库
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.initialize()) {
        Serial.println("❌ 数据库初始化失败: " + db.getLastError());
        return;
    }
    
    // 创建示例转发规则
    createWechatForwardRule();
    createDingTalkForwardRule();
    createWebhookForwardRule();
    
    // 显示所有规则
    listForwardRules();
    
    // 测试推送配置（注意：需要有效的webhook地址才能成功）
    // testPushConfigurations();
    
    // 模拟短信转发测试
    simulateSmsForward();
    
    Serial.println("\n========== 示例演示完成 ==========");
}

// 如果作为独立程序运行
#ifdef PUSH_MANAGER_DEMO_STANDALONE
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    demonstratePushManager();
}

void loop() {
    // 空循环
    delay(1000);
}
#endif