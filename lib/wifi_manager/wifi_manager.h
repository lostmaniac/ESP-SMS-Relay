/**
 * @file wifi_manager.h
 * @brief WiFi管理器 - 负责WiFi热点模式的管理和配置
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 从数据库读取WiFi配置
 * 2. 启动和管理WiFi热点模式
 * 3. 监控WiFi连接状态
 * 4. 提供WiFi配置更新接口
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include "../database_manager/database_manager.h"

/**
 * @enum WiFiManagerStatus
 * @brief WiFi管理器状态枚举
 */
enum WiFiManagerStatus {
    WIFI_MANAGER_NOT_STARTED,    ///< 未开始
    WIFI_MANAGER_INITIALIZING,   ///< 初始化中
    WIFI_MANAGER_AP_STARTING,    ///< AP启动中
    WIFI_MANAGER_AP_ACTIVE,      ///< AP已激活
    WIFI_MANAGER_ERROR           ///< 错误状态
};

/**
 * @struct WiFiConnectionInfo
 * @brief WiFi连接信息结构体
 */
struct WiFiConnectionInfo {
    int connectedClients;        ///< 已连接客户端数量
    String apIP;                 ///< AP的IP地址
    String apMAC;                ///< AP的MAC地址
    unsigned long uptime;        ///< AP运行时间（毫秒）
    bool isActive;               ///< AP是否激活
};

/**
 * @class WiFiManager
 * @brief WiFi管理器类
 * 
 * 负责WiFi热点模式的完整管理
 */
class WiFiManager {
public:
    /**
     * @brief 构造函数
     */
    WiFiManager();
    
    /**
     * @brief 析构函数
     */
    ~WiFiManager();
    
    /**
     * @brief 初始化WiFi管理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 启动WiFi热点
     * @return true 启动成功
     * @return false 启动失败
     */
    bool startAP();
    
    /**
     * @brief 停止WiFi热点
     * @return true 停止成功
     * @return false 停止失败
     */
    bool stopAP();
    
    /**
     * @brief 重启WiFi热点
     * @return true 重启成功
     * @return false 重启失败
     */
    bool restartAP();
    
    /**
     * @brief 更新WiFi配置
     * @param config 新的AP配置
     * @return true 更新成功
     * @return false 更新失败
     */
    bool updateAPConfig(const APConfig& config);
    
    /**
     * @brief 获取当前WiFi状态
     * @return WiFiManagerStatus 当前状态
     */
    WiFiManagerStatus getStatus();
    
    /**
     * @brief 获取WiFi连接信息
     * @return WiFiConnectionInfo 连接信息
     */
    WiFiConnectionInfo getConnectionInfo();
    
    /**
     * @brief 获取当前AP配置
     * @return APConfig 当前配置
     */
    APConfig getCurrentConfig();
    
    /**
     * @brief 检查AP是否激活
     * @return true AP已激活
     * @return false AP未激活
     */
    bool isAPActive();
    
    /**
     * @brief 获取已连接的客户端数量
     * @return int 客户端数量
     */
    int getConnectedClientsCount();
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试
     */
    void setDebugMode(bool enabled);
    
    /**
     * @brief 获取单例实例
     * @return WiFiManager& 单例引用
     */
    static WiFiManager& getInstance();
    
    /**
     * @brief 处理WiFi事件（需要在主循环中调用）
     */
    void handleEvents();

private:
    WiFiManagerStatus status;           ///< 当前状态
    APConfig currentConfig;             ///< 当前AP配置
    WiFiConnectionInfo connectionInfo;  ///< 连接信息
    String lastError;                   ///< 最后的错误信息
    bool debugMode;                     ///< 调试模式
    bool initialized;                   ///< 是否已初始化
    unsigned long apStartTime;          ///< AP启动时间
    unsigned long lastStatusCheck;      ///< 上次状态检查时间
    
    /**
     * @brief 从数据库加载AP配置
     * @return true 加载成功
     * @return false 加载失败
     */
    bool loadAPConfigFromDatabase();
    
    /**
     * @brief 应用AP配置
     * @param config AP配置
     * @return true 应用成功
     * @return false 应用失败
     */
    bool applyAPConfig(const APConfig& config);
    
    /**
     * @brief 更新连接信息
     */
    void updateConnectionInfo();
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 打印调试信息
     * @param message 调试信息
     */
    void debugPrint(const String& message);
    
    /**
     * @brief 验证AP配置
     * @param config AP配置
     * @return true 配置有效
     * @return false 配置无效
     */
    bool validateAPConfig(const APConfig& config);
};

#endif // WIFI_MANAGER_H