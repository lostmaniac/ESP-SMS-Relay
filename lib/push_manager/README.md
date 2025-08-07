# 推送管理器 (Push Manager)

推送管理器是ESP-SMS-Relay项目的核心转发模块，负责根据配置的转发规则将接收到的短信推送到不同的目标平台。

## 功能特性

- 🎯 **智能规则匹配**: 支持号码模式匹配、关键词过滤
- 🔄 **多平台支持**: 企业微信、钉钉、自定义Webhook
- 📝 **模板系统**: 支持自定义消息模板
- 🗄️ **数据库集成**: 与数据库管理器无缝集成
- 🔧 **配置灵活**: JSON格式配置，支持复杂场景
- 📊 **状态跟踪**: 完整的推送状态记录

## 快速开始

### 1. 初始化推送管理器

```cpp
#include "push_manager/push_manager.h"

PushManager& pushManager = PushManager::getInstance();
if (!pushManager.initialize()) {
    Serial.println("推送管理器初始化失败: " + pushManager.getLastError());
    return;
}
```

### 2. 创建转发规则

```cpp
#include "database_manager/database_manager.h"

DatabaseManager& db = DatabaseManager::getInstance();

// 创建企业微信转发规则
ForwardRule rule;
rule.ruleName = "企业微信默认转发";
rule.sourceNumber = ""; // 空表示匹配所有号码
rule.keywords = ""; // 空表示匹配所有内容
rule.pushType = "wechat";
rule.pushConfig = R"({
    "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
    "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
})";
rule.enabled = true;
rule.isDefaultForward = true;

int ruleId = db.addForwardRule(rule);
```

### 3. 处理短信转发

```cpp
// 构建推送上下文
PushContext context;
context.sender = "10086";
context.content = "您的话费余额为100元";
context.timestamp = "241201120000";
context.smsRecordId = 123;

// 执行转发
PushResult result = pushManager.processSmsForward(context);
if (result == PUSH_SUCCESS) {
    Serial.println("转发成功");
}
```

## 支持的推送类型

### 1. 企业微信 (wechat)

```json
{
    "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
    "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
}
```

### 2. 钉钉 (dingtalk)

```json
{
    "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN",
    "template": "🚨 重要短信通知\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}"
}
```

### 3. 自定义Webhook (webhook)

```json
{
    "webhook_url": "https://your-server.com/api/sms-webhook",
    "method": "POST",
    "content_type": "application/json",
    "headers": "Authorization:Bearer TOKEN,X-Source:ESP-SMS-Relay",
    "body_template": "{\"type\":\"sms\",\"from\":\"{sender}\",\"content\":\"{content}\",\"timestamp\":\"{timestamp}\"}"
}
```

## 规则匹配说明

### 号码匹配模式

- `""` 或 `"*"`: 匹配所有号码
- `"10086"`: 精确匹配
- `"100*"`: 前缀匹配（以100开头）
- `"*86"`: 后缀匹配（以86结尾）
- `"10*86"`: 包含匹配（以10开头，86结尾）
- `"10086,95588"`: 多号码匹配（逗号分隔）

### 关键词匹配

- `""`: 匹配所有内容
- `"重要"`: 包含"重要"关键词
- `"重要,紧急,警告"`: 包含任一关键词（逗号分隔）

### 默认转发规则

设置 `isDefaultForward = true` 的规则会忽略号码和关键词匹配，对所有短信生效。

## 模板变量

在消息模板中可以使用以下变量：

- `{sender}`: 发送方号码
- `{content}`: 短信内容
- `{timestamp}`: 格式化的接收时间
- `{sms_id}`: 短信记录ID

## 配置示例

### 银行短信专用转发

```cpp
ForwardRule bankRule;
bankRule.ruleName = "银行短信转发";
bankRule.sourceNumber = "95588,95533,95599"; // 银行号码
bankRule.keywords = "";
bankRule.pushType = "wechat";
bankRule.pushConfig = R"({
    "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=BANK_KEY",
    "template": "🏦 银行短信通知\n\n📞 银行: {sender}\n🕐 时间: {timestamp}\n💰 内容: {content}"
})";
bankRule.enabled = true;
bankRule.isDefaultForward = false;
```

### 重要消息钉钉通知

```cpp
ForwardRule urgentRule;
urgentRule.ruleName = "紧急消息钉钉通知";
urgentRule.sourceNumber = "";
urgentRule.keywords = "重要,紧急,警告,故障";
urgentRule.pushType = "dingtalk";
urgentRule.pushConfig = R"({
    "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=URGENT_TOKEN",
    "template": "🚨 紧急短信通知\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n⚠️ 内容: {content}"
})";
urgentRule.enabled = true;
urgentRule.isDefaultForward = false;
```

## API 参考

### PushManager 类

#### 主要方法

- `getInstance()`: 获取单例实例
- `initialize()`: 初始化推送管理器
- `processSmsForward(context)`: 处理短信转发
- `pushByRule(ruleId, context)`: 按规则ID转发
- `testPushConfig(ruleId, testMessage)`: 测试推送配置

#### 返回值

```cpp
enum PushResult {
    PUSH_SUCCESS = 0,      // 推送成功
    PUSH_FAILED = 1,       // 推送失败
    PUSH_NO_RULE = 2,      // 没有匹配的规则
    PUSH_RULE_DISABLED = 3, // 规则已禁用
    PUSH_CONFIG_ERROR = 4,  // 配置错误
    PUSH_NETWORK_ERROR = 5  // 网络错误
};
```

### PushContext 结构体

```cpp
struct PushContext {
    String sender;         // 发送方号码
    String content;        // 短信内容
    String timestamp;      // 接收时间戳
    int smsRecordId;       // 短信记录ID
};
```

## 错误处理

```cpp
PushResult result = pushManager.processSmsForward(context);
switch (result) {
    case PUSH_SUCCESS:
        Serial.println("转发成功");
        break;
    case PUSH_NO_RULE:
        Serial.println("没有匹配的转发规则");
        break;
    case PUSH_CONFIG_ERROR:
        Serial.println("配置错误: " + pushManager.getLastError());
        break;
    case PUSH_NETWORK_ERROR:
        Serial.println("网络错误: " + pushManager.getLastError());
        break;
    default:
        Serial.println("转发失败: " + pushManager.getLastError());
        break;
}
```

## 调试模式

```cpp
pushManager.setDebugMode(true); // 启用详细日志输出
```

## 注意事项

1. **Webhook地址**: 确保webhook地址可访问且支持HTTPS
2. **网络连接**: 推送需要稳定的网络连接
3. **配置格式**: JSON配置必须格式正确
4. **规则优先级**: 多个规则匹配时都会执行
5. **错误重试**: 目前不支持自动重试，需要应用层处理

## 完整示例

参考 `examples/push_manager_usage.cpp` 文件查看完整的使用示例。

## 更新日志

- v1.0.0: 初始版本，支持企业微信、钉钉、Webhook推送
- 支持规则匹配、模板系统、状态跟踪
- 与数据库管理器集成

## 许可证

本项目采用 MIT 许可证。