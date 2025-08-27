/**
 * @file webhook_channel.cpp
 * @brief Webhook推送渠道实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "webhook_channel.h"
#include "../push_channel_registry.h"
#include "../http_client/http_client.h"
#include "../../../include/constants.h"
#include <ArduinoJson.h>

/**
 * @brief 构造函数
 */
WebhookChannel::WebhookChannel() {
    debugMode = false;
}

/**
 * @brief 析构函数
 */
WebhookChannel::~WebhookChannel() {
}

/**
 * @brief 获取渠道名称
 * @return String 渠道名称
 */
String WebhookChannel::getChannelName() const {
    return "webhook";
}

/**
 * @brief 获取渠道描述
 * @return String 渠道描述
 */
String WebhookChannel::getChannelDescription() const {
    return "通用Webhook推送";
}

/**
 * @brief 执行推送
 * @param config 推送配置（JSON格式）
 * @param context 推送上下文
 * @return PushResult 推送结果
 */
PushResult WebhookChannel::push(const String& config, const PushContext& context) {
    std::map<String, String> configMap = parseConfig(config);
    
    if (!validateConfig(configMap)) {
        return PUSH_CONFIG_ERROR;
    }
    
    String webhookUrl = configMap["webhook_url"];
    String method = configMap["method"];
    if (method.isEmpty()) {
        method = "POST";
    }
    
    String contentType = configMap["content_type"];
    if (contentType.isEmpty()) {
        contentType = "application/json";
    }
    
    // 获取消息模板
    String bodyTemplate = configMap["body_template"];
    if (bodyTemplate.isEmpty()) {
        // 使用默认JSON模板
        bodyTemplate = "{\"sender\":\"{sender}\",\"content\":\"{content}\",\"timestamp\":\"{timestamp}\"}";
    }
    
    String messageBody = applyTemplate(bodyTemplate, context, true); // Webhook需要JSON转义
    
    // 设置请求头
    std::map<String, String> headers;
    headers["Content-Type"] = contentType;
    
    // 添加自定义头部
    String customHeaders = configMap["headers"];
    if (!customHeaders.isEmpty()) {
        std::map<String, String> customHeadersMap = parseCustomHeaders(customHeaders);
        for (const auto& header : customHeadersMap) {
            headers[header.first] = header.second;
        }
    }
    
    debugPrint("推送到Webhook: " + webhookUrl);
    debugPrint("方法: " + method + ", 内容类型: " + contentType);
    debugPrint("消息内容: " + messageBody);
    
    // 发送HTTP请求
    HttpClient& httpClient = HttpClient::getInstance();
    HttpResponse response;
    
    HttpRequest httpRequest;
    httpRequest.url = webhookUrl;
    httpRequest.headers = headers;
    httpRequest.timeout = DEFAULT_HTTP_TIMEOUT_MS;

    if (method.equalsIgnoreCase("POST")) {
        httpRequest.method = HTTP_CLIENT_POST;
        httpRequest.body = messageBody;
    } else if (method.equalsIgnoreCase("PUT")) {
        httpRequest.method = HTTP_CLIENT_PUT;
        httpRequest.body = messageBody;
    } else if (method.equalsIgnoreCase("GET")) {
        httpRequest.method = HTTP_CLIENT_GET;
    } else {
        setError("不支持的HTTP方法: " + method + "，仅支持POST、GET和PUT");
        return PUSH_CONFIG_ERROR;
    }

    response = httpClient.request(httpRequest);
    
    debugPrint("Webhook响应 - 状态码: " + String(response.statusCode) + ", 错误码: " + String(response.error));
    debugPrint("响应内容: " + response.body);
    
    if (response.statusCode >= 200 && response.statusCode < 300) {
        debugPrint("✅ Webhook推送成功");
        return PUSH_SUCCESS;
    } else {
        setError("Webhook推送失败，状态码: " + String(response.statusCode) + ", 错误: " + httpClient.getLastError());
        return (response.error == 0) ? PUSH_FAILED : PUSH_NETWORK_ERROR;
    }
}

/**
 * @brief 测试推送配置
 * @param config 推送配置（JSON格式）
 * @param testMessage 测试消息
 * @return PushResult 推送结果
 */
