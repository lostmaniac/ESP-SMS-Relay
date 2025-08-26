#ifndef DOCS_GUIDE_H
#define DOCS_GUIDE_H

const char DOCS_GUIDE_CONTENT[] PROGMEM = R"rawliteral(
# 转发规则配置指南 (v2)

本文档详细介绍了如何在 ESP-SMS-Relay 系统中配置转发规则，特别是针对不同推送渠道的 `push_config` JSON 对象的配置方法。本文档根据最新代码和用户提供的示例编写。

## 1. 规则通用字段说明

- **名称 (Name)**: 规则的自定义名称。
- **来源号码 (Source Number)**: 匹配短信发送方。支持 `*` 通配符。
- **关键字 (Keywords)**: 匹配短信内容，多个关键字用英文逗号 `,` 分隔。
- **推送类型 (Push Type)**: 选择目标渠道，必须与下文提供的类型名称完全一致。
- **推送配置 (Push Config)**: 定义渠道参数的 JSON 对象。
- **启用 (Enabled)**: 是否激活规则。
- **默认转发 (Default Forward)**: 当没有其他规则匹配时，是否使用此规则。

---

## 2. 推送渠道配置 (`push_config`)

### 2.1 企业微信机器人 (WeCom)

- **推送类型**: `wecom`
- **JSON 字段**:
  - `webhook_url` (必需, 字符串): 机器人的 Webhook 地址。
  - `msg_type` (可选, 字符串): 消息类型, 支持 `text` (默认) 和 `markdown`。
  - `template` (可选, 字符串): 自定义消息模板。支持占位符 `{sender}`, `{content}`, `{timestamp}`, `{sms_id}`。

- **配置示例**:
```json
{
  "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
  "msg_type": "text",
  "template": "来自: {sender}\n内容: {content}"
}
```

### 2.2 飞书机器人 (Feishu)

- **推送类型**: `feishu`
- **JSON 字段**:
  - `webhook_url` (必需, 字符串): 飞书机器人的 Webhook 地址。
  - `secret` (可选, 字符串): 签名校验的密钥。
  - `message_type` (可选, 字符串): 消息类型, 支持 `text` (默认) 和 `post` (富文本)。
  - `title` (可选, 字符串): 消息标题。
  - `message_template` (可选, 字符串): 自定义消息模板。支持占位符 `{sender}`, `{content}` 等。

- **配置示例**:
```json
{
  "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/YOUR_HOOK_ID",
  "secret": "YOUR_SECRET",
  "message_type": "text",
  "title": "短信转发通知",
  "message_template": "发件人: {sender}\n内容: {content}"
}
```

### 2.3 钉钉机器人 (DingTalk)

- **推送类型**: `dingtalk`
- **JSON 字段**:
  - `webhook` (必需, 字符串): 钉钉机器人的 Webhook 地址。
  - `secret` (可选, 字符串): 加签密钥。

- **配置示例**:
```json
{
  "webhook": "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN",
  "secret": "SEC..."
}
```

### 2.4 微信公众号 (WeChat Official)

- **推送类型**: `wechat_official`
- **说明**: 通过调用微信公众号的模板消息接口进行推送。
- **JSON 字段**:
  - `app_id` (必需, 字符串): 您的微信公众号的 AppID。
  - `app_secret` (必需, 字符串): 您的微信公众号的 AppSecret。
  - `open_ids` (必需, 字符串): 接收消息的用户的 OpenID，多个用逗号 `,` 分隔。
  - `template_id` (必需, 字符串): 要使用的模板消息的 ID。

- **配置示例**:
```json
{
  "app_id": "wxfd...",
  "app_secret": "your_app_secret",
  "open_ids": "oGZ...",
  "template_id": "your_template_id"
}
```

### 2.5 通用 Webhook

- **推送类型**: `webhook`
- **JSON 字段**:
  - `url` (必需, 字符串): 目标 URL。
  - `method` (可选, 字符串): HTTP 方法 (e.g., `POST`), 默认为 `POST`。
  - `headers` (可选, 对象): 自定义请求头。
  - `body_template` (必需, 字符串): 请求体模板，内部的 `"` 需转义为 `"`。
  - `is_json` (可选, 布尔值): 是否添加 `Content-Type: application/json` 头，默认为 `true`。

- **配置示例**:
```json
{
  "url": "https://your-service.com/api/sms",
  "headers": {
    "Authorization": "Bearer your_token"
  },
  "body_template": "{\"text\":\"From: {{sms.from}}, Content: {{sms.content}}\"}"
}
```
)rawliteral";

#endif // DOCS_GUIDE_H
