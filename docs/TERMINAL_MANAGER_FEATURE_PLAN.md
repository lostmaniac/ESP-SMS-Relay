# ESP-SMS-Relay 终端管理器功能规划（简化版）

## 概述

终端管理器（Terminal Manager）是ESP-SMS-Relay项目中的一个轻量级模块，专注于提供SMS转发规则的数据库配置管理功能。该模块通过简洁的接口封装数据库操作，为系统提供转发规则的增删改查功能。

## 设计目标

### 1. 简化设计
- 专注于SMS转发规则管理
- 提供简洁的数据库操作接口
- 避免复杂的命令行系统
- 轻量级实现，减少资源占用

### 2. 核心功能
- 转发规则的增删改查
- 规则状态管理（启用/禁用）
- 规则优先级管理
- 规则测试和验证

### 3. 接口设计
- 统一的API接口
- 错误处理机制
- 数据验证功能
- 事务支持

## 模块架构设计

### 核心模块结构

```
lib/terminal_manager/
├── terminal_manager.h          # 终端管理器主接口
├── terminal_manager.cpp        # 终端管理器实现
├── forward_rule_manager.h      # 转发规则管理器
├── forward_rule_manager.cpp    # 转发规则管理实现
└── README.md                   # 模块文档
```

## 数据结构设计

### 1. 转发规则结构

```cpp
/**
 * @struct ForwardRule
 * @brief SMS转发规则结构
 */
struct ForwardRule {
    int id;                        ///< 规则ID（自增主键）
    String name;                   ///< 规则名称
    String description;            ///< 规则描述
    String senderPattern;          ///< 发送者匹配模式（正则表达式）
    String contentPattern;         ///< 内容匹配模式（正则表达式）
    String pushType;               ///< 推送类型（wechat/dingtalk/webhook）
    String pushConfig;             ///< 推送配置（JSON格式）
    int priority;                  ///< 优先级（数字越小优先级越高）
    bool enabled;                  ///< 是否启用
    uint32_t createTime;          ///< 创建时间（Unix时间戳）
    uint32_t updateTime;          ///< 更新时间（Unix时间戳）
    uint32_t lastUsedTime;        ///< 最后使用时间
    int usageCount;               ///< 使用次数
};
```

### 2. 规则查询条件

```cpp
/**
 * @struct RuleQueryCondition
 * @brief 规则查询条件
 */
struct RuleQueryCondition {
    bool filterByEnabled;          ///< 是否按启用状态过滤
    bool enabledValue;             ///< 启用状态值
    bool filterByPushType;         ///< 是否按推送类型过滤
    String pushType;               ///< 推送类型
    bool orderByPriority;          ///< 是否按优先级排序
    bool orderByCreateTime;        ///< 是否按创建时间排序
    int limit;                     ///< 限制返回数量
    int offset;                    ///< 偏移量
};
```

## 核心功能模块

### 1. 终端管理器（TerminalManager）

#### 主要职责
- 提供统一的转发规则管理接口
- 协调数据库操作
- 处理错误和异常
- 管理模块生命周期

#### 核心接口
```cpp
class TerminalManager {
public:
    static TerminalManager& getInstance();
    
    // 初始化和清理
    bool initialize();
    void cleanup();
    
    // 转发规则管理
    int addForwardRule(const ForwardRule& rule);
    bool updateForwardRule(const ForwardRule& rule);
    bool deleteForwardRule(int ruleId);
    ForwardRule getForwardRule(int ruleId);
    std::vector<ForwardRule> getForwardRules(const RuleQueryCondition& condition = {});
    
    // 规则状态管理
    bool enableRule(int ruleId);
    bool disableRule(int ruleId);
    bool setRulePriority(int ruleId, int priority);
    
    // 规则测试和验证
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
    
    // 错误处理
    String getLastError();
    
private:
    TerminalManager();
    ~TerminalManager();
    
    // 禁止拷贝
    TerminalManager(const TerminalManager&) = delete;
    TerminalManager& operator=(const TerminalManager&) = delete;
    
    // 私有成员
    bool initialized;
    String lastError;
    ForwardRuleManager* ruleManager;
};
```

### 2. 转发规则管理器（ForwardRuleManager）

#### 主要职责
- 封装数据库操作
- 提供规则的CRUD操作
- 处理规则验证
- 管理规则缓存

#### 核心功能
- 数据库表初始化
- 规则数据验证
- SQL语句构建和执行
- 结果集处理
- 事务管理

## 数据库设计

### 1. 转发规则表（forward_rules）

