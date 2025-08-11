/**
 * @file push_channel_base.cpp
 * @brief 推送渠道基础类实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "push_channel_base.h"
#include <ArduinoJson.h>

/**
 * @brief 解析推送配置
 * @param configJson 配置JSON字符串
 * @return std::map<String, String> 配置映射
 */
std::map<String, String> PushChannelBase::parseConfig(const String& configJson) {
    std::map<String, String> configMap;
    
    if (configJson.isEmpty()) {
        return configMap;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configJson);
    
    if (error) {
        debugPrint("解析配置JSON失败: " + String(error.c_str()));
        return configMap;
    }
    
    // 遍历JSON对象
    for (JsonPair kv : doc.as<JsonObject>()) {
        configMap[kv.key().c_str()] = kv.value().as<String>();
    }
    
    return configMap;
}

/**
 * @brief 应用消息模板
 * @param templateStr 模板字符串
 * @param context 推送上下文
 * @param escapeForJson 是否为JSON格式转义特殊字符
 * @return String 应用模板后的消息
 */
String PushChannelBase::applyTemplate(const String& templateStr, const PushContext& context, bool escapeForJson) {
    String result = templateStr;
    
    // 替换占位符
    result.replace("{sender}", context.sender);
    result.replace("{content}", context.content);
    result.replace("{timestamp}", formatTimestamp(context.timestamp));
    result.replace("{sms_id}", String(context.smsRecordId));
    
    // 只有在需要JSON转义时才转义特殊字符
    if (escapeForJson) {
        result.replace("\\", "\\\\");
        result.replace("\"", "\\\"");
        result.replace("\n", "\\n");
        result.replace("\r", "\\r");
        result.replace("\t", "\\t");
    }
    
    return result;
}

/**
 * @brief 格式化时间戳
 * @param timestamp PDU时间戳
 * @return String 格式化后的时间
 */
String PushChannelBase::formatTimestamp(const String& timestamp) {
    // PDU时间戳格式: YYMMDDhhmmss (12位数字)
    if (timestamp.length() < 12) {
        return "时间格式错误";
    }
    
    // 提取各个时间组件
    String year = timestamp.substring(0, 2);
    String month = timestamp.substring(2, 4);
    String day = timestamp.substring(4, 6);
    String hour = timestamp.substring(6, 8);
    String minute = timestamp.substring(8, 10);
    String second = timestamp.substring(10, 12);
    
    // 转换年份 (假设20xx年)
    int yearInt = year.toInt();
    if (yearInt >= 0 && yearInt <= 99) {
        yearInt += 2000;
    }
    
    // 格式化为可读格式: YYYY-MM-DD HH:mm:ss
    String formattedTime = String(yearInt) + "-" + 
                          (month.length() == 1 ? "0" + month : month) + "-" +
                          (day.length() == 1 ? "0" + day : day) + " " +
                          (hour.length() == 1 ? "0" + hour : hour) + ":" +
                          (minute.length() == 1 ? "0" + minute : minute) + ":" +
                          (second.length() == 1 ? "0" + second : second);
    
    return formattedTime;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void PushChannelBase::setError(const String& error) {
    lastError = error;
    debugPrint("错误: " + error);
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void PushChannelBase::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[PushChannel] " + message);
    }
}