PushResult WebhookChannel::testConfig(const String& config, const String& testMessage) {
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
PushChannelExample WebhookChannel::getConfigExample() const {
    PushChannelExample example;
    example.channelName = "Webhook";
    example.description = "通过自定义Webhook推送短信通知";
    example.configExample = R"({
  "webhook_url": "https://your-server.com/webhook",
  "method": "POST",
  "content_type": "application/json",
  "headers": "Authorization:Bearer YOUR_TOKEN,X-Custom-Header:value",
  "body_template": "{\"message\":\"{content}\",\"from\":\"{sender}\",\"time\":\"{timestamp}\"}"
})";
    example.usage = R"(使用说明：
1. 设置接收Webhook的服务器URL
2. 选择HTTP方法（POST、GET、PUT）
3. 设置内容类型（application/json、application/x-www-form-urlencoded等）
4. 可添加自定义头部，格式："Header1:Value1,Header2:Value2"
5. 自定义消息体模板，支持占位符：{sender}、{content}、{timestamp}、{sms_id}
6. 模板中的JSON字符串会自动转义特殊字符)";
    
    return example;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String WebhookChannel::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void WebhookChannel::setDebugMode(bool enable) {
    debugMode = enable;
}

/**
 * @brief 获取CLI演示代码
 * @return String CLI演示代码
 */
String WebhookChannel::getCliDemo() const {
    String demo = "// Webhook推送演示\n";
    demo += "void demoWebhookPush() {\n";
    demo += "    WebhookChannel webhook;\n";
    demo += "    webhook.setDebugMode(true);\n";
    demo += "    \n";
    demo += "    // 配置示例1：JSON格式\n";
    demo += "    String jsonConfig = \"{\n";
    demo += "        \\\"webhook_url\\\": \\\"https://your-server.com/webhook\\\",\n";
    demo += "        \\\"method\\\": \\\"POST\\\",\n";
    demo += "        \\\"content_type\\\": \\\"application/json\\\",\n";
    demo += "        \\\"headers\\\": \\\"Authorization:Bearer YOUR_TOKEN\\\",\n";
    demo += "        \\\"body_template\\\": \\\"{\\\\\\\"message\\\\\\\":\\\\\\\"{content}\\\\\\\",\\\\\\\"from\\\\\\\":\\\\\\\"{sender}\\\\\\\",\\\\\\\"time\\\\\\\":\\\\\\\"{timestamp}\\\\\\\"}\\\"\n";
    demo += "    }\";\n";
    demo += "    \n";
    demo += "    // 配置示例2：表单格式\n";
    demo += "    String formConfig = \"{\n";
    demo += "        \\\"webhook_url\\\": \\\"https://your-server.com/form\\\",\n";
    demo += "        \\\"method\\\": \\\"POST\\\",\n";
    demo += "        \\\"content_type\\\": \\\"application/x-www-form-urlencoded\\\",\n";
    demo += "        \\\"body_template\\\": \\\"sender={sender}&content={content}&timestamp={timestamp}\\\"\n";
    demo += "    }\";\n";
    demo += "    \n";
    demo += "    // 测试JSON推送\n";
    demo += "    PushResult result1 = webhook.testConfig(jsonConfig, \\\"这是一条JSON格式的测试消息\\\");\n";
    demo += "    if (result1 == PUSH_SUCCESS) {\n";
    demo += "        Serial.println(\\\"✅ JSON Webhook推送测试成功\\\");\n";
    demo += "    } else {\n";
    demo += "        Serial.println(\\\"❌ JSON Webhook推送测试失败: \\\" + webhook.getLastError());\n";
    demo += "    }\n";
    demo += "    \n";
    demo += "    // 测试表单推送\n";
    demo += "    PushResult result2 = webhook.testConfig(formConfig, \\\"这是一条表单格式的测试消息\\\");\n";
    demo += "    if (result2 == PUSH_SUCCESS) {\n";
    demo += "        Serial.println(\\\"✅ 表单 Webhook推送测试成功\\\");\n";
    demo += "    } else {\n";
    demo += "        Serial.println(\\\"❌ 表单 Webhook推送测试失败: \\\" + webhook.getLastError());\n";
    demo += "    }\n";
    demo += "    \n";
    demo += "    // 显示配置示例\n";
    demo += "    PushChannelExample example = webhook.getConfigExample();\n";
    demo += "    Serial.println(\\\"配置示例:\\\");\n";
    demo += "    Serial.println(example.configExample);\n";
    demo += "    Serial.println(\\\"使用说明:\\\");\n";
    demo += "    Serial.println(example.usage);\n";
    demo += "}";
    
    return demo;
}

/**
 * @brief 获取渠道帮助信息
 * @return PushChannelHelp 帮助信息
 */
