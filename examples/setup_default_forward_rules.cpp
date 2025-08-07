/**
 * @file setup_default_forward_rules.cpp
 * @brief 设置默认转发规则的示例脚本
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本脚本用于初始化系统时创建默认的转发规则
 * 可以在首次启动时调用，或者用于重置转发配置
 */

#include <Arduino.h>
#include "../lib/database_manager/database_manager.h"
#include "../lib/push_manager/push_manager.h"

/**
 * @brief 创建默认的企业微信转发规则
 * @return int 规则ID，-1表示失败
 */
int createDefaultWechatRule() {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    ForwardRule rule;
    rule.ruleName = "默认企业微信转发";
    rule.sourceNumber = ""; // 匹配所有号码
    rule.keywords = ""; // 匹配所有内容
    rule.pushType = "wechat";
    
    // 注意：请替换为您实际的企业微信机器人webhook地址
    rule.pushConfig = R"({
        "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_WECHAT_KEY_HERE",
        "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
    })";
    
    rule.enabled = false; // 默认禁用，需要用户配置正确的webhook后启用
    rule.isDefaultForward = true; // 设为默认转发规则
    
    int ruleId = db.addForwardRule(rule);
    if (ruleId > 0) {
        Serial.printf("✅ 默认企业微信转发规则创建成功，ID: %d\n", ruleId);
        Serial.println("⚠️ 请在数据库中更新webhook_url并启用此规则");
    } else {
        Serial.println("❌ 默认企业微信转发规则创建失败: " + db.getLastError());
    }
    
    return ruleId;
}

/**
 * @brief 创建银行短信专用转发规则
 * @return int 规则ID，-1表示失败
 */
int createBankSmsRule() {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    ForwardRule rule;
    rule.ruleName = "银行短信转发";
    rule.sourceNumber = "95588,95533,95599,95566,95595,95559,95568,95580,95561"; // 主要银行号码
    rule.keywords = ""; // 匹配所有银行短信内容
    rule.pushType = "wechat";
    
    rule.pushConfig = R"({
        "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_BANK_KEY_HERE",
        "template": "🏦 银行短信通知\n\n🏛️ 银行: {sender}\n🕐 时间: {timestamp}\n💰 内容: {content}"
    })";
    
    rule.enabled = false; // 默认禁用
    rule.isDefaultForward = false;
    
    int ruleId = db.addForwardRule(rule);
    if (ruleId > 0) {
        Serial.printf("✅ 银行短信转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ 银行短信转发规则创建失败: " + db.getLastError());
    }
    
    return ruleId;
}

/**
 * @brief 创建重要消息转发规则
 * @return int 规则ID，-1表示失败
 */
int createUrgentMessageRule() {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    ForwardRule rule;
    rule.ruleName = "重要消息转发";
    rule.sourceNumber = ""; // 匹配所有号码
    rule.keywords = "重要,紧急,警告,故障,异常,错误,失败,超时,断线,离线"; // 重要关键词
    rule.pushType = "wechat";
    
    rule.pushConfig = R"({
        "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_URGENT_KEY_HERE",
        "template": "🚨 重要短信通知\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n⚠️ 内容: {content}"
    })";
    
    rule.enabled = false; // 默认禁用
    rule.isDefaultForward = false;
    
    int ruleId = db.addForwardRule(rule);
    if (ruleId > 0) {
        Serial.printf("✅ 重要消息转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ 重要消息转发规则创建失败: " + db.getLastError());
    }
    
    return ruleId;
}

/**
 * @brief 创建验证码短信转发规则
 * @return int 规则ID，-1表示失败
 */
int createVerificationCodeRule() {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    ForwardRule rule;
    rule.ruleName = "验证码短信转发";
    rule.sourceNumber = ""; // 匹配所有号码
    rule.keywords = "验证码,动态码,校验码,确认码,安全码,登录码"; // 验证码关键词
    rule.pushType = "wechat";
    
    rule.pushConfig = R"({
        "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_VERIFICATION_KEY_HERE",
        "template": "🔐 验证码短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n🔑 内容: {content}"
    })";
    
    rule.enabled = false; // 默认禁用
    rule.isDefaultForward = false;
    
    int ruleId = db.addForwardRule(rule);
    if (ruleId > 0) {
        Serial.printf("✅ 验证码短信转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ 验证码短信转发规则创建失败: " + db.getLastError());
    }
    
    return ruleId;
}

/**
 * @brief 创建Webhook示例规则
 * @return int 规则ID，-1表示失败
 */
int createWebhookExampleRule() {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    ForwardRule rule;
    rule.ruleName = "Webhook API转发示例";
    rule.sourceNumber = ""; // 匹配所有号码
    rule.keywords = ""; // 匹配所有内容
    rule.pushType = "webhook";
    
    rule.pushConfig = R"({
        "webhook_url": "https://your-api-server.com/api/sms/receive",
        "method": "POST",
        "content_type": "application/json",
        "headers": "Authorization:Bearer YOUR_API_TOKEN,X-Source:ESP-SMS-Relay,X-Version:1.0",
        "body_template": "{\"event\":\"sms_received\",\"data\":{\"from\":\"{sender}\",\"content\":\"{content}\",\"timestamp\":\"{timestamp}\",\"sms_id\":\"{sms_id}\"}}"
    })";
    
    rule.enabled = false; // 默认禁用
    rule.isDefaultForward = false;
    
    int ruleId = db.addForwardRule(rule);
    if (ruleId > 0) {
        Serial.printf("✅ Webhook示例转发规则创建成功，ID: %d\n", ruleId);
    } else {
        Serial.println("❌ Webhook示例转发规则创建失败: " + db.getLastError());
    }
    
    return ruleId;
}

