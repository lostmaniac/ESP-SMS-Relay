/**
 * @file test_terminal_manager.cpp
 * @brief 终端管理器单元测试
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 本文件包含终端管理器的单元测试用例
 * 测试CLI功能、规则管理、数据库操作等核心功能
 */

#include <Arduino.h>
#include "terminal_manager.h"
#include "database_manager.h"
#include "log_manager.h"

// ==================== 测试框架 ====================

/**
 * @brief 简单的测试断言宏
 */
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            Serial.println("FAIL: " + String(message)); \
            return false; \
        } else { \
            Serial.println("PASS: " + String(message)); \
        } \
    } while(0)

/**
 * @brief 测试结果统计
 */
struct TestResults {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    
    void addResult(bool passed) {
        totalTests++;
        if (passed) {
            passedTests++;
        } else {
            failedTests++;
        }
    }
    
    void printSummary() {
        Serial.println("\n=== Test Summary ===");
        Serial.println("Total Tests: " + String(totalTests));
        Serial.println("Passed: " + String(passedTests));
        Serial.println("Failed: " + String(failedTests));
        Serial.println("Success Rate: " + String((float)passedTests / totalTests * 100, 1) + "%");
    }
};

TestResults testResults;

// ==================== 测试用例 ====================

/**
 * @brief 测试系统初始化
 * @return true 测试通过
 * @return false 测试失败
 */
bool testSystemInitialization() {
    Serial.println("\n--- Testing System Initialization ---");
    
    // 测试数据库管理器初始化
    DatabaseManager& db = DatabaseManager::getInstance();
    TEST_ASSERT(db.initialize(), "Database manager initialization");
    
    // 测试终端管理器初始化
    TerminalManager& tm = TerminalManager::getInstance();
    TEST_ASSERT(tm.initialize(), "Terminal manager initialization");
    
    // 测试初始化状态
    TEST_ASSERT(tm.isInitialized(), "Terminal manager initialized state");
    
    return true;
}

/**
 * @brief 测试规则添加功能
 * @return true 测试通过
 * @return false 测试失败
 */
