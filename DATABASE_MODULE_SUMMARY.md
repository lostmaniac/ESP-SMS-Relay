# 数据库模块实现总结

## 概述

本文档总结了ESP-SMS-Relay项目中数据库模块的完整实现，该模块基于Sqlite3Esp32库，为系统提供了完整的数据持久化解决方案。

## 模块架构

### 核心组件

1. **DatabaseManager类** - 数据库管理器（单例模式）
2. **数据结构定义** - AP配置、转发规则、短信记录
3. **文件系统集成** - 与LittleFS文件系统深度集成
4. **错误处理机制** - 完善的错误检测和恢复

### 文件结构

```
ESP-SMS-Relay/
├── lib/database_manager/
│   ├── database_manager.h          # 数据库管理器头文件
│   └── database_manager.cpp        # 数据库管理器实现
├── examples/
│   ├── database_usage.cpp          # 使用示例
│   └── database_demo.cpp           # 功能演示
├── test/
│   └── test_database_manager.cpp   # 单元测试
└── DATABASE_MODULE_SUMMARY.md      # 本文档
```

## 数据库设计

### 表结构

#### 1. ap_config表 - AP配置
```sql
CREATE TABLE ap_config (
    id INTEGER PRIMARY KEY,
    ssid TEXT NOT NULL,
    password TEXT NOT NULL,
    channel INTEGER DEFAULT 1,
    max_connections INTEGER DEFAULT 4,
    enabled BOOLEAN DEFAULT 1,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);
```

#### 2. forward_rules表 - 转发规则
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

#### 3. sms_records表 - 短信记录
```sql
CREATE TABLE sms_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    from_number TEXT NOT NULL,     -- 发送方号码，支持索引
    content TEXT NOT NULL,         -- 短信内容，支持索引
    received_at INTEGER NOT NULL   -- 接收时间，time保存，支持索引，方便以后按照时间过滤短信
);
```

### 默认配置

- **AP SSID**: ESP-SMS-Relay
- **AP密码**: 12345678
- **信道**: 1
- **最大连接数**: 4
- **启用状态**: true

## 核心功能

### 1. 数据库生命周期管理

```cpp
// 初始化数据库
DatabaseManager& db = DatabaseManager::getInstance();
bool success = db.initialize();

// 检查状态
if (db.isReady()) {
    // 数据库就绪，可以进行操作
}

// 关闭数据库
db.close();
```

### 2. AP配置管理

```cpp
// 获取AP配置
APConfig config = db.getAPConfig();

// 更新AP配置
config.ssid = "New-SSID";
config.password = "newpassword";
bool success = db.updateAPConfig(config);
```

### 3. 转发规则管理

```cpp
// 添加转发规则
ForwardRule rule;
rule.ruleName = "紧急联系人";
rule.sourceNumber = "+86138*";
rule.pushType = "webhook";
rule.pushConfig = "{\"url\":\"http://example.com/webhook\"}";
rule.keywords = "紧急";
rule.enabled = true;

int ruleId = db.addForwardRule(rule);

// 获取所有规则
std::vector<ForwardRule> rules = db.getAllForwardRules();

// 更新规则
rule.keyword = "紧急|急救";
db.updateForwardRule(rule);

// 删除规则
db.deleteForwardRule(ruleId);
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

int recordId = db.addSMSRecord(record);

// 分页查询记录
std::vector<SMSRecord> records = db.getSMSRecords(10, 0); // 获取前10条

// 更新记录
record.forwarded = true;
record.status = "forwarded";
record.forwardedAt = String(millis());
db.updateSMSRecord(record);

// 清理旧记录
int deletedCount = db.deleteOldSMSRecords(7); // 删除7天前的记录
```

## 系统集成

### 1. 文件系统依赖

数据库模块依赖FilesystemManager，确保LittleFS文件系统已正确初始化：

```cpp
// 在system_init.cpp中的集成
FilesystemManager& fsManager = FilesystemManager::getInstance();
if (fsManager.initialize(true)) {
    // 文件系统初始化成功，开始初始化数据库
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    if (dbManager.initialize()) {
        Serial.println("数据库初始化成功");
        
        // 打印数据库信息
        DatabaseInfo info = dbManager.getDatabaseInfo();
        Serial.println("数据库路径: " + info.dbPath);
        Serial.println("数据库大小: " + String(info.dbSize) + " 字节");
        
        // 打印AP配置
        APConfig apConfig = dbManager.getAPConfig();
        Serial.println("AP SSID: " + apConfig.ssid);
        Serial.println("AP密码: " + apConfig.password);
    }
}
```