/**
 * @brief 检查是否已存在转发规则
 * @return true 已存在规则
 * @return false 不存在规则
 */
bool hasExistingRules() {
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    return !rules.empty();
}

/**
 * @brief 设置默认转发规则
 * @param forceRecreate 是否强制重新创建（删除现有规则）
 * @return true 设置成功
 * @return false 设置失败
 */
bool setupDefaultForwardRules(bool forceRecreate = false) {
    Serial.println("\n========== 设置默认转发规则 ==========");
    
    // 初始化数据库
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.initialize()) {
        Serial.println("❌ 数据库初始化失败: " + db.getLastError());
        return false;
    }
    
    // 检查是否已存在规则
    if (hasExistingRules() && !forceRecreate) {
        Serial.println("ℹ️ 已存在转发规则，跳过创建");
        Serial.println("💡 如需重新创建，请调用 setupDefaultForwardRules(true)");
        return true;
    }
    
    if (forceRecreate) {
        Serial.println("⚠️ 强制重新创建模式，将删除所有现有规则");
        // 注意：这里应该添加删除现有规则的代码
        // 为了安全起见，暂时不实现自动删除功能
    }
    
    Serial.println("开始创建默认转发规则...");
    
    int successCount = 0;
    int totalRules = 5;
    
    // 创建各种转发规则
    if (createDefaultWechatRule() > 0) successCount++;
    if (createBankSmsRule() > 0) successCount++;
    if (createUrgentMessageRule() > 0) successCount++;
    if (createVerificationCodeRule() > 0) successCount++;
    if (createWebhookExampleRule() > 0) successCount++;
    
    Serial.printf("\n📊 转发规则创建完成: %d/%d 成功\n", successCount, totalRules);
    
    if (successCount == totalRules) {
        Serial.println("✅ 所有默认转发规则创建成功！");
        Serial.println("\n📝 下一步操作:");
        Serial.println("1. 在数据库中更新各规则的webhook_url为实际地址");
        Serial.println("2. 启用需要的转发规则（设置enabled=1）");
        Serial.println("3. 根据需要调整规则的号码匹配和关键词过滤");
        Serial.println("4. 测试转发功能是否正常工作");
        return true;
    } else {
        Serial.println("⚠️ 部分转发规则创建失败，请检查错误信息");
        return false;
    }
}

/**
 * @brief 显示转发规则配置指南
 */
void showConfigurationGuide() {
    Serial.println("\n========== 转发规则配置指南 ==========");
    Serial.println("\n🔧 配置步骤:");
    Serial.println("\n1. 获取企业微信机器人Webhook地址:");
    Serial.println("   - 在企业微信群中添加机器人");
    Serial.println("   - 复制机器人的Webhook地址");
    Serial.println("   - 格式: https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY");
    
    Serial.println("\n2. 更新数据库中的转发规则:");
    Serial.println("   - 使用Web管理界面或数据库工具");
    Serial.println("   - 更新pushConfig字段中的webhook_url");
    Serial.println("   - 设置enabled=1启用规则");
    
    Serial.println("\n3. 测试转发功能:");
    Serial.println("   - 发送测试短信到设备");
    Serial.println("   - 检查企业微信群是否收到消息");
    Serial.println("   - 查看串口日志确认转发状态");
    
    Serial.println("\n4. 自定义规则:");
    Serial.println("   - 根据需要修改号码匹配模式");
    Serial.println("   - 调整关键词过滤条件");
    Serial.println("   - 自定义消息模板格式");
    
    Serial.println("\n📚 更多信息请参考: lib/push_manager/README.md");
    Serial.println("========================================\n");
}

/**
 * @brief 初始化推送管理器并测试
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeAndTestPushManager() {
    Serial.println("\n========== 初始化推送管理器 ==========");
    
    PushManager& pushManager = PushManager::getInstance();
    pushManager.setDebugMode(true);
    
    if (!pushManager.initialize()) {
        Serial.println("❌ 推送管理器初始化失败: " + pushManager.getLastError());
        return false;
    }
    
    Serial.println("✅ 推送管理器初始化成功");
    
    // 显示当前规则数量
    DatabaseManager& db = DatabaseManager::getInstance();
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    Serial.printf("📋 当前共有 %d 条转发规则\n", rules.size());
    
    int enabledCount = 0;
    for (const auto& rule : rules) {
        if (rule.enabled) {
            enabledCount++;
            Serial.printf("✅ 启用规则: %s (ID: %d)\n", rule.ruleName.c_str(), rule.id);
        } else {
            Serial.printf("⏸️ 禁用规则: %s (ID: %d)\n", rule.ruleName.c_str(), rule.id);
        }
    }
    
    Serial.printf("📊 启用规则数量: %d/%d\n", enabledCount, (int)rules.size());
    
    if (enabledCount == 0) {
        Serial.println("⚠️ 当前没有启用的转发规则，短信将不会被转发");
        Serial.println("💡 请配置并启用至少一条转发规则");
    }
    
    return true;
}

// 如果作为独立程序运行
#ifdef SETUP_FORWARD_RULES_STANDALONE
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // 设置默认转发规则
    setupDefaultForwardRules();
    
    // 显示配置指南
    showConfigurationGuide();
    
    // 初始化并测试推送管理器
    initializeAndTestPushManager();
}

void loop() {
    // 空循环
    delay(1000);
}
#endif