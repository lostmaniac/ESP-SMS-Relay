/**
 * @file task_scheduler.cpp
 * @brief 定时任务调度器模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 提供定时任务调度功能的具体实现
 */

#include "task_scheduler.h"

/**
 * @brief 获取单例实例
 * @return TaskScheduler& 单例引用
 */
TaskScheduler& TaskScheduler::getInstance() {
    static TaskScheduler instance;
    return instance;
}

/**
 * @brief 私有构造函数（单例模式）
 */
TaskScheduler::TaskScheduler() 
    : initialized(false), nextTaskId(1), debugMode(false), lastTaskCheck(0) {
}

/**
 * @brief 析构函数
 */
TaskScheduler::~TaskScheduler() {
    clearAllTasks();
}

/**
 * @brief 初始化任务调度器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool TaskScheduler::initialize() {
    if (initialized) {
        debugPrint("任务调度器已经初始化");
        return true;
    }
    
    debugPrint("初始化任务调度器");
    
    // 清空任务列表
    tasks.clear();
    nextTaskId = 1;
    lastTaskCheck = millis();
    
    initialized = true;
    debugPrint("任务调度器初始化完成");
    return true;
}

/**
 * @brief 检查是否已初始化
 * @return true 已初始化
 * @return false 未初始化
 */
bool TaskScheduler::isInitialized() const {
    return initialized;
}

/**
 * @brief 添加周期性任务
 * @param name 任务名称
 * @param interval 执行间隔（毫秒）
 * @param callback 回调函数
 * @param executeImmediately 是否立即执行一次
 * @return int 任务ID，-1表示失败
 */
int TaskScheduler::addPeriodicTask(const String& name, unsigned long interval, 
                                  std::function<void()> callback, bool executeImmediately) {
    if (!initialized) {
        setError("任务调度器未初始化");
        return -1;
    }
    
    if (!callback) {
        setError("回调函数不能为空");
        return -1;
    }
    
    if (interval == 0) {
        setError("周期性任务间隔不能为0");
        return -1;
    }
    
    ScheduledTask task;
    task.id = generateTaskId();
    task.name = name;
    task.type = TASK_PERIODIC;
    task.interval = interval;
    task.callback = callback;
    task.enabled = true;
    task.executing = false;
    
    unsigned long currentTime = millis();
    if (executeImmediately) {
        task.lastExecuted = 0;
        task.nextExecution = currentTime;
    } else {
        task.lastExecuted = currentTime;
        task.nextExecution = currentTime + interval;
    }
    
    tasks.push_back(task);
    
    debugPrint("添加周期性任务: " + name + ", ID: " + String(task.id) + ", 间隔: " + String(interval) + "ms");
    return task.id;
}

/**
 * @brief 添加一次性任务
 * @param name 任务名称
 * @param delay 延迟执行时间（毫秒）
 * @param callback 回调函数
 * @return int 任务ID，-1表示失败
 */
int TaskScheduler::addOnceTask(const String& name, unsigned long delay, std::function<void()> callback) {
    if (!initialized) {
        setError("任务调度器未初始化");
        return -1;
    }
    
    if (!callback) {
        setError("回调函数不能为空");
        return -1;
    }
    
    ScheduledTask task;
    task.id = generateTaskId();
    task.name = name;
    task.type = TASK_ONCE;
    task.interval = 0;
    task.callback = callback;
    task.enabled = true;
    task.executing = false;
    
    unsigned long currentTime = millis();
    task.lastExecuted = 0;
    task.nextExecution = currentTime + delay;
    
    tasks.push_back(task);
    
    debugPrint("添加一次性任务: " + name + ", ID: " + String(task.id) + ", 延迟: " + String(delay) + "ms");
    return task.id;
}

/**
 * @brief 移除任务
 * @param taskId 任务ID
 * @return true 移除成功
 * @return false 移除失败
 */
bool TaskScheduler::removeTask(int taskId) {
    if (!initialized) {
        setError("任务调度器未初始化");
        return false;
    }
    
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
        if (it->id == taskId) {
            debugPrint("移除任务: " + it->name + ", ID: " + String(taskId));
            tasks.erase(it);
            return true;
        }
    }
    
    setError("未找到任务ID: " + String(taskId));
    return false;
}

/**
 * @brief 启用/禁用任务
 * @param taskId 任务ID
 * @param enabled 是否启用
 * @return true 设置成功
 * @return false 设置失败
 */
bool TaskScheduler::setTaskEnabled(int taskId, bool enabled) {
    if (!initialized) {
        setError("任务调度器未初始化");
        return false;
    }
    
    ScheduledTask* task = findTask(taskId);
    if (!task) {
        setError("未找到任务ID: " + String(taskId));
        return false;
    }
    
    task->enabled = enabled;
    debugPrint("任务 " + task->name + " (ID: " + String(taskId) + ") " + (enabled ? "启用" : "禁用"));
    return true;
}

