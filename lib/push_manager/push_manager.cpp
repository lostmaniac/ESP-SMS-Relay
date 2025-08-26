/**
 * @file push_manager.cpp
 * @brief 推送管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "push_manager.h"
#include "../log_manager/log_manager.h"
#include "../database_manager/database_manager.h"
#include "../http_client/http_diagnostics.h"
#include "../../include/constants.h"
#include <ArduinoJson.h>

// 包含所有推送渠道实现以触发自动注册
#include "channels/wecom_channel.cpp"
#include "channels/wechat_official_channel.cpp"
#include "channels/dingtalk_channel.cpp"
#include "channels/webhook_channel.cpp"
#include "channels/feishu_bot_channel.cpp"

// 单例实例
PushManager& PushManager::getInstance() {
    static PushManager instance;
    return instance;
}

/**
 * @brief 构造函数
 */
PushManager::PushManager() 
    : debugMode(false), initialized(false), cacheLoaded(false) {
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
    if (initialized) {
        return true;
    }
    
    // 首先启用推送管理器的调试模式
    debugMode = true;
    
    debugPrint("初始化推送管理器...");
    
    // 启用渠道注册器的调试模式
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    registry.setDebugMode(true);
    
    // 渠道通过REGISTER_PUSH_CHANNEL宏自动注册，无需手动注册
    debugPrint("检查自动注册的推送渠道...");
    
    // 检查渠道注册状态
    size_t channelCount = registry.getChannelCount();
    debugPrint("当前已注册渠道数量: " + String(channelCount));
    
    std::vector<String> availableChannels = registry.getAvailableChannels();
    debugPrint("可用渠道列表:");
    for (const String& channel : availableChannels) {
        debugPrint("  - " + channel);
    }
    
    if (channelCount == 0) {
        debugPrint("警告: 没有注册任何推送渠道！");
    }
    
    initialized = true;
    debugPrint("推送管理器初始化成功");
    
    // 测试数据库查询功能 - 主动触发一次规则缓存更新
    debugPrint("=== 测试数据库查询功能 ===");
    PushContext testContext;
    testContext.sender = "测试发送方";
    testContext.content = "测试内容";
    testContext.timestamp = "240101120000";
    testContext.smsRecordId = -1;
    
    // 这将触发matchForwardRules方法，从而执行数据库查询
    std::vector<ForwardRule> testRules = matchForwardRules(testContext);
    debugPrint("测试查询完成，获取到 " + String(testRules.size()) + " 条匹配规则");
    debugPrint("=== 数据库查询测试结束 ===");
    
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
 * @param pushType 推送类型
 * @param config 推送配置
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult PushManager::testPushConfig(const String& pushType, const String& config, const String& testMessage) {
    if (!initialized) {
        setError("推送管理器未初始化");
        return PUSH_FAILED;
    }
    
    PushContext testContext;
    testContext.sender = "测试号码";
    testContext.content = testMessage;
    testContext.timestamp = "240101120000"; // 2024-01-01 12:00:00
    testContext.smsRecordId = -1;
    
    return pushToChannel(pushType, config, testContext);
}

/**
 * @brief 根据规则ID测试推送配置
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
    
    // 检查缓存是否已加载，如果没有则加载
    if (!cacheLoaded) {
        debugPrint("规则缓存未加载，开始加载缓存...");
        if (!loadRulesToCache()) {
            debugPrint("加载规则缓存失败: " + lastError);
            return matchedRules; // 返回空列表
        }
    }
    
    debugPrint("开始匹配规则，缓存中共有 " + String(cachedRules.size()) + " 条规则");
    debugPrint("短信发送方: " + context.sender);
    
    for (const auto& rule : cachedRules) {
        debugPrint("检查规则 [" + String(rule.id) + "] " + rule.ruleName + ", 启用状态: " + String(rule.enabled ? "是" : "否"));
        
        // 跳过禁用的规则
        if (!rule.enabled) {
            debugPrint("跳过禁用的规则: " + rule.ruleName);
            continue;
        }
        
        bool matched = false;
        
        // 检查是否为默认转发规则
        if (rule.isDefaultForward) {
            debugPrint("规则 " + rule.ruleName + " 是默认转发规则，直接匹配");
            matched = true;
        } else {
            debugPrint("检查规则 " + rule.ruleName + " 的匹配条件:");
            debugPrint("  来源号码模式: " + rule.sourceNumber);
            debugPrint("  关键词: " + rule.keywords);
            
            // 检查号码匹配
            bool numberMatch = rule.sourceNumber.isEmpty() || 
                              matchPhoneNumber(rule.sourceNumber, context.sender);
            debugPrint("  号码匹配结果: " + String(numberMatch ? "是" : "否"));
            
            // 检查关键词匹配
            bool keywordMatch = rule.keywords.isEmpty() || 
                               matchKeywords(rule.keywords, context.content);
            debugPrint("  关键词匹配结果: " + String(keywordMatch ? "是" : "否"));
            
            matched = numberMatch && keywordMatch;
        }
        
        if (matched) {
            matchedRules.push_back(rule);
            debugPrint("✓ 规则匹配成功: " + rule.ruleName);
        } else {
            debugPrint("✗ 规则不匹配: " + rule.ruleName);
        }
    }
    
    debugPrint("规则匹配完成，共匹配到 " + String(matchedRules.size()) + " 条规则");
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
    
    PushResult result = pushToChannel(rule.pushType, rule.pushConfig, context);
    
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
 * @brief 使用指定渠道执行推送（带重试机制）
 * @param channelName 渠道名称
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult PushManager::pushToChannel(const String& channelName, const String& config, const PushContext& context) {
    if (!initialized) {
        setError("推送管理器未初始化");
        return PUSH_FAILED;
    }
    
    debugPrint("使用渠道推送: " + channelName);
    debugPrint("推送配置: " + config);
    debugPrint("推送内容: " + context.content);
    
    // 通过推送渠道注册器获取具体的推送渠道实例
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    auto channel = registry.createChannel(channelName);
    
    if (!channel) {
        setError("未找到推送渠道: " + channelName);
        debugPrint("❌ 推送失败: 未找到渠道 " + channelName);
        return PUSH_FAILED;
    }
    
    debugPrint("✅ 成功创建推送渠道实例: " + channelName);
    
    // 设置调试模式
    if (debugMode) {
        channel->setDebugMode(true);
    }
    
    // 执行推送，带重试机制
    PushResult result = PUSH_FAILED;
    String lastError = "";
    
    for (int attempt = 1; attempt <= MAX_PUSH_RETRY_COUNT; attempt++) {
        debugPrint("推送尝试 " + String(attempt) + "/" + String(MAX_PUSH_RETRY_COUNT));
        
        // 执行推送
        result = channel->push(config, context);
        
        if (result == PUSH_SUCCESS) {
            debugPrint("✅ 推送成功完成 (尝试 " + String(attempt) + ")");
            break;
        } else {
            // 记录错误信息
            lastError = channel->getLastError();
            debugPrint("❌ 推送失败 (尝试 " + String(attempt) + "): " + lastError);
            
            // 运行HTTP诊断以识别问题原因
            if (lastError.indexOf("HTTP") != -1 || lastError.indexOf("网络") != -1 || lastError.indexOf("连接") != -1) {
                debugPrint("🔍 检测到网络相关错误，运行HTTP诊断...");
                HttpDiagnostics& diagnostics = HttpDiagnostics::getInstance();
                HttpDiagnosticResult diagResult = diagnostics.runFullDiagnostic();
                
                debugPrint("📊 HTTP诊断结果:");
                debugPrint("  - AT命令处理器: " + String(diagResult.atHandlerStatus == HTTP_DIAG_OK ? "正常" : "异常"));
                debugPrint("  - GSM模块: " + String(diagResult.gsmModuleStatus == HTTP_DIAG_OK ? "正常" : "异常"));
                debugPrint("  - 网络连接: " + String(diagResult.networkStatus == HTTP_DIAG_OK ? "正常" : "异常"));
                debugPrint("  - PDP上下文: " + String(diagResult.pdpContextStatus == HTTP_DIAG_OK ? "正常" : "异常"));
                debugPrint("  - HTTP服务: " + String(diagResult.httpServiceStatus == HTTP_DIAG_OK ? "正常" : "异常"));
                debugPrint("  - HTTP功能: " + String(diagResult.httpFunctionStatus == HTTP_DIAG_OK ? "正常" : "异常"));
                
                if (!diagResult.errorMessage.isEmpty()) {
                    debugPrint("  - 错误详情: " + diagResult.errorMessage);
                }
            }
            
            // 如果不是最后一次尝试，等待后重试
            if (attempt < MAX_PUSH_RETRY_COUNT) {
                debugPrint("等待 " + String(PUSH_RETRY_DELAY_MS) + "ms 后重试...");
                delay(PUSH_RETRY_DELAY_MS);
                
                // 垃圾回收：释放当前渠道实例，重新创建
                channel.reset(); // 智能指针自动释放内存
                
                // 重新创建渠道实例
                channel = registry.createChannel(channelName);
                if (!channel) {
                    setError("重试时无法创建推送渠道: " + channelName);
                    debugPrint("❌ 重试失败: 无法重新创建渠道 " + channelName);
                    return PUSH_FAILED;
                }
                
                // 重新设置调试模式
                if (debugMode) {
                    channel->setDebugMode(true);
                }
            }
        }
    }
    
    // 设置最终错误信息
    if (result != PUSH_SUCCESS) {
        setError("推送失败 (" + String(MAX_PUSH_RETRY_COUNT) + "次重试后): " + lastError);
        debugPrint("❌ 推送最终失败，已重试 " + String(MAX_PUSH_RETRY_COUNT) + " 次");
    }
    
    // 垃圾回收：确保渠道实例被正确释放
    channel.reset();
    
    return result;
}

/**
 * @brief 获取所有可用的推送渠道
 * @return std::vector<String> 渠道名称列表
 */
std::vector<String> PushManager::getAvailableChannels() const {
    if (!initialized) {
        return std::vector<String>();
    }
    
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    return registry.getAvailableChannels();
}

/**
 * @brief 获取所有推送渠道的配置示例
 * @return std::vector<PushChannelExample> 配置示例列表
 */
std::vector<PushChannelExample> PushManager::getAllChannelExamples() const {
    if (!initialized) {
        return std::vector<PushChannelExample>();
    }
    
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    std::vector<String> channels = registry.getAvailableChannels();
    std::vector<PushChannelExample> examples;
    
    for (const String& channelName : channels) {
        const PushChannelRegistry::ChannelMetadata* metadata = registry.getChannelMetadata(channelName);
        auto channel = registry.createChannel(channelName);
        if (channel && metadata) {
            PushChannelExample example = channel->getConfigExample();
            example.channelName = channelName;
            example.description = metadata->description;
            examples.push_back(example);
        }
    }
    
    return examples;
}

/**
 * @brief 获取所有推送渠道的帮助信息
 * @return std::vector<PushChannelHelp> 帮助信息列表
 */
std::vector<PushChannelHelp> PushManager::getAllChannelHelp() const {
    if (!initialized) {
        return std::vector<PushChannelHelp>();
    }
    
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    std::vector<String> channels = registry.getAvailableChannels();
    std::vector<PushChannelHelp> helpList;
    
    for (const String& channelName : channels) {
        const PushChannelRegistry::ChannelMetadata* metadata = registry.getChannelMetadata(channelName);
        auto channel = registry.createChannel(channelName);
        if (channel && metadata) {
            PushChannelHelp help = channel->getHelp();
            help.channelName = channelName;
            help.description = metadata->description;
            helpList.push_back(help);
        }
    }
    
    return helpList;
}

/**
 * @brief 获取CLI演示代码
 * @return String 完整的CLI演示代码
 */
String PushManager::getCliDemo() const {
    if (!initialized) {
        return "推送管理器未初始化";
    }
    
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    String registryDemo = "// 推送渠道注册器演示\n// 直接使用注册器管理渠道\n";
    
    String managerDemo = "\n// 推送管理器演示\n";
    managerDemo += "void demoPushManager() {\n";
    managerDemo += "    PushManager& manager = PushManager::getInstance();\n";
    managerDemo += "    manager.setDebugMode(true);\n";
    managerDemo += "    \n";
    managerDemo += "    // 初始化推送管理器\n";
    managerDemo += "    if (!manager.initialize()) {\n";
    managerDemo += "        Serial.println(\"❌ 推送管理器初始化失败: \" + manager.getLastError());\n";
    managerDemo += "        return;\n";
    managerDemo += "    }\n";
    managerDemo += "    \n";
    managerDemo += "    Serial.println(\"✅ 推送管理器初始化成功\");\n";
    managerDemo += "    \n";
    managerDemo += "    // 获取加载统计信息\n";
    managerDemo += "    LoadStatistics stats = manager.getLoadStatistics();\n";
    managerDemo += "    Serial.println(\"\\n渠道加载统计:\");\n";
    managerDemo += "    Serial.println(\"- 总计: \" + String(stats.totalChannels));\n";
    managerDemo += "    Serial.println(\"- 成功: \" + String(stats.loadedChannels));\n";
    managerDemo += "    Serial.println(\"- 失败: \" + String(stats.failedChannels));\n";
    managerDemo += "    \n";
    managerDemo += "    // 获取可用渠道\n";
    managerDemo += "    std::vector<String> channels = manager.getAvailableChannels();\n";
    managerDemo += "    Serial.println(\"\\n可用的推送渠道:\");\n";
    managerDemo += "    for (const String& channel : channels) {\n";
    managerDemo += "        PushChannelRegistry::ChannelMetadata metadata = manager.getChannelMetadata(channel);\n";
    managerDemo += "        Serial.println(\"- \" + channel + \" (\" + metadata.description + \")\");\n";
    managerDemo += "    }\n";
    managerDemo += "    \n";
    managerDemo += "    // 获取配置示例\n";
    managerDemo += "    std::vector<PushChannelExample> examples = manager.getAllChannelExamples();\n";
    managerDemo += "    Serial.println(\"\\n配置示例:\");\n";
    managerDemo += "    for (const PushChannelExample& example : examples) {\n";
    managerDemo += "        Serial.println(\"\\n=== \" + example.channelName + \" ===\");\n";
    managerDemo += "        Serial.println(\"描述: \" + example.description);\n";
    managerDemo += "        Serial.println(\"配置示例:\");\n";
    managerDemo += "        Serial.println(example.configExample);\n";
    managerDemo += "    }\n";
    managerDemo += "    \n";
    managerDemo += "    // 测试推送\n";
    managerDemo += "    String testConfig = \"{\\\"webhook_url\\\":\\\"https://example.com/webhook\\\",\\\"template\\\":\\\"测试消息: {content}\\\"}\";\n";
    managerDemo += "    PushResult result = manager.testPushConfig(\"webhook\", testConfig, \"这是一条测试消息\");\n";
    managerDemo += "    \n";
    managerDemo += "    if (result == PUSH_SUCCESS) {\n";
    managerDemo += "        Serial.println(\"\\n✅ 测试推送成功\");\n";
    managerDemo += "    } else {\n";
    managerDemo += "        Serial.println(\"\\n❌ 测试推送失败: \" + manager.getLastError());\n";
    managerDemo += "    }\n";
    managerDemo += "    \n";
    managerDemo += "    // 测试重新加载渠道\n";
    managerDemo += "    Serial.println(\"\\n测试重新加载渠道...\");\n";
    managerDemo += "    if (manager.reloadChannels()) {\n";
    managerDemo += "        Serial.println(\"✅ 渠道重新加载成功\");\n";
    managerDemo += "        LoadStatistics newStats = manager.getLoadStatistics();\n";
    managerDemo += "        Serial.println(\"新的加载统计: 总计=\" + String(newStats.totalChannels) +\n";
    managerDemo += "                       \", 成功=\" + String(newStats.loadedChannels) +\n";
    managerDemo += "                       \", 失败=\" + String(newStats.failedChannels));\n";
    managerDemo += "    } else {\n";
    managerDemo += "        Serial.println(\"❌ 渠道重新加载失败: \" + manager.getLastError());\n";
    managerDemo += "    }\n";
    managerDemo += "}\n";
    
    return registryDemo + managerDemo;
}

/**
 * @brief 重新加载推送渠道
 * @return true 重新加载成功
 * @return false 重新加载失败
 */
bool PushManager::reloadChannels() {
    if (!initialized) {
        setError("推送管理器未初始化");
        return false;
    }
    
    debugPrint("重新加载推送渠道...");
    
    // 注册表模式下，渠道是静态注册的，无需重新加载
    // 这里可以添加清理缓存等操作
    
    debugPrint("渠道重新加载完成");
    
    return true;
}

/**
 * @brief 获取渠道加载统计信息
 * @return LoadStatistics 加载统计信息
 */
LoadStatistics PushManager::getLoadStatistics() const {
    if (!initialized) {
        return LoadStatistics{0, 0, 0};
    }
    
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    std::vector<String> channels = registry.getAvailableChannels();
    
    return LoadStatistics{(int)channels.size(), (int)channels.size(), 0};
}

/**
 * @brief 获取渠道元数据
 * @param channelName 渠道名称
 * @return PushChannelRegistry::ChannelMetadata 渠道元数据
 */
PushChannelRegistry::ChannelMetadata PushManager::getChannelMetadata(const String& channelName) const {
    if (!initialized) {
        return PushChannelRegistry::ChannelMetadata{"", "", "", "", std::vector<String>(), nullptr};
    }
    
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    const PushChannelRegistry::ChannelMetadata* metadata = registry.getChannelMetadata(channelName);
    if (metadata) {
        return *metadata;
    }
    
    return PushChannelRegistry::ChannelMetadata{"", "", "", "", std::vector<String>(), nullptr};
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

/**
 * @brief 加载规则到缓存
 * @return true 加载成功
 * @return false 加载失败
 */
bool PushManager::loadRulesToCache() {
    if (!initialized) {
        setError("推送管理器未初始化");
        return false;
    }
    
    debugPrint("开始加载转发规则到缓存...");
    
    // 清空现有缓存
    cachedRules.clear();
    
    // 从数据库获取所有转发规则
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    cachedRules = dbManager.getAllForwardRules();
    
    debugPrint("成功加载 " + String(cachedRules.size()) + " 条转发规则到缓存");
    
    // 标记缓存已加载
    cacheLoaded = true;
    
    return true;
}

/**
 * @brief 刷新规则缓存
 * @return true 刷新成功
 * @return false 刷新失败
 */
bool PushManager::refreshRuleCache() {
    if (!initialized) {
        setError("推送管理器未初始化");
        return false;
    }
    
    debugPrint("刷新转发规则缓存...");
    
    // 重置缓存状态
    cacheLoaded = false;
    
    // 重新加载规则
    return loadRulesToCache();
}