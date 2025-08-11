/**
 * @file debug_config.cpp
 * @brief 调试配置格式的临时程序
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 用于检查数据库中转发规则的配置格式
 */

#include <Arduino.h>
#include "database_manager.h"
#include "filesystem_manager.h"
#include <ArduinoJson.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== 配置格式调试程序 ===");
    
    // 初始化文件系统
    FilesystemManager& fs = FilesystemManager::getInstance();
    if (!fs.initialize()) {
        Serial.println("文件系统初始化失败: " + fs.getLastError());
        return;
    }
    Serial.println("文件系统初始化成功");
    
    // 初始化数据库
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.initialize()) {
        Serial.println("数据库初始化失败: " + db.getLastError());
        return;
    }
    
    Serial.println("数据库初始化成功");
    
    // 获取所有转发规则
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    Serial.println("找到 " + String(rules.size()) + " 条转发规则");
    
    // 检查每个规则的配置
    for (const auto& rule : rules) {
        Serial.println("\n--- 规则 ID: " + String(rule.id) + " ---");
        Serial.println("规则名称: " + rule.ruleName);
        Serial.println("推送类型: " + rule.pushType);
        Serial.println("是否启用: " + String(rule.enabled ? "是" : "否"));
        Serial.println("原始配置: " + rule.pushConfig);
        
        // 尝试解析配置
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, rule.pushConfig);
        
        if (error) {
            Serial.println("❌ JSON解析失败: " + String(error.c_str()));
        } else {
            Serial.println("✅ JSON解析成功");
            
            // 检查关键字段
            if (doc["webhook_url"].is<String>()) {
                String webhookUrl = doc["webhook_url"].as<String>();
                Serial.println("webhook_url: " + (webhookUrl.isEmpty() ? "[空]" : webhookUrl));
            } else if (doc["webhook"].is<String>()) {
                String webhook = doc["webhook"].as<String>();
                Serial.println("webhook: " + (webhook.isEmpty() ? "[空]" : webhook));
            } else {
                Serial.println("❌ 未找到webhook_url或webhook字段");
            }
            
            if (doc["template"].is<String>()) {
                String template_str = doc["template"].as<String>();
                Serial.println("template: " + String(template_str.isEmpty() ? "[空]" : "[已设置]"));
            }
            
            // 显示所有字段
            Serial.println("所有字段:");
            for (JsonPair kv : doc.as<JsonObject>()) {
                Serial.println("  " + String(kv.key().c_str()) + ": " + kv.value().as<String>());
            }
        }
    }
    
    Serial.println("\n=== 调试完成 ===");
}

void loop() {
    delay(1000);
}