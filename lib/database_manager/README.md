# 数据库管理器模块 (DatabaseManager)

## 概述

数据库管理器模块负责ESP32 SMS中继系统的SQLite数据库管理，提供统一的数据库操作接口。该模块基于Sqlite3Esp32库实现，支持LittleFS文件系统。

## 功能特性

### 核心功能
- ✅ SQLite数据库的初始化和管理
- ✅ 数据库表结构的自动创建
- ✅ 数据库文件的管理和状态监控
- ✅ 统一的数据库操作接口
- ✅ 错误处理和调试支持

### 数据管理
- ✅ AP配置管理（WiFi热点设置）
- ✅ 短信转发规则管理
- ✅ 短信记录存储和查询
- ✅ 数据清理和维护

### 安全特性
- ✅ 单例模式确保数据库连接唯一性
- ✅ 事务安全的数据操作
- ✅ 内存管理和资源释放
- ✅ 错误恢复机制

## 数据库结构

### 1. AP配置表 (ap_config)
```sql
CREATE TABLE ap_config (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ssid TEXT NOT NULL,                -- WiFi热点名称
    password TEXT NOT NULL,            -- WiFi热点密码
    enabled INTEGER DEFAULT 1,         -- 是否启用AP模式
    channel INTEGER DEFAULT 1,         -- WiFi信道
    max_connections INTEGER DEFAULT 4, -- 最大连接数
    created_at TEXT NOT NULL,          -- 创建时间
    updated_at TEXT NOT NULL           -- 更新时间
);
```

### 2. 转发规则表 (forward_rules)
```sql
CREATE TABLE forward_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    rule_name TEXT NOT NULL,                    -- 规则名称
    source_number TEXT DEFAULT '*',             -- 发送号码，可以为空，支持通配符
    keywords TEXT,                              -- 关键词过滤
    push_type TEXT NOT NULL DEFAULT 'webhook', -- 推送类型：企业微信群机器人，钉钉群机器人，webhook等
    push_config TEXT NOT NULL DEFAULT '{}',    -- 推送配置为json格式，支持配置模板和变量
    enabled INTEGER DEFAULT 1,                 -- 是否使用
    created_at TEXT DEFAULT CURRENT_TIMESTAMP, -- 创建时间
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP  -- 修改时间
);
```

### 3. 短信记录表 (sms_records)
```sql
CREATE TABLE sms_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    from_number TEXT NOT NULL,     -- 发送方号码，支持索引
    content TEXT NOT NULL,         -- 短信内容，支持索引
    received_at INTEGER NOT NULL   -- 接收时间，time保存，支持索引，方便以后按照时间过滤短信
);
```

## 使用方法

### 1. 基本初始化

```cpp
#include "database_manager.h"
#include "filesystem_manager.h"

void setup() {
    Serial.begin(115200);
    
    // 1. 初始化文件系统
    FilesystemManager& fsManager = FilesystemManager::getInstance();
    if (!fsManager.initialize(true)) {
        Serial.println("文件系统初始化失败");
        return;
    }
    
    // 2. 初始化数据库
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    dbManager.setDebugMode(true);
    
    if (!dbManager.initialize()) {
        Serial.println("数据库初始化失败: " + dbManager.getLastError());
        return;
    }
    
    Serial.println("数据库初始化成功");
}
```

### 2. AP配置管理

```cpp
// 获取AP配置
APConfig config = dbManager.getAPConfig();
Serial.println("SSID: " + config.ssid);
Serial.println("密码: " + config.password);

// 更新AP配置
config.ssid = "新的热点名称";
config.password = "新密码123";
config.channel = 6;

if (dbManager.updateAPConfig(config)) {
    Serial.println("AP配置更新成功");
}
```

### 3. 转发规则管理

```cpp
// 添加转发规则
ForwardRule rule;
rule.ruleName = "紧急短信转发";
rule.sourceNumber = "+86138*";        // 支持通配符
rule.pushType = "webhook";
rule.pushConfig = "{\"url\":\"http://example.com/webhook\"}";
rule.keywords = "紧急";
rule.enabled = true;
rule.isDefaultForward = false;  // 是否默认转发（忽略关键词匹配）

int ruleId = dbManager.addForwardRule(rule);
if (ruleId > 0) {
    Serial.println("规则添加成功，ID: " + String(ruleId));
}

// 获取所有规则
std::vector<ForwardRule> rules = dbManager.getAllForwardRules();
for (const auto& r : rules) {
    Serial.println("规则: " + r.name + ", 启用: " + String(r.enabled));
}

// 更新规则
rule.id = ruleId;
rule.enabled = false;
dbManager.updateForwardRule(rule);

// 删除规则
dbManager.deleteForwardRule(ruleId);
```

