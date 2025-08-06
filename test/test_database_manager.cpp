/**
 * @file test_database_manager.cpp
 * @brief 数据库管理器测试
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件包含数据库管理器的单元测试
 */

#include "../lib/database_manager/database_manager.h"
#include "../lib/filesystem_manager/filesystem_manager.h"
#include <Arduino.h>

/**
 * @brief 测试结果统计
 */
struct TestResults {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
};

static TestResults testResults;

/**
 * @brief 断言宏
 */
#define ASSERT_TRUE(condition, message) \
    do { \
        testResults.totalTests++; \
        if (condition) { \
            testResults.passedTests++; \
            Serial.println("✓ PASS: " + String(message)); \
        } else { \
            testResults.failedTests++; \
            Serial.println("✗ FAIL: " + String(message)); \
        } \
    } while(0)

#define ASSERT_FALSE(condition, message) ASSERT_TRUE(!(condition), message)
#define ASSERT_EQUAL(expected, actual, message) ASSERT_TRUE((expected) == (actual), message + " (期望: " + String(expected) + ", 实际: " + String(actual) + ")")
#define ASSERT_NOT_EQUAL(expected, actual, message) ASSERT_TRUE((expected) != (actual), message)

/**
 * @brief 打印测试分隔符
 * @param title 测试标题
 */
void printTestSeparator(const String& title) {
    Serial.println("\n" + String(50, '='));
    Serial.println("测试: " + title);
    Serial.println(String(50, '='));
}

/**
 * @brief 打印测试结果
 */
void printTestResults() {
    Serial.println("\n" + String(50, '='));
    Serial.println("测试结果统计");
    Serial.println(String(50, '='));
    Serial.println("总测试数: " + String(testResults.totalTests));
    Serial.println("通过测试: " + String(testResults.passedTests));
    Serial.println("失败测试: " + String(testResults.failedTests));
    Serial.println("成功率: " + String((float)testResults.passedTests / testResults.totalTests * 100, 1) + "%");
    
    if (testResults.failedTests == 0) {
        Serial.println("\n🎉 所有测试通过！");
    } else {
        Serial.println("\n⚠️  有测试失败，请检查代码");
    }
    Serial.println(String(50, '='));
}

/**
 * @brief 测试数据库初始化
 */
void testDatabaseInitialization() {
    printTestSeparator("数据库初始化测试");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 测试初始状态
    ASSERT_EQUAL(DB_NOT_INITIALIZED, db.getStatus(), "初始状态应为未初始化");
    ASSERT_FALSE(db.isReady(), "初始状态应为未就绪");
    
    // 测试初始化
    bool initResult = db.initialize();
    ASSERT_TRUE(initResult, "数据库初始化应该成功");
    
    if (initResult) {
        ASSERT_EQUAL(DB_READY, db.getStatus(), "初始化后状态应为就绪");
        ASSERT_TRUE(db.isReady(), "初始化后应为就绪状态");
        
        // 测试数据库信息
        DatabaseInfo info = db.getDatabaseInfo();
        ASSERT_TRUE(info.isOpen, "数据库应为打开状态");
        ASSERT_TRUE(info.dbSize > 0, "数据库文件大小应大于0");
        ASSERT_TRUE(info.tableCount >= 3, "应至少有3个表（ap_config, forward_rules, sms_records）");
        
        Serial.println("数据库信息:");
        Serial.println("  路径: " + info.dbPath);
        Serial.println("  大小: " + String(info.dbSize) + " 字节");
        Serial.println("  表数量: " + String(info.tableCount));
        Serial.println("  记录总数: " + String(info.recordCount));
    }
}

/**
 * @brief 测试AP配置管理
 */
void testAPConfigManagement() {
    printTestSeparator("AP配置管理测试");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪，跳过AP配置测试");
        return;
    }
    
    // 测试获取默认AP配置
    APConfig defaultConfig = db.getAPConfig();
    ASSERT_EQUAL("ESP-SMS-Relay", defaultConfig.ssid, "默认SSID应为ESP-SMS-Relay");
    ASSERT_EQUAL("12345678", defaultConfig.password, "默认密码应为12345678");
    ASSERT_TRUE(defaultConfig.enabled, "默认应启用AP模式");
    ASSERT_EQUAL(1, defaultConfig.channel, "默认信道应为1");
    ASSERT_EQUAL(4, defaultConfig.maxConnections, "默认最大连接数应为4");
    
    // 测试更新AP配置
    APConfig newConfig = defaultConfig;
    newConfig.ssid = "Test-SSID";
    newConfig.password = "testpass123";
    newConfig.channel = 6;
    newConfig.maxConnections = 8;
    newConfig.enabled = false;
    
    bool updateResult = db.updateAPConfig(newConfig);
    ASSERT_TRUE(updateResult, "AP配置更新应该成功");
    
    // 验证更新结果
    APConfig updatedConfig = db.getAPConfig();
    ASSERT_EQUAL("Test-SSID", updatedConfig.ssid, "SSID应已更新");
    ASSERT_EQUAL("testpass123", updatedConfig.password, "密码应已更新");
    ASSERT_EQUAL(6, updatedConfig.channel, "信道应已更新");
    ASSERT_EQUAL(8, updatedConfig.maxConnections, "最大连接数应已更新");
    ASSERT_FALSE(updatedConfig.enabled, "启用状态应已更新");
    
    // 恢复默认配置
    db.updateAPConfig(defaultConfig);
}

