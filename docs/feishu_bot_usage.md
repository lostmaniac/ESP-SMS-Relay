# 飞书机器人推送功能使用指南

## 概述

ESP-SMS-Relay 现已支持飞书自定义机器人推送功能，可以将接收到的短信转发到飞书群组中。

## 功能特性

- ✅ 支持文本消息推送
- ✅ 支持富文本消息推送
- ✅ 支持消息卡片推送
- ✅ 支持签名校验（可选）
- ✅ 支持关键词校验（可选）
- ✅ 支持消息模板自定义
- ✅ 支持多种别名：`feishu_bot`、`飞书`、`feishu`

## 配置步骤

### 1. 创建飞书自定义机器人

1. 在飞书群组中，点击群设置
2. 选择「群机器人」→「添加机器人」→「自定义机器人」
3. 设置机器人名称和描述
4. 复制生成的 Webhook 地址
5. （可选）设置安全设置中的签名校验

### 2. 配置转发规则

#### 基础文本消息配置
```json
{
  "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxxxxxxxxxxxxxxxx",
  "message_type": "text",
  "message_template": "📱 短信转发通知\n\n📞 发送方：{sender}\n📄 内容：{content}\n🕐 时间：{timestamp}"
}
```

#### 富文本消息配置（带签名校验）
```json
{
  "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxxxxxxxxxxxxxxxx",
  "message_type": "rich_text",
  "secret": "your_secret_key",
  "title": "短信转发通知",
  "message_template": "**发送方：** {sender}\n**内容：** {content}\n**时间：** {timestamp}"
}
```

#### 消息卡片配置
```json
{
  "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxxxxxxxxxxxxxxxx",
  "message_type": "post",
  "title": "📱 短信转发通知",
  "message_template": "**发送方：** {sender}\n**内容：** {content}\n**时间：** {timestamp}\n\n---\n*由 ESP-SMS-Relay 自动转发*"
}
```

## 配置参数说明

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `webhook_url` | String | ✅ | 飞书机器人的 Webhook 地址 |
| `message_type` | String | ❌ | 消息类型：`text`、`rich_text`、`post`（默认：`text`） |
| `secret` | String | ❌ | 签名密钥，用于安全校验 |
| `title` | String | ❌ | 消息标题（富文本和消息卡片使用） |
| `message_template` | String | ❌ | 消息模板，支持占位符 |

## 消息模板占位符

- `{sender}` - 短信发送方号码
- `{content}` - 短信内容
- `{timestamp}` - 接收时间戳
- `{sms_id}` - 短信记录ID

## 消息类型说明

### 1. 文本消息 (text)
- 最简单的消息格式
- 支持换行和基本格式
- 最大长度：30000字符

### 2. 富文本消息 (rich_text)
- 支持标题和富文本内容
- 支持基本的文本格式
- 标题最大长度：100字符
- 内容最大长度：30000字符

### 3. 消息卡片 (post)
- 最丰富的消息格式
- 支持标题、内容和格式化
- 支持 Markdown 语法
- 标题最大长度：100字符
- 内容最大长度：30000字符

## 安全设置

### 签名校验
如果在飞书机器人设置中启用了签名校验，需要在配置中添加 `secret` 字段：

```json
{
  "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx",
  "secret": "your_secret_key",
  "message_type": "text"
}
```

### 关键词校验
飞书机器人支持关键词校验，可以在机器人设置中配置必须包含的关键词。

## 频率限制

飞书机器人有以下频率限制：
- 每分钟最多 100 次请求
- 每秒最多 5 次请求

## 测试配置

可以通过 Web 界面或 API 测试飞书机器人配置：

```bash
# 通过 API 测试
curl -X POST http://your-device-ip/api/test-push \
  -H "Content-Type: application/json" \
  -d '{
    "pushType": "feishu_bot",
    "config": {
      "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx",
      "message_type": "text"
    },
    "testMessage": "这是一条测试消息"
  }'
```

## 故障排除

### 常见问题

1. **推送失败：Webhook 地址无效**
   - 检查 Webhook 地址是否正确
   - 确认机器人是否已添加到目标群组

2. **推送失败：签名校验失败**
   - 检查 `secret` 字段是否正确
   - 确认时间戳是否同步

3. **推送失败：消息内容过长**
   - 检查消息内容是否超过长度限制
   - 适当缩短消息模板

4. **推送失败：频率限制**
   - 检查是否触发了频率限制
   - 适当增加推送间隔

### 调试模式

启用调试模式可以查看详细的推送日志：

```cpp
PushManager& pushManager = PushManager::getInstance();
pushManager.setDebugMode(true);
```

## 示例配置

### 完整的转发规则示例

```json
{
  "ruleName": "飞书通知",
  "sourceNumber": "",
  "keywords": "",
  "pushType": "feishu_bot",
  "pushConfig": {
    "webhook_url": "https://open.feishu.cn/open-apis/bot/v2/hook/xxxxxxxxxxxxxxxxx",
    "message_type": "rich_text",
    "secret": "your_secret_key",
    "title": "📱 短信转发通知",
    "message_template": "**发送方：** {sender}\n**内容：** {content}\n**时间：** {timestamp}\n\n---\n*设备：ESP-SMS-Relay*"
  },
  "enabled": true
}
```

## 更多信息

- [飞书开放平台文档](https://open.feishu.cn/document/client-docs/bot-v3/add-custom-bot)
- [ESP-SMS-Relay 项目文档](../README.md)
- [推送管理器文档](../lib/push_manager/README.md)