bool testRuleAddition() {
    Serial.println("\n--- Testing Rule Addition ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 创建测试规则
    ForwardRule testRule;
    testRule.name = "Test Rule";
    testRule.description = "Test rule for unit testing";
    testRule.senderPattern = "12345";
    testRule.contentPattern = "*test*";
    testRule.pushType = "webhook";
    testRule.pushConfig = "{\"url\":\"https://test.example.com\"}";
    testRule.priority = 100;
    testRule.enabled = true;
    
    // 测试添加规则
    int ruleId = tm.addForwardRule(testRule);
    TEST_ASSERT(ruleId > 0, "Add valid rule");
    
    // 测试获取规则
    ForwardRule retrievedRule = tm.getForwardRule(ruleId);
    TEST_ASSERT(retrievedRule.id == ruleId, "Retrieve added rule");
    TEST_ASSERT(retrievedRule.name == testRule.name, "Rule name matches");
    TEST_ASSERT(retrievedRule.senderPattern == testRule.senderPattern, "Sender pattern matches");
    
    // 测试无效规则添加
    ForwardRule invalidRule;
    invalidRule.name = ""; // 空名称
    invalidRule.senderPattern = "test";
    invalidRule.pushType = "webhook";
    invalidRule.pushConfig = "{\"url\":\"test\"}";
    
    int invalidRuleId = tm.addForwardRule(invalidRule);
    TEST_ASSERT(invalidRuleId == -1, "Reject invalid rule (empty name)");
    
    return true;
}

/**
 * @brief 测试规则查询功能
 * @return true 测试通过
 * @return false 测试失败
 */
bool testRuleQuery() {
    Serial.println("\n--- Testing Rule Query ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 测试获取所有规则
    std::vector<ForwardRule> allRules = tm.getForwardRules();
    TEST_ASSERT(allRules.size() > 0, "Get all rules");
    
    // 测试按条件查询
    RuleQueryCondition condition;
    condition.filterByEnabled = true;
    condition.enabledValue = true;
    
    std::vector<ForwardRule> enabledRules = tm.getForwardRules(condition);
    TEST_ASSERT(enabledRules.size() > 0, "Get enabled rules");
    
    // 验证查询结果
    for (const ForwardRule& rule : enabledRules) {
        TEST_ASSERT(rule.enabled == true, "All returned rules are enabled");
    }
    
    // 测试规则计数
    int totalCount = tm.getRuleCount();
    int enabledCount = tm.getEnabledRuleCount();
    
    TEST_ASSERT(totalCount >= enabledCount, "Total count >= enabled count");
    TEST_ASSERT(totalCount == (int)allRules.size(), "Total count matches query result");
    
    return true;
}

/**
 * @brief 测试规则状态管理
 * @return true 测试通过
 * @return false 测试失败
 */
bool testRuleStatusManagement() {
    Serial.println("\n--- Testing Rule Status Management ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 获取第一个规则进行测试
    std::vector<ForwardRule> rules = tm.getForwardRules();
    TEST_ASSERT(rules.size() > 0, "Have rules for status testing");
    
    int testRuleId = rules[0].id;
    bool originalStatus = rules[0].enabled;
    
    // 测试禁用规则
    TEST_ASSERT(tm.disableRule(testRuleId), "Disable rule");
    
    ForwardRule disabledRule = tm.getForwardRule(testRuleId);
    TEST_ASSERT(disabledRule.enabled == false, "Rule is disabled");
    
    // 测试启用规则
    TEST_ASSERT(tm.enableRule(testRuleId), "Enable rule");
    
    ForwardRule enabledRule = tm.getForwardRule(testRuleId);
    TEST_ASSERT(enabledRule.enabled == true, "Rule is enabled");
    
    // 恢复原始状态
    if (originalStatus) {
        tm.enableRule(testRuleId);
    } else {
        tm.disableRule(testRuleId);
    }
    
    // 测试优先级设置
    int originalPriority = rules[0].priority;
    int newPriority = 200;
    
    TEST_ASSERT(tm.setRulePriority(testRuleId, newPriority), "Set rule priority");
    
    ForwardRule updatedRule = tm.getForwardRule(testRuleId);
    TEST_ASSERT(updatedRule.priority == newPriority, "Priority updated correctly");
    
    // 恢复原始优先级
    tm.setRulePriority(testRuleId, originalPriority);
    
    return true;
}

/**
 * @brief 测试规则匹配功能
 * @return true 测试通过
 * @return false 测试失败
 */
bool testRuleMatching() {
    Serial.println("\n--- Testing Rule Matching ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 创建测试规则
    ForwardRule matchTestRule;
    matchTestRule.name = "Match Test Rule";
    matchTestRule.description = "Rule for testing pattern matching";
    matchTestRule.senderPattern = "95588";
    matchTestRule.contentPattern = "*余额*";
    matchTestRule.pushType = "webhook";
    matchTestRule.pushConfig = "{\"url\":\"https://test.com\"}";
    matchTestRule.priority = 100;
    matchTestRule.enabled = true;
    
    int ruleId = tm.addForwardRule(matchTestRule);
    TEST_ASSERT(ruleId > 0, "Add match test rule");
    
    // 测试匹配成功的情况
    bool match1 = tm.testRule(ruleId, "95588", "您的账户余额为1000元");
    TEST_ASSERT(match1 == true, "Match bank SMS correctly");
    
    // 测试发送者不匹配
    bool match2 = tm.testRule(ruleId, "12345", "您的账户余额为1000元");
    TEST_ASSERT(match2 == false, "Reject non-matching sender");
    
    // 测试内容不匹配
    bool match3 = tm.testRule(ruleId, "95588", "这是一条普通短信");
    TEST_ASSERT(match3 == false, "Reject non-matching content");
    
    // 测试通配符匹配
    ForwardRule wildcardRule;
    wildcardRule.name = "Wildcard Test";
    wildcardRule.senderPattern = "*";
    wildcardRule.contentPattern = "*验证码*";
    wildcardRule.pushType = "webhook";
    wildcardRule.pushConfig = "{\"url\":\"https://test.com\"}";
    wildcardRule.priority = 90;
    wildcardRule.enabled = true;
    
    int wildcardRuleId = tm.addForwardRule(wildcardRule);
    TEST_ASSERT(wildcardRuleId > 0, "Add wildcard test rule");
    
    bool wildcardMatch = tm.testRule(wildcardRuleId, "任意发送者", "您的验证码是123456");
    TEST_ASSERT(wildcardMatch == true, "Wildcard pattern matching");
    
    // 清理测试规则
    tm.deleteForwardRule(ruleId);
    tm.deleteForwardRule(wildcardRuleId);
    
    return true;
}

/**
 * @brief 测试规则验证功能
 * @return true 测试通过
 * @return false 测试失败
 */
bool testRuleValidation() {
    Serial.println("\n--- Testing Rule Validation ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 测试有效规则
    ForwardRule validRule;
    validRule.name = "Valid Rule";
    validRule.description = "A valid rule for testing";
    validRule.senderPattern = "12345";
    validRule.contentPattern = "*test*";
    validRule.pushType = "webhook";
    validRule.pushConfig = "{\"url\":\"https://example.com\"}";
    validRule.priority = 100;
    validRule.enabled = true;
    
    TEST_ASSERT(tm.validateRuleConfig(validRule), "Valid rule passes validation");
    
    // 测试无效规则 - 空名称
    ForwardRule invalidRule1 = validRule;
    invalidRule1.name = "";
    TEST_ASSERT(!tm.validateRuleConfig(invalidRule1), "Empty name fails validation");
    
    // 测试无效规则 - 空发送者模式
    ForwardRule invalidRule2 = validRule;
    invalidRule2.senderPattern = "";
    TEST_ASSERT(!tm.validateRuleConfig(invalidRule2), "Empty sender pattern fails validation");
    
    // 测试无效规则 - 无效推送类型
    ForwardRule invalidRule3 = validRule;
    invalidRule3.pushType = "invalid_type";
    TEST_ASSERT(!tm.validateRuleConfig(invalidRule3), "Invalid push type fails validation");
    
    // 测试无效规则 - 无效JSON配置
    ForwardRule invalidRule4 = validRule;
    invalidRule4.pushConfig = "invalid json";
    TEST_ASSERT(!tm.validateRuleConfig(invalidRule4), "Invalid JSON config fails validation");
    
    // 测试无效规则 - 优先级超出范围
    ForwardRule invalidRule5 = validRule;
    invalidRule5.priority = 1001;
    TEST_ASSERT(!tm.validateRuleConfig(invalidRule5), "Priority out of range fails validation");
    
    return true;
}

/**
 * @brief 测试批量操作功能
 * @return true 测试通过
 * @return false 测试失败
 */
bool testBatchOperations() {
    Serial.println("\n--- Testing Batch Operations ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 记录原始状态
    int originalEnabledCount = tm.getEnabledRuleCount();
    int totalRules = tm.getRuleCount();
    
    // 测试禁用所有规则
    TEST_ASSERT(tm.disableAllRules(), "Disable all rules");
    TEST_ASSERT(tm.getEnabledRuleCount() == 0, "All rules disabled");
    
    // 测试启用所有规则
    TEST_ASSERT(tm.enableAllRules(), "Enable all rules");
    TEST_ASSERT(tm.getEnabledRuleCount() == totalRules, "All rules enabled");
    
    // 测试导出规则
    std::vector<ForwardRule> exportedRules = tm.exportRules();
    TEST_ASSERT(exportedRules.size() == totalRules, "Export all rules");
    
    // 恢复部分原始状态（这里简化处理）
    if (originalEnabledCount < totalRules) {
        // 如果原来不是全部启用，随机禁用一些规则
        for (int i = 0; i < totalRules - originalEnabledCount; i++) {
            if (i < exportedRules.size()) {
                tm.disableRule(exportedRules[i].id);
            }
        }
    }
    
    return true;
}

/**
 * @brief 测试CLI基本功能
 * @return true 测试通过
 * @return false 测试失败
 */
bool testCLIBasics() {
    Serial.println("\n--- Testing CLI Basics ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 测试CLI启动
    TEST_ASSERT(!tm.isCLIRunning(), "CLI initially not running");
    
    tm.startCLI();
    TEST_ASSERT(tm.isCLIRunning(), "CLI started successfully");
    
    // 测试命令处理
    TEST_ASSERT(tm.processCommand("help"), "Process help command");
    TEST_ASSERT(tm.processCommand("status"), "Process status command");
    TEST_ASSERT(tm.processCommand("list"), "Process list command");
    
    // 测试无效命令
    TEST_ASSERT(tm.processCommand("invalid_command"), "Handle invalid command gracefully");
    
    // 测试空命令
    TEST_ASSERT(tm.processCommand(""), "Handle empty command");
    
    // 停止CLI
    tm.stopCLI();
    TEST_ASSERT(!tm.isCLIRunning(), "CLI stopped successfully");
    
    return true;
}

/**
 * @brief 测试错误处理
 * @return true 测试通过
 * @return false 测试失败
 */
bool testErrorHandling() {
    Serial.println("\n--- Testing Error Handling ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 测试获取不存在的规则
    ForwardRule nonExistentRule = tm.getForwardRule(99999);
    TEST_ASSERT(nonExistentRule.id == -1, "Non-existent rule returns invalid ID");
    
    // 测试删除不存在的规则
    TEST_ASSERT(!tm.deleteForwardRule(99999), "Delete non-existent rule fails");
    
    // 测试启用不存在的规则
    TEST_ASSERT(!tm.enableRule(99999), "Enable non-existent rule fails");
    
    // 测试设置无效优先级
    std::vector<ForwardRule> rules = tm.getForwardRules();
    if (rules.size() > 0) {
        TEST_ASSERT(!tm.setRulePriority(rules[0].id, -1), "Invalid priority (negative) fails");
        TEST_ASSERT(!tm.setRulePriority(rules[0].id, 1001), "Invalid priority (too high) fails");
    }
    
    // 测试错误信息获取
    String lastError = tm.getLastError();
    TEST_ASSERT(!lastError.isEmpty(), "Error message is available");
    
    return true;
}

// ==================== 主测试函数 ====================

/**
 * @brief 运行所有测试用例
 */
void runAllTests() {
    Serial.println("\n========================================");
    Serial.println("    Terminal Manager Unit Tests");
    Serial.println("========================================");
    
    // 运行测试用例
    testResults.addResult(testSystemInitialization());
    testResults.addResult(testRuleAddition());
    testResults.addResult(testRuleQuery());
    testResults.addResult(testRuleStatusManagement());
    testResults.addResult(testRuleMatching());
    testResults.addResult(testRuleValidation());
    testResults.addResult(testBatchOperations());
    testResults.addResult(testCLIBasics());
    testResults.addResult(testErrorHandling());
    
    // 打印测试结果
    testResults.printSummary();
    
    if (testResults.failedTests == 0) {
        Serial.println("\n🎉 All tests passed!");
    } else {
        Serial.println("\n❌ Some tests failed. Please check the output above.");
    }
}

// ==================== Arduino主函数 ====================

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    delay(2000); // 等待串口稳定
    
    Serial.println("Starting Terminal Manager Tests...");
    
    // 运行所有测试
    runAllTests();
    
    Serial.println("\nTests completed. You can now use the CLI.");
    Serial.println("Type 'help' for available commands.");
    
    // 启动CLI供交互使用
    TerminalManager& tm = TerminalManager::getInstance();
    if (tm.isInitialized()) {
        tm.startCLI();
    }
}

void loop() {
    TerminalManager& tm = TerminalManager::getInstance();
    if (tm.isCLIRunning()) {
        tm.handleSerialInput();
    }
    
    delay(10);
}

// ==================== 辅助测试函数 ====================

/**
 * @brief 创建测试规则
 * @param name 规则名称
 * @param sender 发送者模式
 * @param content 内容模式
 * @return ForwardRule 创建的规则
 */
ForwardRule createTestRule(const String& name, const String& sender, const String& content = "") {
    ForwardRule rule;
    rule.name = name;
    rule.description = "Test rule: " + name;
    rule.senderPattern = sender;
    rule.contentPattern = content;
    rule.pushType = "webhook";
    rule.pushConfig = "{\"url\":\"https://test.example.com\"}";
    rule.priority = 100;
    rule.enabled = true;
    
    return rule;
}

/**
 * @brief 清理测试数据
 */
void cleanupTestData() {
    TerminalManager& tm = TerminalManager::getInstance();
    
    // 删除所有测试规则
    std::vector<ForwardRule> rules = tm.getForwardRules();
    for (const ForwardRule& rule : rules) {
        if (rule.name.startsWith("Test") || rule.name.startsWith("Match Test") || 
            rule.name.startsWith("Wildcard Test") || rule.name.startsWith("Valid Rule")) {
            tm.deleteForwardRule(rule.id);
        }
    }
}

/**
 * @brief 性能测试
 */
void performanceTest() {
    Serial.println("\n--- Performance Test ---");
    
    TerminalManager& tm = TerminalManager::getInstance();
    
    unsigned long startTime = millis();
    
    // 批量添加规则
    for (int i = 0; i < 10; i++) {
        ForwardRule rule = createTestRule("Perf Test " + String(i), "sender" + String(i));
        tm.addForwardRule(rule);
    }
    
    unsigned long addTime = millis() - startTime;
    
    // 批量查询
    startTime = millis();
    for (int i = 0; i < 100; i++) {
        tm.getForwardRules();
    }
    unsigned long queryTime = millis() - startTime;
    
    Serial.println("Performance Results:");
    Serial.println("  Add 10 rules: " + String(addTime) + "ms");
    Serial.println("  100 queries: " + String(queryTime) + "ms");
    
    // 清理性能测试数据
    std::vector<ForwardRule> rules = tm.getForwardRules();
    for (const ForwardRule& rule : rules) {
        if (rule.name.startsWith("Perf Test")) {
            tm.deleteForwardRule(rule.id);
        }
    }
}