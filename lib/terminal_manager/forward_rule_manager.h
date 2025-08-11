/**
 * @file forward_rule_manager.h
 * @brief 转发规则管理器 - 封装数据库操作，提供规则的CRUD操作
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 转发规则管理器负责封装数据库操作，提供规则的CRUD操作，
 * 处理规则验证和管理规则缓存。
 */

#ifndef FORWARD_RULE_MANAGER_H
#define FORWARD_RULE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "../database_manager/database_manager.h"

/**
 * @struct RuleQueryCondition
 * @brief 规则查询条件
 */
struct RuleQueryCondition {
    bool filterByEnabled;          ///< 是否按启用状态过滤
    bool enabledValue;             ///< 启用状态值
    bool filterByPushType;         ///< 是否按推送类型过滤
    String pushType;               ///< 推送类型
    bool orderByPriority;          ///< 是否按优先级排序
    bool orderByCreateTime;        ///< 是否按创建时间排序
    int limit;                     ///< 限制返回数量
    int offset;                    ///< 偏移量
    
    /**
     * @brief 默认构造函数
     */
    RuleQueryCondition() : filterByEnabled(false), enabledValue(true),
                          filterByPushType(false), orderByPriority(false),
                          orderByCreateTime(false), limit(-1), offset(0) {}
};

/**
 * @enum RuleValidationError
 * @brief 规则验证错误类型
 */
enum RuleValidationError {
    RULE_VALID = 0,               ///< 规则有效
    RULE_ERROR_EMPTY_NAME,        ///< 规则名称为空
    RULE_ERROR_EMPTY_SENDER,      ///< 发送者模式为空
    RULE_ERROR_EMPTY_PUSH_TYPE,   ///< 推送类型为空
    RULE_ERROR_EMPTY_PUSH_CONFIG, ///< 推送配置为空
    RULE_ERROR_INVALID_PRIORITY,  ///< 无效优先级
    RULE_ERROR_INVALID_REGEX,     ///< 无效正则表达式
    RULE_ERROR_INVALID_JSON,      ///< 无效JSON配置
    RULE_ERROR_INVALID_PUSH_TYPE  ///< 无效推送类型
};

/**
 * @class ForwardRuleManager
 * @brief 转发规则管理器类
 * 
 * 该类负责：
 * - 封装数据库操作
 * - 提供规则的CRUD操作
 * - 处理规则验证
 * - 管理规则缓存
 */
class ForwardRuleManager {
public:
    /**
     * @brief 构造函数
     */
    ForwardRuleManager();
    
    /**
     * @brief 析构函数
     */
    ~ForwardRuleManager();
    
    /**
     * @brief 初始化规则管理器
     * @return bool 成功返回true，失败返回false
     */
    bool initialize();
    
    /**
     * @brief 清理资源
     */
    void cleanup();
    
    // ==================== 规则CRUD操作 ====================
    
    /**
     * @brief 添加转发规则
     * @param rule 规则对象
     * @return int 规则ID，失败返回-1
     */
    int addRule(const ForwardRule& rule);
    
    /**
     * @brief 更新转发规则
     * @param rule 规则对象（必须包含有效的ID）
     * @return bool 成功返回true，失败返回false
     */
    bool updateRule(const ForwardRule& rule);
    
    /**
     * @brief 删除转发规则
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool deleteRule(int ruleId);
    
    /**
     * @brief 获取单个转发规则
     * @param ruleId 规则ID
     * @return ForwardRule 规则对象，失败时ID为-1
     */
    ForwardRule getRule(int ruleId);
    
    /**
     * @brief 获取转发规则列表
     * @param condition 查询条件
     * @return std::vector<ForwardRule> 规则列表
     */
    std::vector<ForwardRule> getRules(const RuleQueryCondition& condition = {});
    
    /**
     * @brief 检查规则是否存在
     * @param ruleId 规则ID
     * @return bool 存在返回true
     */
    bool ruleExists(int ruleId);
    
    /**
     * @brief 检查规则名称是否存在
     * @param name 规则名称
     * @param excludeId 排除的规则ID（用于更新时检查）
     * @return bool 存在返回true
     */
    bool ruleNameExists(const String& name, int excludeId = -1);
    
    // ==================== 规则状态管理 ====================
    
    /**
     * @brief 启用规则
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool enableRule(int ruleId);
    
    /**
     * @brief 禁用规则
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool disableRule(int ruleId);
    
    /**
     * @brief 设置规则启用状态
     * @param ruleId 规则ID
     * @param enabled 是否启用
     * @return bool 成功返回true，失败返回false
     */
    bool setRuleEnabled(int ruleId, bool enabled);
    
    /**
     * @brief 设置规则优先级
     * @param ruleId 规则ID
     * @param priority 优先级（数字越小优先级越高）
     * @return bool 成功返回true，失败返回false
     */
    bool setRulePriority(int ruleId, int priority);
    
    /**
     * @brief 更新规则使用统计
     * @param ruleId 规则ID
     * @return bool 成功返回true，失败返回false
     */
    bool updateRuleUsage(int ruleId);
    
    // ==================== 批量操作 ====================
    
    /**
     * @brief 启用所有规则
     * @return bool 成功返回true，失败返回false
     */
    bool enableAllRules();
    
    /**
     * @brief 禁用所有规则
     * @return bool 成功返回true，失败返回false
     */
    bool disableAllRules();
    
    /**
     * @brief 删除所有规则
     * @return bool 成功返回true，失败返回false
     */
    bool deleteAllRules();
    
