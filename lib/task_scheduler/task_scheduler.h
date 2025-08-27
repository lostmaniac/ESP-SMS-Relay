/**
 * @file task_scheduler.h
 * @brief 定时任务调度器模块
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 提供定时任务调度功能，支持周期性任务执行
 * 主要用于数据库清理、系统维护等定期任务
 */

#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <Arduino.h>
#include <vector>
#include <functional>

/**
 * @enum TaskType
 * @brief 任务类型枚举
 */
enum TaskType {
    TASK_ONCE,      ///< 一次性任务
    TASK_PERIODIC   ///< 周期性任务
};

/**
 * @struct ScheduledTask
 * @brief 调度任务结构体
 */
struct ScheduledTask {
    int id;                                 ///< 任务ID
    String name;                            ///< 任务名称
    TaskType type;                          ///< 任务类型
    unsigned long interval;                 ///< 执行间隔（毫秒）
    unsigned long lastExecuted;            ///< 上次执行时间
    unsigned long nextExecution;           ///< 下次执行时间
    std::function<void()> callback;        ///< 回调函数
    bool enabled;                           ///< 是否启用
    bool executing;                         ///< 是否正在执行
    
    /**
     * @brief 构造函数
     */
    ScheduledTask() : id(0), type(TASK_ONCE), interval(0), lastExecuted(0), 
                     nextExecution(0), enabled(true), executing(false) {}
};

/**
 * @class TaskScheduler
 * @brief 定时任务调度器类
 * 
 * 提供定时任务的注册、执行、管理功能
 * 支持一次性任务和周期性任务
 */
class TaskScheduler {
public:
    /**
     * @brief 获取单例实例
     * @return TaskScheduler& 单例引用
     */
    static TaskScheduler& getInstance();

    /**
     * @brief 初始化任务调度器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();

    /**
     * @brief 检查是否已初始化
     * @return true 已初始化
     * @return false 未初始化
     */
    bool isInitialized() const;

    /**
     * @brief 添加周期性任务
     * @param name 任务名称
     * @param interval 执行间隔（毫秒）
     * @param callback 回调函数
     * @param executeImmediately 是否立即执行一次
     * @return int 任务ID，-1表示失败
     */
    int addPeriodicTask(const String& name, unsigned long interval, 
                       std::function<void()> callback, bool executeImmediately = false);

    /**
     * @brief 添加一次性任务
     * @param name 任务名称
     * @param delay 延迟执行时间（毫秒）
     * @param callback 回调函数
     * @return int 任务ID，-1表示失败
     */
    int addOnceTask(const String& name, unsigned long delay, std::function<void()> callback);

    /**
     * @brief 启用/禁用任务
     * @param taskId 任务ID
     * @param enabled 是否启用
     * @return true 设置成功
     * @return false 设置失败
     */
    bool setTaskEnabled(int taskId, bool enabled);

    /**
     * @brief 处理任务调度（需要在主循环中调用）
     */
    void handleTasks();

    /**
     * @brief 获取任务数量
     * @return int 任务数量
     */
    int getTaskCount() const;

    /**
     * @brief 获取启用的任务数量
     * @return int 启用的任务数量
     */
    int getEnabledTaskCount() const;

    /**
     * @brief 获取任务信息
     * @param taskId 任务ID
     * @return String 任务信息，空字符串表示未找到
     */
    String getTaskInfo(int taskId) const;

    /**
     * @brief 获取所有任务信息
     * @return String 所有任务信息
     */
    String getAllTasksInfo() const;

    /**
     * @brief 清理所有任务
     */
    void clearAllTasks();

    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError() const;

    /**
     * @brief 启用调试模式
     * @param enable 是否启用
     */
    void setDebugMode(bool enable);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    TaskScheduler();

    /**
     * @brief 析构函数
     */
    ~TaskScheduler();

    /**
     * @brief 禁用拷贝构造函数
     */
    TaskScheduler(const TaskScheduler&) = delete;

    /**
     * @brief 禁用赋值操作符
     */
    TaskScheduler& operator=(const TaskScheduler&) = delete;

    /**
     * @brief 生成新的任务ID
     * @return int 新的任务ID
     */
    int generateTaskId();

    /**
     * @brief 查找任务
     * @param taskId 任务ID
     * @return ScheduledTask* 任务指针，nullptr表示未找到
     */
    ScheduledTask* findTask(int taskId);

    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);

    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debugPrint(const String& message);

private:
    bool initialized;                       ///< 是否已初始化
    std::vector<ScheduledTask> tasks;       ///< 任务列表
    int nextTaskId;                         ///< 下一个任务ID
    String lastError;                       ///< 最后的错误信息
    bool debugMode;                         ///< 调试模式
    unsigned long lastTaskCheck;           ///< 上次任务检查时间
};

#endif // TASK_SCHEDULER_H