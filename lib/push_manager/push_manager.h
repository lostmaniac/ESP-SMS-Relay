/**
 * @file push_manager.h
 * @brief 推送管理器 - 统一管理短信转发推送功能
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 根据转发规则匹配短信
 * 2. 支持多种推送类型（企业微信、钉钉、Webhook等）
 * 3. 推送模板管理
 * 4. 推送状态跟踪
 */

#ifndef PUSH_MANAGER_H
#define PUSH_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "../database_manager/database_manager.h"
#include "../http_client/http_client.h"

/**
 * @enum PushResult
 * @brief 推送结果枚举
 */
enum PushResult {
    PUSH_SUCCESS = 0,      ///< 推送成功
    PUSH_FAILED = 1,       ///< 推送失败
    PUSH_NO_RULE = 2,      ///< 没有匹配的规则
    PUSH_RULE_DISABLED = 3, ///< 规则已禁用
    PUSH_CONFIG_ERROR = 4,  ///< 配置错误
    PUSH_NETWORK_ERROR = 5  ///< 网络错误
};

/**
 * @struct PushContext
 * @brief 推送上下文结构体
 */
struct PushContext {
    String sender;         ///< 发送方号码
    String content;        ///< 短信内容
    String timestamp;      ///< 接收时间戳
    int smsRecordId;       ///< 短信记录ID
};

/**
 * @struct PushTemplate
 * @brief 推送模板结构体
 */
struct PushTemplate {
    String title;          ///< 消息标题模板
    String content;        ///< 消息内容模板
    String format;         ///< 消息格式（text, markdown, json等）
};

/**
 * @class PushManager
 * @brief 推送管理器类
 * 
 * 负责统一管理短信转发推送功能
 */
class PushManager {
public:
    /**
     * @brief 获取单例实例
     * @return PushManager& 单例引用
     */
    static PushManager& getInstance();

    /**
     * @brief 初始化推送管理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();

    /**
     * @brief 处理短信推送
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult processSmsForward(const PushContext& context);

    /**
     * @brief 根据规则ID推送短信
     * @param ruleId 转发规则ID
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult pushByRule(int ruleId, const PushContext& context);

    /**
     * @brief 测试推送配置
     * @param ruleId 转发规则ID
     * @param testMessage 测试消息
     * @return PushResult 推送结果
     */
    PushResult testPushConfig(int ruleId, const String& testMessage = "测试消息");

    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError() const;

    /**
     * @brief 启用调试模式
     * @param enable 是否启用
     */
    void setDebugMode(bool enable);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    PushManager();

    /**
     * @brief 析构函数
     */
    ~PushManager();

    /**
     * @brief 禁用拷贝构造函数
     */
    PushManager(const PushManager&) = delete;

    /**
     * @brief 禁用赋值操作符
     */
    PushManager& operator=(const PushManager&) = delete;

    /**
     * @brief 匹配转发规则
     * @param context 推送上下文
     * @return std::vector<ForwardRule> 匹配的规则列表
     */
    std::vector<ForwardRule> matchForwardRules(const PushContext& context);

    /**
     * @brief 检查号码是否匹配
     * @param pattern 号码模式（支持通配符）
     * @param number 实际号码
     * @return true 匹配
     * @return false 不匹配
     */
    bool matchPhoneNumber(const String& pattern, const String& number);

    /**
     * @brief 检查关键词是否匹配
     * @param keywords 关键词列表（逗号分隔）
     * @param content 短信内容
     * @return true 匹配
     * @return false 不匹配
     */
    bool matchKeywords(const String& keywords, const String& content);

    /**
     * @brief 执行推送
     * @param rule 转发规则
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult executePush(const ForwardRule& rule, const PushContext& context);

    /**
     * @brief 推送到企业微信
     * @param config 推送配置（JSON格式）
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult pushToWechat(const String& config, const PushContext& context);

    /**
     * @brief 推送到钉钉
     * @param config 推送配置（JSON格式）
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult pushToDingTalk(const String& config, const PushContext& context);

    /**
     * @brief 推送到Webhook
     * @param config 推送配置（JSON格式）
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult pushToWebhook(const String& config, const PushContext& context);

    /**
     * @brief 解析推送配置
     * @param configJson 配置JSON字符串
     * @return std::map<String, String> 配置映射
     */
    std::map<String, String> parseConfig(const String& configJson);

    /**
     * @brief 应用消息模板
     * @param templateStr 模板字符串
     * @param context 推送上下文
     * @param escapeForJson 是否为JSON格式转义特殊字符
     * @return String 应用模板后的消息
     */
    String applyTemplate(const String& templateStr, const PushContext& context, bool escapeForJson = false);

    /**
     * @brief 格式化时间戳
     * @param timestamp PDU时间戳
     * @return String 格式化后的时间
     */
    String formatTimestamp(const String& timestamp);

    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);

    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debugPrint(const String& message);

private:
    String lastError;              ///< 最后的错误信息
    bool debugMode;                ///< 调试模式
    bool initialized;              ///< 是否已初始化
    std::vector<ForwardRule> cachedRules; ///< 缓存的转发规则
    unsigned long lastRuleUpdate;  ///< 上次规则更新时间
    static const unsigned long RULE_CACHE_TIMEOUT = 60000; ///< 规则缓存超时时间（毫秒）
};

#endif // PUSH_MANAGER_H