    /**
     * @brief 批量导入规则
     * @param rules 规则列表
     * @return bool 成功返回true，失败返回false
     */
    bool importRules(const std::vector<ForwardRule>& rules);
    
    // ==================== 统计信息 ====================
    
    /**
     * @brief 获取规则总数
     * @return int 规则总数
     */
    int getRuleCount();
    
    /**
     * @brief 获取启用规则数量
     * @return int 启用规则数量
     */
    int getEnabledRuleCount();
    
    /**
     * @brief 获取最常用的规则
     * @param limit 限制数量
     * @return std::vector<ForwardRule> 最常用规则列表
     */
    std::vector<ForwardRule> getMostUsedRules(int limit = 10);
    
    // ==================== 规则验证和测试 ====================
    
    /**
     * @brief 验证规则配置是否有效
     * @param rule 规则对象
     * @return RuleValidationError 验证结果
     */
    RuleValidationError validateRule(const ForwardRule& rule);
    
    /**
     * @brief 测试规则是否匹配
     * @param ruleId 规则ID
     * @param sender 发送者号码
     * @param content 短信内容
     * @return bool 匹配返回true，不匹配返回false
     */
    bool testRuleMatch(int ruleId, const String& sender, const String& content);
    
    /**
     * @brief 测试规则模式匹配
     * @param pattern 匹配模式
     * @param text 待匹配文本
     * @return bool 匹配返回true
     */
    bool testPatternMatch(const String& pattern, const String& text);
    
    // ==================== 错误处理 ====================
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 获取验证错误描述
     * @param error 验证错误类型
     * @return String 错误描述
     */
    static String getValidationErrorDescription(RuleValidationError error);
    
private:
    // ==================== 私有方法 ====================
    
    /**
     * @brief 初始化数据库表
     * @return bool 成功返回true
     */
    bool initializeDatabase();
    
    /**
     * @brief 构建查询SQL
     * @param condition 查询条件
     * @return String SQL语句
     */
    String buildQuerySQL(const RuleQueryCondition& condition);
    
    /**
     * @brief 从结果集构建规则对象
     * @param result 数据库查询结果
     * @return ForwardRule 规则对象
     */
    ForwardRule buildRuleFromResult(void* result);
    
    /**
     * @brief 验证JSON格式
     * @param jsonStr JSON字符串
     * @return bool 有效返回true
     */
    bool validateJSON(const String& jsonStr);
    
    /**
     * @brief 验证正则表达式
     * @param pattern 正则表达式
     * @return bool 有效返回true
     */
    bool validateRegex(const String& pattern);
    
    /**
     * @brief 验证正则表达式模式
     * @param pattern 正则表达式模式
     * @return bool 有效返回true
     */
    bool validateRegexPattern(const String& pattern);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 获取当前时间戳
     * @return unsigned long Unix时间戳
     */
    unsigned long getCurrentTimestamp();
    
    /**
     * @brief 模式匹配
     * @param text 待匹配文本
     * @param pattern 匹配模式
     * @return bool 匹配返回true
     */
    bool matchPattern(const String& text, const String& pattern);
    
    /**
     * @brief 简单通配符匹配
     * @param text 待匹配文本
     * @param pattern 匹配模式
     * @return bool 匹配返回true
     */
    bool simpleWildcardMatch(const String& text, const String& pattern);
    
    /**
     * @brief 测试规则匹配（重载版本）
     * @param rule 规则对象
     * @param sender 发送者号码
     * @param content 短信内容
     * @return bool 匹配返回true
     */
    bool testRuleMatch(const ForwardRule& rule, const String& sender, const String& content);
    
    /**
     * @brief 从数据库行解析规则
     * @param row 数据库行数据
     * @return ForwardRule 规则对象
     */
    ForwardRule parseRuleFromRow(const std::vector<String>& row);
    
    /**
     * @brief 设置所有规则的启用状态
     * @param enabled 启用状态
     * @return bool 成功返回true
     */
    bool setAllRulesEnabled(bool enabled);
    
    // ==================== 缓存管理 ====================
    
    /**
     * @brief 添加到缓存
     * @param rule 规则对象
     */
    void addToCache(const ForwardRule& rule);
    
    /**
     * @brief 添加或更新缓存（兼容性方法）
     * @param rule 规则对象
     */
    void addOrUpdateCache(const ForwardRule& rule);
    
    /**
     * @brief 刷新缓存
     */
    void refreshCache();
    
    /**
     * @brief 更新缓存
     * @param rule 规则对象
     */
    void updateCache(const ForwardRule& rule);
    
    /**
     * @brief 从缓存移除
     * @param ruleId 规则ID
     */
    void removeFromCache(int ruleId);
    
    /**
     * @brief 更新缓存中规则的启用状态
     * @param ruleId 规则ID
     * @param enabled 启用状态
     */
    void updateRuleEnabledInCache(int ruleId, bool enabled);
    
    /**
     * @brief 更新缓存中规则的优先级
     * @param ruleId 规则ID
     * @param priority 优先级
     */
    void updateRulePriorityInCache(int ruleId, int priority);
    
    /**
     * @brief 更新缓存中规则的使用统计
     * @param ruleId 规则ID
     */
    void updateRuleUsageInCache(int ruleId);
    
    // ==================== 私有成员变量 ====================
    
    bool initialized;              ///< 初始化状态
    String lastError;             ///< 最后的错误信息
    
    // 缓存相关
    bool enableCache;             ///< 是否启用缓存
    size_t cacheSize;             ///< 缓存大小
    std::vector<ForwardRule> ruleCache; ///< 规则缓存
};

#endif // FORWARD_RULE_MANAGER_H