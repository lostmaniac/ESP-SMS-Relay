# ESP-SMS-Relay 项目 AI 编辑器规则

## 项目概述

本项目是一个基于ESP32-S3的SMS中继系统，具有以下核心功能：
- SMS接收和转发
- WiFi管理和Web服务器
- 数据库管理（SQLite）
- 推送管理（企业微信、钉钉、Webhook）
- 模块化架构设计

## 代码规范

### 1. 文件组织结构

```
项目根目录/
├── src/                    # 主程序源码
├── lib/                    # 自定义库模块
│   ├── [模块名]/
│   │   ├── [模块名].h     # 头文件
│   │   ├── [模块名].cpp   # 实现文件
│   │   └── README.md      # 模块文档（可选）
├── include/                # 全局头文件
├── data/                   # 数据文件
├── docs/                   # 项目文档
├── examples/               # 示例代码
└── test/                   # 测试代码
```

### 2. 命名规范

#### 文件命名
- 头文件：`module_name.h`
- 源文件：`module_name.cpp`
- 文档文件：`README.md`
- 配置文件：`config.h`

#### 代码命名
- **类名**：使用PascalCase，如 `DatabaseManager`、`WiFiManager`
- **函数名**：使用camelCase，如 `initialize()`、`handleEvents()`
- **变量名**：使用camelCase，如 `systemStatus`、`lastError`
- **常量名**：使用UPPER_SNAKE_CASE，如 `SIM_SERIAL_NUM`、`MAX_RETRY_COUNT`
- **枚举值**：使用UPPER_SNAKE_CASE，如 `SYSTEM_RUNNING`、`PUSH_SUCCESS`

#### 宏定义
- 使用UPPER_SNAKE_CASE
- 添加项目前缀避免冲突，如 `ESP_SMS_DEBUG_MODE`

### 3. 注释规范

#### 文件级注释
```cpp
/**
 * @file filename.h
 * @brief 文件简要描述
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 详细描述文件的功能和用途
 */
```

#### 类级注释
```cpp
/**
 * @class ClassName
 * @brief 类的简要描述
 * 
 * 详细描述类的功能、用途和使用方法
 */
```

#### 函数级注释
```cpp
/**
 * @brief 函数简要描述
 * @param param1 参数1描述
 * @param param2 参数2描述
 * @return 返回值描述
 */
```

#### 结构体/枚举注释
```cpp
/**
 * @struct StructName
 * @brief 结构体简要描述
 */
struct StructName {
    int member1;    ///< 成员1描述
    String member2; ///< 成员2描述
};

/**
 * @enum EnumName
 * @brief 枚举简要描述
 */
enum EnumName {
    VALUE1,  ///< 值1描述
    VALUE2   ///< 值2描述
};
```

#### 行注释
- 使用 `//` 进行单行注释
- 复杂逻辑必须添加注释说明
- 重要的配置参数需要注释

## 架构设计原则

### 1. 模块化设计
- 每个功能模块独立成库
- 模块间通过明确的接口通信
- 避免循环依赖
- 单一职责原则

### 2. 单例模式使用
- 系统级管理器使用单例模式
- 确保资源的唯一性和全局访问
- 示例：`DatabaseManager::getInstance()`

### 3. 错误处理
- 所有公共接口必须有错误处理
- 使用枚举定义错误类型
- 提供 `getLastError()` 方法获取详细错误信息
- 关键操作失败不应导致系统崩溃

### 4. 内存管理
- 及时释放动态分配的内存
- 使用RAII原则管理资源
- 避免内存泄漏
- 合理使用栈内存和堆内存

## 模块开发规范

### 1. 新模块创建

当需要创建新模块时，必须：

1. 在 `lib/` 目录下创建模块文件夹
2. 创建头文件和实现文件
3. 添加完整的文档注释
4. 如果是管理器类，实现单例模式
5. 提供初始化和清理方法
6. 实现错误处理机制

### 2. 模块接口设计

```cpp
class ModuleManager {
public:
    // 单例获取
    static ModuleManager& getInstance();
    
    // 初始化
    bool initialize();
    
    // 核心功能方法
    bool doSomething();
    
    // 状态查询
    bool isReady();
    
    // 错误处理
    String getLastError();
    
    // 清理资源
    void cleanup();
    
private:
    ModuleManager();
    ~ModuleManager();
    
    // 禁止拷贝
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
    
    // 私有成员
    bool initialized;
    String lastError;
};
```

