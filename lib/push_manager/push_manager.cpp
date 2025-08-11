/**
 * @file push_manager.cpp
 * @brief 推送管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "push_manager.h"
#include "../log_manager/log_manager.h"
#include <ArduinoJson.h>

// 单例实例
PushManager& PushManager::getInstance() {
    static PushManager instance;
    return instance;
}

/**
 * @brief 构造函数
 */
PushManager::PushManager() 
    : debugMode(false), initialized(false), lastRuleUpdate(0) {
}

/**
 * @brief 析构函数
 */
PushManager::~PushManager() {
}

/**
 * @brief 初始化推送管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool PushManager::initialize() {
    debugPrint("正在初始化推送管理器...");
    
    // 检查数据库管理器是否就绪
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    if (!dbManager.isReady()) {
        setError("数据库管理器未就绪");
        return false;
    }
    
    // 检查HTTP客户端是否可用
    HttpClient& httpClient = HttpClient::getInstance();
    if (!httpClient.initialize()) {
        setError("HTTP客户端初始化失败: " + httpClient.getLastError());
        return false;
    }
    
    // 加载转发规则
    cachedRules = dbManager.getAllForwardRules();
    lastRuleUpdate = millis();
    
    initialized = true;
    debugPrint("推送管理器初始化成功，加载了 " + String(cachedRules.size()) + " 条转发规则");
    
    return true;
}

/**
 * @brief 处理短信推送
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::processSmsForward(const PushContext& context) {
    if (!initialized) {
        setError("推送管理器未初始化");
        return PUSH_FAILED;
    }
    
    debugPrint("开始处理短信推送，发送方: " + context.sender + ", 内容: " + context.content.substring(0, 50) + "...");
    
    // 匹配转发规则
    std::vector<ForwardRule> matchedRules = matchForwardRules(context);
    
    if (matchedRules.empty()) {
        debugPrint("没有匹配的转发规则");
        return PUSH_NO_RULE;
    }
    
    // 执行所有匹配的规则
    bool hasSuccess = false;
    PushResult lastResult = PUSH_FAILED;
    
    for (const auto& rule : matchedRules) {
        debugPrint("执行转发规则: " + rule.ruleName + " (ID: " + String(rule.id) + ")");
        
        PushResult result = executePush(rule, context);
        if (result == PUSH_SUCCESS) {
            hasSuccess = true;
            lastResult = PUSH_SUCCESS;
        } else {
            lastResult = result;
        }
    }
    
    return hasSuccess ? PUSH_SUCCESS : lastResult;
}

/**
 * @brief 根据规则ID推送短信
 * @param ruleId 转发规则ID
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::pushByRule(int ruleId, const PushContext& context) {
    if (!initialized) {
        setError("推送管理器未初始化");
        return PUSH_FAILED;
    }
    
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    ForwardRule rule = dbManager.getForwardRuleById(ruleId);
    
    if (rule.id <= 0) {
        setError("转发规则不存在: " + String(ruleId));
        return PUSH_NO_RULE;
    }
    
    if (!rule.enabled) {
        setError("转发规则已禁用: " + rule.ruleName);
        return PUSH_RULE_DISABLED;
    }
    
    return executePush(rule, context);
}

/**
 * @brief 测试推送配置
 * @param ruleId 转发规则ID
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult PushManager::testPushConfig(int ruleId, const String& testMessage) {
    PushContext testContext;
    testContext.sender = "测试号码";
    testContext.content = testMessage;
    testContext.timestamp = "240101120000"; // 2024-01-01 12:00:00
    testContext.smsRecordId = -1;
    
    return pushByRule(ruleId, testContext);
}

/**
 * @brief 匹配转发规则
 * @param context 推送上下文
 * @return std::vector<ForwardRule> 匹配的规则列表
 */
