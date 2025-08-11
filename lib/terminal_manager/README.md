# Terminal Manager - CLI命令行界面

## 概述

终端管理器（Terminal Manager）是ESP-SMS-Relay项目的核心模块之一，专门负责SMS转发规则的数据库配置管理。它提供了一个功能完整的CLI（命令行界面），支持通过串口进行交互式操作。

## 主要功能

### 核心功能
- **转发规则管理**：增删改查SMS转发规则
- **规则状态控制**：启用/禁用规则，设置优先级
- **规则测试验证**：测试规则匹配，验证配置
- **批量操作**：批量启用/禁用/删除规则
- **数据导入导出**：规则的导入和导出功能
- **统计信息**：规则使用统计和系统状态

### CLI特性
- **交互式命令行**：支持实时命令输入和执行
- **命令补全提示**：提供详细的帮助信息
- **错误处理**：完善的错误提示和处理机制
- **串口支持**：通过UART串口进行操作

## 快速开始

### 1. 初始化系统

```cpp
#include "terminal_manager.h"
#include "database_manager.h"

void setup() {
    Serial.begin(115200);
    
    // 初始化数据库管理器
    DatabaseManager& db = DatabaseManager::getInstance();
    if (!db.initialize()) {
        Serial.println("Database initialization failed!");
        return;
    }
    
    // 初始化终端管理器
    TerminalManager& tm = TerminalManager::getInstance();
    if (!tm.initialize()) {
        Serial.println("Terminal manager initialization failed!");
        return;
    }
    
    // 启动CLI
    tm.startCLI();
}

void loop() {
    TerminalManager& tm = TerminalManager::getInstance();
    tm.handleSerialInput();
}
```

### 2. 基本使用

启动系统后，在串口监视器中输入命令：

```
sms-relay> help
```

## CLI命令参考

### 通用命令

| 命令 | 别名 | 描述 |
|------|------|------|
| `help` | `h` | 显示帮助信息 |
| `status` | `stat` | 显示系统状态 |
| `clear` | `cls` | 清屏 |
| `exit` | `quit`, `q` | 退出CLI |

### 规则管理命令

#### 列出规则
```bash
# 列出所有规则
list
ls

# 只列出启用的规则
list enabled

# 只列出禁用的规则
list disabled
```

#### 添加规则
```bash
add <name> <sender_pattern> <push_type> <push_config> [description] [content_pattern]

# 示例：添加银行短信转发规则
add "Bank Alert" "95588" "wechat" "{\"webhook\":\"https://qyapi.weixin.qq.com/xxx\"}"
```

#### 删除规则
```bash
delete <rule_id>
del <rule_id>
rm <rule_id>

# 示例：删除ID为1的规则
delete 1
```

#### 启用/禁用规则
```bash
# 启用规则
enable <rule_id>
en <rule_id>

# 禁用规则
disable <rule_id>
dis <rule_id>

# 示例
enable 1
disable 2
```

#### 测试规则
```bash
test <rule_id> <sender> <content>

# 示例：测试银行短信匹配
test 1 "95588" "您的账户余额为1000元"
```

### 数据管理命令

#### 导出规则
```bash
export
```

#### 导入规则
```bash
import
```

## 转发规则配置

### 规则结构

```cpp
struct ForwardRule {
    int id;                    // 规则ID（自动生成）
    String name;               // 规则名称
    String description;        // 规则描述
    String senderPattern;      // 发送者匹配模式
    String contentPattern;     // 内容匹配模式（可选）
    String pushType;           // 推送类型
    String pushConfig;         // 推送配置（JSON格式）
    int priority;              // 优先级（0-1000）
    bool enabled;              // 是否启用
    // ... 其他时间和统计字段
};
```

### 支持的推送类型

1. **企业微信（wechat）**
   ```json
   {
     "webhook": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx"
   }
   ```

2. **钉钉（dingtalk）**
   ```json
   {
     "webhook": "https://oapi.dingtalk.com/robot/send?access_token=xxx"
   }
   ```

3. **Webhook（webhook）**
   ```json
   {
     "url": "https://api.example.com/webhook",
     "method": "POST",
     "headers": {
       "Authorization": "Bearer token"
     }
   }
   ```

4. **邮件（email）**
   ```json
   {
     "smtp_server": "smtp.gmail.com",
     "smtp_port": 587,
     "username": "user@gmail.com",
     "password": "password",
     "to": "recipient@example.com"
   }
   ```

### 模式匹配规则

- **精确匹配**：`95588` 只匹配 "95588"
- **通配符匹配**：
  - `*` 匹配任意字符串
  - `?` 匹配单个字符
  - `*银行*` 匹配包含"银行"的任意字符串
  - `95???` 匹配以"95"开头的5位数字

## 使用示例

### 示例1：银行短信转发

```bash
# 添加银行短信转发规则
add "工商银行通知" "95588" "wechat" "{\"webhook\":\"https://qyapi.weixin.qq.com/xxx\"}"

# 测试规则
test 1 "95588" "您的账户余额为1000元"

# 查看规则列表
list
```