### 3. 配置管理
- 所有配置参数集中在 `include/config.h`
- 使用宏定义配置常量
- 提供默认值
- 添加详细注释说明

### 4. 调试支持
- 重要模块提供调试模式
- 使用 `setDebugMode(bool enabled)` 方法
- 调试信息输出到Serial

## 数据库设计规范

### 1. 表设计原则
- 使用有意义的表名和字段名
- 主键使用自增整数
- 时间字段使用统一格式
- 添加创建时间和更新时间字段

### 2. 数据操作
- 所有数据库操作通过 `DatabaseManager` 进行
- 使用事务确保数据一致性
- 提供批量操作接口
- 实现数据清理机制

## 网络通信规范

### 1. HTTP客户端
- 统一使用 `HTTPClient` 库
- 设置合理的超时时间
- 实现重试机制
- 处理网络异常

### 2. Web服务器
- RESTful API设计
- JSON格式数据交换
- 统一的错误响应格式
- 安全验证机制

## 测试规范

### 1. 单元测试
- 每个模块提供测试用例
- 测试文件命名：`test_module_name.cpp`
- 覆盖主要功能和边界情况

### 2. 集成测试
- 模块间交互测试
- 系统级功能测试
- 性能测试

## 文档规范

### 1. 模块文档
- 每个重要模块提供 `README.md`
- 包含功能描述、API文档、使用示例
- 定期更新文档内容

### 2. 项目文档
- 在 `docs/` 目录维护项目文档
- 包含架构设计、部署指南、故障排除

## 版本控制规范

### 1. 提交信息
- 使用有意义的提交信息
- 格式：`[模块名] 简要描述`
- 详细描述放在提交信息体中

### 2. 分支管理
- `main` 分支为稳定版本
- 功能开发使用 `feature/` 分支
- 修复使用 `fix/` 分支

## 性能优化指南

### 1. 内存优化
- 避免频繁的内存分配
- 使用对象池复用对象
- 及时释放不需要的资源

### 2. 网络优化
- 合理设置连接超时
- 实现连接复用
- 压缩传输数据

### 3. 数据库优化
- 使用索引提高查询性能
- 定期清理过期数据
- 批量操作减少I/O

## 安全规范

### 1. 数据安全
- 敏感信息加密存储
- 输入数据验证
- SQL注入防护

### 2. 网络安全
- HTTPS通信
- API访问控制
- 防止DDoS攻击

## AI编辑器特殊指令

### 1. 代码生成要求
- 必须包含完整的文档注释
- 遵循项目命名规范
- 实现错误处理机制
- 考虑内存管理

### 2. 模块修改要求
- 修改前先理解现有架构
- 保持接口兼容性
- 更新相关文档
- 添加必要的测试

### 3. 调试和优化
- 优先使用现有的调试机制
- 性能优化不能牺牲代码可读性
- 重要修改需要备份

### 4. 依赖管理
- 新增依赖需要在 `platformio.ini` 中声明
- 优先使用项目已有的库
- 避免引入过重的依赖

## 模块功能边界和使用规范（硬性规定）

### 重要原则
- **单一职责**: 每个模块只负责特定功能领域
- **功能独占**: 特定功能只能通过指定模块实现
- **禁止跨越**: 严禁在其他模块中实现不属于该模块的功能
- **接口统一**: 所有模块间交互必须通过公开接口进行

### 1. 数据库操作规范（强制性）

#### 允许的操作
- **仅限 `database_manager` 模块**进行所有数据库相关操作
- 其他模块必须通过 `DatabaseManager::getInstance()` 调用数据库功能

#### 严格禁止
- ❌ 在任何其他模块中直接使用 SQLite API
- ❌ 在其他模块中创建数据库连接
- ❌ 在其他模块中执行 SQL 语句
- ❌ 在其他模块中进行数据库事务操作

#### 正确使用方式
```cpp
// 正确：通过 DatabaseManager 操作数据库
DatabaseManager& db = DatabaseManager::getInstance();
db.addSMSRecord(record);

// 错误：直接使用 SQLite API
// sqlite3_exec(db, "INSERT INTO...", NULL, NULL, NULL); // 禁止！
```

### 2. 文件系统操作规范（强制性）

#### 允许的操作
- **仅限 `filesystem_manager` 模块**进行文件系统操作
- 其他模块通过 `FilesystemManager::getInstance()` 访问文件系统功能

