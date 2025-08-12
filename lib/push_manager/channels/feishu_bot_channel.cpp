/**
 * @file feishu_bot_channel.cpp
 * @brief 飞书机器人推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "feishu_bot_channel.h"
#include "../push_channel_registry.h"
#include "../../http_client/http_client.h"
#include <ArduinoJson.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>

/**
 * @brief 构造函数
 */
FeishuBotChannel::FeishuBotChannel() {
    debugMode = false;
}

/**
 * @brief 析构函数
 */
FeishuBotChannel::~FeishuBotChannel() {
}

/**
 * @brief 获取渠道名称
 * @return String 渠道名称
 */
String FeishuBotChannel::getChannelName() const {
    return "feishu_bot";
}

/**
 * @brief 获取渠道描述
 * @return String 渠道描述
 */
String FeishuBotChannel::getChannelDescription() const {
    return "飞书自定义机器人推送";
}

/**
 * @brief 执行推送
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult FeishuBotChannel::push(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    if (!validateConfig(configMap)) {
        return PUSH_CONFIG_ERROR;
    }
    
    String webhookUrl = configMap["webhook_url"];
    String messageType = configMap["message_type"];
    String secret = configMap["secret"];
    String title = configMap["title"];
    
    // 应用消息模板
    String messageTemplate = configMap["message_template"];
    if (messageTemplate.isEmpty()) {
        messageTemplate = "短信转发通知\n发送方：{sender}\n内容：{content}\n时间：{timestamp}";
    }
    
    String content = applyTemplate(messageTemplate, context, false);
    
    // 应用标题模板
    if (title.isEmpty()) {
        title = "短信转发通知";
    }
    title = applyTemplate(title, context, false);
    
    debugPrint("推送到飞书机器人: " + webhookUrl);
    debugPrint("消息类型: " + messageType);
    debugPrint("标题: " + title);
    debugPrint("内容: " + content);
    
    FeishuMessageType msgType = parseMessageType(messageType);
    bool success = false;
    
    switch (msgType) {
        case FEISHU_TEXT:
            success = sendTextMessage(webhookUrl, content, secret);
            break;
        case FEISHU_RICH_TEXT:
            success = sendRichTextMessage(webhookUrl, title, content, secret);
            break;
        case FEISHU_POST:
            success = sendPostMessage(webhookUrl, title, content, secret);
            break;
        default:
            setError("不支持的消息类型: " + messageType);
            return PUSH_CONFIG_ERROR;
    }
    
    if (success) {
        debugPrint("✅ 飞书机器人推送成功");
        return PUSH_SUCCESS;
    } else {
        return PUSH_FAILED;
    }
}

/**
 * @brief 测试推送配置
 * @param config 推送配置（JSON格式）
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult FeishuBotChannel::testConfig(const String& config, const String& testMessage) {
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
PushChannelExample FeishuBotChannel::getConfigExample() const {
    PushChannelExample example;
    example.channelName = "飞书机器人";
    example.description = "通过飞书自定义机器人推送短信通知";
    example.configExample = R"({
  "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxxxxxxxxxxxxxxxx",
  "message_type": "text",
  "secret": "your_secret_key",
  "title": "短信转发通知",
  "message_template": "📱 短信转发通知\n\n📞 发送方：{sender}\n📄 内容：{content}\n🕐 时间：{timestamp}"
})";
    example.usage = R"(使用说明：
1. 在飞书群组中添加自定义机器人，获取Webhook地址
2. message_type支持：text（文本）、rich_text（富文本）、post（消息卡片）
3. secret为可选的签名密钥，用于验证请求安全性
4. title为消息标题（富文本和消息卡片类型使用）
5. message_template支持占位符：{sender}、{content}、{timestamp}、{sms_id}
6. 消息内容最大30000字符，标题最大100字符)";
    
    return example;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String FeishuBotChannel::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void FeishuBotChannel::setDebugMode(bool enable) {
    debugMode = enable;
}

/**
 * @brief 获取CLI演示代码
 * @return String CLI演示代码
 */
