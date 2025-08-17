/**
 * @file dingtalk_channel.cpp
 * @brief 钉钉推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "dingtalk_channel.h"
#include "../push_channel_registry.h"
#include "../http_client/http_client.h"
#include "../../../include/constants.h"
#include <ArduinoJson.h>
#include <mbedtls/md.h>
#include <base64.h>
#include <time.h>

/**
 * @brief 构造函数
 */
DingtalkChannel::DingtalkChannel() {
    debugMode = false;
}

/**
 * @brief 析构函数
 */
DingtalkChannel::~DingtalkChannel() {
}

/**
 * @brief 获取渠道名称
 * @return String 渠道名称
 */
String DingtalkChannel::getChannelName() const {
    return "dingtalk";
}

/**
 * @brief 获取渠道描述
 * @return String 渠道描述
 */
String DingtalkChannel::getChannelDescription() const {
    return "钉钉机器人推送";
}

/**
 * @brief 执行推送
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult DingtalkChannel::push(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    if (!validateConfig(configMap)) {
        return PUSH_CONFIG_ERROR;
    }
    
    String webhookUrl = configMap["webhook_url"];
    String secret = configMap["secret"];
    
    // 如果配置了secret，需要生成签名
    if (!secret.isEmpty()) {
        String timestamp = String(time(nullptr) * 1000); // 使用Unix时间戳（毫秒）
        String sign = generateSign(timestamp, secret);
        
        // 添加签名参数到URL
        char separator = (webhookUrl.indexOf('?') == -1) ? '?' : '&';
        webhookUrl += separator + "timestamp=" + timestamp + "&sign=" + sign;
    }
    
    // 获取消息模板
    String messageTemplate = configMap["template"];
    if (messageTemplate.isEmpty()) {
        // 使用默认模板
        messageTemplate = "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}";
    }
    
    String message = applyTemplate(messageTemplate, context);
    
    // 获取消息类型
    String msgType = configMap["msg_type"];
    if (msgType.isEmpty()) {
        msgType = "text";
    }
    
    String messageBody = buildMessageBody(message, msgType);
    
    // 设置请求头
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    
    debugPrint("推送到钉钉: " + webhookUrl);
    debugPrint("消息内容: " + messageBody);
    
    // 发送HTTP请求
    HttpClient& httpClient = HttpClient::getInstance();
    HttpResponse response = httpClient.post(webhookUrl, messageBody, headers, DEFAULT_HTTP_TIMEOUT_MS);
    
    debugPrint("钉钉响应 - 状态码: " + String(response.statusCode) + ", 错误码: " + String(response.error));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode == 200) {
        debugPrint("✅ 钉钉推送成功");
        return PUSH_SUCCESS;
    } else {
        setError("钉钉推送失败，状态码: " + String(response.statusCode) + ", 错误: " + httpClient.getLastError());
        return (response.error == 0) ? PUSH_FAILED : PUSH_NETWORK_ERROR;
    }
}

/**
 * @brief 测试推送配置
 * @param config 推送配置（JSON格式）
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult DingtalkChannel::testConfig(const String& config, const String& testMessage) {
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
PushChannelExample DingtalkChannel::getConfigExample() const {
    PushChannelExample example;
    example.channelName = "钉钉";
    example.description = "通过钉钉机器人推送短信通知";
    
    String configExample = "{\n";
    configExample += "  \"webhook_url\": \"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\",\n";
    configExample += "  \"secret\": \"YOUR_SECRET\",\n";
    configExample += "  \"template\": \"📱 收到新短信\\n\\n📞 发送方: {sender}\\n🕐 时间: {timestamp}\\n📄 内容: {content}\",\n";
    configExample += "  \"msg_type\": \"text\"\n";
    configExample += "}";
    example.configExample = configExample;
    
    String usage = "使用说明：\n";
    usage += "1. 在钉钉群中添加自定义机器人\n";
    usage += "2. 获取机器人的Webhook URL和Secret（可选）\n";
    usage += "3. 将URL填入webhook_url字段\n";
    usage += "4. 如果启用了加签验证，填入secret字段\n";
    usage += "5. 可自定义消息模板，支持占位符：{sender}、{content}、{timestamp}、{sms_id}\n";
    usage += "6. msg_type支持text和markdown两种格式";
    example.usage = usage;
    
    return example;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String DingtalkChannel::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void DingtalkChannel::setDebugMode(bool enable) {
    debugMode = enable;
}

/**
 * @brief 获取CLI演示代码
 * @return String CLI演示代码
 */