#### 严格禁止
- ❌ 在其他模块中直接使用 LittleFS API
- ❌ 在其他模块中进行文件读写操作
- ❌ 在其他模块中管理文件系统挂载

### 3. 网络通信规范（强制性）

#### HTTP 客户端操作
- **仅限 `http_client` 模块**进行 HTTP 请求
- 其他模块通过 `HTTPClientManager::getInstance()` 发送请求

#### WiFi 管理
- **仅限 `wifi_manager` 模块**管理 WiFi 连接
- 其他模块通过 `WiFiManager::getInstance()` 获取网络状态

#### Web 服务器
- **仅限 `web_server` 模块**处理 Web 服务
- 路由和处理器注册只能在该模块内进行

### 4. 硬件通信规范（强制性）

#### GSM/SMS 通信
- **仅限 `gsm_service` 模块**进行 GSM 模块通信
- **仅限 `sms_handler` 模块**处理 SMS 业务逻辑
- **仅限 `phone_caller` 模块**处理电话功能

#### UART 通信
- **仅限 `uart_monitor` 和 `uart_dispatcher` 模块**进行串口通信
- **仅限 `at_command_handler` 模块**处理 AT 命令

### 5. 系统管理规范（强制性）

#### 系统初始化
- **仅限 `system_init` 模块**进行系统级初始化
- **仅限 `module_manager` 模块**管理模块生命周期

#### 配置管理
- **仅限 `config_manager` 模块**进行配置读写
- 其他模块通过 `ConfigManager::getInstance()` 访问配置

#### 日志管理
- **仅限 `log_manager` 模块**进行日志记录和管理
- 其他模块通过 `LogManager::getInstance()` 记录日志

### 6. 业务逻辑规范（强制性）

#### 推送管理
- **仅限 `push_manager` 模块**处理消息推送
- 支持企业微信、钉钉、Webhook 等推送方式
- 其他模块通过 `PushManager::getInstance()` 发送推送

#### 运营商配置
- **仅限 `carrier_config` 模块**管理运营商相关配置
- **仅限 `network_config` 模块**管理网络配置

## 模块功能详细定义

### 系统核心模块

#### `system_init` - 系统初始化管理
**职责范围**:
- 系统启动流程控制
- 硬件初始化协调
- 系统状态管理
- 启动任务创建和管理

**必须提供的接口**:
- `bool initialize(bool runTests = true)`
- `bool start()`
- `SystemStatus getSystemStatus()`
- `String getLastError()`
- `bool restart()`

#### `module_manager` - 模块生命周期管理
**职责范围**:
- 模块注册和发现
- 模块初始化顺序控制
- 模块依赖关系管理
- 模块状态监控

**必须提供的接口**:
- `bool registerModule(ModuleInterface* module)`
- `bool initializeAllModules()`
- `bool startAllModules()`
- `ModuleStatus getModuleStatus(const String& moduleName)`

#### `config_manager` - 配置管理
**职责范围**:
- 系统配置读写
- 配置文件管理
- 配置验证和默认值
- 配置热更新

**必须提供的接口**:
- `String getConfig(const String& key, const String& defaultValue = "")`
- `bool setConfig(const String& key, const String& value)`
- `bool saveConfig()`
- `bool loadConfig()`

### 通信模块

#### `wifi_manager` - WiFi连接管理
**职责范围**:
- WiFi 连接和断开
- AP 模式管理
- 网络状态监控
- WiFi 配置管理

**必须提供的接口**:
- `bool connectToWiFi(const String& ssid, const String& password)`
- `bool startAP(const String& ssid, const String& password)`
- `WiFiStatus getStatus()`
- `String getIPAddress()`

#### `gsm_service` - GSM模块通信
**职责范围**:
- GSM 模块初始化
- 网络注册管理
- 信号强度监控
- SIM 卡状态检查

**必须提供的接口**:
- `bool initialize()`
- `bool isNetworkRegistered()`
- `int getSignalStrength()`
- `String getIMEI()`

#### `web_server` - Web服务器
**职责范围**:
- HTTP 服务器管理
- 路由注册和处理
- 静态文件服务
- API 接口提供

**必须提供的接口**:
- `bool start(int port = 80)`
- `void handleClient()`
- `bool addRoute(const String& path, WebHandler handler)`
- `bool isRunning()`

#### `http_client` - HTTP客户端
**职责范围**:
- HTTP 请求发送
- 响应处理
- 连接管理
- 错误重试