### 4. 短信记录管理

```cpp
// 添加短信记录
SMSRecord record;
record.fromNumber = "+8613800000001";
record.toNumber = "+8613800000000";
record.content = "这是一条测试短信";
record.ruleId = 1;
record.forwarded = false;
record.status = "received";

int recordId = dbManager.addSMSRecord(record);

// 获取短信记录
std::vector<SMSRecord> records = dbManager.getSMSRecords(10, 0);
for (const auto& r : records) {
    Serial.println("短信: " + r.content.substring(0, 20) + "...");
}

// 更新记录状态
record.id = recordId;
record.forwarded = true;
record.status = "forwarded";
dbManager.updateSMSRecord(record);

// 清理过期记录
int deletedCount = dbManager.deleteOldSMSRecords(30); // 删除30天前的记录
Serial.println("删除了 " + String(deletedCount) + " 条过期记录");
```

### 5. 数据库状态监控

```cpp
// 检查数据库状态
if (dbManager.isReady()) {
    Serial.println("数据库就绪");
}

// 获取数据库信息
DatabaseInfo info = dbManager.getDatabaseInfo();
Serial.println("数据库大小: " + String(info.dbSize) + " bytes");
Serial.println("表数量: " + String(info.tableCount));
Serial.println("记录总数: " + String(info.recordCount));

// 获取错误信息
String error = dbManager.getLastError();
if (!error.isEmpty()) {
    Serial.println("错误: " + error);
}
```

## 默认配置

### AP配置默认值
- **SSID**: `ESP-SMS-Relay`
- **密码**: `12345678`
- **启用状态**: `true`
- **信道**: `1`
- **最大连接数**: `4`

### 数据库文件
- **路径**: `/littlefs/sms_relay.db`
- **文件系统**: LittleFS
- **自动创建**: 支持

## 性能特性

### 内存使用
- 单例模式，内存占用最小化
- 查询结果缓存优化
- 及时释放资源

### 查询优化
- 关键字段建立索引
- 分页查询支持
- 批量操作优化

### 错误处理
- 完整的错误信息记录
- 自动错误恢复
- 调试模式支持

## 注意事项

### 使用前提
1. 必须先初始化LittleFS文件系统
2. 确保有足够的存储空间
3. 建议启用调试模式进行开发

### 最佳实践
1. 使用单例模式访问数据库
2. 及时处理错误信息
3. 定期清理过期数据
4. 合理设置查询限制

### 限制说明
1. 单线程访问，不支持并发
2. 内存限制下的查询结果数量
3. 文件系统空间限制

## 示例代码

完整的使用示例请参考：
- `examples/database_usage.cpp` - 完整功能演示
- `examples/simple_database_example.cpp` - 简单使用示例

## 依赖项

- **Sqlite3Esp32**: SQLite数据库支持
- **LittleFS**: 文件系统支持
- **FilesystemManager**: 文件系统管理器
- **Arduino**: 基础框架

## 版本信息

- **当前版本**: 1.0
- **兼容性**: ESP32-S3
- **最后更新**: 2024

## 故障排除

### 常见问题

1. **数据库初始化失败**
   - 检查LittleFS是否正确挂载
   - 确认存储空间是否充足
   - 查看错误日志信息

2. **查询结果为空**
   - 确认数据库连接状态
   - 检查SQL语句语法
   - 验证表结构是否正确

3. **内存不足**
   - 减少查询结果数量
   - 使用分页查询
   - 及时释放不用的数据

### 调试方法

```cpp
// 启用调试模式
dbManager.setDebugMode(true);

// 检查数据库状态
DatabaseStatus status = dbManager.getStatus();
Serial.println("数据库状态: " + String(status));

// 获取详细错误信息
String error = dbManager.getLastError();
Serial.println("错误信息: " + error);
```