std::vector<ForwardRule> PushManager::matchForwardRules(const PushContext& context) {
    std::vector<ForwardRule> matchedRules;
    
    // 检查是否需要更新规则缓存
    if (millis() - lastRuleUpdate > RULE_CACHE_TIMEOUT) {
        DatabaseManager& dbManager = DatabaseManager::getInstance();
        cachedRules = dbManager.getAllForwardRules();
        lastRuleUpdate = millis();
        debugPrint("更新转发规则缓存，共 " + String(cachedRules.size()) + " 条规则");
    }
    
    for (const auto& rule : cachedRules) {
        // 跳过禁用的规则
        if (!rule.enabled) {
            continue;
        }
        
        bool matched = false;
        
        // 检查是否为默认转发规则
        if (rule.isDefaultForward) {
            matched = true;
        } else {
            // 检查号码匹配
            bool numberMatch = rule.sourceNumber.isEmpty() || 
                              matchPhoneNumber(rule.sourceNumber, context.sender);
            
            // 检查关键词匹配
            bool keywordMatch = rule.keywords.isEmpty() || 
                               matchKeywords(rule.keywords, context.content);
            
            matched = numberMatch && keywordMatch;
        }
        
        if (matched) {
            matchedRules.push_back(rule);
            debugPrint("规则匹配: " + rule.ruleName);
        }
    }
    
    return matchedRules;
}

/**
 * @brief 检查号码是否匹配
 * @param pattern 号码模式（支持通配符）
 * @param number 实际号码
 * @return true 匹配
 * @return false 不匹配
 */
bool PushManager::matchPhoneNumber(const String& pattern, const String& number) {
    // 简单的通配符匹配实现
    if (pattern == "*" || pattern.isEmpty()) {
        return true;
    }
    
    // 精确匹配
    if (pattern == number) {
        return true;
    }
    
    // 前缀匹配（以*结尾）
    if (pattern.endsWith("*")) {
        String prefix = pattern.substring(0, pattern.length() - 1);
        return number.startsWith(prefix);
    }
    
    // 后缀匹配（以*开头）
    if (pattern.startsWith("*")) {
        String suffix = pattern.substring(1);
        return number.endsWith(suffix);
    }
    
    // 包含匹配（中间有*）
    int starIndex = pattern.indexOf('*');
    if (starIndex > 0 && starIndex < pattern.length() - 1) {
        String prefix = pattern.substring(0, starIndex);
        String suffix = pattern.substring(starIndex + 1);
        return number.startsWith(prefix) && number.endsWith(suffix);
    }
    
    return false;
}

/**
 * @brief 检查关键词是否匹配
 * @param keywords 关键词列表（逗号分隔）
 * @param content 短信内容
 * @return true 匹配
 * @return false 不匹配
 */