String DingtalkChannel::getCliDemo() const {
    String demo = "// 钉钉推送演示\n";
    demo += "void demoDingtalkPush() {\n";
    demo += "    DingtalkChannel dingtalk;\n";
    demo += "    dingtalk.setDebugMode(true);\n";
    demo += "    \n";
    demo += "    // 配置示例\n";
    demo += "    String config = \"{\\\"webhook_url\\\":\\\"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\\\",\\\"secret\\\":\\\"YOUR_SECRET\\\",\\\"template\\\":\\\"📱 收到新短信\\\\n\\\\n📞 发送方: {sender}\\\\n🕐 时间: {timestamp}\\\\n📄 内容: {content}\\\",\\\"msg_type\\\":\\\"text\\\"}\";\n";
    demo += "    \n";
    demo += "    // 测试推送\n";
    demo += "    PushResult result = dingtalk.testConfig(config, \"这是一条测试消息\");\n";
    demo += "    \n";
    demo += "    if (result == PUSH_SUCCESS) {\n";
    demo += "        Serial.println(\"✅ 钉钉推送测试成功\");\n";
    demo += "    } else {\n";
    demo += "        Serial.println(\"❌ 钉钉推送测试失败: \" + dingtalk.getLastError());\n";
    demo += "    }\n";
    demo += "    \n";
    demo += "    // 显示配置示例\n";
    demo += "    PushChannelExample example = dingtalk.getConfigExample();\n";
    demo += "    Serial.println(\"配置示例:\");\n";
    demo += "    Serial.println(example.configExample);\n";
    demo += "    Serial.println(\"使用说明:\");\n";
    demo += "    Serial.println(example.usage);\n";
    demo += "}";
    
    return demo;
}

/**
 * @brief 验证配置参数
 * @param configMap 配置映射
 * @return bool 配置是否有效
 */
bool DingtalkChannel::validateConfig(const std::map<String, String>& configMap) {
    auto it = configMap.find("webhook_url");
    if (it == configMap.end() || it->second.isEmpty()) {
        setError("钉钉配置缺少webhook_url");
        return false;
    }
    
    // 验证URL格式
    String webhookUrl = it->second;
    if (!webhookUrl.startsWith("https://oapi.dingtalk.com/")) {
        setError("钉钉webhook_url格式不正确，应以https://oapi.dingtalk.com/开头");
        return false;
    }
    
    return true;
}

/**
 * @brief 构建钉钉消息体
 * @param message 消息内容
 * @param msgType 消息类型（text/markdown）
 * @return String JSON消息体
 */
String DingtalkChannel::buildMessageBody(const String& message, const String& msgType) {
    JsonDocument doc;
    doc["msgtype"] = msgType;
    
    if (msgType == "markdown") {
        doc["markdown"]["title"] = "短信通知";
        doc["markdown"]["text"] = message;
    } else {
        doc["text"]["content"] = message;
    }
    
    String messageBody;
    serializeJson(doc, messageBody);
    
    return messageBody;
}

/**
 * @brief 生成签名（如果配置了secret）
 * @param timestamp 时间戳
 * @param secret 密钥
 * @return String 签名
 */
String DingtalkChannel::generateSign(const String& timestamp, const String& secret) {
    // 钉钉签名算法：HMAC-SHA256
    String stringToSign = timestamp + "\n" + secret;
    
    // 使用mbedtls计算HMAC-SHA256
    unsigned char hmac[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign.c_str(), stringToSign.length());
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    
    // Base64编码
    String encoded = base64::encode((uint8_t*)hmac, 32);
    
    // URL编码（简单实现）
    encoded.replace("+", "%2B");
    encoded.replace("/", "%2F");
    encoded.replace("=", "%3D");
    
    return encoded;
}

/**
 * @brief 获取渠道帮助信息
 * @return PushChannelHelp 帮助信息
 */
PushChannelHelp DingtalkChannel::getHelp() const {
    PushChannelHelp help;
    help.channelName = "dingtalk";
    help.description = "钉钉机器人推送渠道，支持向钉钉群发送消息";
    
    help.configFields = "配置字段说明:\n";
    help.configFields += "- webhook_url: 钉钉机器人Webhook地址 (必填)\n";
    help.configFields += "- secret: 钉钉机器人密钥，用于签名验证 (可选)\n";
    help.configFields += "- template: 消息模板，支持占位符 (可选)\n";
    help.configFields += "- msg_type: 消息类型，支持text/markdown (默认text)\n";
    
    help.ruleExample = "转发规则示例:\n";
    help.ruleExample += "1. 基本配置:\n";
    help.ruleExample += "   {\"webhook_url\":\"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\"}\n\n";
    help.ruleExample += "2. 带签名验证:\n";
    help.ruleExample += "   {\"webhook_url\":\"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\",\"secret\":\"YOUR_SECRET\"}\n\n";
    help.ruleExample += "3. 自定义模板:\n";
    help.ruleExample += "   {\"webhook_url\":\"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\",\"template\":\"新短信: {content}\"}";
    
    help.troubleshooting = "常见问题解决:\n";
    help.troubleshooting += "1. 推送失败: 检查webhook_url和access_token是否正确\n";
    help.troubleshooting += "2. 签名验证失败: 检查secret密钥是否正确\n";
    help.troubleshooting += "3. 消息被限流: 钉钉机器人有频率限制，请控制发送频率\n";
    help.troubleshooting += "4. 格式错误: 确保配置为有效的JSON格式";
    
    return help;
}

// 自动注册钉钉渠道
REGISTER_PUSH_CHANNEL("dingtalk", DingtalkChannel, (std::vector<String>{"钉钉", "dingding", "ding"}));