/**
 * @file wechat_official_channel.h
 * @brief 微信公众号推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件实现微信公众号推送功能，支持向关注用户发送模板消息
 */

#ifndef WECHAT_OFFICIAL_CHANNEL_H
#define WECHAT_OFFICIAL_CHANNEL_H

#include "../push_channel_base.h"
#include "../push_channel_registry.h"
#include <map>
#include <vector>

/**
 * @class WechatOfficialChannel
 * @brief 微信公众号推送渠道类
 * 
 * 实现微信公众号模板消息推送功能，支持：
 * - 获取access_token
 * - 发送模板消息
 * - 多用户推送
 */
class WechatOfficialChannel : public PushChannelBase {
public:
    /**
     * @brief 构造函数
     */
    WechatOfficialChannel();

    /**
     * @brief 析构函数
     */
    virtual ~WechatOfficialChannel();

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
     * @brief 获取微信公众号access_token
     * @param appId 应用ID
     * @param appSecret 应用密钥
     * @return String access_token，失败返回空字符串
     */
    String getAccessToken(const String& appId, const String& appSecret);

    /**
     * @brief 发送模板消息
     * @param accessToken 访问令牌
     * @param openId 用户openid
     * @param templateId 模板ID
     * @param data 模板数据
     * @param url 跳转链接（可选）
     * @return bool 发送是否成功
     */
    bool sendTemplateMessage(const String& accessToken, const String& openId, 
                           const String& templateId, const String& data, const String& url = "");



    /**
     * @brief 构建模板消息数据
     * @param context 推送上下文
     * @return String 模板数据JSON
     */
    String buildTemplateData(const PushContext& context);

    /**
     * @brief 解析openid列表
     * @param openIdStr 逗号分隔的openid字符串
     * @return std::vector<String> openid列表
     */
    std::vector<String> parseOpenIds(const String& openIdStr);

    /**
     * @brief 检查access_token是否有效
     * @param accessToken 访问令牌
     * @return bool 是否有效
     */
    bool isAccessTokenValid(const String& accessToken);

    /**
     * @brief 解析配置字符串
     * @param config JSON配置字符串
     * @return std::map<String, String> 配置映射
     */
    std::map<String, String> parseConfig(const String& config);

    /**
     * @brief 应用消息模板
     * @param messageTemplate 消息模板
     * @param context 推送上下文
     * @return String 应用模板后的消息
     */
    String applyTemplate(const String& messageTemplate, const PushContext& context);

    /**
     * @brief 格式化时间戳
     * @param timestamp 原始时间戳
     * @return String 格式化后的时间
     */
    String formatTimestamp(const String& timestamp);

    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);

    /**
     * @brief 打印调试信息
     * @param message 调试信息
     */
    void debugPrint(const String& message);

private:
    String lastError;            ///< 最后的错误信息
    bool debugMode;              ///< 调试模式
    String cachedAccessToken;    ///< 缓存的access_token
    unsigned long tokenExpireTime; ///< token过期时间
    static const unsigned long TOKEN_CACHE_DURATION = 7000000; ///< token缓存时长（毫秒，约2小时）
    static const int WECHAT_TEMPLATE_CONTENT_MAX_LENGTH = 180; ///< 微信模板消息内容最大长度（预留20字符给其他模板内容）
};

#endif // WECHAT_OFFICIAL_CHANNEL_H