/**
 * @brief 处理任务调度（需要在主循环中调用）
 */
void TaskScheduler::handleTasks() {
    if (!initialized) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // 避免频繁检查，至少间隔100ms
    if (currentTime - lastTaskCheck < 100) {
        return;
    }
    lastTaskCheck = currentTime;
    
    // 遍历所有任务
    for (auto it = tasks.begin(); it != tasks.end();) {
        ScheduledTask& task = *it;
        
        // 跳过禁用或正在执行的任务
        if (!task.enabled || task.executing) {
            ++it;
            continue;
        }
        
        // 检查是否到了执行时间
        if (currentTime >= task.nextExecution) {
            debugPrint("执行任务: " + task.name + " (ID: " + String(task.id) + ")");
            
            // 标记为正在执行
            task.executing = true;
            
            // 执行回调函数
            try {
                task.callback();
            } catch (...) {
                debugPrint("任务执行异常: " + task.name);
            }
            
            // 更新执行时间
            task.lastExecuted = currentTime;
            task.executing = false;
            
            // 处理不同类型的任务
            if (task.type == TASK_ONCE) {
                // 一次性任务执行后删除
                debugPrint("一次性任务完成，删除: " + task.name);
                it = tasks.erase(it);
                continue;
            } else if (task.type == TASK_PERIODIC) {
                // 周期性任务更新下次执行时间
                task.nextExecution = currentTime + task.interval;
            }
        }
        
        ++it;
    }
}

/**
 * @brief 获取任务数量
 * @return int 任务数量
 */
int TaskScheduler::getTaskCount() const {
    return tasks.size();
}

/**
 * @brief 获取启用的任务数量
 * @return int 启用的任务数量
 */
int TaskScheduler::getEnabledTaskCount() const {
    int count = 0;
    for (const auto& task : tasks) {
        if (task.enabled) {
            count++;
        }
    }
    return count;
}

/**
 * @brief 获取任务信息
 * @param taskId 任务ID
 * @return String 任务信息，空字符串表示未找到
 */
String TaskScheduler::getTaskInfo(int taskId) const {
    for (const auto& task : tasks) {
        if (task.id == taskId) {
            String info = "任务ID: " + String(task.id) + ", 名称: " + task.name;
            info += ", 类型: " + String(task.type == TASK_ONCE ? "一次性" : "周期性");
            info += ", 状态: " + String(task.enabled ? "启用" : "禁用");
            if (task.type == TASK_PERIODIC) {
                info += ", 间隔: " + String(task.interval) + "ms";
            }
            info += ", 下次执行: " + String(task.nextExecution);
            return info;
        }
    }
    return "";
}

/**
 * @brief 获取所有任务信息
 * @return String 所有任务信息
 */
String TaskScheduler::getAllTasksInfo() const {
    String info = "任务调度器状态: " + String(initialized ? "已初始化" : "未初始化");
    info += ", 任务总数: " + String(getTaskCount());
    info += ", 启用任务数: " + String(getEnabledTaskCount()) + "\n";
    
    for (const auto& task : tasks) {
        info += getTaskInfo(task.id) + "\n";
    }
    
    return info;
}

/**
 * @brief 清理所有任务
 */
void TaskScheduler::clearAllTasks() {
    debugPrint("清理所有任务");
    tasks.clear();
    nextTaskId = 1;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String TaskScheduler::getLastError() const {
    return lastError;
}

/**
 * @brief 启用调试模式
 * @param enable 是否启用
 */
void TaskScheduler::setDebugMode(bool enable) {
    debugMode = enable;
    debugPrint("调试模式: " + String(enable ? "启用" : "禁用"));
}

/**
 * @brief 生成新的任务ID
 * @return int 新的任务ID
 */
int TaskScheduler::generateTaskId() {
    return nextTaskId++;
}

/**
 * @brief 查找任务
 * @param taskId 任务ID
 * @return ScheduledTask* 任务指针，nullptr表示未找到
 */
ScheduledTask* TaskScheduler::findTask(int taskId) {
    for (auto& task : tasks) {
        if (task.id == taskId) {
            return &task;
        }
    }
    return nullptr;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void TaskScheduler::setError(const String& error) {
    lastError = error;
    debugPrint("错误: " + error);
}

/**
 * @brief 调试输出
 * @param message 调试信息
 */
void TaskScheduler::debugPrint(const String& message) {
    if (debugMode) {
        Serial.println("[TaskScheduler] " + message);
    }
}