/**
 * @file wechat_channel.cpp
 * @brief 企业微信推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "wechat_channel.h"
#include "../push_channel_registry.h"
#include "../http_client/http_client.h"
#include <ArduinoJson.h>

/**
 * @brief 构造函数
 */
WechatChannel::WechatChannel() {
    debugMode = false;
}

/**
 * @brief 析构函数
 */
WechatChannel::~WechatChannel() {
}

/**
 * @brief 获取渠道名称
 * @return String 渠道名称
 */
String WechatChannel::getChannelName() const {
    return "wechat";
}

/**
 * @brief 获取渠道描述
 * @return String 渠道描述
 */
String WechatChannel::getChannelDescription() const {
    return "企业微信机器人推送";
}

/**
 * @brief 执行推送
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult WechatChannel::push(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    if (!validateConfig(configMap)) {
        return PUSH_CONFIG_ERROR;
    }
    
    String webhookUrl = configMap["webhook_url"];
    
    // 获取消息模板
    String messageTemplate = configMap["template"];
    if (messageTemplate.isEmpty()) {
        // 使用默认模板
        messageTemplate = "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}";
    }
    
    String message = applyTemplate(messageTemplate, context);
    
    // 如果没有配置webhook_url，则只进行本地文字处理
    if (webhookUrl.isEmpty()) {
        debugPrint("企业微信纯文字模式 - 消息内容: " + message);
        debugPrint("✅ 企业微信纯文字处理成功");
        return PUSH_SUCCESS;
    }
    
    // 获取消息类型
    String msgType = configMap["msg_type"];
    if (msgType.isEmpty()) {
        msgType = "text";
    }
    
    String messageBody = buildMessageBody(message, msgType);
    
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
 * @brief 测试推送配置
 * @param config 推送配置（JSON格式）
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult WechatChannel::testConfig(const String& config, const String& testMessage) {
    PushContext testContext;
    testContext.sender = "测试号码";
    testContext.content = testMessage;
    testContext.timestamp = "240101120000"; // 2024-01-01 12:00:00
    testContext.smsRecordId = -1;
    
    return push(config, testContext);
}

/**
 * @brief 获取配置示例
 * @return PushChannelExample 配置示例
 */
PushChannelExample WechatChannel::getConfigExample() const {
    PushChannelExample example;
    example.channelName = "企业微信";
    example.description = "通过企业微信机器人推送短信通知";
    example.configExample = R"({
  "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
  "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}",
  "msg_type": "text"
})";
    example.usage = R"(使用说明：
1. 在企业微信群中添加机器人
2. 获取机器人的Webhook URL
3. 将URL填入webhook_url字段
4. 可自定义消息模板，支持占位符：{sender}、{content}、{timestamp}、{sms_id}
5. msg_type支持text和markdown两种格式)";
    
    return example;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String WechatChannel::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void WechatChannel::setDebugMode(bool enable) {
    debugMode = enable;
}

/**
 * @brief 获取CLI演示代码
 * @return String CLI演示代码
 */
String WechatChannel::getCliDemo() const {
    String demo = "// 企业微信推送演示\n";
    demo += "void demoWechatPush() {\n";
    demo += "    WechatChannel wechat;\n";
    demo += "    wechat.setDebugMode(true);\n";
    demo += "    \n";
    demo += "    // 配置示例\n";
    demo += "    String config = \"{\\\"webhook_url\\\":\\\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\\\",\\\"template\\\":\\\"📱 收到新短信\\\\n\\\\n📞 发送方: {sender}\\\\n🕐 时间: {timestamp}\\\\n📄 内容: {content}\\\",\\\"msg_type\\\":\\\"text\\\"}\";\n";
    demo += "    \n";
    demo += "    // 测试推送\n";
    demo += "    PushResult result = wechat.testConfig(config, \"这是一条测试消息\");\n";
    demo += "    \n";
    demo += "    if (result == PUSH_SUCCESS) {\n";
    demo += "        Serial.println(\"✅ 企业微信推送测试成功\");\n";
    demo += "    } else {\n";
    demo += "        Serial.println(\"❌ 企业微信推送测试失败: \" + wechat.getLastError());\n";
    demo += "    }\n";
    demo += "    \n";
    demo += "    // 显示配置示例\n";
    demo += "    PushChannelExample example = wechat.getConfigExample();\n";
    demo += "    Serial.println(\"配置示例:\");\n";
    demo += "    Serial.println(example.configExample);\n";
    demo += "    Serial.println(\"使用说明:\");\n";
    demo += "    Serial.println(example.usage);\n";
    demo += "}\n";
    
    return demo;
}