String FeishuBotChannel::getCliDemo() const {
    String demo = "// 飞书机器人推送演示\n";
    demo += "void demoFeishuBotPush() {\n";
    demo += "    FeishuBotChannel feishu;\n";
    demo += "    feishu.setDebugMode(true);\n";
    demo += "    \n";
    demo += "    // 配置示例1：文本消息（无签名）\n";
    demo += "    String textConfig = \"{\n";
    demo += "        \\\"webhook_url\\\": \\\"https://open.feishu.cn/open-apis/bot/v2/hook/xxx\\\",\n";
    demo += "        \\\"message_type\\\": \\\"text\\\",\n";
    demo += "        \\\"message_template\\\": \\\"📱 短信通知\\\\n发送方：{sender}\\\\n内容：{content}\\\"\n";
    demo += "    }\";\n";
    demo += "    \n";
    demo += "    // 配置示例2：富文本消息（带签名）\n";
    demo += "    String richConfig = \"{\n";
    demo += "        \\\"webhook_url\\\": \\\"https://open.feishu.cn/open-apis/bot/v2/hook/xxx\\\",\n";
    demo += "        \\\"message_type\\\": \\\"rich_text\\\",\n";
    demo += "        \\\"secret\\\": \\\"your_secret_key\\\",\n";
    demo += "        \\\"title\\\": \\\"短信转发通知\\\",\n";
    demo += "        \\\"message_template\\\": \\\"发送方：{sender}\\\\n内容：{content}\\\\n时间：{timestamp}\\\"\n";
    demo += "    }\";\n";
    demo += "    \n";
    demo += "    // 配置示例3：消息卡片\n";
    demo += "    String postConfig = \"{\n";
    demo += "        \\\"webhook_url\\\": \\\"https://open.feishu.cn/open-apis/bot/v2/hook/xxx\\\",\n";
    demo += "        \\\"message_type\\\": \\\"post\\\",\n";
    demo += "        \\\"title\\\": \\\"📱 短信转发通知\\\",\n";
    demo += "        \\\"message_template\\\": \\\"**发送方：** {sender}\\\\n**内容：** {content}\\\\n**时间：** {timestamp}\\\"\n";
    demo += "    }\";\n";
    demo += "    \n";
    demo += "    // 测试推送\n";
    demo += "    PushResult result = feishu.testConfig(textConfig, \\\"这是一条测试消息\\\");\n";
    demo += "    if (result == PUSH_SUCCESS) {\n";
    demo += "        Serial.println(\\\"飞书推送测试成功\\\");\n";
    demo += "    } else {\n";
    demo += "        Serial.println(\\\"飞书推送测试失败: \\\" + feishu.getLastError());\n";
    demo += "    }\n";
    demo += "}";
    
    return demo;
}

/**
 * @brief 获取渠道帮助信息
 * @return PushChannelHelp 帮助信息
 */
PushChannelHelp FeishuBotChannel::getHelp() const {
    PushChannelHelp help;
    help.channelName = "飞书机器人";
    help.description = "通过飞书自定义机器人向群组推送短信通知";
    help.configFields = R"(配置字段说明：
• webhook_url: 飞书机器人Webhook地址（必填）
• message_type: 消息类型，支持text/rich_text/post（默认text）
• secret: 签名密钥，用于安全校验（可选）
• title: 消息标题，用于富文本和消息卡片（可选）
• message_template: 消息模板，支持占位符（可选）)";
    help.ruleExample = R"(转发规则示例：
{
  "ruleName": "飞书通知",
  "sourceNumber": "",
  "keywords": "",
  "pushType": "feishu_bot",
  "pushConfig": {
    "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx",
    "message_type": "rich_text",
    "secret": "your_secret",
    "title": "短信通知",
    "message_template": "发送方：{sender}\n内容：{content}"
  }
})";
    help.troubleshooting = R"(故障排除：
1. 检查Webhook地址是否正确
2. 确认机器人已添加到目标群组
3. 验证签名密钥是否正确（如果启用）
4. 检查消息内容是否超过长度限制
5. 确认网络连接正常
6. 查看飞书机器人频率限制（100次/分钟，5次/秒）)";
    
    return help;
}

/**
 * @brief 验证配置参数
 * @param configMap 配置映射
 * @return bool 配置是否有效
 */
bool FeishuBotChannel::validateConfig(const std::map<String, String>& configMap) {
    // 检查必填字段
    if (configMap.find("webhook_url") == configMap.end() || configMap.at("webhook_url").isEmpty()) {
        setError("缺少必填字段：webhook_url");
        return false;
    }
    
    String webhookUrl = configMap.at("webhook_url");
    if (!webhookUrl.startsWith("https://open.feishu.cn/open-apis/bot/v2/hook/")) {
        setError("无效的飞书Webhook地址格式");
        return false;
    }
    
    // 检查消息类型
    String messageType = "text"; // 默认值
    if (configMap.find("message_type") != configMap.end()) {
        messageType = configMap.at("message_type");
    }
    
    if (messageType != "text" && messageType != "rich_text" && messageType != "post") {
        setError("不支持的消息类型：" + messageType + "，支持：text、rich_text、post");
        return false;
    }
    
    return true;
}

