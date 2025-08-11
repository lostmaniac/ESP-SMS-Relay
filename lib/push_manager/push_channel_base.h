/**
 * @file push_channel_base.h
 * @brief 推送渠道基础接口定义
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件定义了推送渠道的基础接口和公共结构体
 */

#ifndef PUSH_CHANNEL_BASE_H
#define PUSH_CHANNEL_BASE_H

#include <Arduino.h>
#include <map>

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
 * @struct PushChannelExample
 * @brief 推送渠道配置示例结构体
 */
struct PushChannelExample {
    String channelName;    ///< 渠道名称
    String description;    ///< 渠道描述
    String configExample;  ///< 配置示例JSON
    String usage;          ///< 使用说明
    String helpText;       ///< 帮助文本
};

/**
 * @struct PushChannelHelp
 * @brief 推送渠道帮助信息结构体
 */
struct PushChannelHelp {
    String channelName;        ///< 渠道名称
    String description;        ///< 渠道描述
    String configFields;       ///< 配置字段说明
    String ruleExample;        ///< 转发规则示例
    String troubleshooting;    ///< 故障排除
};

/**
 * @class PushChannelBase
 * @brief 推送渠道基础抽象类
 * 
 * 所有推送渠道都必须继承此类并实现相应接口
 */
class PushChannelBase {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~PushChannelBase() = default;

    /**
     * @brief 获取渠道名称
     * @return String 渠道名称
     */
    virtual String getChannelName() const = 0;

    /**
     * @brief 获取渠道描述
     * @return String 渠道描述
     */
    virtual String getChannelDescription() const = 0;

    /**
     * @brief 执行推送
     * @param config 推送配置（JSON格式）
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    virtual PushResult push(const String& config, const PushContext& context) = 0;

    /**
     * @brief 测试推送配置
     * @param config 推送配置（JSON格式）
     * @param testMessage 测试消息
     * @return PushResult 推送结果
     */
    virtual PushResult testConfig(const String& config, const String& testMessage = "测试消息") = 0;

    /**
     * @brief 获取配置示例
     * @return PushChannelExample 配置示例
     */
    virtual PushChannelExample getConfigExample() const = 0;

    /**
     * @brief 获取帮助信息
     * @return PushChannelHelp 帮助信息
     */
    virtual PushChannelHelp getHelp() const = 0;

    /**
     * @brief 获取CLI演示代码
     * @return String CLI演示代码
     */
    virtual String getCliDemo() const = 0;

    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    virtual String getLastError() const = 0;

    /**
     * @brief 启用调试模式
     * @param enable 是否启用
     */
    virtual void setDebugMode(bool enable) = 0;

protected:
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

protected:
    String lastError;      ///< 最后的错误信息
    bool debugMode = false; ///< 调试模式
};

#endif // PUSH_CHANNEL_BASE_H