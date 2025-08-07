# 默认转发功能说明

## 概述

在 `forward_rules` 表中新增了 `is_default_forward` 字段，用于标识是否为默认转发规则。当此字段为 `true` 时，该规则将忽略关键词匹配，直接转发所有符合来源号码条件的短信。

## 字段说明

### is_default_forward
- **类型**: BOOLEAN (INTEGER)
- **默认值**: 0 (false)
- **说明**: 是否为默认转发规则
  - `true` (1): 默认转发，忽略关键词匹配
  - `false` (0): 普通规则，需要匹配关键词
- **代码逻辑**: `rule.isDefaultForward = row.at("is_default_forward").toInt() == 1;`
  - 数据库值为0时，布尔值为false（默认行为）
  - 数据库值为1时，布尔值为true（默认转发）

## 工作原理

### 普通转发规则
```
来源号码匹配 AND 关键词匹配 → 执行转发
```

### 默认转发规则
```
来源号码匹配 AND is_default_forward=true → 直接转发（忽略关键词）
```

## 使用示例

### 创建默认转发规则

```cpp
// 创建一个默认转发规则，转发所有来自银行的短信
ForwardRule defaultRule;
defaultRule.ruleName = "银行短信默认转发";
defaultRule.sourceNumber = "+86955*";  // 银行短信号码
defaultRule.keywords = "";              // 关键词可以为空
defaultRule.pushType = "webhook";
defaultRule.pushConfig = "{\"url\":\"https://api.example.com/webhook\"}";
defaultRule.enabled = true;
defaultRule.isDefaultForward = true;    // 设置为默认转发

int ruleId = db.addForwardRule(defaultRule);
```

### 创建普通转发规则

```cpp
// 创建一个普通转发规则，只转发包含特定关键词的短信
ForwardRule normalRule;
normalRule.ruleName = "重要通知转发";
normalRule.sourceNumber = "*";          // 匹配所有号码
normalRule.keywords = "紧急,重要,通知";   // 必须包含这些关键词
normalRule.pushType = "wechat";
normalRule.pushConfig = "{\"webhook_url\":\"https://qyapi.weixin.qq.com/webhook\"}";
normalRule.enabled = true;
normalRule.isDefaultForward = false;    // 普通规则，需要匹配关键词

int ruleId = db.addForwardRule(normalRule);
```

## 应用场景

### 1. 银行短信全量转发
- 银行短信通常都很重要，可以设置默认转发
- 避免因关键词设置不全而遗漏重要信息

### 2. 特定联系人消息转发
- 对于重要联系人的所有短信都进行转发
- 不需要进行关键词过滤

### 3. 系统通知转发
- 系统服务号的所有通知都进行转发
- 确保不遗漏任何系统消息

## 注意事项

1. **优先级**: 默认转发规则的优先级高于关键词匹配
2. **性能**: 默认转发规则处理速度更快，因为跳过了关键词匹配步骤
3. **流量**: 使用默认转发可能会增加推送流量，请根据实际需求设置
4. **来源号码**: 即使是默认转发，仍然需要匹配来源号码条件

## 数据库结构变更

### 表结构更新

```sql
ALTER TABLE forward_rules ADD COLUMN is_default_forward INTEGER DEFAULT 0;
```

### 完整表结构

```sql
CREATE TABLE forward_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rule_name TEXT NOT NULL,
    source_number TEXT DEFAULT '*',
    keywords TEXT,
    push_type TEXT NOT NULL DEFAULT 'webhook',
    push_config TEXT NOT NULL DEFAULT '{}',
    enabled INTEGER DEFAULT 1,
    is_default_forward INTEGER DEFAULT 0,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);
```

## API 接口

所有现有的转发规则管理 API 都已支持新的 `isDefaultForward` 字段：

- `addForwardRule(const ForwardRule& rule)`
- `updateForwardRule(const ForwardRule& rule)`
- `getAllForwardRules()`
- `getForwardRuleById(int ruleId)`

## 测试验证

项目中包含了完整的测试用例，验证默认转发功能的正确性：

- 创建默认转发规则
- 创建普通转发规则
- 验证字段读写正确性
- 验证更新操作正确性

运行测试：
```bash
platformio test
```