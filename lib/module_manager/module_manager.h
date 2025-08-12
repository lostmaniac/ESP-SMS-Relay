/**
 * @file module_manager.h
 * @brief 模块管理器 - 统一管理所有功能模块的初始化和调用
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 统一管理所有功能模块的生命周期
 * 2. 提供简洁的API供其他模块调用
 * 3. 处理模块间的依赖关系
 * 4. 提供模块状态查询功能
 */

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <Arduino.h>

// 前向声明
class PhoneCaller;
class HttpClient;
class AtCommandHandler;

/**
 * @enum ModuleStatus
 * @brief 模块状态枚举
 */
enum ModuleStatus {
    MODULE_NOT_INITIALIZED,  ///< 模块未初始化
    MODULE_INITIALIZING,     ///< 模块正在初始化
    MODULE_READY,           ///< 模块就绪
    MODULE_ERROR,           ///< 模块错误
    MODULE_DISABLED         ///< 模块已禁用
};

/**
 * @enum ModuleType
 * @brief 模块类型枚举
 */
enum ModuleType {
    MODULE_GSM_BASIC,       ///< GSM基础模块
    MODULE_AT_COMMAND,      ///< AT命令处理模块
    MODULE_HTTP_CLIENT,     ///< HTTP客户端模块
    MODULE_PHONE_CALLER,    ///< 电话拨打模块
    MODULE_UART_MONITOR,    ///< 串口监听模块
    MODULE_COUNT            ///< 模块总数
};

/**
 * @class ModuleManager
 * @brief 模块管理器类
 * 
 * 负责管理所有功能模块的初始化、状态监控和调用接口
 */
class ModuleManager {
public:
    /**
     * @brief 构造函数
     */
    ModuleManager();
    
    /**
     * @brief 析构函数
     */
    ~ModuleManager();
    
    /**
     * @brief 初始化所有模块
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initializeAllModules();
    
    /**
     * @brief 初始化指定模块
     * @param moduleType 模块类型
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initializeModule(ModuleType moduleType);
    
    /**
     * @brief 获取模块状态
     * @param moduleType 模块类型
     * @return ModuleStatus 模块状态
     */
    ModuleStatus getModuleStatus(ModuleType moduleType);
    
    /**
     * @brief 检查所有模块是否就绪
     * @return true 所有模块就绪
     * @return false 存在未就绪模块
     */
    bool areAllModulesReady();
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 运行模块测试
     * @return true 测试通过
     * @return false 测试失败
     */
    bool runModuleTests();
    
    /**
     * @brief 启动后台任务
     * @return true 启动成功
     * @return false 启动失败
     */
    bool startBackgroundTasks();
    
    /**
     * @brief 获取单例实例
     * @return ModuleManager& 单例引用
     */
    static ModuleManager& getInstance();

private:
    ModuleStatus moduleStatuses[MODULE_COUNT];  ///< 模块状态数组
    String lastError;                          ///< 最后的错误信息
    bool initialized;                          ///< 管理器是否已初始化
    
    /**
     * @brief 初始化GSM基础模块
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initGsmBasicModule();
    
    /**
     * @brief 初始化电话拨打模块
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initPhoneCallerModule();
    
    /**
     * @brief 初始化AT命令处理模块
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initAtCommandModule();
    
    /**
     * @brief 初始化HTTP客户端模块
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initHttpClientModule();
    
    /**
     * @brief 初始化串口监听模块
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initUartMonitorModule();
    
    /**
     * @brief 设置模块状态
     * @param moduleType 模块类型
     * @param status 模块状态
     */
    void setModuleStatus(ModuleType moduleType, ModuleStatus status);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
};

// 全局访问函数声明
/**
 * @brief 获取电话拨打器实例
 * @return PhoneCaller* 电话拨打器指针，如果未初始化则返回nullptr
 */
PhoneCaller* getPhoneCaller();

/**
 * @brief 获取HTTP客户端实例
 * @return HttpClient* HTTP客户端指针，如果未初始化则返回nullptr
 */
HttpClient* getHttpClient();

/**
 * @brief 获取AT命令处理器实例
 * @return AtCommandHandler* AT命令处理器指针，如果未初始化则返回nullptr
 */
AtCommandHandler* getAtCommandHandler();

#endif // MODULE_MANAGER_H