/**
 * @brief 发送文本消息
 * @param webhookUrl Webhook地址
 * @param content 消息内容
 * @param secret 签名密钥（可选）
 * @return bool 发送是否成功
 */
bool FeishuBotChannel::sendTextMessage(const String& webhookUrl, const String& content, const String& secret) {
    if (content.length() > FEISHU_MESSAGE_MAX_LENGTH) {
        setError("消息内容超过最大长度限制（" + String(FEISHU_MESSAGE_MAX_LENGTH) + "字符）");
        return false;
    }
    
    String messageJson = buildTextMessageJson(content);
    return sendToFeishu(webhookUrl, messageJson, secret);
}

/**
 * @brief 发送富文本消息
 * @param webhookUrl Webhook地址
 * @param title 消息标题
 * @param content 消息内容
 * @param secret 签名密钥（可选）
 * @return bool 发送是否成功
 */
bool FeishuBotChannel::sendRichTextMessage(const String& webhookUrl, const String& title, 
                                         const String& content, const String& secret) {
    if (title.length() > FEISHU_TITLE_MAX_LENGTH) {
        setError("标题超过最大长度限制（" + String(FEISHU_TITLE_MAX_LENGTH) + "字符）");
        return false;
    }
    
    if (content.length() > FEISHU_MESSAGE_MAX_LENGTH) {
        setError("消息内容超过最大长度限制（" + String(FEISHU_MESSAGE_MAX_LENGTH) + "字符）");
        return false;
    }
    
    String messageJson = buildRichTextMessageJson(title, content);
    return sendToFeishu(webhookUrl, messageJson, secret);
}

/**
 * @brief 发送消息卡片
 * @param webhookUrl Webhook地址
 * @param title 卡片标题
 * @param content 卡片内容
 * @param secret 签名密钥（可选）
 * @return bool 发送是否成功
 */
bool FeishuBotChannel::sendPostMessage(const String& webhookUrl, const String& title, 
                                     const String& content, const String& secret) {
    if (title.length() > FEISHU_TITLE_MAX_LENGTH) {
        setError("标题超过最大长度限制（" + String(FEISHU_TITLE_MAX_LENGTH) + "字符）");
        return false;
    }
    
    if (content.length() > FEISHU_MESSAGE_MAX_LENGTH) {
        setError("消息内容超过最大长度限制（" + String(FEISHU_MESSAGE_MAX_LENGTH) + "字符）");
        return false;
    }
    
    String messageJson = buildPostMessageJson(title, content);
    return sendToFeishu(webhookUrl, messageJson, secret);
}

/**
 * @brief 生成签名
 * @param timestamp 时间戳
 * @param secret 签名密钥
 * @return String 签名字符串
 */
String FeishuBotChannel::generateSignature(const String& timestamp, const String& secret) {
    String stringToSign = timestamp + "\n" + secret;
    
    // 使用HMAC-SHA256生成签名
    unsigned char hmacResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToSign.c_str(), stringToSign.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
    
    // Base64编码
    size_t olen = 0;
    unsigned char base64Buffer[64]; // 足够存储32字节的base64编码结果
    
    int ret = mbedtls_base64_encode(base64Buffer, sizeof(base64Buffer), &olen, hmacResult, 32);
    if (ret != 0) {
        return ""; // 编码失败
    }
    
    String signature = String((char*)base64Buffer);
    return signature;
}

/**
 * @brief 构建文本消息JSON
 * @param content 消息内容
 * @return String 消息JSON
 */
String FeishuBotChannel::buildTextMessageJson(const String& content) {
    JsonDocument doc;
    doc["msg_type"] = "text";
    doc["content"]["text"] = content;
    
    String json;
    serializeJson(doc, json);
    return json;
}

/**
 * @brief 构建富文本消息JSON
 * @param title 消息标题
 * @param content 消息内容
 * @return String 消息JSON
 */
String FeishuBotChannel::buildRichTextMessageJson(const String& title, const String& content) {
    JsonDocument doc;
    doc["msg_type"] = "rich_text";
    doc["content"]["rich_text"]["title"] = title;
    
    // 将内容按行分割并构建富文本元素
    JsonArray elements = doc["content"]["rich_text"]["content"].add<JsonArray>();
    
    int startPos = 0;
    int endPos = content.indexOf('\n');
    
    while (endPos != -1 || startPos < content.length()) {
        String line;
        if (endPos != -1) {
            line = content.substring(startPos, endPos);
            startPos = endPos + 1;
            endPos = content.indexOf('\n', startPos);
        } else {
            line = content.substring(startPos);
            startPos = content.length();
        }
        
        if (!line.isEmpty()) {
            JsonArray lineElements = elements.add<JsonArray>();
            JsonObject textElement = lineElements.add<JsonObject>();
            textElement["tag"] = "text";
            textElement["text"] = line;
        }
    }
    
    String json;
    serializeJson(doc, json);
    return json;
}

