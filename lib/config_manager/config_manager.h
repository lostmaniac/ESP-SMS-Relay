/**
 * @file config_manager.h
 * @brief 配置管理模块 - 统一管理系统配置
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 系统配置的统一管理
 * 2. 配置参数的读取和设置
 * 3. 配置的持久化存储
 * 4. 配置验证和默认值管理
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @struct UartConfig
 * @brief 串口配置结构体
 */
struct UartConfig {
    int baudRate;           ///< 波特率
    int rxPin;             ///< 接收引脚
    int txPin;             ///< 发送引脚
    int serialNumber;      ///< 串口号
    unsigned long timeout; ///< 超时时间(ms)
};

/**
 * @struct SmsConfig
 * @brief 短信配置结构体
 */
struct SmsConfig {
    String smsCenterNumber;    ///< 短信中心号码
    String testPhoneNumber;    ///< 测试电话号码
    int maxRetries;           ///< 最大重试次数
    unsigned long sendTimeout; ///< 发送超时时间(ms)
    bool enableNotification;   ///< 是否启用新短信通知
};

/**
 * @struct GsmConfig
 * @brief GSM模块配置结构体
 */
struct GsmConfig {
    unsigned long initTimeout;     ///< 初始化超时时间(ms)
    unsigned long commandTimeout;  ///< 命令超时时间(ms)
    int maxInitRetries;           ///< 最大初始化重试次数
    int signalThreshold;          ///< 信号强度阈值
    bool autoReconnect;           ///< 是否自动重连
};

/**
 * @struct SystemConfig
 * @brief 系统配置结构体
 */
struct SystemConfig {
    bool enableDebug;             ///< 是否启用调试模式
    bool runTestsOnStartup;       ///< 启动时是否运行测试
    unsigned long watchdogTimeout; ///< 看门狗超时时间(ms)
    int logLevel;                 ///< 日志级别
    String deviceName;            ///< 设备名称
};

/**
 * @class ConfigManager
 * @brief 配置管理器类
 * 
 * 负责系统配置的统一管理和持久化存储
 */
class ConfigManager {
public:
    /**
     * @brief 构造函数
     */
    ConfigManager();
    
    /**
     * @brief 析构函数
     */
    ~ConfigManager();
    
    /**
     * @brief 初始化配置管理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 加载配置
     * @return true 加载成功
     * @return false 加载失败
     */
    bool loadConfig();
    
    /**
     * @brief 保存配置
     * @return true 保存成功
     * @return false 保存失败
     */
    bool saveConfig();
    
    /**
     * @brief 重置为默认配置
     * @return true 重置成功
     * @return false 重置失败
     */
    bool resetToDefaults();
    
    /**
     * @brief 验证配置
     * @return true 配置有效
     * @return false 配置无效
     */
    bool validateConfig();
    
    // 配置获取方法
    /**
     * @brief 获取串口配置
     * @return UartConfig 串口配置
     */
    UartConfig getUartConfig();
    
    /**
     * @brief 获取短信配置
     * @return SmsConfig 短信配置
     */
    SmsConfig getSmsConfig();
    
    /**
     * @brief 获取GSM配置
     * @return GsmConfig GSM配置
     */
    GsmConfig getGsmConfig();
    
    /**
     * @brief 获取系统配置
     * @return SystemConfig 系统配置
     */
    SystemConfig getSystemConfig();
    
    // 配置设置方法
    /**
     * @brief 设置串口配置
     * @param config 串口配置
     */
    void setUartConfig(const UartConfig& config);
    
    /**
     * @brief 设置短信配置
     * @param config 短信配置
     */
    void setSmsConfig(const SmsConfig& config);
    
    /**
     * @brief 设置GSM配置
     * @param config GSM配置
     */
    void setGsmConfig(const GsmConfig& config);
    
    /**
     * @brief 设置系统配置
     * @param config 系统配置
     */
    void setSystemConfig(const SystemConfig& config);
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 获取单例实例
     * @return ConfigManager& 单例引用
     */
    static ConfigManager& getInstance();
    
    /**
     * @brief 打印当前配置
     */
    void printConfig();

private:
    Preferences preferences;    ///< NVS存储对象
    UartConfig uartConfig;     ///< 串口配置
    SmsConfig smsConfig;       ///< 短信配置
    GsmConfig gsmConfig;       ///< GSM配置
    SystemConfig systemConfig; ///< 系统配置
    String lastError;          ///< 最后的错误信息
    bool initialized;          ///< 是否已初始化
    
    /**
     * @brief 设置默认配置
     */
    void setDefaultConfig();
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
};

#endif // CONFIG_MANAGER_H