/**
 * @brief 测试转发规则管理
 */
void testForwardRuleManagement() {
    printTestSeparator("转发规则管理测试");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪，跳过转发规则测试");
        return;
    }
    
    // 获取初始规则数量
    std::vector<ForwardRule> initialRules = db.getAllForwardRules();
    int initialCount = initialRules.size();
    
    // 测试添加转发规则
    ForwardRule rule1;
    rule1.name = "测试规则1";
    rule1.sourceNumber = "+86138*";
    rule1.targetNumber = "+8613800000000";
    rule1.keyword = "紧急";
    rule1.enabled = true;
    
    int ruleId1 = db.addForwardRule(rule1);
    ASSERT_TRUE(ruleId1 > 0, "添加转发规则应返回有效ID");
    
    // 测试添加第二个规则
    ForwardRule rule2;
    rule2.name = "测试规则2";
    rule2.sourceNumber = "+86139*";
    rule2.targetNumber = "+8613900000000";
    rule2.keyword = "通知";
    rule2.enabled = false;
    
    int ruleId2 = db.addForwardRule(rule2);
    ASSERT_TRUE(ruleId2 > 0, "添加第二个转发规则应返回有效ID");
    ASSERT_NOT_EQUAL(ruleId1, ruleId2, "两个规则ID应不同");
    
    // 验证规则数量增加
    std::vector<ForwardRule> rulesAfterAdd = db.getAllForwardRules();
    ASSERT_EQUAL(initialCount + 2, rulesAfterAdd.size(), "规则数量应增加2");
    
    // 测试根据ID获取规则
    ForwardRule retrievedRule1 = db.getForwardRuleById(ruleId1);
    ASSERT_EQUAL(ruleId1, retrievedRule1.id, "获取的规则ID应匹配");
    ASSERT_EQUAL("测试规则1", retrievedRule1.name, "规则名称应匹配");
    ASSERT_EQUAL("+86138*", retrievedRule1.sourceNumber, "源号码应匹配");
    ASSERT_EQUAL("+8613800000000", retrievedRule1.targetNumber, "目标号码应匹配");
    ASSERT_EQUAL("紧急", retrievedRule1.keyword, "关键词应匹配");
    ASSERT_TRUE(retrievedRule1.enabled, "启用状态应匹配");
    
    // 测试更新规则
    retrievedRule1.name = "更新后的规则1";
    retrievedRule1.keyword = "更新关键词";
    retrievedRule1.enabled = false;
    
    bool updateResult = db.updateForwardRule(retrievedRule1);
    ASSERT_TRUE(updateResult, "更新转发规则应该成功");
    
    // 验证更新结果
    ForwardRule updatedRule = db.getForwardRuleById(ruleId1);
    ASSERT_EQUAL("更新后的规则1", updatedRule.name, "规则名称应已更新");
    ASSERT_EQUAL("更新关键词", updatedRule.keyword, "关键词应已更新");
    ASSERT_FALSE(updatedRule.enabled, "启用状态应已更新");
    
    // 测试删除规则
    bool deleteResult1 = db.deleteForwardRule(ruleId1);
    ASSERT_TRUE(deleteResult1, "删除转发规则应该成功");
    
    bool deleteResult2 = db.deleteForwardRule(ruleId2);
    ASSERT_TRUE(deleteResult2, "删除第二个转发规则应该成功");
    
    // 验证规则数量恢复
    std::vector<ForwardRule> rulesAfterDelete = db.getAllForwardRules();
    ASSERT_EQUAL(initialCount, rulesAfterDelete.size(), "删除后规则数量应恢复");
    
    // 测试获取不存在的规则
    ForwardRule nonExistentRule = db.getForwardRuleById(ruleId1);
    ASSERT_EQUAL(-1, nonExistentRule.id, "不存在的规则ID应返回-1");
}

/**
 * @brief 测试短信记录管理
 */
