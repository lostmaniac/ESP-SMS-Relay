/**
 * @file test_database_cleanup.cpp
 * @brief 数据库清理功能测试程序
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件用于测试数据库自动清理功能
 */

#include <Arduino.h>
#include "database_manager.h"
#include "task_scheduler.h"
#include "log_manager.h"

/**
 * @brief 创建测试短信记录
 * @param count 创建的记录数量
 */
void createTestSMSRecords(int count) {
    DatabaseManager& db = DatabaseManager::getInstance();
    
    Serial.println("开始创建 " + String(count) + " 条测试短信记录...");
    
    for (int i = 0; i < count; i++) {
        SMSRecord record;
        record.sender = "1390000" + String(i % 10000);
        record.content = "测试短信内容 " + String(i);
        record.timestamp = millis() - (i * 1000); // 模拟不同时间
        record.processed = (i % 2 == 0);
        record.forwarded = (i % 3 == 0);
        
        if (!db.addSMSRecord(record)) {
            Serial.println("创建第 " + String(i) + " 条记录失败: " + db.getLastError());
            break;
        }
        
        if (i % 1000 == 0) {
            Serial.println("已创建 " + String(i) + " 条记录...");
        }
    }
    
    Serial.println("测试记录创建完成！");
}

/**
 * @brief 测试数据库清理功能
 */
void testDatabaseCleanup() {
    Serial.println("\n=== 数据库清理功能测试 ===");
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 获取当前短信记录数量
    int currentCount = db.getSMSRecordCount();
    Serial.println("当前短信记录数量: " + String(currentCount));
    
    // 如果记录数少于15000条，创建一些测试记录
    if (currentCount < 15000) {
        Serial.println("记录数量不足，创建测试记录...");
        createTestSMSRecords(15000 - currentCount);
        currentCount = db.getSMSRecordCount();
        Serial.println("创建后的记录数量: " + String(currentCount));
    }
    
    // 测试检查和清理功能
    Serial.println("\n开始测试自动清理功能...");
    db.checkAndCleanupSMSRecords();
    
    // 检查清理后的记录数量
    int afterCleanupCount = db.getSMSRecordCount();
    Serial.println("清理后的记录数量: " + String(afterCleanupCount));
    
    if (afterCleanupCount <= 10000) {
        Serial.println("✅ 数据库清理功能测试通过！");
    } else {
        Serial.println("❌ 数据库清理功能测试失败！");
    }
}

/**
 * @brief 测试定时任务调度器
 */
void testTaskScheduler() {
    Serial.println("\n=== 定时任务调度器测试 ===");
    
    TaskScheduler& scheduler = TaskScheduler::getInstance();
    
    if (!scheduler.initialize()) {
        Serial.println("❌ 任务调度器初始化失败: " + scheduler.getLastError());
        return;
    }
    
    Serial.println("✅ 任务调度器初始化成功");
    
    // 添加一个测试任务
    int taskId = scheduler.addPeriodicTask("测试清理任务", 5000, []() {
        Serial.println("🔄 执行定时清理任务...");
        DatabaseManager& db = DatabaseManager::getInstance();
        db.checkAndCleanupSMSRecords();
    });
    
    if (taskId > 0) {
        Serial.println("✅ 测试任务添加成功，任务ID: " + String(taskId));
        Serial.println("任务信息: " + scheduler.getTaskInfo(taskId));
    } else {
        Serial.println("❌ 测试任务添加失败: " + scheduler.getLastError());
    }
    
    Serial.println("当前任务数量: " + String(scheduler.getTaskCount()));
    Serial.println("启用的任务数量: " + String(scheduler.getEnabledTaskCount()));
}

/**
 * @brief 主测试函数
 */
void runDatabaseCleanupTest() {
    Serial.println("\n" + String('=', 50));
    Serial.println("    数据库清理功能测试程序");
    Serial.println("    Version: 1.0.0");
    Serial.println(String('=', 50));
    
    // 初始化日志管理器
    LogManager& logManager = LogManager::getInstance();
    if (!logManager.initialize()) {
        Serial.println("❌ 日志管理器初始化失败");
        return;
    }
    
    // 初始化数据库管理器
    DatabaseManager& db = DatabaseManager::getInstance();
    db.setDebugMode(true);
    if (!db.initialize()) {
        Serial.println("❌ 数据库管理器初始化失败: " + db.getLastError());
        return;
    }
    
    Serial.println("✅ 系统初始化完成");
    
    // 运行测试
    testDatabaseCleanup();
    testTaskScheduler();
    
    Serial.println("\n🎉 所有测试完成！");
}