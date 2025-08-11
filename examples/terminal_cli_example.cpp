/**
 * @file terminal_cli_example.cpp
 * @brief 终端管理器CLI使用示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本示例展示如何在ESP-SMS-Relay项目中使用终端管理器的CLI功能
 * 包括初始化、启动CLI、处理串口输入等操作
 */

#include <Arduino.h>
#include "terminal_manager.h"
#include "database_manager.h"
#include "log_manager.h"

// ==================== 全局变量 ====================

TerminalManager& terminalManager = TerminalManager::getInstance();
DatabaseManager& databaseManager = DatabaseManager::getInstance();
LogManager& logManager = LogManager::getInstance();

// ==================== 系统初始化 ====================

/**
 * @brief 初始化系统组件
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool initializeSystem() {
    // 初始化串口
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n=== ESP-SMS-Relay System Starting ===");
    
    // 初始化日志管理器
    if (!logManager.initialize()) {
        Serial.println("Failed to initialize log manager");
        return false;
    }
    
    logManager.log("System initialization started");
    
    // 初始化数据库管理器
    if (!databaseManager.initialize()) {
        Serial.println("Failed to initialize database manager");
        logManager.log("Database initialization failed");
        return false;
    }
    
    logManager.log("Database manager initialized");
    
    // 初始化终端管理器
    if (!terminalManager.initialize()) {
        Serial.println("Failed to initialize terminal manager");
        logManager.log("Terminal manager initialization failed");
        return false;
    }
    
    logManager.log("Terminal manager initialized");
    
    Serial.println("System initialization completed successfully!");
    logManager.log("System initialization completed");
    
    return true;
}

// ==================== 示例数据创建 ====================

/**
 * @brief 创建一些示例转发规则
 */
void createExampleRules() {
    Serial.println("\nCreating example forward rules...");
    
    // 示例规则1：银行短信转发到企业微信
    ForwardRule bankRule;
    bankRule.name = "Bank Notifications";
    bankRule.description = "Forward bank SMS to WeChat Work";
    bankRule.senderPattern = "95588";
    bankRule.contentPattern = "*余额*";
    bankRule.pushType = "wechat";
    bankRule.pushConfig = "{\"webhook\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx\"}";
    bankRule.priority = 100;
    bankRule.enabled = true;
    
    int bankRuleId = terminalManager.addForwardRule(bankRule);
    if (bankRuleId > 0) {
        Serial.println("Created bank rule with ID: " + String(bankRuleId));
    } else {
        Serial.println("Failed to create bank rule: " + terminalManager.getLastError());
    }
    
    // 示例规则2：验证码转发到钉钉
    ForwardRule codeRule;
    codeRule.name = "Verification Codes";
    codeRule.description = "Forward verification codes to DingTalk";
    codeRule.senderPattern = "*";
    codeRule.contentPattern = "*验证码*";
    codeRule.pushType = "dingtalk";
    codeRule.pushConfig = "{\"webhook\":\"https://oapi.dingtalk.com/robot/send?access_token=xxx\"}";
    codeRule.priority = 90;
    codeRule.enabled = true;
    
    int codeRuleId = terminalManager.addForwardRule(codeRule);
    if (codeRuleId > 0) {
        Serial.println("Created verification code rule with ID: " + String(codeRuleId));
    } else {
        Serial.println("Failed to create verification code rule: " + terminalManager.getLastError());
    }
    
    // 示例规则3：系统通知转发到Webhook
    ForwardRule systemRule;
    systemRule.name = "System Alerts";
    systemRule.description = "Forward system alerts to webhook";
    systemRule.senderPattern = "10086";
    systemRule.contentPattern = "*";
    systemRule.pushType = "webhook";
    systemRule.pushConfig = "{\"url\":\"https://api.example.com/webhook\",\"method\":\"POST\"}";
    systemRule.priority = 80;
    systemRule.enabled = false; // 默认禁用
    
    int systemRuleId = terminalManager.addForwardRule(systemRule);
    if (systemRuleId > 0) {
        Serial.println("Created system alert rule with ID: " + String(systemRuleId));
    } else {
        Serial.println("Failed to create system alert rule: " + terminalManager.getLastError());
    }
    
    Serial.println("Example rules creation completed.");
}

// ==================== CLI演示功能 ====================

/**
 * @brief 演示CLI基本功能
 */