```sql
CREATE TABLE IF NOT EXISTS forward_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT,
    sender_pattern TEXT NOT NULL,
    content_pattern TEXT,
    push_type TEXT NOT NULL,
    push_config TEXT NOT NULL,
    priority INTEGER DEFAULT 100,
    enabled INTEGER DEFAULT 1,
    create_time INTEGER NOT NULL,
    update_time INTEGER NOT NULL,
    last_used_time INTEGER DEFAULT 0,
    usage_count INTEGER DEFAULT 0
);
```

### 2. 索引设计

```sql
-- 优先级和启用状态索引
CREATE INDEX IF NOT EXISTS idx_rules_priority_enabled 
ON forward_rules(priority ASC, enabled DESC);

-- 推送类型索引
CREATE INDEX IF NOT EXISTS idx_rules_push_type 
ON forward_rules(push_type);

-- 创建时间索引
CREATE INDEX IF NOT EXISTS idx_rules_create_time 
ON forward_rules(create_time DESC);

-- 使用统计索引
CREATE INDEX IF NOT EXISTS idx_rules_usage 
ON forward_rules(usage_count DESC, last_used_time DESC);
```

## API接口详细设计

### 1. 规则管理接口

#### 添加规则
```cpp
/**
 * @brief 添加转发规则
 * @param rule 规则对象
 * @return int 规则ID，失败返回-1
 */
int addForwardRule(const ForwardRule& rule);
```

#### 更新规则
```cpp
/**
 * @brief 更新转发规则
 * @param rule 规则对象（必须包含有效的ID）
 * @return bool 成功返回true，失败返回false
 */
bool updateForwardRule(const ForwardRule& rule);
```

#### 删除规则
```cpp
/**
 * @brief 删除转发规则
 * @param ruleId 规则ID
 * @return bool 成功返回true，失败返回false
 */
bool deleteForwardRule(int ruleId);
```

#### 查询规则
```cpp
/**
 * @brief 获取单个转发规则
 * @param ruleId 规则ID
 * @return ForwardRule 规则对象，失败时ID为-1
 */
ForwardRule getForwardRule(int ruleId);

/**
 * @brief 获取转发规则列表
 * @param condition 查询条件
 * @return std::vector<ForwardRule> 规则列表
 */
std::vector<ForwardRule> getForwardRules(const RuleQueryCondition& condition = {});
```

### 2. 规则状态管理接口

#### 启用/禁用规则
```cpp
/**
 * @brief 启用规则
 * @param ruleId 规则ID
 * @return bool 成功返回true，失败返回false
 */
bool enableRule(int ruleId);

/**
 * @brief 禁用规则
 * @param ruleId 规则ID
 * @return bool 成功返回true，失败返回false
 */
bool disableRule(int ruleId);
```

#### 设置优先级
```cpp
/**
 * @brief 设置规则优先级
 * @param ruleId 规则ID
 * @param priority 优先级（数字越小优先级越高）
 * @return bool 成功返回true，失败返回false
 */
bool setRulePriority(int ruleId, int priority);
```

### 3. 规则测试和验证接口

#### 测试规则
```cpp
/**
 * @brief 测试规则是否匹配
 * @param ruleId 规则ID
 * @param sender 发送者号码
 * @param content 短信内容
 * @return bool 匹配返回true，不匹配返回false
 */
bool testRule(int ruleId, const String& sender, const String& content);
```

#### 验证规则配置
```cpp
/**
 * @brief 验证规则配置是否有效
 * @param rule 规则对象
 * @return bool 有效返回true，无效返回false
 */
bool validateRuleConfig(const ForwardRule& rule);
```

## 错误处理机制

### 1. 错误类型定义

```cpp
/**
 * @enum TerminalError
 * @brief 终端管理器错误类型
 */
enum TerminalError {
    TERMINAL_SUCCESS = 0,          ///< 成功
    TERMINAL_ERROR_NOT_INITIALIZED, ///< 未初始化
    TERMINAL_ERROR_DATABASE,       ///< 数据库错误
    TERMINAL_ERROR_INVALID_PARAM,  ///< 无效参数
    TERMINAL_ERROR_RULE_NOT_FOUND, ///< 规则不存在
    TERMINAL_ERROR_RULE_EXISTS,    ///< 规则已存在
    TERMINAL_ERROR_VALIDATION,     ///< 验证失败
    TERMINAL_ERROR_UNKNOWN         ///< 未知错误
};
```

### 2. 错误处理策略