### 2. 依赖库配置

在`platformio.ini`中已配置：

```ini
lib_deps = 
    siara-cc/Sqlite3Esp32@^2.5

build_flags = 
    -DUSE_LITTLEFS=1
```

## 性能特性

### 1. 内存管理

- 使用单例模式，避免多实例内存浪费
- 及时释放查询结果内存
- 使用std::vector进行动态内存管理

### 2. 性能优化

- 数据库连接复用
- 批量操作支持
- 索引优化（主键自动索引）
- 分页查询减少内存占用

### 3. 错误处理

- 完善的错误检测机制
- 自动重试机制
- 详细的错误日志
- 数据库状态监控

## 测试验证

### 1. 单元测试

`test/test_database_manager.cpp`提供了完整的单元测试：

- 数据库初始化测试
- AP配置管理测试
- 转发规则管理测试
- 短信记录管理测试
- 错误处理测试
- 性能测试

### 2. 功能演示

`examples/database_demo.cpp`提供了功能演示：

- 完整的使用流程演示
- 性能测试演示
- 错误处理演示

### 3. 使用示例

`examples/database_usage.cpp`提供了基本使用示例。

## 安全考虑

### 1. 数据保护

- 数据库文件存储在LittleFS中，具有一定的保护性
- 敏感信息（如密码）建议加密存储
- 定期备份重要数据

### 2. 访问控制

- 使用单例模式控制数据库访问
- 提供统一的API接口
- 参数验证和边界检查

## 故障排除

### 1. 常见问题

**问题**: 数据库初始化失败
**解决**: 
- 检查LittleFS是否正确初始化
- 检查存储空间是否充足
- 查看错误日志获取详细信息

**问题**: 查询结果为空
**解决**:
- 检查数据库是否已正确初始化
- 验证查询条件是否正确
- 检查表是否已创建并包含数据

**问题**: 性能问题
**解决**:
- 使用分页查询减少内存占用
- 定期清理旧数据
- 避免频繁的数据库连接操作

### 2. 调试方法

```cpp
// 启用调试模式
DatabaseManager& db = DatabaseManager::getInstance();

// 检查数据库状态
DatabaseStatus status = db.getStatus();
Serial.println("数据库状态: " + String(status));

// 获取错误信息
String error = db.getLastError();
if (!error.isEmpty()) {
    Serial.println("错误: " + error);
}

// 获取数据库信息
DatabaseInfo info = db.getDatabaseInfo();
Serial.println("数据库信息: " + info.dbPath);
```

## 扩展建议

### 1. 功能扩展

- 添加数据备份和恢复功能
- 实现数据库版本管理和迁移
- 添加数据加密功能
- 实现数据同步功能

### 2. 性能优化

- 添加查询缓存机制
- 实现连接池管理
- 优化SQL查询语句
- 添加数据库压缩功能

### 3. 监控和维护

- 添加数据库健康检查
- 实现自动清理机制
- 添加性能监控指标
- 实现数据库修复功能

## 版本信息

- **版本**: 1.0.0
- **创建日期**: 2024
- **依赖库**: Sqlite3Esp32 v2.5+
- **支持平台**: ESP32-S3
- **文件系统**: LittleFS

## 总结

数据库模块已成功实现并集成到ESP-SMS-Relay系统中，提供了完整的数据持久化解决方案。模块具有以下特点：

✅ **功能完整**: 支持AP配置、转发规则、短信记录的完整管理
✅ **性能优化**: 内存管理良好，支持分页查询和批量操作
✅ **错误处理**: 完善的错误检测和恢复机制
✅ **易于使用**: 提供简洁的API接口和详细的文档
✅ **测试完备**: 包含单元测试、功能演示和使用示例
✅ **系统集成**: 已成功集成到系统初始化流程中

该模块为ESP-SMS-Relay项目提供了可靠的数据存储基础，支持系统的长期稳定运行。