void demonstrateCLIFeatures() {
    Serial.println("\n=== CLI Features Demonstration ===");
    
    // 显示当前规则数量
    int totalRules = terminalManager.getRuleCount();
    int enabledRules = terminalManager.getEnabledRuleCount();
    
    Serial.println("Current status:");
    Serial.println("  Total rules: " + String(totalRules));
    Serial.println("  Enabled rules: " + String(enabledRules));
    Serial.println("  Disabled rules: " + String(totalRules - enabledRules));
    
    // 测试规则匹配
    Serial.println("\nTesting rule matching:");
    
    // 测试银行短信
    bool bankMatch = terminalManager.testRule(1, "95588", "您的账户余额为1000元");
    Serial.println("  Bank SMS test: " + String(bankMatch ? "MATCH" : "NO MATCH"));
    
    // 测试验证码短信
    bool codeMatch = terminalManager.testRule(2, "12345", "您的验证码是123456");
    Serial.println("  Verification code test: " + String(codeMatch ? "MATCH" : "NO MATCH"));
    
    Serial.println("\nCLI is now ready for interactive use.");
    Serial.println("Type 'help' for available commands.");
}

// ==================== 主程序 ====================

/**
 * @brief Arduino setup函数
 */
void setup() {
    // 初始化系统
    if (!initializeSystem()) {
        Serial.println("System initialization failed! Halting.");
        while (true) {
            delay(1000);
        }
    }
    
    // 创建示例规则（仅在首次运行时）
    if (terminalManager.getRuleCount() == 0) {
        createExampleRules();
    }
    
    // 演示CLI功能
    demonstrateCLIFeatures();
    
    // 启动CLI
    terminalManager.startCLI();
    
    Serial.println("\n=== System Ready ===");
}

/**
 * @brief Arduino loop函数
 */
void loop() {
    // 处理CLI输入
    if (terminalManager.isCLIRunning()) {
        terminalManager.handleSerialInput();
    }
    
    // 这里可以添加其他系统任务
    // 例如：处理SMS、网络通信等
    
    // 简单的心跳日志（每30秒）
    static unsigned long lastHeartbeat = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastHeartbeat > 30000) {
        logManager.log("System heartbeat - Rules: " + String(terminalManager.getRuleCount()) + 
                      ", Enabled: " + String(terminalManager.getEnabledRuleCount()));
        lastHeartbeat = currentTime;
    }
    
    delay(10); // 小延迟以避免过度占用CPU
}

// ==================== 辅助功能 ====================

/**
 * @brief 系统状态监控
 */
void printSystemStatus() {
    Serial.println("\n=== System Status ===");
    Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
    Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("Terminal Manager: " + String(terminalManager.isInitialized() ? "Ready" : "Not Ready"));
    Serial.println("Database Manager: " + String(databaseManager.isInitialized() ? "Ready" : "Not Ready"));
    Serial.println("CLI Running: " + String(terminalManager.isCLIRunning() ? "Yes" : "No"));
}

/**
 * @brief 错误处理和恢复
 */
void handleSystemError(const String& error) {
    Serial.println("System Error: " + error);
    logManager.log("ERROR: " + error);
    
    // 这里可以添加错误恢复逻辑
    // 例如：重启模块、发送告警等
}

/**
 * @brief 演示批量操作
 */
void demonstrateBatchOperations() {
    Serial.println("\n=== Batch Operations Demo ===");
    
    // 禁用所有规则
    Serial.println("Disabling all rules...");
    if (terminalManager.disableAllRules()) {
        Serial.println("All rules disabled successfully");
    } else {
        Serial.println("Failed to disable all rules: " + terminalManager.getLastError());
    }
    
    delay(1000);
    
    // 启用所有规则
    Serial.println("Enabling all rules...");
    if (terminalManager.enableAllRules()) {
        Serial.println("All rules enabled successfully");
    } else {
        Serial.println("Failed to enable all rules: " + terminalManager.getLastError());
    }
    
    // 显示最常用的规则
    Serial.println("\nMost used rules:");
    std::vector<ForwardRule> mostUsed = terminalManager.getMostUsedRules(3);
    for (const ForwardRule& rule : mostUsed) {
        Serial.println("  [" + String(rule.id) + "] " + rule.name + 
                      " (Used " + String(rule.usageCount) + " times)");
    }
}

/**
 * @brief 演示规则导出
 */
void demonstrateRuleExport() {
    Serial.println("\n=== Rule Export Demo ===");
    
    std::vector<ForwardRule> allRules = terminalManager.exportRules();
    
    Serial.println("Exporting " + String(allRules.size()) + " rules:");
    
    for (const ForwardRule& rule : allRules) {
        Serial.println("Rule: " + rule.name);
        Serial.println("  ID: " + String(rule.id));
        Serial.println("  Sender Pattern: " + rule.senderPattern);
        Serial.println("  Push Type: " + rule.pushType);
        Serial.println("  Enabled: " + String(rule.enabled ? "Yes" : "No"));
        Serial.println();
    }
}