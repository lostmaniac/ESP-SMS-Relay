/**
 * @file wechat_channel.h
 * @brief 企业微信推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件实现企业微信机器人推送功能
 */

#ifndef WECOM_CHANNEL_H
#define WECOM_CHANNEL_H

#include "../push_channel_base.h"
#include "../push_channel_registry.h"

/**
 * @class WechatChannel
 * @brief 企业微信推送渠道类
 * 
 * 实现企业微信机器人推送功能
 */
class WecomChannel : public PushChannelBase {
public:
    /**
     * @brief 构造函数
     */
    WecomChannel();

    /**
     * @brief 析构函数
     */
    virtual ~WecomChannel();

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
     * @brief 构建企业微信消息体
     * @param message 消息内容
     * @param msgType 消息类型（text/markdown）
     * @return String JSON消息体
     */
    String buildMessageBody(const String& message, const String& msgType = "text");
};

#endif // WECOM_CHANNEL_H