# 数据库自动清理功能说明

## 功能概述

为了防止短信数据库无限增长导致存储空间不足，系统新增了自动数据库清理功能。该功能会在系统启动时检查短信记录数量，如果超过1万条则自动清理，并且每24小时执行一次定期清理任务，确保短信数量始终保持在1万条以下。

## 功能特性

### 1. 启动时检查
- 系统启动时自动检查短信记录数量
- 如果超过10,000条，立即执行清理
- 清理后保留最新的10,000条记录

### 2. 定期清理任务
- 每24小时自动执行一次清理检查
- 使用定时任务调度器管理
- 确保数据库大小始终在合理范围内

### 3. 安全清理策略
- 按时间戳排序，保留最新的记录
- 删除最旧的记录
- 使用事务确保数据一致性

## 新增API接口

### DatabaseManager 新增方法

#### `getSMSRecordCount()`
```cpp
/**
 * @brief 获取短信记录总数
 * @return int 短信记录数量，-1表示失败
 */
int getSMSRecordCount();
```

#### `cleanupSMSRecordsByCount()`
```cpp
/**
 * @brief 按数量清理短信记录
 * @param maxCount 保留的最大记录数
 * @return bool 清理是否成功
 */
bool cleanupSMSRecordsByCount(int maxCount = 10000);
```

#### `checkAndCleanupSMSRecords()`
```cpp
/**
 * @brief 检查并执行短信记录清理
 * @param threshold 触发清理的阈值（默认10000）
 * @param keepCount 清理后保留的记录数（默认10000）
 * @return bool 检查和清理是否成功
 */
bool checkAndCleanupSMSRecords(int threshold = 10000, int keepCount = 10000);
```

## 定时任务调度器

### TaskScheduler 类

新增的定时任务调度器提供了灵活的任务管理功能：

#### 主要功能
- 支持一次性任务和周期性任务
- 任务启用/禁用控制
- 任务信息查询
- 错误处理和调试支持

#### 核心方法

```cpp
// 获取单例实例
TaskScheduler& scheduler = TaskScheduler::getInstance();

// 初始化
scheduler.initialize();

// 添加周期性任务
int taskId = scheduler.addPeriodicTask("清理任务", 24*60*60*1000, []() {
    DatabaseManager::getInstance().checkAndCleanupSMSRecords();
});

// 在主循环中处理任务
scheduler.handleTasks();
```

## 系统集成

### 1. 系统初始化集成

在 `system_init.cpp` 中的数据库初始化完成后：

```cpp
// 检查短信记录数量并执行清理
int smsCount = databaseManager.getSMSRecordCount();
if (smsCount > 10000) {
    LOG_INFO(LOG_MODULE_SYSTEM, "检测到短信记录过多(" + String(smsCount) + "条)，开始清理...");
    if (databaseManager.checkAndCleanupSMSRecords()) {
        int newCount = databaseManager.getSMSRecordCount();
        LOG_INFO(LOG_MODULE_SYSTEM, "短信记录清理完成，当前数量: " + String(newCount));
    }
}

// 初始化定时任务调度器
TaskScheduler& taskScheduler = TaskScheduler::getInstance();
if (taskScheduler.initialize()) {
    // 添加24小时定期清理任务
    taskScheduler.addPeriodicTask("数据库清理任务", 24 * 60 * 60 * 1000, []() {
        DatabaseManager::getInstance().checkAndCleanupSMSRecords();
    });
}
```

### 2. 主循环集成

在 `main.cpp` 的主循环中：

```cpp
void loop() {
    // 处理CLI输入
    if (terminalManager.isCLIRunning()) {
        terminalManager.handleSerialInput();
    }
    
    // 处理定时任务调度
    TaskScheduler& taskScheduler = TaskScheduler::getInstance();
    taskScheduler.handleTasks();
    
    // 其他系统任务...
}
```

## 配置参数

可以通过修改以下参数来调整清理策略：

```cpp
// 在 config.h 中定义
#define SMS_CLEANUP_THRESHOLD 10000    // 触发清理的阈值
#define SMS_CLEANUP_KEEP_COUNT 10000   // 清理后保留的记录数
#define SMS_CLEANUP_INTERVAL (24*60*60*1000)  // 清理间隔（毫秒）
```

## 日志记录

系统会记录以下清理相关的日志：

- 启动时的记录数量检查
- 清理操作的开始和结束
- 清理前后的记录数量
- 清理操作的执行时间
- 任务调度器的状态信息

## 性能考虑

### 1. 清理性能
- 使用 `DELETE` 语句配合 `LIMIT` 和 `ORDER BY`
- 避免全表扫描
- 使用事务确保原子性

### 2. 内存使用
- 清理操作不会加载所有记录到内存
- 使用流式处理方式
- 及时释放资源

### 3. 任务调度开销
- 任务检查频率控制在合理范围
- 避免频繁的时间计算
- 使用高效的任务查找算法

## 故障排除

### 常见问题

1. **清理任务未执行**
   - 检查任务调度器是否正确初始化
   - 确认主循环中调用了 `handleTasks()`
   - 查看错误日志

2. **清理效果不明显**
   - 检查清理阈值设置
   - 确认数据库连接正常
   - 查看清理操作的日志

3. **系统性能下降**
   - 检查清理操作的执行频率
   - 优化清理SQL语句
   - 考虑在低峰时段执行清理

### 调试方法

```cpp
// 启用调试模式
DatabaseManager::getInstance().setDebugMode(true);
TaskScheduler::getInstance().setDebugMode(true);

// 手动触发清理测试
DatabaseManager::getInstance().checkAndCleanupSMSRecords();

// 查看任务信息
TaskScheduler& scheduler = TaskScheduler::getInstance();
Serial.println(scheduler.getAllTasksInfo());
```

## 测试验证

系统提供了测试程序 `test/test_database_cleanup.cpp` 用于验证清理功能：

```cpp
// 创建测试记录
createTestSMSRecords(15000);

// 测试清理功能
testDatabaseCleanup();

// 测试任务调度器
testTaskScheduler();
```

## 未来扩展

1. **可配置的清理策略**
   - 支持按时间范围清理
   - 支持按发送方清理
   - 支持保留重要短信

2. **清理统计信息**
   - 记录清理历史
   - 提供清理效果分析
   - 存储空间使用趋势

3. **智能清理算法**
   - 根据系统负载调整清理频率
   - 预测存储空间使用情况
   - 自适应清理策略