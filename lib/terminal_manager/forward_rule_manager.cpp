/**
 * @file forward_rule_manager.cpp
 * @brief 转发规则管理器实现 - 封装SMS转发规则的数据库操作
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "forward_rule_manager.h"
#include "database_manager.h"
#include "log_manager.h"
#include "../push_manager/push_channel_registry.h"
#include "../push_manager/push_manager.h"
#include <ArduinoJson.h>
#include <regex>
#include <algorithm>

// ==================== 构造函数和析构函数 ====================

ForwardRuleManager::ForwardRuleManager() 
    : initialized(false), enableCache(true), cacheSize(50) {
}

ForwardRuleManager::~ForwardRuleManager() {
    cleanup();
}

// ==================== 初始化和清理 ====================

bool ForwardRuleManager::initialize() {
    if (initialized) {
        return true;
    }
    
    // 确保数据库管理器已初始化
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.isReady()) {
        lastError = "Database manager not initialized";
        return false;
    }
    
    // 数据库表由DatabaseManager负责创建，这里不需要额外操作
    
    // 初始化缓存
    if (enableCache) {
        refreshCache();
    }
    
    initialized = true;
    return true;
}

void ForwardRuleManager::cleanup() {
    if (enableCache) {
        ruleCache.clear();
    }
    initialized = false;
}

// ==================== 数据库表创建 ====================
// 注意：数据库表的创建由DatabaseManager负责，这里不需要实现

// ==================== 规则CRUD操作 ====================

int ForwardRuleManager::addRule(const ForwardRule& rule) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return -1;
    }
    
    // 调试输出：显示接收到的规则配置
    Serial.println("[DEBUG] Adding rule with config: '" + rule.pushConfig + "'");
    Serial.println("[DEBUG] Config length: " + String(rule.pushConfig.length()));
    
    // 验证规则
    RuleValidationError validationResult = validateRule(rule);
    if (validationResult != RULE_VALID) {
        String errorDesc = getValidationErrorDescription(validationResult);
        // 如果是JSON验证错误，添加详细的错误信息
        if (validationResult == RULE_ERROR_INVALID_JSON) {
            // 保存JSON验证的详细错误信息
            String jsonError = lastError;
            errorDesc = "Invalid push config format (must be valid JSON): " + jsonError + ". Received config: '" + rule.pushConfig + "'";
        }
        lastError = "Rule validation failed: " + errorDesc;
        return -1;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 直接使用DatabaseManager的公共接口，不需要手动构建SQL
    
    // 使用DatabaseManager的公共接口
    int ruleId = db.addForwardRule(rule);
    if (ruleId <= 0) {
        lastError = "Failed to insert rule: " + db.getLastError();
        return -1;
    }
    
    // 更新缓存
    if (enableCache) {
        ForwardRule newRule = rule;
        newRule.id = ruleId;
        addToCache(newRule);
    }
    
    // 刷新推送管理器缓存
    PushManager::getInstance().refreshRuleCache();
    
    return ruleId;
}

bool ForwardRuleManager::updateRule(const ForwardRule& rule) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    if (rule.id <= 0) {
        lastError = "Invalid rule ID";
        return false;
    }
    
    // 验证规则
    RuleValidationError validationResult = validateRule(rule);
    if (validationResult != RULE_VALID) {
        lastError = "Rule validation failed: " + getValidationErrorDescription(validationResult);
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 直接使用DatabaseManager的公共接口，不需要手动构建SQL
    
    // 使用DatabaseManager的公共接口
    if (!db.updateForwardRule(rule)) {
        lastError = "Failed to update rule: " + db.getLastError();
        return false;
    }
    
    // 更新缓存
    if (enableCache) {
        updateCache(rule);
    }
    
    return true;
}

bool ForwardRuleManager::deleteRule(int ruleId) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    if (ruleId <= 0) {
        lastError = "Invalid rule ID";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 使用DatabaseManager的公共接口
    if (!db.deleteForwardRule(ruleId)) {
        lastError = "Failed to delete rule: " + db.getLastError();
        return false;
    }
    
    // 更新缓存
    if (enableCache) {
        removeFromCache(ruleId);
    }
    
    // 刷新推送管理器缓存
    PushManager::getInstance().refreshRuleCache();
    
    return true;
}

ForwardRule ForwardRuleManager::getRule(int ruleId) {
    ForwardRule emptyRule;
    emptyRule.id = -1;
    
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return emptyRule;
    }
    
    if (ruleId <= 0) {
        lastError = "Invalid rule ID";
        return emptyRule;
    }
    
    // 先尝试从缓存获取
    if (enableCache) {
        for (const ForwardRule& rule : ruleCache) {
            if (rule.id == ruleId) {
                return rule;
            }
        }
    }
    
    // 从数据库查询
    DatabaseManager& db = DatabaseManager::getInstance();
    
    ForwardRule rule = db.getForwardRuleById(ruleId);
    if (rule.id == 0) {
        lastError = "Rule not found";
        return emptyRule;
    }
    
    // 添加到缓存
    if (enableCache) {
        addToCache(rule);
    }
    
    return rule;
}

std::vector<ForwardRule> ForwardRuleManager::getRules(const RuleQueryCondition& condition) {
    std::vector<ForwardRule> rules;
    
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return rules;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 构建SQL查询
    String sql = "SELECT * FROM forward_rules";
    std::vector<String> params;
    std::vector<String> whereClauses;
    
    // 添加过滤条件
    if (condition.filterByEnabled) {
        whereClauses.push_back("enabled = ?");
        params.push_back(String(condition.enabledValue ? 1 : 0));
    }
    
    if (condition.filterByPushType && !condition.pushType.isEmpty()) {
        whereClauses.push_back("push_type = ?");
        params.push_back(condition.pushType);
    }
    
    // 添加WHERE子句
    if (!whereClauses.empty()) {
        sql += " WHERE ";
        for (size_t i = 0; i < whereClauses.size(); i++) {
            if (i > 0) sql += " AND ";
            sql += whereClauses[i];
        }
    }
    
    // 添加排序
    if (condition.orderByPriority) {
        sql += " ORDER BY priority DESC, id ASC";
    } else {
        sql += " ORDER BY id ASC";
    }
    
    // 添加限制
    if (condition.limit > 0) {
        sql += " LIMIT " + String(condition.limit);
        if (condition.offset > 0) {
            sql += " OFFSET " + String(condition.offset);
        }
    }
    
    // 由于executeQuery需要回调函数，我们使用getAllForwardRules然后进行过滤
    std::vector<ForwardRule> allRules = db.getAllForwardRules();
    
    // 根据条件过滤规则
    for (const ForwardRule& rule : allRules) {
        bool match = true;
        
        // 检查启用状态
        if (condition.filterByEnabled && rule.enabled != condition.enabledValue) {
            match = false;
        }
        
        // 检查推送类型匹配
        if (match && condition.filterByPushType && !condition.pushType.isEmpty()) {
            if (rule.pushType != condition.pushType) {
                match = false;
            }
        }
        
        if (match) {
            rules.push_back(rule);
        }
    }
    
    // 排序
    if (condition.orderByPriority) {
        // 由于 ForwardRule 结构体中没有 priority 字段，简单按 ID 排序
        std::sort(rules.begin(), rules.end(), [](const ForwardRule& a, const ForwardRule& b) {
            return a.id < b.id; // 按ID升序排序
        });
    }
    
    // 应用限制和偏移
    if (condition.limit > 0) {
        int startIndex = condition.offset;
        int endIndex = std::min(startIndex + condition.limit, (int)rules.size());
        
        if (startIndex < rules.size()) {
            rules = std::vector<ForwardRule>(rules.begin() + startIndex, rules.begin() + endIndex);
        } else {
            rules.clear();
        }
    }
    
    return rules;
}

// ==================== 规则状态管理 ====================

bool ForwardRuleManager::enableRule(int ruleId) {
    return setRuleEnabled(ruleId, true);
}

bool ForwardRuleManager::disableRule(int ruleId) {
    return setRuleEnabled(ruleId, false);
}

bool ForwardRuleManager::setRuleEnabled(int ruleId, bool enabled) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    if (ruleId <= 0) {
        lastError = "Invalid rule ID";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 由于DatabaseManager没有专门的更新启用状态接口，我们需要先获取规则再更新
    ForwardRule rule = db.getForwardRuleById(ruleId);
    if (rule.id == 0) {
        lastError = "Rule not found: " + String(ruleId);
        return false;
    }
    
    rule.enabled = enabled;
    
    if (!db.updateForwardRule(rule)) {
        lastError = "Failed to update rule status: " + db.getLastError();
        return false;
    }
    
    // 更新缓存
    if (enableCache) {
        updateRuleEnabledInCache(ruleId, enabled);
    }
    
    return true;
}

bool ForwardRuleManager::setRulePriority(int ruleId, int priority) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    if (ruleId <= 0) {
        lastError = "Invalid rule ID";
        return false;
    }
    
    if (priority < 0 || priority > 1000) {
        lastError = "Priority must be between 0 and 1000";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 由于DatabaseManager没有专门的更新优先级接口，我们需要先获取规则再更新
    ForwardRule rule = db.getForwardRuleById(ruleId);
    if (rule.id == 0) {
        lastError = "Rule not found: " + String(ruleId);
        return false;
    }
    
    // priority 字段不存在于 ForwardRule 结构体中，跳过优先级设置
    
    if (!db.updateForwardRule(rule)) {
        lastError = "Failed to update rule priority: " + db.getLastError();
        return false;
    }
    
    // 更新缓存
    if (enableCache) {
        updateRulePriorityInCache(ruleId, priority);
    }
    
    return true;
}

// ==================== 规则验证和测试 ====================

RuleValidationError ForwardRuleManager::validateRule(const ForwardRule& rule) {
    // 检查必填字段
    if (rule.ruleName.isEmpty()) {
        return RULE_ERROR_EMPTY_NAME;
    }
    
    // sourceNumber 可以为空（支持通配符和默认转发），不需要检查
    // if (rule.sourceNumber.isEmpty()) {
    //     return RULE_ERROR_EMPTY_SENDER;
    // }
    
    if (rule.pushType.isEmpty()) {
        return RULE_ERROR_EMPTY_PUSH_TYPE;
    }
    
    if (rule.pushConfig.isEmpty()) {
        return RULE_ERROR_EMPTY_PUSH_CONFIG;
    }
    
    // 验证sourceNumber的正则表达式模式（如果不为空且不是简单通配符）
    if (!rule.sourceNumber.isEmpty() && !validateRegexPattern(rule.sourceNumber)) {
        return RULE_ERROR_INVALID_REGEX;
    }
    
    // 验证keywords的正则表达式模式（如果不为空且不是简单通配符）
    if (!rule.keywords.isEmpty() && !validateRegexPattern(rule.keywords)) {
        return RULE_ERROR_INVALID_REGEX;
    }
    
    // 检查推送类型（动态检查已注册的渠道）
    PushChannelRegistry& registry = PushChannelRegistry::getInstance();
    if (!registry.isChannelSupported(rule.pushType)) {
        return RULE_ERROR_INVALID_PUSH_TYPE;
    }
    
    // 检查推送配置格式（使用ArduinoJson进行真正的JSON验证）
    if (!validateJSON(rule.pushConfig)) {
        return RULE_ERROR_INVALID_JSON;
    }
    
    return RULE_VALID;
}

bool ForwardRuleManager::testRuleMatch(int ruleId, const String& sender, const String& content) {
    ForwardRule rule = getRule(ruleId);
    if (rule.id == -1) {
        lastError = "Rule not found";
        return false;
    }
    
    return testRuleMatch(rule, sender, content);
}

bool ForwardRuleManager::testRuleMatch(const ForwardRule& rule, const String& sender, const String& content) {
    // 检查发送者模式匹配
    if (!matchPattern(sender, rule.sourceNumber)) {
        return false;
    }
    
    // 检查内容模式匹配（如果设置了）
    if (!rule.keywords.isEmpty()) {
        if (!matchPattern(content, rule.keywords)) {
            return false;
        }
    }
    
    return true;
}

// ==================== 统计信息 ====================

int ForwardRuleManager::getRuleCount() {
    if (!initialized) {
        return 0;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 使用高效的COUNT查询而不是获取所有规则
    return db.getForwardRuleCount();
}

int ForwardRuleManager::getEnabledRuleCount() {
    if (!initialized) {
        return 0;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 使用高效的COUNT查询而不是获取所有规则
    return db.getEnabledForwardRuleCount();
}

std::vector<ForwardRule> ForwardRuleManager::getMostUsedRules(int limit) {
    if (!initialized) {
        return std::vector<ForwardRule>();
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 使用getAllForwardRules获取所有规则，然后手动排序
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    
    // 按使用次数和最后使用时间排序
    // 由于ForwardRule结构体中没有usageCount和lastUsedTime字段，
    // 这里简单按照ID排序
    std::sort(rules.begin(), rules.end(), [](const ForwardRule& a, const ForwardRule& b) {
        return a.id < b.id; // 按ID升序排序
    });
    
    // 如果指定了限制，则截取前N个
    if (limit > 0 && rules.size() > static_cast<size_t>(limit)) {
        rules.resize(limit);
    }
    
    return rules;
}

bool ForwardRuleManager::updateRuleUsage(int ruleId) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    if (ruleId <= 0) {
        lastError = "Invalid rule ID";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 由于DatabaseManager没有专门的更新使用次数接口，我们需要先获取规则再更新
    ForwardRule rule = db.getForwardRuleById(ruleId);
    if (rule.id == 0) {
        lastError = "Rule not found: " + String(ruleId);
        return false;
    }
    
    // 由于ForwardRule结构体中没有usageCount和lastUsedTime字段，
    // 这里只更新updatedAt字段
    rule.updatedAt = getCurrentTimestamp();
    
    if (!db.updateForwardRule(rule)) {
        lastError = "Failed to update rule usage: " + db.getLastError();
        return false;
    }
    
    // 更新缓存
    if (enableCache) {
        updateRuleUsageInCache(ruleId);
    }
    
    return true;
}

// ==================== 批量操作 ====================

bool ForwardRuleManager::enableAllRules() {
    return setAllRulesEnabled(true);
}

bool ForwardRuleManager::disableAllRules() {
    return setAllRulesEnabled(false);
}

bool ForwardRuleManager::setAllRulesEnabled(bool enabled) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 由于DatabaseManager没有批量更新接口，我们需要逐个更新规则
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    for (ForwardRule& rule : rules) {
        rule.enabled = enabled;
        if (!db.updateForwardRule(rule)) {
            lastError = "Failed to update rule " + String(rule.id) + ": " + db.getLastError();
            return false;
        }
    }
    
    // 刷新缓存
    if (enableCache) {
        refreshCache();
    }
    
    return true;
}

bool ForwardRuleManager::deleteAllRules() {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    // 由于DatabaseManager没有批量删除接口，我们需要逐个删除规则
    std::vector<ForwardRule> rules = db.getAllForwardRules();
    for (const ForwardRule& rule : rules) {
        if (!db.deleteForwardRule(rule.id)) {
            lastError = "Failed to delete rule " + String(rule.id) + ": " + db.getLastError();
            return false;
        }
    }
    
    // 清空缓存
    if (enableCache) {
        ruleCache.clear();
    }
    
    return true;
}

bool ForwardRuleManager::importRules(const std::vector<ForwardRule>& rules) {
    if (!initialized) {
        lastError = "ForwardRuleManager not initialized";
        return false;
    }
    
    DatabaseManager& db = DatabaseManager::getInstance();
    
    bool success = true;
    
    for (const ForwardRule& rule : rules) {
        // 验证规则
        RuleValidationError validationResult = validateRule(rule);
        if (validationResult != RULE_VALID) {
            lastError = "Rule validation failed for '" + rule.ruleName + "': " + 
                       getValidationErrorDescription(validationResult);
            success = false;
            break;
        }
        
        // 使用DatabaseManager的公共接口插入规则
        int newRuleId = db.addForwardRule(rule);
        if (newRuleId <= 0) {
            lastError = "Failed to import rule '" + rule.ruleName + "': " + db.getLastError();
            success = false;
            break;
        }
    }
    
    // 刷新缓存
    if (success && enableCache) {
        refreshCache();
    }
    
    return success;
}

// ==================== 缓存管理 ====================

void ForwardRuleManager::refreshCache() {
    if (!enableCache || !initialized) {
        return;
    }
    
    ruleCache.clear();
    
    // 加载最常用的规则到缓存
    RuleQueryCondition condition;
    condition.limit = cacheSize;
    condition.orderByPriority = true;
    
    std::vector<ForwardRule> rules = getRules(condition);
    for (const ForwardRule& rule : rules) {
        ruleCache.push_back(rule);
    }
}

void ForwardRuleManager::addToCache(const ForwardRule& rule) {
    if (!enableCache) {
        return;
    }
    
    // 检查是否已存在
    for (size_t i = 0; i < ruleCache.size(); i++) {
        if (ruleCache[i].id == rule.id) {
            ruleCache[i] = rule; // 更新
            return;
        }
    }
    
    // 添加新规则
    ruleCache.push_back(rule);
    
    // 如果超过缓存大小，移除最少使用的
    if (ruleCache.size() > cacheSize) {
        // 简单策略：移除第一个（最老的）
        ruleCache.erase(ruleCache.begin());
    }
}

void ForwardRuleManager::addOrUpdateCache(const ForwardRule& rule) {
    // 这是addToCache的别名，保持兼容性
    addToCache(rule);
}

void ForwardRuleManager::updateCache(const ForwardRule& rule) {
    if (!enableCache) {
        return;
    }
    
    for (size_t i = 0; i < ruleCache.size(); i++) {
        if (ruleCache[i].id == rule.id) {
            ruleCache[i] = rule;
            return;
        }
    }
}

void ForwardRuleManager::removeFromCache(int ruleId) {
    if (!enableCache) {
        return;
    }
    
    for (auto it = ruleCache.begin(); it != ruleCache.end(); ++it) {
        if (it->id == ruleId) {
            ruleCache.erase(it);
            return;
        }
    }
}

void ForwardRuleManager::updateRuleEnabledInCache(int ruleId, bool enabled) {
    if (!enableCache) {
        return;
    }
    
    for (ForwardRule& rule : ruleCache) {
        if (rule.id == ruleId) {
            rule.enabled = enabled;
            rule.updatedAt = String(getCurrentTimestamp());  // 使用 updatedAt 而不是 updateTime
            return;
        }
    }
}

void ForwardRuleManager::updateRulePriorityInCache(int ruleId, int priority) {
    if (!enableCache) {
        return;
    }
    
    for (ForwardRule& rule : ruleCache) {
        if (rule.id == ruleId) {
            // priority 字段不存在于 ForwardRule 结构体中，跳过优先级设置
            rule.updatedAt = String(getCurrentTimestamp());  // 使用 updatedAt 而不是 updateTime
            return;
        }
    }
}

void ForwardRuleManager::updateRuleUsageInCache(int ruleId) {
    if (!enableCache) {
        return;
    }
    
    for (ForwardRule& rule : ruleCache) {
        if (rule.id == ruleId) {
            // usageCount 和 lastUsedTime 字段不存在于 ForwardRule 结构体中
            // 只更新 updatedAt 字段
            rule.updatedAt = String(getCurrentTimestamp());
            return;
        }
    }
}

// ==================== 辅助方法 ====================

ForwardRule ForwardRuleManager::parseRuleFromRow(const std::vector<String>& row) {
    ForwardRule rule;
    
    if (row.size() >= 8) {
        rule.id = row[0].toInt();
        rule.ruleName = row[1];
        rule.sourceNumber = row[2];
        rule.keywords = row[3];
        rule.pushType = row[4];
        rule.pushConfig = row[5];
        rule.enabled = (row[6].toInt() == 1);
        rule.isDefaultForward = (row[7].toInt() == 1);
        
        if (row.size() >= 10) {
            rule.createdAt = row[8];
            rule.updatedAt = row[9];
        }
    }
    
    return rule;
}

bool ForwardRuleManager::matchPattern(const String& text, const String& pattern) {
    // 简单的模式匹配实现
    // 支持通配符 * 和 ?
    
    if (pattern.isEmpty()) {
        return true;
    }
    
    if (pattern == "*") {
        return true;
    }
    
    // 如果模式不包含通配符，进行精确匹配
    if (pattern.indexOf('*') == -1 && pattern.indexOf('?') == -1) {
        return text == pattern;
    }
    
    // 简单的通配符匹配实现
    return simpleWildcardMatch(text, pattern);
}

bool ForwardRuleManager::simpleWildcardMatch(const String& text, const String& pattern) {
    int textLen = text.length();
    int patternLen = pattern.length();
    
    // 动态规划匹配
    std::vector<std::vector<bool>> dp(textLen + 1, std::vector<bool>(patternLen + 1, false));
    
    dp[0][0] = true;
    
    // 处理模式开头的 *
    for (int j = 1; j <= patternLen; j++) {
        if (pattern.charAt(j - 1) == '*') {
            dp[0][j] = dp[0][j - 1];
        }
    }
    
    for (int i = 1; i <= textLen; i++) {
        for (int j = 1; j <= patternLen; j++) {
            char textChar = text.charAt(i - 1);
            char patternChar = pattern.charAt(j - 1);
            
            if (patternChar == '*') {
                dp[i][j] = dp[i - 1][j] || dp[i][j - 1];
            } else if (patternChar == '?' || textChar == patternChar) {
                dp[i][j] = dp[i - 1][j - 1];
            }
        }
    }
    
    return dp[textLen][patternLen];
}

unsigned long ForwardRuleManager::getCurrentTimestamp() {
    // 返回当前时间戳（秒）
    // 这里需要根据实际的时间获取方式实现
    return millis() / 1000; // 简单实现，实际应该使用RTC或NTP时间
}

String ForwardRuleManager::getValidationErrorDescription(RuleValidationError error) {
    switch (error) {
        case RULE_VALID:
            return "Rule is valid";
        case RULE_ERROR_EMPTY_NAME:
            return "Rule name cannot be empty";
        case RULE_ERROR_EMPTY_SENDER:
            return "Sender pattern cannot be empty";
        case RULE_ERROR_EMPTY_PUSH_TYPE:
            return "Push type cannot be empty";
        case RULE_ERROR_EMPTY_PUSH_CONFIG:
            return "Push config cannot be empty";
        case RULE_ERROR_INVALID_PRIORITY:
            return "Priority must be between 0 and 1000";
        case RULE_ERROR_INVALID_REGEX:
            return "Invalid regular expression pattern";
        case RULE_ERROR_INVALID_JSON:
            return "Invalid push config format (must be valid JSON)";
        case RULE_ERROR_INVALID_PUSH_TYPE:
            return "Invalid push type (must be wechat, dingtalk, webhook, or wechat_official)";
        default:
            return "Unknown validation error";
    }
}

bool ForwardRuleManager::validateRegexPattern(const String& pattern) {
    // 如果是空字符串，认为是有效的（可选字段）
    if (pattern.isEmpty()) {
        return true;
    }
    
    // 简单的通配符模式（*和?）总是有效的
    if (pattern == "*" || pattern == "?" || pattern.indexOf('*') != -1 || pattern.indexOf('?') != -1) {
        return true;
    }
    
    // 对于其他模式，进行基本的正则表达式语法检查
    // 检查是否包含不平衡的括号
    int openBrackets = 0;
    int openParens = 0;
    int openBraces = 0;
    
    for (int i = 0; i < pattern.length(); i++) {
        char c = pattern.charAt(i);
        
        switch (c) {
            case '[':
                openBrackets++;
                break;
            case ']':
                openBrackets--;
                if (openBrackets < 0) return false;
                break;
            case '(':
                openParens++;
                break;
            case ')':
                openParens--;
                if (openParens < 0) return false;
                break;
            case '{':
                openBraces++;
                break;
            case '}':
                openBraces--;
                if (openBraces < 0) return false;
                break;
            case '\\':
                // 跳过转义字符
                i++;
                if (i >= pattern.length()) return false;
                break;
        }
    }
    
    // 检查是否所有括号都已闭合
    return (openBrackets == 0 && openParens == 0 && openBraces == 0);
}

String ForwardRuleManager::getLastError() {
    return lastError;
}

bool ForwardRuleManager::validateJSON(const String& jsonStr) {
    // 检查空字符串
    if (jsonStr.isEmpty()) {
        lastError = "JSON string is empty";
        return false;
    }
    
    // 检查基本的JSON格式
    String trimmed = jsonStr;
    trimmed.trim();
    
    if (!trimmed.startsWith("{") || !trimmed.endsWith("}")) {
        lastError = "JSON must start with '{' and end with '}'";
        return false;
    }
    
    // 使用ArduinoJson库进行JSON验证
    JsonDocument doc;
    
    // 尝试解析JSON
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    // 如果解析失败，返回false并记录详细错误
    if (error) {
        lastError = "JSON parse error: " + String(error.c_str());
        return false;
    }
    
    // 检查是否为空对象
    if (doc.isNull()) {
        lastError = "JSON document is null";
        return false;
    }
    
    // 检查是否为对象类型
    if (!doc.is<JsonObject>()) {
        lastError = "JSON must be an object, not array or primitive";
        return false;
    }
    
    return true;
}