### 示例2：验证码转发

```bash
# 添加验证码转发规则
add "验证码转发" "*" "dingtalk" "{\"webhook\":\"https://oapi.dingtalk.com/xxx\"}" "转发所有验证码" "*验证码*"

# 测试规则
test 2 "12345" "您的验证码是123456，请在5分钟内使用"
```

### 示例3：批量管理

```bash
# 查看系统状态
status

# 禁用所有规则
# 注意：这需要通过API调用，CLI中暂未实现

# 导出所有规则
export
```

## API接口

### 核心API

```cpp
class TerminalManager {
public:
    // 单例获取
    static TerminalManager& getInstance();
    
    // 初始化和清理
    bool initialize();
    void cleanup();
    
    // 规则管理
    int addForwardRule(const ForwardRule& rule);
    bool updateForwardRule(const ForwardRule& rule);
    bool deleteForwardRule(int ruleId);
    ForwardRule getForwardRule(int ruleId);
    std::vector<ForwardRule> getForwardRules(const RuleQueryCondition& condition = {});
    
    // 规则状态管理
    bool enableRule(int ruleId);
    bool disableRule(int ruleId);
    bool setRulePriority(int ruleId, int priority);
    
    // 规则测试
    bool testRule(int ruleId, const String& sender, const String& content);
    bool validateRuleConfig(const ForwardRule& rule);
    
    // 统计信息
    int getRuleCount();
    int getEnabledRuleCount();
    std::vector<ForwardRule> getMostUsedRules(int limit = 10);
    
    // 批量操作
    bool enableAllRules();
    bool disableAllRules();
    bool deleteAllRules();
    bool importRules(const std::vector<ForwardRule>& rules);
    std::vector<ForwardRule> exportRules();
    
    // CLI接口
    void startCLI();
    void stopCLI();
    bool isCLIRunning();
    void handleSerialInput();
    bool processCommand(const String& command);
    
    // 错误处理
    String getLastError();
};
```

## 配置选项

```cpp
struct TerminalConfig {
    bool enabled;              // 是否启用终端管理器
    int maxRules;              // 最大规则数量
    bool enableCache;          // 是否启用缓存
    int cacheSize;             // 缓存大小
    bool enableValidation;     // 是否启用规则验证
    bool enableLogging;        // 是否启用日志记录
};
```

## 错误处理

### 常见错误类型

```cpp
enum TerminalError {
    TERMINAL_SUCCESS = 0,
    TERMINAL_NOT_INITIALIZED,
    TERMINAL_DATABASE_ERROR,
    TERMINAL_RULE_NOT_FOUND,
    TERMINAL_VALIDATION_FAILED,
    TERMINAL_CACHE_ERROR
};
```

### 错误处理示例

```cpp
TerminalManager& tm = TerminalManager::getInstance();

int ruleId = tm.addForwardRule(rule);
if (ruleId <= 0) {
    Serial.println("Failed to add rule: " + tm.getLastError());
    // 处理错误
}
```

## 性能优化

### 缓存机制
- 启用缓存可以提高频繁访问规则的性能
- 缓存大小可配置，默认50条规则
- 自动缓存最常用的规则

### 数据库优化
- 使用索引提高查询性能
- 支持分页查询大量规则
- 批量操作减少数据库I/O

## 注意事项

1. **内存管理**：及时释放不需要的资源
2. **并发安全**：避免多线程同时操作
3. **数据备份**：重要规则建议定期导出备份
4. **规则验证**：添加规则前进行格式验证
5. **错误处理**：检查所有API调用的返回值

## 故障排除

### 常见问题

1. **CLI无响应**
   - 检查串口连接和波特率设置
   - 确认终端管理器已正确初始化

2. **规则添加失败**
   - 检查规则格式是否正确
   - 验证推送配置JSON格式
   - 确认数据库连接正常

3. **规则匹配异常**
   - 检查模式匹配语法
   - 测试规则匹配逻辑
   - 验证规则优先级设置

### 调试方法

```cpp
// 启用调试日志
LogManager::getInstance().setDebugMode(true);

// 检查系统状态
TerminalManager& tm = TerminalManager::getInstance();
Serial.println("Rules count: " + String(tm.getRuleCount()));
Serial.println("Last error: " + tm.getLastError());
```

## 扩展开发

### 添加新命令

1. 在 `terminal_manager.h` 中声明新的命令处理函数
2. 在 `terminal_manager.cpp` 中实现命令逻辑
3. 在 `processCommand` 函数中添加命令路由
4. 更新帮助信息

### 添加新的推送类型

1. 在规则验证中添加新类型支持
2. 实现对应的推送逻辑
3. 更新配置示例和文档

## 版本历史

- **v1.0.0** - 初始版本，基本CLI功能
- **v1.1.0** - 添加批量操作和导入导出
- **v1.2.0** - 优化缓存机制和性能

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。

## 联系方式

如有问题或建议，请通过项目的 Issue 页面联系我们。