bool PushManager::matchKeywords(const String& keywords, const String& content) {
    if (keywords.isEmpty()) {
        return true;
    }
    
    // 分割关键词
    String keywordsCopy = keywords;
    keywordsCopy.trim();
    
    int startIndex = 0;
    int commaIndex = keywordsCopy.indexOf(',');
    
    while (commaIndex != -1 || startIndex < keywordsCopy.length()) {
        String keyword;
        if (commaIndex != -1) {
            keyword = keywordsCopy.substring(startIndex, commaIndex);
            startIndex = commaIndex + 1;
            commaIndex = keywordsCopy.indexOf(',', startIndex);
        } else {
            keyword = keywordsCopy.substring(startIndex);
            startIndex = keywordsCopy.length();
        }
        
        keyword.trim();
        if (!keyword.isEmpty() && content.indexOf(keyword) != -1) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 执行推送
 * @param rule 转发规则
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::executePush(const ForwardRule& rule, const PushContext& context) {
    debugPrint("执行推送，类型: " + rule.pushType);
    
    PushResult result = PUSH_FAILED;
    
    if (rule.pushType == "wechat" || rule.pushType == "企业微信") {
        result = pushToWechat(rule.pushConfig, context);
    } else if (rule.pushType == "dingtalk" || rule.pushType == "钉钉") {
        result = pushToDingTalk(rule.pushConfig, context);
    } else if (rule.pushType == "webhook") {
        result = pushToWebhook(rule.pushConfig, context);
    } else {
        setError("不支持的推送类型: " + rule.pushType);
        result = PUSH_CONFIG_ERROR;
    }
    
    // 更新短信记录的转发状态
    if (context.smsRecordId > 0) {
        DatabaseManager& dbManager = DatabaseManager::getInstance();
        SMSRecord record = dbManager.getSMSRecordById(context.smsRecordId);
        if (record.id > 0) {
            record.ruleId = rule.id;
            record.forwarded = (result == PUSH_SUCCESS);
            record.status = (result == PUSH_SUCCESS) ? "forwarded" : "failed";
            if (result == PUSH_SUCCESS) {
                record.forwardedAt = formatTimestamp(context.timestamp);
            }
            dbManager.updateSMSRecord(record);
        }
    }
    
    return result;
}

/**
 * @brief 推送到企业微信
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::pushToWechat(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    String webhookUrl = configMap["webhook_url"];
    if (webhookUrl.isEmpty()) {
        setError("企业微信配置缺少webhook_url");
        return PUSH_CONFIG_ERROR;
    }
    
    // 获取消息模板
    String messageTemplate = configMap["template"];
    if (messageTemplate.isEmpty()) {
        // 使用默认模板
        messageTemplate = "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}";
    }
    
    String message = applyTemplate(messageTemplate, context);
    
    // 构建企业微信消息体
    JsonDocument doc;
    doc["msgtype"] = "text";
    doc["text"]["content"] = message;
    
    String messageBody;
    serializeJson(doc, messageBody);
    
    // 设置请求头
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    
    debugPrint("推送到企业微信: " + webhookUrl);
    debugPrint("消息内容: " + messageBody);
    
    // 发送HTTP请求
    HttpClient& httpClient = HttpClient::getInstance();
    HttpResponse response = httpClient.post(webhookUrl, messageBody, headers, 30000);
    
    debugPrint("企业微信响应 - 状态码: " + String(response.statusCode) + ", 错误码: " + String(response.error));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode == 200) {
        debugPrint("✅ 企业微信推送成功");
        return PUSH_SUCCESS;
    } else {
        setError("企业微信推送失败，状态码: " + String(response.statusCode) + ", 错误: " + httpClient.getLastError());
        return (response.error == 0) ? PUSH_FAILED : PUSH_NETWORK_ERROR;
    }
}

/**
 * @brief 推送到钉钉
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::pushToDingTalk(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    String webhookUrl = configMap["webhook_url"];
    if (webhookUrl.isEmpty()) {
        setError("钉钉配置缺少webhook_url");
        return PUSH_CONFIG_ERROR;
    }
    
    // 获取消息模板
    String messageTemplate = configMap["template"];
    if (messageTemplate.isEmpty()) {
        // 使用默认模板
        messageTemplate = "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}";
    }
    
    String message = applyTemplate(messageTemplate, context);
    
    // 构建钉钉消息体
    JsonDocument doc;
    doc["msgtype"] = "text";
    doc["text"]["content"] = message;
    
    String messageBody;
    serializeJson(doc, messageBody);
    
    // 设置请求头
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    
    debugPrint("推送到钉钉: " + webhookUrl);
    debugPrint("消息内容: " + messageBody);
    
    // 发送HTTP请求
    HttpClient& httpClient = HttpClient::getInstance();
    HttpResponse response = httpClient.post(webhookUrl, messageBody, headers, 30000);
    
    debugPrint("钉钉响应 - 状态码: " + String(response.statusCode) + ", 错误码: " + String(response.error));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode == 200) {
        debugPrint("✅ 钉钉推送成功");
        return PUSH_SUCCESS;
    } else {
        setError("钉钉推送失败，状态码: " + String(response.statusCode) + ", 错误: " + httpClient.getLastError());
        return (response.error == 0) ? PUSH_FAILED : PUSH_NETWORK_ERROR;
    }
}

/**
 * @brief 推送到Webhook
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::pushToWebhook(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    String webhookUrl = configMap["webhook_url"];
    if (webhookUrl.isEmpty()) {
        setError("Webhook配置缺少webhook_url");
        return PUSH_CONFIG_ERROR;
    }
    
    String method = configMap["method"];
    if (method.isEmpty()) {
        method = "POST";
    }
    
    String contentType = configMap["content_type"];
    if (contentType.isEmpty()) {
        contentType = "application/json";
    }
    
    // 获取消息模板
    String bodyTemplate = configMap["body_template"];
    if (bodyTemplate.isEmpty()) {
        // 使用默认JSON模板
        bodyTemplate = "{\"sender\":\"{sender}\",\"content\":\"{content}\",\"timestamp\":\"{timestamp}\"}";
    }
    
    String messageBody = applyTemplate(bodyTemplate, context, true); // Webhook需要JSON转义
    
    // 设置请求头
    std::map<String, String> headers;
    headers["Content-Type"] = contentType;
    
    // 添加自定义头部
    String customHeaders = configMap["headers"];
    if (!customHeaders.isEmpty()) {
        // 解析自定义头部（简单实现）
        // 格式: "Header1:Value1,Header2:Value2"
        int startIndex = 0;
        int commaIndex = customHeaders.indexOf(',');
        
        while (commaIndex != -1 || startIndex < customHeaders.length()) {
            String headerPair;
            if (commaIndex != -1) {
                headerPair = customHeaders.substring(startIndex, commaIndex);
                startIndex = commaIndex + 1;
                commaIndex = customHeaders.indexOf(',', startIndex);
            } else {
                headerPair = customHeaders.substring(startIndex);
                startIndex = customHeaders.length();
            }
            
            int colonIndex = headerPair.indexOf(':');
            if (colonIndex != -1) {
                String headerName = headerPair.substring(0, colonIndex);
                String headerValue = headerPair.substring(colonIndex + 1);
                headerName.trim();
                headerValue.trim();
                headers[headerName] = headerValue;
            }
        }
    }
    
    debugPrint("推送到Webhook: " + webhookUrl);
    debugPrint("方法: " + method + ", 内容类型: " + contentType);
    debugPrint("消息内容: " + messageBody);
    
    // 发送HTTP请求
    HttpClient& httpClient = HttpClient::getInstance();
    HttpResponse response;
    
    if (method.equalsIgnoreCase("POST")) {
        response = httpClient.post(webhookUrl, messageBody, headers, 30000);
    } else if (method.equalsIgnoreCase("GET")) {
        response = httpClient.get(webhookUrl, headers, 30000);
    } else {
        setError("不支持的HTTP方法: " + method + "，仅支持POST和GET");
        return PUSH_CONFIG_ERROR;
    }
    
    debugPrint("Webhook响应 - 状态码: " + String(response.statusCode) + ", 错误码: " + String(response.error));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode >= 200 && response.statusCode < 300) {
        debugPrint("✅ Webhook推送成功");
        return PUSH_SUCCESS;
    } else {
        setError("Webhook推送失败，状态码: " + String(response.statusCode) + ", 错误: " + httpClient.getLastError());
        return (response.error == 0) ? PUSH_FAILED : PUSH_NETWORK_ERROR;
    }
}

/**
 * @brief 解析推送配置
 * @param configJson 配置JSON字符串
 * @return std::map<String, String> 配置映射
 */
std::map<String, String> PushManager::parseConfig(const String& configJson) {
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
String PushManager::applyTemplate(const String& templateStr, const PushContext& context, bool escapeForJson) {
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
String PushManager::formatTimestamp(const String& timestamp) {
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
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String PushManager::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void PushManager::setDebugMode(bool enable) {
    debugMode = enable;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void PushManager::setError(const String& error) {
    lastError = error;
    debugPrint("错误: " + error);
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void PushManager::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[PushManager] " + message);
    }
}