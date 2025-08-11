# 推送渠道配置帮助系统

## 概述

本文档介绍了ESP-SMS-Relay项目中新增的推送渠道配置帮助系统，该系统为用户提供了详细的渠道配置指导。

## 功能特性

### 1. 增强的帮助命令

- **通用帮助**: `help` 或 `h` - 显示所有可用命令和快速配置示例
- **渠道特定帮助**: `help <渠道名>` - 显示特定推送渠道的详细配置说明

### 2. 支持的渠道

- **企业微信** (`wechat`)
- **钉钉** (`dingtalk`) 
- **自定义Webhook** (`webhook`)

## 使用方法

### 查看所有帮助信息

```
sms-relay> help
```

显示内容包括：
- 所有可用命令列表
- 可用推送渠道概览
- 快速配置示例
- 详细帮助提示

### 查看特定渠道配置

#### 企业微信配置帮助
```
sms-relay> help wechat
```

#### 钉钉配置帮助
```
sms-relay> help dingtalk
```

#### Webhook配置帮助
```
sms-relay> help webhook
```

## 配置字段详解

### 企业微信 (wechat)

**必填字段：**
- `webhook_url`: 企业微信机器人Webhook地址

**可选字段：**
- `msg_type`: 消息类型 (text/markdown，默认text)
- `mentioned_list`: @指定用户，多个用逗号分隔
- `mentioned_mobile_list`: @指定手机号，多个用逗号分隔
- `template`: 消息模板，支持占位符

**配置示例：**
```json
{
  "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
  "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}",
  "msg_type": "text"
}
```

### 钉钉 (dingtalk)

**必填字段：**
- `webhook_url`: 钉钉机器人Webhook地址

**可选字段：**
- `secret`: 钉钉机器人密钥，用于签名验证
- `template`: 消息模板，支持占位符
- `msg_type`: 消息类型 (text/markdown，默认text)

**配置示例：**
```json
{
  "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN",
  "secret": "YOUR_SECRET",
  "template": "📱 收到新短信\n\n📞 发送方: {sender}\n🕐 时间: {timestamp}\n📄 内容: {content}",
  "msg_type": "text"
}
```

### 自定义Webhook (webhook)

**必填字段：**
- `webhook_url`: 目标URL地址

**可选字段：**
- `method`: HTTP方法 (GET/POST/PUT，默认POST)
- `content_type`: 内容类型 (默认application/json)
- `headers`: 自定义HTTP头部，格式为key1:value1,key2:value2
- `body_template`: 消息体模板，支持占位符

**配置示例：**
```json
{
  "webhook_url": "https://api.example.com/webhook",
  "method": "POST",
  "content_type": "application/json",
  "headers": "Authorization:Bearer YOUR_TOKEN,Content-Type:application/json",
  "body_template": "{\"message\":\"{content}\",\"from\":\"{sender}\",\"time\":\"{timestamp}\"}"
}
```

## 占位符说明

所有渠道的模板都支持以下占位符：

- `{sender}`: 短信发送方号码
- `{content}`: 短信内容
- `{timestamp}`: 接收时间戳
- `{sms_id}`: 短信记录ID

## 完整添加规则示例

### 企业微信规则
```
add "银行提醒" "95588" "wechat" "{\"webhook_url\":\"https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY\"}" "余额" false
```

### 钉钉规则
```
add "钉钉通知" "10086" "dingtalk" "{\"webhook_url\":\"https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN\"}" "流量" false
```

### Webhook规则
```
add "自定义推送" "*" "webhook" "{\"webhook_url\":\"https://api.example.com/webhook\",\"method\":\"POST\"}" "" false
```

## 故障排除

### 常见问题

1. **推送失败**
   - 检查webhook_url和密钥是否正确
   - 确认网络连接正常
   - 验证JSON格式是否正确

2. **配置格式错误**
   - 确保JSON格式正确，注意引号和逗号
   - 使用转义字符处理特殊字符
   - 检查必填字段是否完整

3. **权限问题**
   - 确保机器人已添加到目标群聊
   - 检查机器人权限设置
   - 验证访问令牌有效性

### 调试技巧

1. 使用 `test` 命令测试规则匹配
2. 查看系统状态了解错误信息
3. 逐步简化配置排查问题

## 技术实现

### 架构设计

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  TerminalManager│    │   PushManager   │    │ PushChannelBase │
│                 │    │                 │    │                 │
│ - executeHelp() │───▶│ getAllChannel() │───▶│   getHelp()     │
│ - showChannel() │    │ getAllExamples()│    │ getExample()    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 核心组件

1. **TerminalManager**: 处理CLI命令和帮助显示
2. **PushManager**: 管理推送渠道，提供统一接口
3. **PushChannelBase**: 各渠道的基类，提供帮助信息接口

### 数据结构

```cpp
struct PushChannelHelp {
    String channelName;      // 渠道名称
    String description;      // 渠道描述
    String configFields;     // 配置字段说明
    String ruleExample;      // 规则示例
    String troubleshooting;  // 故障排除信息
};

struct PushChannelExample {
    String channelName;      // 渠道名称
    String description;      // 渠道描述
    String configExample;    // 配置示例
    String usage;           // 使用说明
    String helpText;        // 帮助文本
};
```

## 更新日志

### v1.0.0 (2024-01-XX)
- 新增渠道特定帮助命令
- 实现详细配置字段说明
- 添加完整的配置示例
- 提供故障排除指导
- 支持快速添加命令模板

---

**注意**: 在使用配置示例时，请将 `YOUR_KEY`、`YOUR_TOKEN`、`YOUR_SECRET` 等占位符替换为实际的密钥和令牌。