/**
 * @file wechat_official_channel.cpp
 * @brief 微信公众号推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "wechat_official_channel.h"
#include "../push_channel_registry.h"
#include "../../http_client/http_client.h"
#include "../../../include/constants.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

/**
 * @brief 构造函数
 */
WechatOfficialChannel::WechatOfficialChannel() {
    debugMode = false;
    tokenExpireTime = 0;
}

/**
 * @brief 析构函数
 */
WechatOfficialChannel::~WechatOfficialChannel() {
}

/**
 * @brief 获取渠道名称
 * @return String 渠道名称
 */
String WechatOfficialChannel::getChannelName() const {
    return "wechat_official";
}

/**
 * @brief 获取渠道描述
 * @return String 渠道描述
 */
String WechatOfficialChannel::getChannelDescription() const {
    return "微信公众号推送";
}

/**
 * @brief 执行推送
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult WechatOfficialChannel::push(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    if (!validateConfig(configMap)) {
        return PUSH_CONFIG_ERROR;
    }
    
    String appId = configMap["app_id"];
    String appSecret = configMap["app_secret"];
    String openIds = configMap["open_ids"];
    
    // 获取access_token
    String accessToken = getAccessToken(appId, appSecret);
    if (accessToken.isEmpty()) {
        setError("获取微信公众号access_token失败");
        return PUSH_NETWORK_ERROR;
    }
    
    debugPrint("获取到access_token: " + accessToken.substring(0, 20) + "...");
    
    // 解析openid列表
    std::vector<String> openIdList = parseOpenIds(openIds);
    if (openIdList.empty()) {
        setError("openid列表为空");
        return PUSH_CONFIG_ERROR;
    }
    
    // 获取模板ID（必需）
    String templateId = configMap["template_id"];
    if (templateId.isEmpty()) {
        setError("模板ID不能为空，微信公众号推送仅支持模板消息");
        return PUSH_CONFIG_ERROR;
    }
    
    int successCount = 0;
    int totalCount = openIdList.size();
    
    for (const String& openId : openIdList) {
        // 发送模板消息
        String templateData = buildTemplateData(context);
        bool success = sendTemplateMessage(accessToken, openId, templateId, templateData, "");
        debugPrint("向 " + openId + " 发送模板消息: " + (success ? "成功" : "失败"));
        
        if (success) {
            successCount++;
        }
        
        // 避免频率限制，添加延迟，期间重置看门狗
        if (totalCount > 1) {
            for (int i = 0; i < 10; i++) {
                esp_task_wdt_reset();
                vTaskDelay(1);
            }
        }
    }
    
    debugPrint("推送完成，成功: " + String(successCount) + "/" + String(totalCount));
    
    if (successCount == 0) {
        setError("所有用户推送失败");
        return PUSH_FAILED;
    } else if (successCount < totalCount) {
        setError("部分用户推送失败，成功: " + String(successCount) + "/" + String(totalCount));
        return PUSH_SUCCESS; // 部分成功也算成功
    } else {
        return PUSH_SUCCESS;
    }
}

/**
 * @brief 测试推送配置
 * @param config 推送配置（JSON格式）
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult WechatOfficialChannel::testConfig(const String& config, const String& testMessage) {
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
PushChannelExample WechatOfficialChannel::getConfigExample() const {
    PushChannelExample example;
    example.channelName = "微信公众号";
    example.description = "通过微信公众号向关注用户推送短信通知";
    example.configExample = R"({
  "app_id": "wx1234567890abcdef",
  "app_secret": "your_app_secret_here",
  "open_ids": "openid1,openid2,openid3",
  "template_id": "j7JAxTe0RLjPRUqcONvE7LeHeQdoH5yDu8XpECaP-ws"
})";
    example.usage = R"(使用说明：
1. 在微信公众平台获取AppID和AppSecret
2. 获取关注用户的OpenID
3. 配置模板消息ID（必填）
4. 模板格式已固定为：发件人和短信内容两个字段
5. 模板消息需要用户关注公众号且48小时内有交互
6. 模板内容格式：发件人：{{sender.DATA}} 短信内容：{{content.DATA}})";
        return example;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String WechatOfficialChannel::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void WechatOfficialChannel::setDebugMode(bool enable) {
    debugMode = enable;
}

/**
 * @brief 获取CLI演示代码
 * @return String CLI演示代码
 */
