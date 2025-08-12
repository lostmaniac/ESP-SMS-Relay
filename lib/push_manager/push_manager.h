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
#include "push_channel_registry.h"

/**
 * @brief 加载统计信息结构
 */
struct LoadStatistics {
    int totalChannels;   ///< 总渠道数
    int loadedChannels;  ///< 已加载渠道数
    int failedChannels;  ///< 加载失败渠道数
};

// 使用push_channel_base.h中定义的PushResult和PushContext
// 这里不再重复定义，避免冲突

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
     * @param pushType 推送类型
     * @param config 推送配置
     * @param testMessage 测试消息
     * @return PushResult 推送结果
     */
    PushResult testPushConfig(const String& pushType, const String& config, const String& testMessage = "测试消息");

    /**
     * @brief 根据规则ID测试推送配置
     * @param ruleId 转发规则ID
     * @param testMessage 测试消息
     * @return PushResult 推送结果
     */
    PushResult testPushConfig(int ruleId, const String& testMessage = "测试消息");

    /**
     * @brief 获取所有可用的推送渠道
     * @return std::vector<String> 渠道名称列表
     */
    std::vector<String> getAvailableChannels() const;

    /**
     * @brief 获取所有推送渠道的配置示例
     * @return std::vector<PushChannelExample> 配置示例列表
     */
    std::vector<PushChannelExample> getAllChannelExamples() const;

    /**
     * @brief 获取所有推送渠道的帮助信息
     * @return std::vector<PushChannelHelp> 帮助信息列表
     */
    std::vector<PushChannelHelp> getAllChannelHelp() const;

    /**
     * @brief 获取CLI演示代码
     * @return String 完整的CLI演示代码
     */
    String getCliDemo() const;

    /**
     * @brief 重新加载推送渠道
     * @return true 重新加载成功
     * @return false 重新加载失败
     */
    bool reloadChannels();

    /**
     * @brief 获取渠道加载统计信息
     * @return LoadStatistics 加载统计信息
     */
    LoadStatistics getLoadStatistics() const;

    /**
     * @brief 获取渠道元数据
     * @param channelName 渠道名称
     * @return PushChannelRegistry::ChannelMetadata 渠道元数据
     */
    PushChannelRegistry::ChannelMetadata getChannelMetadata(const String& channelName) const;

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

    /**
     * @brief 手动刷新规则缓存
     * @return true 刷新成功
     * @return false 刷新失败
     */
    bool refreshRuleCache();

    /**
     * @brief 加载规则到缓存
     * @return true 加载成功
     * @return false 加载失败
     */
    bool loadRulesToCache();

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
     * @brief 使用指定渠道执行推送
     * @param channelName 渠道名称
     * @param config 推送配置（JSON格式）
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult pushToChannel(const String& channelName, const String& config, const PushContext& context);



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
    bool cacheLoaded;              ///< 缓存是否已加载
};

#endif // PUSH_MANAGER_H