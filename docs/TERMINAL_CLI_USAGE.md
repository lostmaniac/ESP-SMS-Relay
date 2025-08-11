# 终端管理器CLI使用指南

## 概述

本文档介绍ESP-SMS-Relay项目中终端管理器CLI（命令行界面）的使用方法。该CLI系统专门用于管理SMS转发规则的数据库配置，提供了完整的规则管理功能。

## 系统架构

### 核心组件

- **TerminalManager**: 终端管理器主类，提供CLI接口和规则管理功能
- **ForwardRuleManager**: 转发规则管理器，封装数据库操作
- **ForwardRule**: 转发规则数据结构
- **CLI命令系统**: 交互式命令行界面

### 文件结构

```
lib/terminal_manager/
├── terminal_manager.h          # 终端管理器头文件
├── terminal_manager.cpp        # 终端管理器实现
├── forward_rule_manager.h      # 规则管理器头文件
├── forward_rule_manager.cpp    # 规则管理器实现
└── README.md                   # 详细文档

examples/
└── terminal_cli_example.cpp    # 使用示例

test/
└── test_terminal_manager.cpp   # 单元测试

docs/
├── TERMINAL_MANAGER_FEATURE_PLAN.md  # 功能规划文档
└── TERMINAL_CLI_USAGE.md             # 本使用指南
```

## 快速开始

### 1. 初始化系统

```cpp
#include "terminal_manager.h"
#include "database_manager.h"

void setup() {
    Serial.begin(115200);
    
    // 初始化数据库
    DatabaseManager& db = DatabaseManager::getInstance();
    db.initialize();
    
    // 初始化终端管理器
    TerminalManager& tm = TerminalManager::getInstance();
    tm.initialize();
    
    // 启动CLI
    tm.startCLI();
    
    Serial.println("CLI started. Type 'help' for commands.");
}

void loop() {
    TerminalManager& tm = TerminalManager::getInstance();
    tm.handleSerialInput();
    delay(10);
}
```

### 2. 基本使用流程

1. 上传代码到ESP32-S3
2. 打开串口监视器（115200波特率）
3. 等待系统初始化完成
4. 输入命令进行操作

## CLI命令参考

### 通用命令

| 命令 | 描述 | 示例 |
|------|------|------|
| `help` | 显示帮助信息 | `help` |
| `status` | 显示系统状态 | `status` |
| `clear` | 清屏 | `clear` |
| `exit` | 退出CLI | `exit` |

### 规则管理命令

#### 查看规则
```bash
# 列出所有规则
list

# 列出启用的规则
list enabled

# 列出禁用的规则
list disabled

# 显示规则详情
show <rule_id>
```

#### 添加规则
```bash
# 交互式添加规则
add

# 命令行添加规则
add "银行通知" "95588" "*余额*" webhook "{\"url\":\"https://hook.example.com\"}"
```

#### 修改规则
```bash
# 启用规则
enable <rule_id>

# 禁用规则
disable <rule_id>

# 设置优先级
priority <rule_id> <priority>

# 删除规则
delete <rule_id>
```

#### 测试规则
```bash
# 测试规则匹配
test <rule_id> "发送者" "短信内容"

# 示例
test 1 "95588" "您的账户余额为1000元"
```

### 批量操作命令

```bash
# 启用所有规则
enable_all

# 禁用所有规则
disable_all

# 删除所有规则（需要确认）
delete_all
```

### 数据管理命令

```bash
# 导出规则到JSON
export

# 从JSON导入规则
import <json_data>

# 备份规则
backup

# 恢复规则
restore
```

## 转发规则配置

### 规则结构

```cpp
struct ForwardRule {
    int id;                    // 规则ID（自动生成）
    String name;               // 规则名称
    String description;        // 规则描述
    String senderPattern;      // 发送者匹配模式
    String contentPattern;     // 内容匹配模式
    String pushType;           // 推送类型
    String pushConfig;         // 推送配置（JSON格式）
    int priority;              // 优先级（1-1000）
    bool enabled;              // 是否启用
    String createdAt;          // 创建时间
    String updatedAt;          // 更新时间
};
```