PushChannelHelp WebhookChannel::getHelp() const {
    PushChannelHelp help;
    help.channelName = "webhook";
    help.description = "Webhook推送渠道，支持向指定URL发送HTTP请求";
    
    help.configFields = "配置字段说明:\n"
                       "- webhook_url: 目标URL地址 (必填)\n"
                       "- method: HTTP方法，支持GET/POST/PUT (默认POST)\n"
                       "- content_type: 内容类型 (默认application/json)\n"
                       "- headers: 自定义HTTP头部，格式为key1:value1,key2:value2\n"
                       "- body_template: 消息体模板，支持占位符{sender},{content},{timestamp},{sms_id}\n";
    
    help.ruleExample = "转发规则示例:\n"
                      "1. 基本配置:\n"
                      "   {\"webhook_url\":\"https://api.example.com/webhook\",\"method\":\"POST\"}\n\n"
                      "2. 带自定义头部:\n"
                      "   {\"webhook_url\":\"https://api.example.com/webhook\",\"method\":\"POST\",\"headers\":\"Authorization:Bearer token123,Content-Type:application/json\"}\n\n"
                      "3. 自定义消息模板:\n"
                      "   {\"webhook_url\":\"https://api.example.com/webhook\",\"body_template\":\"{\\\"message\\\":\\\"{content}\\\",\\\"from\\\":\\\"{sender}\\\",\\\"time\\\":\\\"{timestamp}\\\"}\"}";
    
    help.troubleshooting = "常见问题解决:\n"
                          "1. 推送失败: 检查webhook_url是否正确，网络是否连通\n"
                          "2. 超时错误: 检查目标服务器响应速度\n"
                          "3. 认证失败: 检查headers中的认证信息是否正确\n"
                          "4. 格式错误: 确保配置为有效的JSON格式\n"
                          "5. 模板错误: 检查body_template中的占位符是否正确";
    
    return help;
}

/**
 * @brief 验证配置参数
 * @param configMap 配置映射
 * @return bool 配置是否有效
 */
bool WebhookChannel::validateConfig(const std::map<String, String>& configMap) {
    auto it = configMap.find("webhook_url");
    if (it == configMap.end() || it->second.isEmpty()) {
        setError("Webhook配置缺少webhook_url");
        return false;
    }
    
    // 验证URL格式
    String webhookUrl = it->second;
    if (!webhookUrl.startsWith("http://") && !webhookUrl.startsWith("https://")) {
        setError("Webhook URL格式不正确，应以http://或https://开头");
        return false;
    }
    
    // 验证HTTP方法
    auto methodIt = configMap.find("method");
    if (methodIt != configMap.end() && !methodIt->second.isEmpty()) {
        if (!isValidHttpMethod(methodIt->second)) {
            setError("不支持的HTTP方法: " + methodIt->second);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 解析自定义头部
 * @param headersStr 头部字符串（格式："Header1:Value1,Header2:Value2"）
 * @return std::map<String, String> 头部映射
 */
std::map<String, String> WebhookChannel::parseCustomHeaders(const String& headersStr) {
    std::map<String, String> headers;
    
    if (headersStr.isEmpty()) {
        return headers;
    }
    
    // 解析自定义头部
    int startIndex = 0;
    int commaIndex = headersStr.indexOf(',');
    
    while (commaIndex != -1 || startIndex < headersStr.length()) {
        String headerPair;
        if (commaIndex != -1) {
            headerPair = headersStr.substring(startIndex, commaIndex);
            startIndex = commaIndex + 1;
            commaIndex = headersStr.indexOf(',', startIndex);
        } else {
            headerPair = headersStr.substring(startIndex);
            startIndex = headersStr.length();
        }
        
        int colonIndex = headerPair.indexOf(':');
        if (colonIndex != -1) {
            String headerName = headerPair.substring(0, colonIndex);
            String headerValue = headerPair.substring(colonIndex + 1);
            headerName.trim();
            headerValue.trim();
            if (!headerName.isEmpty() && !headerValue.isEmpty()) {
                headers[headerName] = headerValue;
            }
        }
    }
    
    return headers;
}

/**
 * @brief 验证HTTP方法
 * @param method HTTP方法
 * @return bool 方法是否支持
 */
bool WebhookChannel::isValidHttpMethod(const String& method) {
    String upperMethod = method;
    upperMethod.toUpperCase();
    
    return (upperMethod == "GET" || upperMethod == "POST" || upperMethod == "PUT");
}

// 自动注册Webhook渠道
REGISTER_PUSH_CHANNEL("webhook", WebhookChannel, (std::vector<String>{"webhook", "http", "api"}));