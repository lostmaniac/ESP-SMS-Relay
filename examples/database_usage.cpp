/**
 * @file database_usage.cpp
 * @brief 数据库使用示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该示例展示如何使用DatabaseManager进行数据库操作
 */

#include "../lib/database_manager/database_manager.h"
#include "../lib/filesystem_manager/filesystem_manager.h"
#include <Arduino.h>

/**
 * @brief 数据库使用示例函数
 */
void databaseUsageExample() {
    Serial.println("\n=== 数据库管理器使用示例 ===");
    
    // 1. 初始化文件系统
    Serial.println("\n1. 初始化文件系统...");
    FilesystemManager& fsManager = FilesystemManager::getInstance();
    fsManager.setDebugMode(true);
    
    if (!fsManager.initialize(true)) {
        Serial.println("文件系统初始化失败: " + fsManager.getLastError());
        return;
    }
    
    Serial.println("文件系统初始化成功");
    
    // 2. 初始化数据库
    Serial.println("\n2. 初始化数据库...");
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    dbManager.setDebugMode(true);
    
    if (!dbManager.initialize()) {
        Serial.println("数据库初始化失败: " + dbManager.getLastError());
        return;
    }
    
    Serial.println("数据库初始化成功");
    
    // 3. 获取数据库信息
    Serial.println("\n3. 数据库信息:");
    DatabaseInfo dbInfo = dbManager.getDatabaseInfo();
    Serial.println("数据库路径: " + dbInfo.dbPath);
    Serial.println("数据库大小: " + String(dbInfo.dbSize) + " bytes");
    Serial.println("表数量: " + String(dbInfo.tableCount));
    Serial.println("记录总数: " + String(dbInfo.recordCount));
    Serial.println("数据库版本: " + dbInfo.version);
    Serial.println("数据库状态: " + String(dbInfo.isOpen ? "已打开" : "已关闭"));
    
    // 4. 测试AP配置
    Serial.println("\n4. 测试AP配置...");
    APConfig apConfig = dbManager.getAPConfig();
    Serial.println("当前AP配置:");
    Serial.println("  SSID: " + apConfig.ssid);
    Serial.println("  密码: " + apConfig.password);
    Serial.println("  启用: " + String(apConfig.enabled ? "是" : "否"));
    Serial.println("  信道: " + String(apConfig.channel));
    Serial.println("  最大连接数: " + String(apConfig.maxConnections));
    
    // 更新AP配置
    apConfig.ssid = "ESP-SMS-Relay-Updated";
    apConfig.password = "newpassword123";
    apConfig.channel = 6;
    
    if (dbManager.updateAPConfig(apConfig)) {
        Serial.println("AP配置更新成功");
        
        // 验证更新
        APConfig updatedConfig = dbManager.getAPConfig();
        Serial.println("更新后的AP配置:");
        Serial.println("  SSID: " + updatedConfig.ssid);
        Serial.println("  密码: " + updatedConfig.password);
        Serial.println("  信道: " + String(updatedConfig.channel));
    } else {
        Serial.println("AP配置更新失败: " + dbManager.getLastError());
    }
    
    // 5. 测试转发规则
    Serial.println("\n5. 测试转发规则...");
    
    // 添加转发规则
    ForwardRule rule1;
    rule1.ruleName = "测试规则1";
    rule1.sourceNumber = "+86138*";
    rule1.pushType = "webhook";
    rule1.pushConfig = "{\"url\":\"http://example.com/webhook\"}";
    rule1.keywords = "紧急";
    rule1.enabled = true;
    
    int ruleId1 = dbManager.addForwardRule(rule1);
    if (ruleId1 > 0) {
        Serial.println("转发规则1添加成功，ID: " + String(ruleId1));
    } else {
        Serial.println("转发规则1添加失败: " + dbManager.getLastError());
    }
    
    // 添加第二个规则
    ForwardRule rule2;
    rule2.ruleName = "测试规则2";
    rule2.sourceNumber = "+86139*";
    rule2.pushType = "wechat";
    rule2.pushConfig = "{\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx\"}";
    rule2.keywords = "通知";
    rule2.enabled = false;
    
    int ruleId2 = dbManager.addForwardRule(rule2);
    if (ruleId2 > 0) {
        Serial.println("转发规则2添加成功，ID: " + String(ruleId2));
    } else {
        Serial.println("转发规则2添加失败: " + dbManager.getLastError());
    }
    
    // 获取所有转发规则
    std::vector<ForwardRule> rules = dbManager.getAllForwardRules();
    Serial.println("\n当前转发规则数量: " + String(rules.size()));
    for (const auto& rule : rules) {
        Serial.println("规则ID: " + String(rule.id) + ", 名称: " + rule.ruleName + 
                       ", 源号码: " + rule.sourceNumber + ", 推送类型: " + rule.pushType + 
                       ", 关键词: " + rule.keywords + ", 启用: " + String(rule.enabled ? "是" : "否"));
    }
    
    // 更新规则
    if (ruleId1 > 0) {
        ForwardRule updateRule = dbManager.getForwardRuleById(ruleId1);
        updateRule.keyword = "更新后的关键词";
        updateRule.enabled = false;
        
        if (dbManager.updateForwardRule(updateRule)) {
            Serial.println("规则更新成功");
        } else {
            Serial.println("规则更新失败: " + dbManager.getLastError());
        }
    }
    
    // 6. 测试短信记录
    Serial.println("\n6. 测试短信记录...");
    
    // 添加短信记录
    SMSRecord record1;
    record1.fromNumber = "+8613800000001";
    record1.toNumber = "+8613800000000";
    record1.content = "这是一条测试短信，包含紧急关键词";
    record1.ruleId = ruleId1;
    record1.forwarded = false;
    record1.status = "received";
    
    int recordId1 = dbManager.addSMSRecord(record1);
    if (recordId1 > 0) {
        Serial.println("短信记录1添加成功，ID: " + String(recordId1));
    } else {
        Serial.println("短信记录1添加失败: " + dbManager.getLastError());
    }
    
    // 添加第二条记录
    SMSRecord record2;
    record2.fromNumber = "+8613900000001";
    record2.toNumber = "+8613900000000";
    record2.content = "这是另一条测试短信，包含通知关键词";
    record2.ruleId = ruleId2;
    record2.forwarded = true;
    record2.status = "forwarded";
    
    int recordId2 = dbManager.addSMSRecord(record2);
    if (recordId2 > 0) {
        Serial.println("短信记录2添加成功，ID: " + String(recordId2));
    } else {
        Serial.println("短信记录2添加失败: " + dbManager.getLastError());
    }
    
    // 获取短信记录
    std::vector<SMSRecord> records = dbManager.getSMSRecords(10, 0);
    Serial.println("\n当前短信记录数量: " + String(records.size()));
    for (const auto& record : records) {
        Serial.println("记录ID: " + String(record.id) + ", 发送方: " + record.fromNumber + 
                      ", 接收方: " + record.toNumber + ", 内容: " + record.content.substring(0, 20) + "..." +
                      ", 规则ID: " + String(record.ruleId) + ", 已转发: " + String(record.forwarded ? "是" : "否") +
                      ", 状态: " + record.status);
    }
    
    // 更新短信记录
    if (recordId1 > 0) {
        SMSRecord updateRecord = dbManager.getSMSRecordById(recordId1);
        updateRecord.forwarded = true;
        updateRecord.status = "forwarded";
        updateRecord.forwardedAt = String(millis());
        
        if (dbManager.updateSMSRecord(updateRecord)) {
            Serial.println("短信记录更新成功");
        } else {
            Serial.println("短信记录更新失败: " + dbManager.getLastError());
        }
    }
    
    // 7. 测试数据清理
    Serial.println("\n7. 测试数据清理...");
    
    // 删除过期记录（这里设置为0天，即删除所有记录用于测试）
    // 在实际使用中，应该设置合理的天数，如30天
    int deletedCount = dbManager.deleteOldSMSRecords(0);
    Serial.println("删除的过期记录数: " + String(deletedCount));
    
    // 删除测试规则
    if (ruleId1 > 0) {
        if (dbManager.deleteForwardRule(ruleId1)) {
            Serial.println("测试规则1删除成功");
        } else {
            Serial.println("测试规则1删除失败: " + dbManager.getLastError());
        }
    }
    
    if (ruleId2 > 0) {
        if (dbManager.deleteForwardRule(ruleId2)) {
            Serial.println("测试规则2删除成功");
        } else {
            Serial.println("测试规则2删除失败: " + dbManager.getLastError());
        }
    }
    
    // 8. 最终状态检查
    Serial.println("\n8. 最终状态检查...");
    DatabaseInfo finalInfo = dbManager.getDatabaseInfo();
    Serial.println("最终数据库信息:");
    Serial.println("  数据库大小: " + String(finalInfo.dbSize) + " bytes");
    Serial.println("  表数量: " + String(finalInfo.tableCount));
    Serial.println("  记录总数: " + String(finalInfo.recordCount));
    Serial.println("  数据库状态: " + String(dbManager.isReady() ? "就绪" : "未就绪"));
    
    Serial.println("\n=== 数据库管理器测试完成 ===");
}

/**
 * @brief 简单的数据库操作示例
 */
void simpleDatabaseExample() {
    Serial.println("\n=== 简单数据库操作示例 ===");
    
    // 获取数据库管理器实例
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 检查数据库是否就绪
    if (!db.isReady()) {
        Serial.println("数据库未就绪，请先初始化");
        return;
    }
    
    // 获取AP配置
    APConfig config = db.getAPConfig();
    Serial.println("当前WiFi热点配置:");
    Serial.println("  名称: " + config.ssid);
    Serial.println("  密码: " + config.password);
    
    // 获取转发规则数量
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    Serial.println("当前转发规则数量: " + String(rules.size()));
    
    // 获取短信记录数量
    std::vector<SMSRecord> records = db.getSMSRecords(1, 0);
    Serial.println("短信记录数量: " + String(records.size()));
    
    Serial.println("=== 简单示例完成 ===");
}