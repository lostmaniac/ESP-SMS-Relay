/**
 * @file filesystem_usage.cpp
 * @brief 文件系统使用示例
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该示例展示如何使用FilesystemManager进行文件操作
 */

#include "../lib/filesystem_manager/filesystem_manager.h"
#include <Arduino.h>

/**
 * @brief 文件系统使用示例函数
 */
void filesystem_usage_example() {
    Serial.println("\n=== 文件系统使用示例 ===");
    
    // 获取文件系统管理器实例
    FilesystemManager& fs = FilesystemManager::getInstance();
    fs.setDebugMode(true);
    
    // 初始化文件系统
    if (!fs.initialize()) {
        Serial.println("文件系统初始化失败: " + fs.getLastError());
        return;
    }
    
    // 获取文件系统信息
    FilesystemInfo info = fs.getFilesystemInfo();
    Serial.println("\n文件系统信息:");
    Serial.println("总空间: " + String(info.totalBytes) + " 字节");
    Serial.println("已使用: " + String(info.usedBytes) + " 字节");
    Serial.println("可用空间: " + String(info.freeBytes) + " 字节");
    Serial.println("使用率: " + String(info.usagePercent, 1) + "%");
    
    // 创建测试文件
    String testFilePath = "/test.txt";
    Serial.println("\n创建测试文件: " + testFilePath);
    
    File testFile = fs.getFS().open(testFilePath, "w");
    if (testFile) {
        testFile.println("这是一个测试文件");
        testFile.println("文件系统工作正常");
        testFile.println("时间戳: " + String(millis()));
        testFile.close();
        Serial.println("测试文件创建成功");
    } else {
        Serial.println("测试文件创建失败");
        return;
    }
    
    // 检查文件是否存在
    if (fs.fileExists(testFilePath)) {
        Serial.println("文件存在确认: " + testFilePath);
    } else {
        Serial.println("文件不存在: " + testFilePath);
        return;
    }
    
    // 读取文件内容
    Serial.println("\n读取文件内容:");
    File readFile = fs.getFS().open(testFilePath, "r");
    if (readFile) {
        while (readFile.available()) {
            String line = readFile.readStringUntil('\n');
            Serial.println("  " + line);
        }
        readFile.close();
    } else {
        Serial.println("无法打开文件进行读取");
    }
    
    // 删除测试文件
    Serial.println("\n--- 删除测试文件 ---");
    if (fs.getFS().remove(testFilePath)) {
        Serial.println("测试文件删除成功");
    } else {
        Serial.println("测试文件删除失败");
    }
    
    // 验证文件是否已删除
    if (!fs.fileExists(testFilePath)) {
        Serial.println("确认: 测试文件已不存在");
    } else {
        Serial.println("警告: 测试文件仍然存在");
    }
    
    Serial.println("\n=== 文件系统示例完成 ===");
}