String WechatOfficialChannel::getCliDemo() const {
    String demo = "// 微信公众号推送演示\n";
    demo += "void demoWechatOfficialPush() {\n";
    demo += "    WechatOfficialChannel wechatOfficial;\n";
    demo += "    wechatOfficial.setDebugMode(true);\n";
    demo += "    \n";
    demo += "    // 配置示例\n";
    demo += "    String config = \"{\\\"app_id\\\":\\\"wx1234567890abcdef\\\",\\\"app_secret\\\":\\\"your_app_secret\\\",\\\"open_ids\\\":\\\"openid1,openid2\\\",\\\"template_id\\\":\\\"j7JAxTe0RLjPRUqcONvE7LeHeQdoH5yDu8XpECaP-ws\\\"}\";\n";
    demo += "    \n";
    demo += "    // 测试推送\n";
    demo += "    PushResult result = wechatOfficial.testConfig(config, \"这是一条测试消息\");\n";
    demo += "    \n";
    demo += "    if (result == PUSH_SUCCESS) {\n";
    demo += "        Serial.println(\"✅ 微信公众号推送测试成功\");\n";
    demo += "    } else {\n";
    demo += "        Serial.println(\"❌ 微信公众号推送测试失败: \" + wechatOfficial.getLastError());\n";
    demo += "    }\n";
    demo += "    \n";
    demo += "    // 显示配置示例\n";
    demo += "    PushChannelExample example = wechatOfficial.getConfigExample();\n";
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
PushChannelHelp WechatOfficialChannel::getHelp() const {
    PushChannelHelp help;
    help.channelName = "wechat_official";
    help.description = "微信公众号推送渠道，支持向关注用户发送模板消息或客服消息";
    
    help.configFields = "配置字段说明:\n"
                       "- app_id: 微信公众号AppID (必填)\n"
                       "- app_secret: 微信公众号AppSecret (必填)\n"
                       "- open_ids: 用户OpenID列表，多个用逗号分隔 (必填)\n"
                       "- template_id: 模板消息ID (必填)\n";
    
    help.ruleExample = "转发规则示例:\n"
                      "模板消息推送配置:\n"
                      "{\"app_id\":\"wx123\",\"app_secret\":\"secret\",\"open_ids\":\"openid1\",\"template_id\":\"j7JAxTe0RLjPRUqcONvE7LeHeQdoH5yDu8XpECaP-ws\"}";
    
    help.troubleshooting = "常见问题解决:\n"
                          "1. access_token获取失败: 检查AppID和AppSecret是否正确\n"
                          "2. 模板消息发送失败: 确保用户关注公众号且48小时内有交互\n"
                          "3. OpenID无效: 确保OpenID格式正确且用户仍关注公众号\n"
                          "4. 频率限制: 微信API有调用频率限制，请控制发送频率\n"
                          "5. 模板ID无效: 确保模板已在公众平台审核通过\n"
                          "6. 网络错误: 检查网络连接和防火墙设置\n"
                          "7. 内容被截断: 微信模板消息内容限制200字符，超长内容会自动截断并添加'...'\n"
                          "8. 模板格式固定: 系统使用固定的模板格式，包含发件人和短信内容两个字段";
    
    return help;
}

/**
 * @brief 验证配置参数
 * @param configMap 配置映射
 * @return bool 配置是否有效
 */
bool WechatOfficialChannel::validateConfig(const std::map<String, String>& configMap) {
    // 检查必填字段
    auto appIdIt = configMap.find("app_id");
    if (appIdIt == configMap.end() || appIdIt->second.isEmpty()) {
        setError("微信公众号配置缺少app_id");
        return false;
    }
    
    auto appSecretIt = configMap.find("app_secret");
    if (appSecretIt == configMap.end() || appSecretIt->second.isEmpty()) {
        setError("微信公众号配置缺少app_secret");
        return false;
    }
    
    auto openIdsIt = configMap.find("open_ids");
    if (openIdsIt == configMap.end() || openIdsIt->second.isEmpty()) {
        setError("微信公众号配置缺少open_ids");
        return false;
    }
    
    // 验证AppID格式
    String appId = appIdIt->second;
    if (!appId.startsWith("wx") || appId.length() != 18) {
        setError("微信公众号AppID格式不正确，应以wx开头且长度为18位");
        return false;
    }
    
    return true;
}

/**
 * @brief 获取微信公众号access_token
 * @param appId 应用ID
 * @param appSecret 应用密钥
 * @return String access_token，失败返回空字符串
 */
String WechatOfficialChannel::getAccessToken(const String& appId, const String& appSecret) {
    // 检查缓存的token是否有效
    if (!cachedAccessToken.isEmpty() && millis() < tokenExpireTime) {
        debugPrint("使用缓存的access_token");
        return cachedAccessToken;
    }
    
    String url = "https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid=" + appId + "&secret=" + appSecret;
    
    debugPrint("获取access_token: " + url);
    
    HttpClient& httpClient = HttpClient::getInstance();
    HttpRequest request;
    request.url = url;
    request.method = HTTP_CLIENT_GET;
    request.timeout = DEFAULT_HTTP_TIMEOUT_MS;
    HttpResponse response = httpClient.request(request);
    
    debugPrint("access_token响应 - 状态码: " + String(response.statusCode));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode != 200) {
        setError("获取access_token失败，HTTP状态码: " + String(response.statusCode));
        return "";
    }
    
    // 解析JSON响应
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response.body);
    
    if (error) {
        setError("解析access_token响应失败: " + String(error.c_str()));
        return "";
    }
    
    if (doc["errcode"].is<int>()) {
        int errcode = doc["errcode"];
        String errmsg = doc["errmsg"].as<String>();
        setError("微信API错误: " + String(errcode) + " - " + errmsg);
        return "";
    }
    
    if (!doc["access_token"].is<String>()) {
        setError("响应中未找到access_token");
        return "";
    }
    
    String accessToken = doc["access_token"].as<String>();
    int expiresIn = doc["expires_in"].as<int>();
    
    // 缓存token，提前5分钟过期
    cachedAccessToken = accessToken;
    tokenExpireTime = millis() + (expiresIn - 300) * 1000;
    
    debugPrint("获取access_token成功，有效期: " + String(expiresIn) + "秒");
    
    return accessToken;
}