void testSMSRecordManagement() {
    printTestSeparator("短信记录管理测试");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪，跳过短信记录测试");
        return;
    }
    
    // 获取初始记录数量
    std::vector<SMSRecord> initialRecords = db.getSMSRecords(1000, 0);
    int initialCount = initialRecords.size();
    
    // 测试添加短信记录
    SMSRecord record1;
    record1.fromNumber = "+8613800000001";
    record1.toNumber = "+8613800000000";
    record1.content = "这是一条测试短信，包含紧急关键词";
    record1.ruleId = 1;
    record1.forwarded = false;
    record1.status = "received";
    
    int recordId1 = db.addSMSRecord(record1);
    ASSERT_TRUE(recordId1 > 0, "添加短信记录应返回有效ID");
    
    // 测试添加第二条记录
    SMSRecord record2;
    record2.fromNumber = "+8613900000001";
    record2.toNumber = "+8613900000000";
    record2.content = "这是另一条测试短信，包含通知关键词";
    record2.ruleId = 2;
    record2.forwarded = true;
    record2.status = "forwarded";
    
    int recordId2 = db.addSMSRecord(record2);
    ASSERT_TRUE(recordId2 > 0, "添加第二条短信记录应返回有效ID");
    ASSERT_NOT_EQUAL(recordId1, recordId2, "两个记录ID应不同");
    
    // 验证记录数量增加
    std::vector<SMSRecord> recordsAfterAdd = db.getSMSRecords(1000, 0);
    ASSERT_EQUAL(initialCount + 2, recordsAfterAdd.size(), "记录数量应增加2");
    
    // 测试根据ID获取记录
    SMSRecord retrievedRecord1 = db.getSMSRecordById(recordId1);
    ASSERT_EQUAL(recordId1, retrievedRecord1.id, "获取的记录ID应匹配");
    ASSERT_EQUAL("+8613800000001", retrievedRecord1.fromNumber, "发送方号码应匹配");
    ASSERT_EQUAL("+8613800000000", retrievedRecord1.toNumber, "接收方号码应匹配");
    ASSERT_TRUE(retrievedRecord1.content.indexOf("测试短信") >= 0, "短信内容应包含关键词");
    ASSERT_EQUAL(1, retrievedRecord1.ruleId, "规则ID应匹配");
    ASSERT_FALSE(retrievedRecord1.forwarded, "转发状态应匹配");
    ASSERT_EQUAL("received", retrievedRecord1.status, "状态应匹配");
    
    // 测试更新记录
    retrievedRecord1.forwarded = true;
    retrievedRecord1.status = "forwarded";
    retrievedRecord1.forwardedAt = String(millis());
    
    bool updateResult = db.updateSMSRecord(retrievedRecord1);
    ASSERT_TRUE(updateResult, "更新短信记录应该成功");
    
    // 验证更新结果
    SMSRecord updatedRecord = db.getSMSRecordById(recordId1);
    ASSERT_TRUE(updatedRecord.forwarded, "转发状态应已更新");
    ASSERT_EQUAL("forwarded", updatedRecord.status, "状态应已更新");
    ASSERT_FALSE(updatedRecord.forwardedAt.isEmpty(), "转发时间应已设置");
    
    // 测试分页查询
    std::vector<SMSRecord> page1 = db.getSMSRecords(1, 0);
    ASSERT_TRUE(page1.size() <= 1, "第一页应最多包含1条记录");
    
    std::vector<SMSRecord> page2 = db.getSMSRecords(1, 1);
    if (recordsAfterAdd.size() > 1) {
        ASSERT_TRUE(page2.size() <= 1, "第二页应最多包含1条记录");
        if (page1.size() > 0 && page2.size() > 0) {
            ASSERT_NOT_EQUAL(page1[0].id, page2[0].id, "不同页的记录ID应不同");
        }
    }
    
    // 测试删除过期记录（删除所有记录用于测试）
    int deletedCount = db.deleteOldSMSRecords(0);
    ASSERT_TRUE(deletedCount >= 2, "应至少删除2条记录");
    
    // 验证记录已删除
    std::vector<SMSRecord> recordsAfterDelete = db.getSMSRecords(1000, 0);
    ASSERT_TRUE(recordsAfterDelete.size() < recordsAfterAdd.size(), "删除后记录数量应减少");
    
    // 测试获取不存在的记录
    SMSRecord nonExistentRecord = db.getSMSRecordById(recordId1);
    ASSERT_EQUAL(-1, nonExistentRecord.id, "不存在的记录ID应返回-1");
}

/**
 * @brief 测试数据库错误处理
 */
