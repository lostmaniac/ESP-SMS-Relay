# Push Manager 推送管理模块

## 概述

Push Manager 是 ESP-SMS-Relay 项目的推送管理模块，采用模块化设计，支持多种推送渠道，具有高度的可扩展性和可维护性。

## 架构设计

### 核心组件

```
push_manager/
├── push_channel_base.h/cpp      # 推送渠道基类
├── push_channel_factory.h/cpp   # 推送渠道工厂
├── push_manager.h/cpp           # 推送管理器
├── wechat_channel.h/cpp         # 企业微信推送渠道
├── dingtalk_channel.h/cpp       # 钉钉推送渠道
├── webhook_channel.h/cpp        # Webhook推送渠道
├── push_cli_demo.h/cpp          # CLI演示程序
└── README.md                    # 本文档
```

### 设计模式

1. **注册表模式**: `PushChannelRegistry` 负责注册和管理推送渠道实例
2. **单例模式**: 核心管理类采用单例模式确保全局唯一性
3. **策略模式**: 不同推送渠道实现统一接口，可灵活切换
4. **模板方法模式**: 基类定义通用流程，子类实现具体细节

## 功能特性

### 支持的推送渠道

- **企业微信 (wechat)**: 支持企业微信群机器人推送
- **钉钉 (dingtalk)**: 支持钉钉群机器人推送（含签名验证）
- **Webhook (webhook)**: 支持通用HTTP Webhook推送

### 核心功能

- ✅ 多渠道推送支持
- ✅ 配置验证和测试
- ✅ 消息模板系统
- ✅ 错误处理和重试
- ✅ 调试模式支持
- ✅ CLI演示程序
- ✅ 动态渠道注册
- ✅ 配置示例生成

## 使用方法

### 基本使用

```cpp
#include "push_manager.h"

// 获取推送管理器实例
PushManager& manager = PushManager::getInstance();

// 初始化
if (!manager.initialize()) {
    Serial.println("初始化失败: " + manager.getLastError());
    return;
}

// 创建推送上下文
PushContext context;
context.sender = "13800138000";
context.content = "测试短信内容";
context.timestamp = "241201120000";
context.smsRecordId = 1;

// 执行推送
PushResult result = manager.processSmsForward(context);
if (result == PUSH_SUCCESS) {
    Serial.println("推送成功");
} else {
    Serial.println("推送失败: " + manager.getLastError());
}
```

### 测试推送配置

```cpp
// 测试企业微信配置
String wechatConfig = R"({
    "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
    "message_template": "收到来自 {sender} 的短信：{content}"
})";

bool result = manager.testPushConfig("wechat", wechatConfig);
if (result) {
    Serial.println("配置测试成功");
} else {
    Serial.println("配置测试失败: " + manager.getLastError());
}
```

### 获取可用渠道

```cpp
// 获取所有可用渠道
std::vector<String> channels = manager.getAvailableChannels();
for (const String& channel : channels) {
    Serial.println("可用渠道: " + channel);
}

// 获取所有渠道的配置示例
String examples = manager.getAllChannelExamples();
Serial.println(examples);
```

### CLI演示程序

```cpp
#include "push_cli_demo.h"

void setup() {
    Serial.begin(115200);
    
    // 运行CLI演示程序
    PushCliDemo& demo = PushCliDemo::getInstance();
    demo.run();
}
```

## 配置示例

### 企业微信配置

```json
{
    "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY",
    "message_template": "📱 短信转发通知\n\n发送方: {sender}\n时间: {timestamp}\n内容: {content}\n\n短信ID: {sms_id}"
}
```

### 钉钉配置

```json
{
    "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN",
    "secret": "YOUR_SECRET",
    "message_template": "📱 短信转发通知\n\n发送方: {sender}\n时间: {timestamp}\n内容: {content}\n\n短信ID: {sms_id}"
}
```

### Webhook配置

```json
{
    "webhook_url": "https://your-server.com/webhook",
    "method": "POST",
    "headers": "Content-Type: application/json\nAuthorization: Bearer YOUR_TOKEN",
    "message_template": "{\"sender\": \"{sender}\", \"content\": \"{content}\", \"timestamp\": \"{timestamp}\", \"sms_id\": {sms_id}}"
}
```

## 扩展新渠道

### 1. 创建渠道类