/**
 * @brief 发送模板消息
 * @param accessToken 访问令牌
 * @param openId 用户openid
 * @param templateId 模板ID
 * @param data 模板数据
 * @param url 跳转链接（可选）
 * @return bool 发送是否成功
 */
bool WechatOfficialChannel::sendTemplateMessage(const String& accessToken, const String& openId, 
                                              const String& templateId, const String& data, const String& url) {
    String apiUrl = "https://api.weixin.qq.com/cgi-bin/message/template/send?access_token=" + accessToken;
    
    JsonDocument doc;
    doc["touser"] = openId;
    doc["template_id"] = templateId;
    if (!url.isEmpty()) {
        doc["url"] = url;
    }
    
    // 解析模板数据
    JsonDocument dataDoc;
    DeserializationError error = deserializeJson(dataDoc, data);
    if (!error) {
        doc["data"] = dataDoc;
    } else {
        debugPrint("模板数据解析失败，使用默认格式");
        doc["data"]["content"]["value"] = data;
        doc["data"]["content"]["color"] = "#173177";
    }
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    debugPrint("发送模板消息: " + apiUrl);
    debugPrint("请求体: " + requestBody);
    
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    
    HttpClient& httpClient = HttpClient::getInstance();
    HttpResponse response = httpClient.post(apiUrl, requestBody, headers, DEFAULT_HTTP_TIMEOUT_MS);
    
    debugPrint("模板消息响应 - 状态码: " + String(response.statusCode));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode != 200) {
        setError("发送模板消息失败，HTTP状态码: " + String(response.statusCode));
        return false;
    }
    
    // 解析响应
    JsonDocument responseDoc;
    error = deserializeJson(responseDoc, response.body);
    
    if (error) {
        setError("解析模板消息响应失败: " + String(error.c_str()));
        return false;
    }
    
    int errcode = responseDoc["errcode"].as<int>();
    if (errcode != 0) {
        String errmsg = responseDoc["errmsg"].as<String>();
        setError("模板消息发送失败: " + String(errcode) + " - " + errmsg);
        return false;
    }
    
    return true;
}



/**
 * @brief 构建模板消息数据
 * @param context 推送上下文
 * @return String 模板数据JSON
 */
String WechatOfficialChannel::buildTemplateData(const PushContext& context) {
    JsonDocument doc;
    
    // 使用固定的模板格式 - 适配微信公众号模板消息结构
    // 模板内容：发件人：{{sender.DATA}} 短信内容：{{content.DATA}}
    doc["sender"]["value"] = context.sender;
    doc["sender"]["color"] = "#173177";
    
    // 微信公众号模板消息内容长度限制为200个字符
    String content = context.content;
    // 移除换行符，微信模板消息不支持换行符
    content.replace("\n", " ");
    content.replace("\r", " ");
    if (content.length() > WECHAT_TEMPLATE_CONTENT_MAX_LENGTH) {
        content = content.substring(0, WECHAT_TEMPLATE_CONTENT_MAX_LENGTH) + "...";
    }
    doc["content"]["value"] = content;
    doc["content"]["color"] = "#173177";
    
    String result;
    serializeJson(doc, result);
    return result;
}

