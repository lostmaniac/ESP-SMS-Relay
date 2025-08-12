/**
 * @file feishu_bot_channel.h
 * @brief 飞书机器人推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件实现飞书自定义机器人推送功能，支持：
 * - 文本消息推送
 * - 富文本消息推送
 * - 签名校验（可选）
 * - 关键词校验（可选）
 */

#ifndef FEISHU_BOT_CHANNEL_H
#define FEISHU_BOT_CHANNEL_H

#include "../push_channel_base.h"
#include "../push_channel_registry.h"
#include <map>

/**
 * @enum FeishuMessageType
 * @brief 飞书消息类型枚举
 */
enum FeishuMessageType {
    FEISHU_TEXT = 0,        ///< 文本消息
    FEISHU_RICH_TEXT = 1,   ///< 富文本消息
    FEISHU_POST = 2         ///< 消息卡片
};

/**
 * @class FeishuBotChannel
 * @brief 飞书机器人推送渠道类
 * 
 * 实现飞书自定义机器人推送功能，支持：
 * - 多种消息类型（文本、富文本、消息卡片）
 * - 签名校验保证安全性
 * - 关键词校验
 * - 消息模板自定义
 */
class FeishuBotChannel : public PushChannelBase {
public:
    /**
     * @brief 构造函数
     */
    FeishuBotChannel();

    /**
     * @brief 析构函数
     */
    virtual ~FeishuBotChannel();

    /**
     * @brief 获取渠道名称
     * @return String 渠道名称
     */
    String getChannelName() const override;

    /**
     * @brief 获取渠道描述
     * @return String 渠道描述
     */
    String getChannelDescription() const override;

    /**
     * @brief 执行推送
     * @param config 推送配置（JSON格式）
     * @param context 推送上下文
     * @return PushResult 推送结果
     */
    PushResult push(const String& config, const PushContext& context) override;

    /**
     * @brief 测试推送配置
     * @param config 推送配置（JSON格式）
     * @param testMessage 测试消息
     * @return PushResult 推送结果
     */
    PushResult testConfig(const String& config, const String& testMessage = "测试消息") override;

    /**
     * @brief 获取配置示例
     * @return PushChannelExample 配置示例
     */
    PushChannelExample getConfigExample() const override;

    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError() const override;

    /**
     * @brief 启用调试模式
     * @param enable 是否启用
     */
    void setDebugMode(bool enable) override;

    /**
     * @brief 获取CLI演示代码
     * @return String CLI演示代码
     */
    String getCliDemo() const override;
    
    /**
     * @brief 获取渠道帮助信息
     * @return PushChannelHelp 帮助信息
     */
    PushChannelHelp getHelp() const override;

private:
    /**
     * @brief 验证配置参数
     * @param configMap 配置映射
     * @return bool 配置是否有效
     */
    bool validateConfig(const std::map<String, String>& configMap);

    /**
     * @brief 发送文本消息
     * @param webhookUrl Webhook地址
     * @param content 消息内容
     * @param secret 签名密钥（可选）
     * @return bool 发送是否成功
     */
    bool sendTextMessage(const String& webhookUrl, const String& content, const String& secret = "");

    /**
     * @brief 发送富文本消息
     * @param webhookUrl Webhook地址
     * @param title 消息标题
     * @param content 消息内容
     * @param secret 签名密钥（可选）
     * @return bool 发送是否成功
     */
    bool sendRichTextMessage(const String& webhookUrl, const String& title, 
                           const String& content, const String& secret = "");

    /**
     * @brief 发送消息卡片
     * @param webhookUrl Webhook地址
     * @param title 卡片标题
     * @param content 卡片内容
     * @param secret 签名密钥（可选）
     * @return bool 发送是否成功
     */
    bool sendPostMessage(const String& webhookUrl, const String& title, 
                        const String& content, const String& secret = "");

    /**
     * @brief 生成签名
     * @param timestamp 时间戳
     * @param secret 签名密钥
     * @return String 签名字符串
     */
    String generateSignature(const String& timestamp, const String& secret);

    /**
     * @brief 构建文本消息JSON
     * @param content 消息内容
     * @return String 消息JSON
     */
    String buildTextMessageJson(const String& content);

    /**
     * @brief 构建富文本消息JSON
     * @param title 消息标题
     * @param content 消息内容
     * @return String 消息JSON
     */
    String buildRichTextMessageJson(const String& title, const String& content);

    /**
     * @brief 构建消息卡片JSON
     * @param title 卡片标题
     * @param content 卡片内容
     * @return String 消息JSON
     */
    String buildPostMessageJson(const String& title, const String& content);

    /**
     * @brief 解析消息类型
     * @param typeStr 类型字符串
     * @return FeishuMessageType 消息类型
     */
    FeishuMessageType parseMessageType(const String& typeStr);

    /**
     * @brief 发送HTTP请求到飞书
     * @param webhookUrl Webhook地址
     * @param messageJson 消息JSON
     * @param secret 签名密钥（可选）
     * @return bool 发送是否成功
     */
    bool sendToFeishu(const String& webhookUrl, const String& messageJson, const String& secret = "");

    /**
     * @brief 获取当前时间戳（秒）
     * @return String 时间戳字符串
     */
    String getCurrentTimestamp();

    /**
     * @brief 转义JSON字符串
     * @param str 原始字符串
     * @return String 转义后的字符串
     */
    String escapeJsonString(const String& str);

private:
    static const int FEISHU_MESSAGE_MAX_LENGTH = 30000; ///< 飞书消息最大长度
    static const int FEISHU_TITLE_MAX_LENGTH = 100;     ///< 飞书标题最大长度
};

#endif // FEISHU_BOT_CHANNEL_H