**必须提供的接口**:
- `HTTPResponse get(const String& url, const String& headers = "")`
- `HTTPResponse post(const String& url, const String& data, const String& headers = "")`
- `bool setTimeout(int timeoutMs)`

### 数据管理模块

#### `database_manager` - SQLite数据库管理
**职责范围**:
- 数据库连接管理
- 表结构创建和维护
- 数据 CRUD 操作
- 事务管理
- 数据清理和维护

**必须提供的接口**:
- `bool initialize()`
- `int addSMSRecord(const SMSRecord& record)`
- `std::vector<SMSRecord> getSMSRecords(int limit = 50, int offset = 0)`
- `int addForwardRule(const ForwardRule& rule)`
- `std::vector<ForwardRule> getAllForwardRules()`
- `bool updateAPConfig(const APConfig& config)`
- `APConfig getAPConfig()`

#### `filesystem_manager` - 文件系统管理
**职责范围**:
- LittleFS 文件系统管理
- 文件读写操作
- 目录管理
- 存储空间监控

**必须提供的接口**:
- `bool initialize(bool formatOnFail = false)`
- `bool writeFile(const String& path, const String& content)`
- `String readFile(const String& path)`
- `bool deleteFile(const String& path)`
- `size_t getTotalBytes()`
- `size_t getUsedBytes()`

### 业务模块

#### `sms_handler` - 短信处理
**职责范围**:
- 短信接收处理
- 短信内容解析
- 短信转发逻辑
- 短信状态管理

**必须提供的接口**:
- `bool processSMS(const String& sender, const String& content)`
- `bool forwardSms(const SMSRecord& record)`
- `std::vector<SMSRecord> getUnprocessedSMS()`

#### `push_manager` - 推送管理
**职责范围**:
- 多平台推送支持
- 推送规则匹配
- 消息模板处理
- 推送状态跟踪

**必须提供的接口**:
- `PushResult processSmsForward(const PushContext& context)`
- `PushResult pushByRule(int ruleId, const PushContext& context)`
- `bool testPushConfig(const String& pushType, const String& config)`

#### `phone_caller` - 电话功能
**职责范围**:
- 电话拨打功能
- 通话状态管理
- 来电处理
- 通话记录

**必须提供的接口**:
- `bool makeCall(const String& phoneNumber)`
- `bool hangUp()`
- `CallStatus getCallStatus()`

### 工具模块

#### `log_manager` - 日志管理
**职责范围**:
- 日志记录和输出
- 日志级别管理
- 日志文件管理
- 日志格式化

**必须提供的接口**:
- `void log(LogLevel level, const String& message)`
- `void debug(const String& message)`
- `void info(const String& message)`
- `void warn(const String& message)`
- `void error(const String& message)`

#### `uart_monitor` - 串口监控
**职责范围**:
- 串口数据监控
- 数据包解析
- 串口状态管理

#### `at_command_handler` - AT命令处理
**职责范围**:
- AT 命令发送和接收
- 响应解析
- 命令队列管理
- 超时处理

**必须提供的接口**:
- `ATResponse sendCommand(const String& command, int timeoutMs = 5000)`
- `bool isReady()`
- `void setEchoMode(bool enabled)`

## 模块使用违规检查清单

### AI编辑器必须检查的违规行为

1. **数据库操作违规**
   - 检查是否在非 `database_manager` 模块中使用 SQLite API
   - 检查是否直接操作数据库文件

2. **文件系统操作违规**
   - 检查是否在非 `filesystem_manager` 模块中使用 LittleFS API
   - 检查是否直接进行文件 I/O 操作

3. **网络通信违规**
   - 检查是否在非指定模块中创建 HTTP 连接
   - 检查是否绕过 WiFi 管理器直接操作 WiFi

4. **硬件通信违规**
   - 检查是否在非指定模块中直接操作串口
   - 检查是否绕过 GSM 服务直接发送 AT 命令

5. **配置管理违规**
   - 检查是否在非 `config_manager` 模块中直接读写配置文件
   - 检查是否硬编码配置值而不使用配置管理器

### 违规处理原则

1. **发现违规时的处理**
   - 立即停止代码生成
   - 提示正确的模块使用方式
   - 建议通过正确的接口实现功能

2. **功能缺失时的处理**
   - 优先扩展现有模块功能
   - 在对应模块中添加所需接口
   - 避免创建功能重复的新模块

## 常用模块参考

---

**注意**: 本规则文档会根据项目发展持续更新，请AI编辑器在开发过程中严格遵循这些规范，确保代码质量和项目的可维护性。