/**
 * @brief 解析openid列表
 * @param openIdStr 逗号分隔的openid字符串
 * @return std::vector<String> openid列表
 */
std::vector<String> WechatOfficialChannel::parseOpenIds(const String& openIdStr) {
    std::vector<String> openIds;
    
    if (openIdStr.isEmpty()) {
        return openIds;
    }
    
    String str = openIdStr;
    str.trim();
    
    int start = 0;
    int end = str.indexOf(',');
    
    while (end != -1) {
        String openId = str.substring(start, end);
        openId.trim();
        if (!openId.isEmpty()) {
            openIds.push_back(openId);
        }
        start = end + 1;
        end = str.indexOf(',', start);
    }
    
    // 处理最后一个openid
    String lastOpenId = str.substring(start);
    lastOpenId.trim();
    if (!lastOpenId.isEmpty()) {
        openIds.push_back(lastOpenId);
    }
    
    return openIds;
}

/**
 * @brief 检查access_token是否有效
 * @param accessToken 访问令牌
 * @return bool 是否有效
 */
bool WechatOfficialChannel::isAccessTokenValid(const String& accessToken) {
    if (accessToken.isEmpty()) {
        return false;
    }
    
    // 简单检查token格式（微信access_token通常是一串字母数字组合）
    if (accessToken.length() < 100) {
        return false;
    }
    
    return true;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void WechatOfficialChannel::setError(const String& error) {
    lastError = error;
    if (debugMode) {
        Serial.println("[WechatOfficialChannel] 错误: " + error);
    }
}

/**
 * @brief 打印调试信息
 * @param message 调试信息
 */
void WechatOfficialChannel::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[WechatOfficialChannel] " + message);
    }
}

/**
 * @brief 格式化时间戳
 * @param timestamp 原始时间戳
 * @return String 格式化后的时间
 */
String WechatOfficialChannel::formatTimestamp(const String& timestamp) {
    if (timestamp.length() >= 12) {
        // 格式: YYMMDDHHMMSS -> YYYY-MM-DD HH:MM:SS
        String year = "20" + timestamp.substring(0, 2);
        String month = timestamp.substring(2, 4);
        String day = timestamp.substring(4, 6);
        String hour = timestamp.substring(6, 8);
        String minute = timestamp.substring(8, 10);
        String second = timestamp.substring(10, 12);
        
        return year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second;
    }
    return timestamp;
}

/**
 * @brief 应用消息模板
 * @param messageTemplate 消息模板
 * @param context 推送上下文
 * @return String 应用模板后的消息
 */
String WechatOfficialChannel::applyTemplate(const String& messageTemplate, const PushContext& context) {
    String result = messageTemplate;
    result.replace("{sender}", context.sender);
    result.replace("{content}", context.content);
    result.replace("{timestamp}", formatTimestamp(context.timestamp));
    result.replace("{sms_id}", String(context.smsRecordId));
    return result;
}

/**
 * @brief 解析配置字符串
 * @param config JSON配置字符串
 * @return std::map<String, String> 配置映射
 */
std::map<String, String> WechatOfficialChannel::parseConfig(const String& config) {
    std::map<String, String> configMap;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, config);
    
    if (error) {
        setError("配置JSON解析失败: " + String(error.c_str()));
        return configMap;
    }
    
    // 提取配置字段
    if (doc["app_id"].is<String>()) {
        configMap["app_id"] = doc["app_id"].as<String>();
    }
    if (doc["app_secret"].is<String>()) {
        configMap["app_secret"] = doc["app_secret"].as<String>();
    }
    if (doc["open_ids"].is<String>()) {
        configMap["open_ids"] = doc["open_ids"].as<String>();
    }
    if (doc["template_id"].is<String>()) {
        configMap["template_id"] = doc["template_id"].as<String>();
    }
    if (doc["url"].is<String>()) {
        configMap["url"] = doc["url"].as<String>();
    }
    if (doc["template"].is<String>()) {
        configMap["template"] = doc["template"].as<String>();
    }
    
    return configMap;
}

// 自动注册微信公众号渠道
REGISTER_PUSH_CHANNEL("wechat_official", WechatOfficialChannel, (std::vector<String>{"微信公众号", "公众号", "wechat_mp"}));