```cpp
// 创建 your_channel.h
#include "push_channel_base.h"

class YourChannel : public PushChannelBase {
public:
    YourChannel();
    virtual ~YourChannel();
    
    String getChannelName() const override;
    String getDescription() const override;
    PushResult push(const String& config, const PushContext& context) override;
    bool testConfig(const String& config) override;
    String getConfigExample() const override;
    String getCliDemo() const override;
    
private:
    bool validateConfig(const std::map<String, String>& configMap);
    String buildMessageBody(const std::map<String, String>& configMap, const PushContext& context);
};
```

### 2. 实现渠道功能

```cpp
// 实现 your_channel.cpp
#include "your_channel.h"

String YourChannel::getChannelName() const {
    return "your_channel";
}

PushResult YourChannel::push(const String& config, const PushContext& context) {
    // 实现推送逻辑
    return PUSH_SUCCESS;
}

// ... 其他方法实现
```

### 3. 注册到工厂

在对应渠道的 `.cpp` 文件中使用注册宏：

```cpp
#include "your_channel.h"

// 在your_channel.cpp文件末尾添加注册宏
REGISTER_PUSH_CHANNEL(YourChannel, "your_channel", (std::vector<String>{"your_channel", "your"}));
```

## API 参考

### PushManager 主要接口

| 方法 | 描述 | 返回值 |
|------|------|--------|
| `getInstance()` | 获取单例实例 | `PushManager&` |
| `initialize()` | 初始化推送管理器 | `bool` |
| `processSmsForward(context)` | 处理短信转发 | `PushResult` |
| `testPushConfig(type, config)` | 测试推送配置 | `bool` |
| `getAvailableChannels()` | 获取可用渠道列表 | `std::vector<String>` |
| `getAllChannelExamples()` | 获取所有渠道配置示例 | `String` |
| `getLastError()` | 获取最后错误信息 | `String` |
| `setDebugMode(enabled)` | 设置调试模式 | `void` |

### PushResult 枚举

```cpp
enum PushResult {
    PUSH_SUCCESS = 0,           // 推送成功
    PUSH_ERROR_CONFIG = 1,      // 配置错误
    PUSH_ERROR_NETWORK = 2,     // 网络错误
    PUSH_ERROR_AUTH = 3,        // 认证错误
    PUSH_ERROR_RATE_LIMIT = 4,  // 频率限制
    PUSH_ERROR_UNKNOWN = 5      // 未知错误
};
```

### PushContext 结构

```cpp
struct PushContext {
    String sender;          // 发送方号码
    String content;         // 短信内容
    String timestamp;       // 时间戳
    int smsRecordId;        // 短信记录ID
};
```

## 调试和故障排除

### 启用调试模式

```cpp
PushManager::getInstance().setDebugMode(true);
PushChannelRegistry::getInstance().setDebugMode(true);
```

### 常见问题

1. **推送失败**: 检查网络连接和配置参数
2. **配置错误**: 验证JSON格式和必需参数
3. **认证失败**: 检查API密钥和签名算法
4. **频率限制**: 适当延迟推送请求

### 错误代码说明

- `PUSH_ERROR_CONFIG`: 配置参数缺失或格式错误
- `PUSH_ERROR_NETWORK`: 网络连接失败或超时
- `PUSH_ERROR_AUTH`: API密钥错误或签名验证失败
- `PUSH_ERROR_RATE_LIMIT`: 超出API调用频率限制
- `PUSH_ERROR_UNKNOWN`: 其他未知错误

## 性能优化

### 内存管理

- 使用智能指针管理渠道实例
- 及时释放HTTP连接资源
- 避免大量字符串拷贝操作

### 网络优化

- 设置合理的连接超时时间
- 实现连接复用机制
- 支持异步推送操作

## 安全考虑

### 配置安全

- 不在代码中硬编码敏感信息
- 使用安全的配置存储方式
- 定期轮换API密钥

### 网络安全

- 使用HTTPS进行数据传输
- 验证服务器证书
- 实现请求签名验证

## 版本历史

- **v1.0.0**: 初始版本，支持企业微信、钉钉、Webhook推送
- **v1.1.0**: 添加CLI演示程序和配置示例生成
- **v1.2.0**: 优化错误处理和调试功能

## 贡献指南

1. 遵循现有代码风格和命名规范
2. 添加完整的文档注释
3. 实现单元测试
4. 更新README文档
5. 提交前进行代码审查

## 许可证

本模块遵循 ESP-SMS-Relay 项目的许可证协议。