- 所有公共接口都有明确的返回值
- 提供详细的错误信息
- 使用 `getLastError()` 获取错误详情
- 关键操作失败时记录日志

## 配置管理

### 1. 模块配置

```cpp
/**
 * @struct TerminalConfig
 * @brief 终端管理器配置
 */
struct TerminalConfig {
    bool enabled;                  ///< 模块启用
    int maxRules;                 ///< 最大规则数量
    bool enableCache;             ///< 启用缓存
    int cacheSize;                ///< 缓存大小
    bool enableValidation;        ///< 启用验证
    bool enableLogging;           ///< 启用日志
};
```

### 2. 默认配置

```cpp
// 默认配置值
static const TerminalConfig DEFAULT_CONFIG = {
    .enabled = true,
    .maxRules = 100,
    .enableCache = true,
    .cacheSize = 50,
    .enableValidation = true,
    .enableLogging = true
};
```

## 性能优化

### 1. 缓存机制

- 缓存常用规则
- LRU缓存策略
- 缓存失效机制
- 内存使用控制

### 2. 数据库优化

- 合理的索引设计
- 批量操作支持
- 事务优化
- 连接池管理

### 3. 内存管理

- 及时释放资源
- 避免内存泄漏
- 合理的对象生命周期
- 内存使用监控

## 使用示例

### 1. 基本使用

```cpp
// 获取管理器实例
TerminalManager& tm = TerminalManager::getInstance();

// 初始化
if (!tm.initialize()) {
    Serial.println("终端管理器初始化失败: " + tm.getLastError());
    return;
}

// 创建转发规则
ForwardRule rule;
rule.name = "测试规则";
rule.description = "测试用的转发规则";
rule.senderPattern = "10086";
rule.contentPattern = ".*余额.*";
rule.pushType = "wechat";
rule.pushConfig = "{\"webhook\":\"https://example.com/webhook\"}";
rule.priority = 10;
rule.enabled = true;

// 添加规则
int ruleId = tm.addForwardRule(rule);
if (ruleId > 0) {
    Serial.println("规则添加成功，ID: " + String(ruleId));
} else {
    Serial.println("规则添加失败: " + tm.getLastError());
}
```

### 2. 规则查询

```cpp
// 查询所有启用的规则
RuleQueryCondition condition;
condition.filterByEnabled = true;
condition.enabledValue = true;
condition.orderByPriority = true;

std::vector<ForwardRule> rules = tm.getForwardRules(condition);
Serial.println("找到 " + String(rules.size()) + " 条启用的规则");

for (const auto& rule : rules) {
    Serial.println("规则: " + rule.name + ", 优先级: " + String(rule.priority));
}
```

### 3. 规则测试

```cpp
// 测试规则是否匹配
String sender = "10086";
String content = "您的账户余额为100元";

if (tm.testRule(ruleId, sender, content)) {
    Serial.println("规则匹配成功");
} else {
    Serial.println("规则不匹配");
}
```

## 集成方案

### 1. 与现有模块集成

- 通过 `DatabaseManager` 进行数据库操作
- 与 `PushManager` 配合进行消息推送
- 与 `SmsHandler` 集成进行规则匹配
- 与 `ConfigManager` 集成进行配置管理

### 2. 模块依赖

```cpp
// 依赖的模块
#include "database_manager.h"
#include "config_manager.h"
#include "log_manager.h"
```

### 3. 初始化顺序

1. DatabaseManager 初始化
2. ConfigManager 初始化
3. TerminalManager 初始化
4. 其他业务模块初始化

## 测试策略

### 1. 单元测试

- 规则CRUD操作测试
- 数据验证测试
- 错误处理测试
- 性能测试

### 2. 集成测试

- 与数据库模块集成测试
- 与推送模块集成测试
- 端到端功能测试

### 3. 压力测试

- 大量规则处理测试
- 并发操作测试
- 内存使用测试

## 部署和维护

### 1. 部署配置

- 数据库表自动创建
- 默认配置应用
- 数据迁移支持

### 2. 监控和维护

- 规则使用统计
- 性能指标监控
- 错误日志记录
- 定期数据清理

## 总结

简化版的终端管理器专注于SMS转发规则的数据库管理，提供了完整的规则CRUD操作、状态管理、测试验证等功能。通过封装数据库操作，为系统提供了简洁、高效的转发规则管理接口。

该设计遵循项目的模块化架构原则，采用单例模式和错误处理机制，确保与现有系统的良好集成。通过合理的数据库设计和性能优化，能够满足SMS转发系统的规则管理需求。