/**
 * @brief 获取渠道帮助信息
 * @return PushChannelHelp 帮助信息
 */
PushChannelHelp WechatChannel::getHelp() const {
    PushChannelHelp help;
    help.channelName = "wechat";
    help.description = "企业微信机器人推送渠道，支持向企业微信群发送消息";
    
    help.configFields = "配置字段说明:\n"
                       "- webhook_url: 企业微信机器人Webhook地址 (必填)\n"
                       "- msg_type: 消息类型，支持text/markdown (默认text)\n"
                       "- mentioned_list: @指定用户，多个用逗号分隔 (可选)\n"
                       "- mentioned_mobile_list: @指定手机号，多个用逗号分隔 (可选)\n"
                       "- template: 消息模板，支持占位符{sender},{content},{timestamp},{sms_id} (可选)\n";
    
    help.ruleExample = "转发规则示例:\n"
                      "1. 基本配置:\n"
                      "   {\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\"}\n\n"
                      "2. @指定用户:\n"
                      "   {\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\",\"mentioned_list\":\"@all\"}\n\n"
                      "3. @指定手机号:\n"
                      "   {\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\",\"mentioned_mobile_list\":\"13800138000,13900139000\"}\n\n"
                      "4. 自定义模板:\n"
                      "   {\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\",\"template\":\"新短信通知\\n发送方: {sender}\\n内容: {content}\\n时间: {timestamp}\"}";
    
    help.troubleshooting = "常见问题解决:\n"
                          "1. 推送失败: 检查webhook_url和key是否正确\n"
                          "2. @功能无效: 确保机器人有@权限，用户ID或手机号格式正确\n"
                          "3. 消息被限流: 企业微信机器人有频率限制，请控制发送频率\n"
                          "4. 格式错误: 确保配置为有效的JSON格式\n"
                          "5. 权限不足: 确保机器人已添加到目标群聊中";
    
    return help;
}

/**
 * @brief 验证配置参数
 * @param configMap 配置映射
 * @return bool 配置是否有效
 */
bool WechatChannel::validateConfig(const std::map<String, String>& configMap) {
    auto it = configMap.find("webhook_url");
    
    // webhook_url现在是可选的，如果提供了则验证格式
    if (it != configMap.end() && !it->second.isEmpty()) {
        String webhookUrl = it->second;
        if (!webhookUrl.startsWith("https://qyapi.weixin.qq.com/")) {
            setError("企业微信webhook_url格式不正确，应以https://qyapi.weixin.qq.com/开头");
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 构建企业微信消息体
 * @param message 消息内容
 * @param msgType 消息类型（text/markdown）
 * @return String JSON消息体
 */
String WechatChannel::buildMessageBody(const String& message, const String& msgType) {
    JsonDocument doc;
    doc["msgtype"] = msgType;
    
    if (msgType == "markdown") {
        doc["markdown"]["content"] = message;
    } else {
        doc["text"]["content"] = message;
    }
    
    String messageBody;
    serializeJson(doc, messageBody);
    
    return messageBody;
}

// 自动注册企业微信渠道
REGISTER_PUSH_CHANNEL("wechat", WechatChannel, (std::vector<String>{"企业微信", "微信", "wework"}));