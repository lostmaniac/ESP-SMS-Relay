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
    
    // 创建配置文件示例
    String configPath = "/config/system.conf";
    Serial.println("\n创建配置文件: " + configPath);
    
    // 创建目录（SPIFFS中目录是虚拟的）
    fs.createDirectory("/config");
    
    File configFile = fs.getFS().open(configPath, "w");
    if (configFile) {
        configFile.println("# 系统配置文件");
        configFile.println("debug_mode=true");
        configFile.println("log_level=INFO");
        configFile.println("network_timeout=30000");
        configFile.println("sms_retry_count=3");
        configFile.close();
        Serial.println("配置文件创建成功");
    } else {
        Serial.println("配置文件创建失败");
    }
    
    // 列出根目录下的文件
    Serial.println("\n根目录文件列表:");
    File root = fs.getFS().open("/");
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                Serial.println("  [目录] " + String(file.name()));
            } else {
                Serial.println("  [文件] " + String(file.name()) + " (" + String(file.size()) + " 字节)");
            }
            file = root.openNextFile();
        }
        root.close();
    }
    
    // 更新文件系统信息
    info = fs.getFilesystemInfo();
    Serial.println("\n更新后的文件系统信息:");
    Serial.println("已使用: " + String(info.usedBytes) + " 字节");
    Serial.println("可用空间: " + String(info.freeBytes) + " 字节");
    Serial.println("使用率: " + String(info.usagePercent, 1) + "%");
    
    // 清理测试文件（可选）
    Serial.println("\n清理测试文件...");
    if (fs.deleteFile(testFilePath)) {
        Serial.println("测试文件删除成功");
    } else {
        Serial.println("测试文件删除失败: " + fs.getLastError());
    }
    
    Serial.println("\n=== 文件系统示例完成 ===");
}

/**
 * @brief 创建日志文件示例
 */
void create_log_file_example() {
    Serial.println("\n=== 创建日志文件示例 ===");
    
    FilesystemManager& fs = FilesystemManager::getInstance();
    
    if (!fs.isReady()) {
        Serial.println("文件系统未就绪");
        return;
    }
    
    // 创建日志文件
    String logPath = "/logs/system.log";
    fs.createDirectory("/logs");
    
    File logFile = fs.getFS().open(logPath, "a"); // 追加模式
    if (logFile) {
        String timestamp = String(millis());
        logFile.println("[" + timestamp + "] 系统启动");
        logFile.println("[" + timestamp + "] 文件系统初始化完成");
        logFile.println("[" + timestamp + "] 开始运行主程序");
        logFile.close();
        Serial.println("日志文件创建成功: " + logPath);
    } else {
        Serial.println("日志文件创建失败");
    }
}

/**
 * @brief 文件系统性能测试
 */
void filesystem_performance_test() {
    Serial.println("\n=== 文件系统性能测试 ===");
    
    FilesystemManager& fs = FilesystemManager::getInstance();
    
    if (!fs.isReady()) {
        Serial.println("文件系统未就绪");
        return;
    }
    
    // 写入性能测试
    unsigned long startTime = millis();
    String perfTestPath = "/perf_test.txt";
    
    File perfFile = fs.getFS().open(perfTestPath, "w");
    if (perfFile) {
        for (int i = 0; i < 100; i++) {
            perfFile.println("性能测试行 " + String(i) + " - 这是一个较长的测试字符串用于测试写入性能");
        }
        perfFile.close();
        
        unsigned long writeTime = millis() - startTime;
        Serial.println("写入100行数据耗时: " + String(writeTime) + " 毫秒");
        
        // 读取性能测试
        startTime = millis();
        perfFile = fs.getFS().open(perfTestPath, "r");
        if (perfFile) {
            int lineCount = 0;
            while (perfFile.available()) {
                String line = perfFile.readStringUntil('\n');
                lineCount++;
            }
            perfFile.close();
            
            unsigned long readTime = millis() - startTime;
            Serial.println("读取" + String(lineCount) + "行数据耗时: " + String(readTime) + " 毫秒");
        }
        
        // 清理测试文件
        fs.deleteFile(perfTestPath);
    }
    
    Serial.println("=== 性能测试完成 ===");
}