### 支持的推送类型

1. **webhook** - HTTP Webhook推送
   ```json
   {
     "url": "https://your-webhook-url.com",
     "method": "POST",
     "headers": {
       "Content-Type": "application/json",
       "Authorization": "Bearer your-token"
     }
   }
   ```

2. **wechat** - 企业微信推送
   ```json
   {
     "webhook_url": "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=your-key",
     "msgtype": "text"
   }
   ```

3. **dingtalk** - 钉钉推送
   ```json
   {
     "webhook_url": "https://oapi.dingtalk.com/robot/send?access_token=your-token",
     "msgtype": "text",
     "secret": "your-secret"
   }
   ```

### 模式匹配规则

- `*` - 匹配任意字符串
- `?` - 匹配单个字符
- 精确匹配 - 不使用通配符
- 正则表达式 - 使用 `/pattern/` 格式

#### 示例

```bash
# 匹配所有银行短信
senderPattern: "95*"
contentPattern: "*余额*"

# 匹配验证码短信
senderPattern: "*"
contentPattern: "*验证码*"

# 精确匹配特定号码
senderPattern: "10086"
contentPattern: "*"
```

## 使用示例

### 示例1：添加银行余额通知规则

```bash
> add
请输入规则名称: 银行余额通知
请输入规则描述: 转发银行余额变动通知
请输入发送者模式: 95588
请输入内容模式: *余额*
请输入推送类型 (webhook/wechat/dingtalk): webhook
请输入推送配置: {"url":"https://your-webhook.com/bank"}
请输入优先级 (1-1000): 100
是否启用 (y/n): y

规则添加成功，ID: 1
```

### 示例2：测试规则匹配

```bash
> test 1 "95588" "您的账户余额为1000.00元"
✓ 规则匹配成功
发送者匹配: ✓
内容匹配: ✓
推送类型: webhook
```

### 示例3：批量管理规则

```bash
> list
ID | 名称           | 发送者    | 状态   | 优先级
1  | 银行余额通知   | 95588     | 启用   | 100
2  | 验证码转发     | *         | 禁用   | 90
3  | 移动通知       | 10086     | 启用   | 80

> enable 2
规则 2 已启用

> priority 3 120
规则 3 优先级已设置为 120
```

## API接口

### 核心API

```cpp
class TerminalManager {
public:
    // 单例获取
    static TerminalManager& getInstance();
    
    // 初始化
    bool initialize();
    
    // CLI控制
    void startCLI();
    void stopCLI();
    bool isCLIRunning();
    
    // 命令处理
    bool processCommand(const String& command);
    void handleSerialInput();
    
    // 规则管理
    int addForwardRule(const ForwardRule& rule);
    bool updateForwardRule(const ForwardRule& rule);
    bool deleteForwardRule(int ruleId);
    ForwardRule getForwardRule(int ruleId);
    std::vector<ForwardRule> getForwardRules(const RuleQueryCondition& condition = {});
    
    // 规则状态
    bool enableRule(int ruleId);
    bool disableRule(int ruleId);
    bool setRulePriority(int ruleId, int priority);
    
    // 规则测试
    bool testRule(int ruleId, const String& sender, const String& content);
    bool validateRuleConfig(const ForwardRule& rule);
    
    // 统计信息
    int getRuleCount();
    int getEnabledRuleCount();
    
    // 批量操作
    bool enableAllRules();
    bool disableAllRules();
    bool deleteAllRules();
    
    // 数据导入导出
    std::vector<ForwardRule> exportRules();
    bool importRules(const std::vector<ForwardRule>& rules);
    
    // 错误处理
    String getLastError();
};
```

## 配置选项

### 编译时配置

在 `include/config.h` 中可以配置以下选项：