void testDatabaseErrorHandling() {
    printTestSeparator("数据库错误处理测试");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 测试在数据库未初始化时的操作
    DatabaseManager testDb; // 创建新实例用于测试
    
    // 注意：由于使用单例模式，这里主要测试错误信息的获取
    String lastError = db.getLastError();
    Serial.println("最后错误信息: " + (lastError.isEmpty() ? "无" : lastError));
    
    // 测试数据库状态
    DatabaseStatus status = db.getStatus();
    ASSERT_TRUE(status == DB_READY || status == DB_ERROR, "数据库状态应为就绪或错误");
    
    // 测试数据库信息获取
    DatabaseInfo info = db.getDatabaseInfo();
    if (db.isReady()) {
        ASSERT_TRUE(info.isOpen, "就绪状态下数据库应为打开");
        ASSERT_FALSE(info.dbPath.isEmpty(), "数据库路径不应为空");
    }
}

/**
 * @brief 测试数据库性能
 */
void testDatabasePerformance() {
    printTestSeparator("数据库性能测试");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪，跳过性能测试");
        return;
    }
    
    const int testRecordCount = 10;
    unsigned long startTime, endTime;
    
    // 测试批量插入性能
    startTime = millis();
    std::vector<int> recordIds;
    
    for (int i = 0; i < testRecordCount; i++) {
        SMSRecord record;
        record.fromNumber = "+861380000" + String(1000 + i);
        record.toNumber = "+8613800000000";
        record.content = "性能测试短信 #" + String(i);
        record.ruleId = 0;
        record.forwarded = false;
        record.status = "received";
        
        int recordId = db.addSMSRecord(record);
        if (recordId > 0) {
            recordIds.push_back(recordId);
        }
    }
    
    endTime = millis();
    unsigned long insertTime = endTime - startTime;
    
    ASSERT_EQUAL(testRecordCount, recordIds.size(), "应成功插入所有测试记录");
    Serial.println("插入 " + String(testRecordCount) + " 条记录耗时: " + String(insertTime) + " ms");
    Serial.println("平均插入时间: " + String((float)insertTime / testRecordCount, 2) + " ms/条");
    
    // 测试查询性能
    startTime = millis();
    std::vector<SMSRecord> allRecords = db.getSMSRecords(1000, 0);
    endTime = millis();
    unsigned long queryTime = endTime - startTime;
    
    Serial.println("查询 " + String(allRecords.size()) + " 条记录耗时: " + String(queryTime) + " ms");
    
    // 清理测试数据
    startTime = millis();
    int deletedCount = db.deleteOldSMSRecords(0);
    endTime = millis();
    unsigned long deleteTime = endTime - startTime;
    
    Serial.println("删除 " + String(deletedCount) + " 条记录耗时: " + String(deleteTime) + " ms");
    ASSERT_TRUE(deletedCount >= testRecordCount, "应删除至少" + String(testRecordCount) + "条记录");
}

/**
 * @brief 运行所有数据库测试
 */
void runDatabaseTests() {
    Serial.println("\n" + String(60, '='));
    Serial.println("开始数据库管理器测试");
    Serial.println(String(60, '='));
    
    // 重置测试结果
    testResults = TestResults();
    
    // 首先确保文件系统已初始化
    FilesystemManager& fsManager = FilesystemManager::getInstance();
    if (!fsManager.isReady()) {
        Serial.println("正在初始化文件系统...");
        fsManager.setDebugMode(false); // 关闭调试模式以减少输出
        if (!fsManager.initialize(true)) {
            Serial.println("文件系统初始化失败，无法进行数据库测试");
            return;
        }
    }
    
    // 运行测试
    testDatabaseInitialization();
    testAPConfigManagement();
    testForwardRuleManagement();
    testSMSRecordManagement();
    testDatabaseErrorHandling();
    testDatabasePerformance();
    
    // 打印测试结果
    printTestResults();
}

/**
 * @brief 快速数据库功能验证
 */
void quickDatabaseTest() {
    Serial.println("\n=== 快速数据库功能验证 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪");
        return;
    }
    
    // 快速验证基本功能
    APConfig config = db.getAPConfig();
    Serial.println("✓ AP配置读取成功: " + config.ssid);
    
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    Serial.println("✓ 转发规则查询成功: " + String(rules.size()) + " 条规则");
    
    std::vector<SMSRecord> records = db.getSMSRecords(5, 0);
    Serial.println("✓ 短信记录查询成功: " + String(records.size()) + " 条记录");
    
    DatabaseInfo info = db.getDatabaseInfo();
    Serial.println("✓ 数据库信息获取成功: " + String(info.dbSize) + " 字节");
    
    Serial.println("=== 快速验证完成 ===");
}