/**
 * @brief 构建消息卡片JSON
 * @param title 卡片标题
 * @param content 卡片内容
 * @return String 消息JSON
 */
String FeishuBotChannel::buildPostMessageJson(const String& title, const String& content) {
    JsonDocument doc;
    doc["msg_type"] = "post";
    doc["content"]["post"]["zh_cn"]["title"] = title;
    
    // 构建卡片内容
    JsonArray contentArray = doc["content"]["post"]["zh_cn"]["content"].add<JsonArray>();
    
    // 将内容按行分割
    int startPos = 0;
    int endPos = content.indexOf('\n');
    
    while (endPos != -1 || startPos < content.length()) {
        String line;
        if (endPos != -1) {
            line = content.substring(startPos, endPos);
            startPos = endPos + 1;
            endPos = content.indexOf('\n', startPos);
        } else {
            line = content.substring(startPos);
            startPos = content.length();
        }
        
        if (!line.isEmpty()) {
            JsonObject textElement = contentArray.add<JsonObject>();
            textElement["tag"] = "text";
            textElement["text"] = line;
        }
    }
    
    String json;
    serializeJson(doc, json);
    return json;
}

/**
 * @brief 解析消息类型
 * @param typeStr 类型字符串
 * @return FeishuMessageType 消息类型
 */
FeishuMessageType FeishuBotChannel::parseMessageType(const String& typeStr) {
    if (typeStr.equalsIgnoreCase("rich_text")) {
        return FEISHU_RICH_TEXT;
    } else if (typeStr.equalsIgnoreCase("post")) {
        return FEISHU_POST;
    } else {
        return FEISHU_TEXT; // 默认为文本消息
    }
}

/**
 * @brief 发送HTTP请求到飞书
 * @param webhookUrl Webhook地址
 * @param messageJson 消息JSON
 * @param secret 签名密钥（可选）
 * @return bool 发送是否成功
 */
bool FeishuBotChannel::sendToFeishu(const String& webhookUrl, const String& messageJson, const String& secret) {
    HttpClient& httpClient = HttpClient::getInstance();
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    
    String requestBody = messageJson;
    
    // 如果提供了签名密钥，添加签名
    if (!secret.isEmpty()) {
        String timestamp = getCurrentTimestamp();
        String signature = generateSignature(timestamp, secret);
        
        // 修改请求体，添加签名信息
        JsonDocument doc;
        deserializeJson(doc, messageJson);
        doc["timestamp"] = timestamp;
        doc["sign"] = signature;
        
        requestBody = "";
        serializeJson(doc, requestBody);
        
        debugPrint("添加签名 - 时间戳: " + timestamp + ", 签名: " + signature);
    }
    
    debugPrint("发送到飞书的请求体: " + requestBody);
    
    HttpResponse response = httpClient.post(webhookUrl, requestBody, headers, 30000);
    
    debugPrint("飞书响应 - 状态码: " + String(response.statusCode) + ", 错误码: " + String(response.error));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode == 200) {
        // 解析飞书响应
        JsonDocument responseDoc;
        DeserializationError error = deserializeJson(responseDoc, response.body);
        
        if (!error) {
            int code = responseDoc["code"] | -1;
            String msg = responseDoc["msg"] | "unknown";
            
            if (code == 0) {
                return true;
            } else {
                setError("飞书API错误 - 代码: " + String(code) + ", 消息: " + msg);
                return false;
            }
        } else {
            setError("解析飞书响应失败: " + String(error.c_str()));
            return false;
        }
    } else {
        setError("HTTP请求失败，状态码: " + String(response.statusCode) + ", 错误: " + httpClient.getLastError());
        return false;
    }
}

/**
 * @brief 获取当前时间戳（秒）
 * @return String 时间戳字符串
 */
String FeishuBotChannel::getCurrentTimestamp() {
    // 使用millis()生成时间戳（秒）
    unsigned long currentTime = millis() / 1000;
    return String(currentTime);
}

/**
 * @brief 转义JSON字符串
 * @param str 原始字符串
 * @return String 转义后的字符串
 */
String FeishuBotChannel::escapeJsonString(const String& str) {
    String escaped = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        switch (c) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}

// 注册飞书机器人推送渠道
static bool feishuBotChannelRegistered = PushChannelRegistry::getInstance().registerChannel(
    "feishu_bot", 
    []() -> std::unique_ptr<PushChannelBase> { return std::unique_ptr<PushChannelBase>(new FeishuBotChannel()); },
    {"飞书", "feishu"}
);