```cpp
// CLI配置
#define CLI_PROMPT "ESP-SMS> "           // CLI提示符
#define CLI_MAX_COMMAND_LENGTH 256       // 最大命令长度
#define CLI_HISTORY_SIZE 10              // 命令历史大小

// 规则配置
#define MAX_RULES_COUNT 100              // 最大规则数量
#define MAX_RULE_NAME_LENGTH 64          // 规则名称最大长度
#define MAX_PATTERN_LENGTH 128           // 模式最大长度

// 数据库配置
#define DB_RULES_TABLE "forward_rules"   // 规则表名
#define DB_CACHE_SIZE 50                 // 缓存大小
```

### 运行时配置

```cpp
// 设置调试模式
tm.setDebugMode(true);

// 设置CLI提示符
tm.setPrompt("MyESP> ");

// 设置命令超时
tm.setCommandTimeout(5000);
```

## 错误处理

### 常见错误类型

```cpp
enum TerminalError {
    TERMINAL_SUCCESS = 0,
    TERMINAL_NOT_INITIALIZED,
    TERMINAL_DATABASE_ERROR,
    TERMINAL_INVALID_RULE,
    TERMINAL_RULE_NOT_FOUND,
    TERMINAL_VALIDATION_ERROR,
    TERMINAL_CLI_ERROR
};
```

### 错误处理示例

```cpp
TerminalManager& tm = TerminalManager::getInstance();

int ruleId = tm.addForwardRule(rule);
if (ruleId == -1) {
    String error = tm.getLastError();
    Serial.println("添加规则失败: " + error);
    return;
}

Serial.println("规则添加成功，ID: " + String(ruleId));
```

## 性能优化

### 内存管理

- 使用对象池复用ForwardRule对象
- 及时释放不需要的字符串
- 限制同时加载的规则数量

### 数据库优化

- 使用索引提高查询性能
- 批量操作减少I/O
- 定期清理过期数据

### CLI优化

- 命令缓存减少重复解析
- 异步处理长时间操作
- 分页显示大量数据

## 注意事项

### 内存限制

- ESP32-S3有限的RAM，避免加载过多规则
- 使用分页查询处理大量数据
- 及时释放不需要的对象

### 串口通信

- 确保串口波特率设置正确（115200）
- 避免发送过长的命令
- 处理串口缓冲区溢出

### 数据持久化

- 定期备份重要规则
- 处理数据库文件损坏情况
- 实现数据恢复机制

## 故障排除

### 常见问题

1. **CLI无响应**
   - 检查串口连接
   - 确认波特率设置
   - 重启设备

2. **规则添加失败**
   - 检查规则格式
   - 验证JSON配置
   - 查看错误信息

3. **数据库错误**
   - 检查存储空间
   - 重新初始化数据库
   - 恢复备份数据

4. **内存不足**
   - 减少同时加载的规则
   - 清理不需要的数据
   - 重启设备

### 调试方法

```cpp
// 启用调试模式
tm.setDebugMode(true);

// 查看系统状态
tm.processCommand("status");

// 检查错误信息
String error = tm.getLastError();
Serial.println("Last error: " + error);
```

## 扩展开发

### 添加新命令

1. 在 `terminal_manager.h` 中声明新的处理函数
2. 在 `terminal_manager.cpp` 中实现处理逻辑
3. 在 `processCommand` 中添加命令映射
4. 更新帮助信息

### 添加新的推送类型

1. 在 `ForwardRuleManager` 中添加验证逻辑
2. 在推送管理器中实现推送功能
3. 更新配置文档

### 自定义CLI界面

1. 修改提示符和输出格式
2. 添加颜色支持（如果终端支持）
3. 实现命令自动补全
4. 添加命令历史功能

## 版本历史

- **v1.0.0** - 初始版本，基本CLI功能
- **v1.1.0** - 添加规则验证和测试功能
- **v1.2.0** - 支持批量操作和数据导入导出
- **v1.3.0** - 性能优化和错误处理改进

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进本项目。

## 联系方式

如有问题或建议，请通过以下方式联系：

- 项目仓库：[ESP-SMS-Relay](https://github.com/your-repo/ESP-SMS-Relay)
- 邮箱：your-email@example.com

---

*本文档最后更新：2024年*