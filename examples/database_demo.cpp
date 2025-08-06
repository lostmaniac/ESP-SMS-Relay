/**
 * @file database_demo.cpp
 * @brief 数据库模块演示程序
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件演示如何使用数据库管理器进行各种数据库操作
 */

#include "../lib/database_manager/database_manager.h"
#include "../lib/filesystem_manager/filesystem_manager.h"
#include <Arduino.h>

/**
 * @brief 演示AP配置管理
 */
void demonstrateAPConfig() {
    Serial.println("\n=== AP配置管理演示 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 获取当前AP配置
    APConfig currentConfig = db.getAPConfig();
    Serial.println("当前AP配置:");
    Serial.println("  SSID: " + currentConfig.ssid);
    Serial.println("  密码: " + currentConfig.password);
    Serial.println("  信道: " + String(currentConfig.channel));
    Serial.println("  最大连接数: " + String(currentConfig.maxConnections));
    Serial.println("  启用状态: " + String(currentConfig.enabled ? "是" : "否"));
    
    // 临时修改配置进行演示
    APConfig demoConfig = currentConfig;
    demoConfig.ssid = "Demo-WiFi";
    demoConfig.password = "demo123456";
    demoConfig.channel = 11;
    demoConfig.maxConnections = 6;
    
    Serial.println("\n更新AP配置为演示配置...");
    if (db.updateAPConfig(demoConfig)) {
        Serial.println("✓ AP配置更新成功");
        
        // 验证更新
        APConfig updatedConfig = db.getAPConfig();
        Serial.println("更新后的配置:");
        Serial.println("  SSID: " + updatedConfig.ssid);
        Serial.println("  密码: " + updatedConfig.password);
        Serial.println("  信道: " + String(updatedConfig.channel));
    } else {
        Serial.println("✗ AP配置更新失败");
    }
    
    // 恢复原始配置
    Serial.println("\n恢复原始配置...");
    if (db.updateAPConfig(currentConfig)) {
        Serial.println("✓ 原始配置已恢复");
    }
}

/**
 * @brief 演示转发规则管理
 */
void demonstrateForwardRules() {
    Serial.println("\n=== 转发规则管理演示 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 显示当前所有规则
    std::vector<ForwardRule> existingRules = db.getAllForwardRules();
    Serial.println("当前转发规则数量: " + String(existingRules.size()));
    
    for (const auto& rule : existingRules) {
        Serial.println("  规则 #" + String(rule.id) + ": " + rule.name + 
                      " (" + rule.sourceNumber + " -> " + rule.targetNumber + ")");
    }
    
    // 添加演示规则
    Serial.println("\n添加演示转发规则...");
    
    ForwardRule demoRule1;
    demoRule1.name = "紧急联系人转发";
    demoRule1.sourceNumber = "+86138*";
    demoRule1.targetNumber = "+8613800000000";
    demoRule1.keyword = "紧急";
    demoRule1.enabled = true;
    
    int ruleId1 = db.addForwardRule(demoRule1);
    if (ruleId1 > 0) {
        Serial.println("✓ 规则1添加成功，ID: " + String(ruleId1));
    } else {
        Serial.println("✗ 规则1添加失败");
    }
    
    ForwardRule demoRule2;
    demoRule2.name = "工作通知转发";
    demoRule2.sourceNumber = "+86139*";
    demoRule2.targetNumber = "+8613900000000";
    demoRule2.keyword = "会议";
    demoRule2.enabled = false;
    
    int ruleId2 = db.addForwardRule(demoRule2);
    if (ruleId2 > 0) {
        Serial.println("✓ 规则2添加成功，ID: " + String(ruleId2));
    } else {
        Serial.println("✗ 规则2添加失败");
    }
    
    // 显示更新后的规则列表
    std::vector<ForwardRule> updatedRules = db.getAllForwardRules();
    Serial.println("\n更新后的转发规则数量: " + String(updatedRules.size()));
    
    // 修改规则演示
    if (ruleId1 > 0) {
        Serial.println("\n修改规则演示...");
        ForwardRule ruleToUpdate = db.getForwardRuleById(ruleId1);
        ruleToUpdate.keyword = "紧急|急救";
        ruleToUpdate.enabled = false;
        
        if (db.updateForwardRule(ruleToUpdate)) {
            Serial.println("✓ 规则更新成功");
            ForwardRule updatedRule = db.getForwardRuleById(ruleId1);
            Serial.println("  新关键词: " + updatedRule.keyword);
            Serial.println("  启用状态: " + String(updatedRule.enabled ? "是" : "否"));
        }
    }
    
    // 清理演示数据
    Serial.println("\n清理演示数据...");
    if (ruleId1 > 0 && db.deleteForwardRule(ruleId1)) {
        Serial.println("✓ 规则1已删除");
    }
    if (ruleId2 > 0 && db.deleteForwardRule(ruleId2)) {
        Serial.println("✓ 规则2已删除");
    }
}

/**
 * @brief 演示短信记录管理
 */
void demonstrateSMSRecords() {
    Serial.println("\n=== 短信记录管理演示 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 显示当前记录数量
    std::vector<SMSRecord> existingRecords = db.getSMSRecords(10, 0);
    Serial.println("当前短信记录数量: " + String(existingRecords.size()));
    
    // 添加演示记录
    Serial.println("\n添加演示短信记录...");
    
    SMSRecord demoRecord1;
    demoRecord1.fromNumber = "+8613800000001";
    demoRecord1.toNumber = "+8613800000000";
    demoRecord1.content = "这是一条包含紧急关键词的测试短信";
    demoRecord1.ruleId = 1;
    demoRecord1.forwarded = false;
    demoRecord1.status = "received";
    
    int recordId1 = db.addSMSRecord(demoRecord1);
    if (recordId1 > 0) {
        Serial.println("✓ 记录1添加成功，ID: " + String(recordId1));
    }
    
    SMSRecord demoRecord2;
    demoRecord2.fromNumber = "+8613900000001";
    demoRecord2.toNumber = "+8613900000000";
    demoRecord2.content = "这是一条关于会议通知的短信";
    demoRecord2.ruleId = 2;
    demoRecord2.forwarded = true;
    demoRecord2.status = "forwarded";
    demoRecord2.forwardedAt = String(millis());
    
    int recordId2 = db.addSMSRecord(demoRecord2);
    if (recordId2 > 0) {
        Serial.println("✓ 记录2添加成功，ID: " + String(recordId2));
    }
    
    SMSRecord demoRecord3;
    demoRecord3.fromNumber = "+8613700000001";
    demoRecord3.toNumber = "+8613700000000";
    demoRecord3.content = "普通短信，无需转发";
    demoRecord3.ruleId = 0;
    demoRecord3.forwarded = false;
    demoRecord3.status = "received";
    
    int recordId3 = db.addSMSRecord(demoRecord3);
    if (recordId3 > 0) {
        Serial.println("✓ 记录3添加成功，ID: " + String(recordId3));
    }
    
    // 显示最新记录
    Serial.println("\n最新短信记录:");
    std::vector<SMSRecord> latestRecords = db.getSMSRecords(5, 0);
    for (const auto& record : latestRecords) {
        Serial.println("  #" + String(record.id) + ": " + record.fromNumber + 
                      " -> " + record.toNumber);
        Serial.println("    内容: " + record.content.substring(0, 30) + 
                      (record.content.length() > 30 ? "..." : ""));
        Serial.println("    状态: " + record.status + 
                      (record.forwarded ? " (已转发)" : " (未转发)"));
    }
    
    // 演示分页查询
    Serial.println("\n分页查询演示 (每页2条):");
    for (int page = 0; page < 3; page++) {
        std::vector<SMSRecord> pageRecords = db.getSMSRecords(2, page * 2);
        if (pageRecords.empty()) break;
        
        Serial.println("  第" + String(page + 1) + "页:");
        for (const auto& record : pageRecords) {
            Serial.println("    #" + String(record.id) + ": " + 
                          record.fromNumber + " (" + record.status + ")");
        }
    }
    
    // 演示记录更新
    if (recordId1 > 0) {
        Serial.println("\n更新记录演示...");
        SMSRecord recordToUpdate = db.getSMSRecordById(recordId1);
        recordToUpdate.forwarded = true;
        recordToUpdate.status = "forwarded";
        recordToUpdate.forwardedAt = String(millis());
        
        if (db.updateSMSRecord(recordToUpdate)) {
            Serial.println("✓ 记录更新成功");
            SMSRecord updatedRecord = db.getSMSRecordById(recordId1);
            Serial.println("  转发状态: " + String(updatedRecord.forwarded ? "是" : "否"));
            Serial.println("  转发时间: " + updatedRecord.forwardedAt);
        }
    }
    
    // 清理演示数据（保留最近的记录）
    Serial.println("\n清理旧记录演示...");
    int deletedCount = db.deleteOldSMSRecords(7); // 保留7天内的记录
    Serial.println("✓ 清理了 " + String(deletedCount) + " 条旧记录");
}

/**
 * @brief 演示数据库信息查询
 */
void demonstrateDatabaseInfo() {
    Serial.println("\n=== 数据库信息查询演示 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 获取数据库状态
    DatabaseStatus status = db.getStatus();
    String statusStr;
    switch (status) {
        case DB_NOT_INITIALIZED: statusStr = "未初始化"; break;
        case DB_INITIALIZING: statusStr = "初始化中"; break;
        case DB_READY: statusStr = "就绪"; break;
        case DB_ERROR: statusStr = "错误"; break;
        default: statusStr = "未知"; break;
    }
    
    Serial.println("数据库状态: " + statusStr);
    Serial.println("数据库就绪: " + String(db.isReady() ? "是" : "否"));
    
    // 获取详细信息
    if (db.isReady()) {
        DatabaseInfo info = db.getDatabaseInfo();
        Serial.println("\n数据库详细信息:");
        Serial.println("  路径: " + info.dbPath);
        Serial.println("  文件大小: " + String(info.dbSize) + " 字节");
        Serial.println("  表数量: " + String(info.tableCount));
        Serial.println("  记录总数: " + String(info.recordCount));
        Serial.println("  打开状态: " + String(info.isOpen ? "是" : "否"));
        Serial.println("  最后更新: " + info.lastModified);
    }
    
    // 获取错误信息
    String lastError = db.getLastError();
    if (!lastError.isEmpty()) {
        Serial.println("\n最后错误: " + lastError);
    } else {
        Serial.println("\n✓ 无错误记录");
    }
}

/**
 * @brief 演示数据库性能测试
 */
void demonstratePerformance() {
    Serial.println("\n=== 数据库性能演示 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪，跳过性能测试");
        return;
    }
    
    const int testCount = 5;
    unsigned long startTime, endTime;
    
    // 测试插入性能
    Serial.println("\n插入性能测试 (" + String(testCount) + " 条记录):");
    startTime = millis();
    
    std::vector<int> testRecordIds;
    for (int i = 0; i < testCount; i++) {
        SMSRecord record;
        record.fromNumber = "+861380000" + String(100 + i);
        record.toNumber = "+8613800000000";
        record.content = "性能测试短信 #" + String(i) + " - " + String(millis());
        record.ruleId = 0;
        record.forwarded = false;
        record.status = "received";
        
        int recordId = db.addSMSRecord(record);
        if (recordId > 0) {
            testRecordIds.push_back(recordId);
        }
    }
    
    endTime = millis();
    Serial.println("  插入耗时: " + String(endTime - startTime) + " ms");
    Serial.println("  平均耗时: " + String((float)(endTime - startTime) / testCount, 2) + " ms/条");
    Serial.println("  成功插入: " + String(testRecordIds.size()) + "/" + String(testCount) + " 条");
    
    // 测试查询性能
    Serial.println("\n查询性能测试:");
    startTime = millis();
    std::vector<SMSRecord> allRecords = db.getSMSRecords(100, 0);
    endTime = millis();
    
    Serial.println("  查询耗时: " + String(endTime - startTime) + " ms");
    Serial.println("  查询结果: " + String(allRecords.size()) + " 条记录");
    
    // 测试更新性能
    if (!testRecordIds.empty()) {
        Serial.println("\n更新性能测试:");
        startTime = millis();
        
        for (int recordId : testRecordIds) {
            SMSRecord record = db.getSMSRecordById(recordId);
            if (record.id > 0) {
                record.status = "processed";
                db.updateSMSRecord(record);
            }
        }
        
        endTime = millis();
        Serial.println("  更新耗时: " + String(endTime - startTime) + " ms");
        Serial.println("  平均耗时: " + String((float)(endTime - startTime) / testRecordIds.size(), 2) + " ms/条");
    }
    
    // 清理测试数据
    Serial.println("\n清理测试数据...");
    startTime = millis();
    int deletedCount = db.deleteOldSMSRecords(0);
    endTime = millis();
    
    Serial.println("  清理耗时: " + String(endTime - startTime) + " ms");
    Serial.println("  清理记录: " + String(deletedCount) + " 条");
}

/**
 * @brief 运行完整的数据库演示
 */
void runDatabaseDemo() {
    Serial.println("\n" + String(60, '='));
    Serial.println("数据库管理器功能演示");
    Serial.println(String(60, '='));
    
    // 确保文件系统已初始化
    FilesystemManager& fsManager = FilesystemManager::getInstance();
    if (!fsManager.isReady()) {
        Serial.println("正在初始化文件系统...");
        fsManager.setDebugMode(false);
        if (!fsManager.initialize(true)) {
            Serial.println("文件系统初始化失败，无法运行演示");
            return;
        }
        Serial.println("✓ 文件系统初始化成功");
    }
    
    // 确保数据库已初始化
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.isReady()) {
        Serial.println("正在初始化数据库...");
        if (!db.initialize()) {
            Serial.println("数据库初始化失败，无法运行演示");
            return;
        }
        Serial.println("✓ 数据库初始化成功");
    }
    
    // 运行各项演示
    demonstrateDatabaseInfo();
    demonstrateAPConfig();
    demonstrateForwardRules();
    demonstrateSMSRecords();
    demonstratePerformance();
    
    Serial.println("\n" + String(60, '='));
    Serial.println("数据库演示完成");
    Serial.println(String(60, '='));
}

/**
 * @brief 简化的数据库功能验证
 */
void quickDatabaseDemo() {
    Serial.println("\n=== 快速数据库功能验证 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    if (!db.isReady()) {
        Serial.println("数据库未就绪，请先初始化");
        return;
    }
    
    // 基本功能验证
    Serial.println("1. 数据库状态: " + String(db.isReady() ? "就绪" : "未就绪"));
    
    APConfig config = db.getAPConfig();
    Serial.println("2. AP配置: " + config.ssid + " (" + config.password + ")");
    
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    Serial.println("3. 转发规则: " + String(rules.size()) + " 条");
    
    std::vector<SMSRecord> records = db.getSMSRecords(3, 0);
    Serial.println("4. 短信记录: " + String(records.size()) + " 条 (最近3条)");
    
    DatabaseInfo info = db.getDatabaseInfo();
    Serial.println("5. 数据库大小: " + String(info.dbSize) + " 字节");
    
    Serial.println("=== 验证完成 ===");
}