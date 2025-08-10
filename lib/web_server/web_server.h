/**
 * @file web_server.h
 * @brief Web服务器模块 - 提供HTTP服务和网页访问功能
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 启动和管理HTTP服务器
 * 2. 处理HTTP请求和响应
 * 3. 提供Web界面访问
 * 4. 处理API接口调用
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "../database_manager/database_manager.h"
#include "../wifi_manager/wifi_manager.h"
#include "../filesystem_manager/filesystem_manager.h"

/**
 * @enum WebServerStatus
 * @brief Web服务器状态枚举
 */
enum WebServerStatus {
    WEB_SERVER_NOT_STARTED,      ///< 未开始
    WEB_SERVER_STARTING,         ///< 启动中
    WEB_SERVER_RUNNING,          ///< 运行中
    WEB_SERVER_STOPPED,          ///< 已停止
    WEB_SERVER_ERROR             ///< 错误状态
};

/**
 * @struct WebServerConfig
 * @brief Web服务器配置结构体
 */
struct WebServerConfig {
    int port;                    ///< 服务器端口
    bool enableCORS;             ///< 是否启用CORS
    bool enableAuth;             ///< 是否启用认证
    String authUsername;         ///< 认证用户名
    String authPassword;         ///< 认证密码
    int maxConnections;          ///< 最大连接数
};

/**
 * @struct WebServerStats
 * @brief Web服务器统计信息结构体
 */
struct WebServerStats {
    unsigned long uptime;        ///< 运行时间（毫秒）
    int totalRequests;           ///< 总请求数
    int activeConnections;       ///< 活跃连接数
    unsigned long lastRequestTime; ///< 最后请求时间
    String lastRequestPath;      ///< 最后请求路径
};

/**
 * @class WebServer
 * @brief Web服务器类
 * 
 * 提供完整的HTTP服务器功能
 */
class WebServerManager {
public:
    /**
     * @brief 构造函数
     */
    WebServerManager();
    
    /**
     * @brief 析构函数
     */
    ~WebServerManager();
    
    /**
     * @brief 初始化Web服务器
     * @param config 服务器配置
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize(const WebServerConfig& config = getDefaultConfig());
    
    /**
     * @brief 启动Web服务器
     * @return true 启动成功
     * @return false 启动失败
     */
    bool start();
    
    /**
     * @brief 停止Web服务器
     * @return true 停止成功
     * @return false 停止失败
     */
    bool stop();
    
    /**
     * @brief 重启Web服务器
     * @return true 重启成功
     * @return false 重启失败
     */
    bool restart();
    
    /**
     * @brief 处理客户端请求（需要在主循环中调用）
     */
    void handleClient();
    
    /**
     * @brief 获取服务器状态
     * @return WebServerStatus 当前状态
     */
    WebServerStatus getStatus();
    
    /**
     * @brief 获取服务器统计信息
     * @return WebServerStats 统计信息
     */
    WebServerStats getStats();
    
    /**
     * @brief 获取服务器配置
     * @return WebServerConfig 当前配置
     */
    WebServerConfig getConfig();
    
    /**
     * @brief 更新服务器配置
     * @param config 新配置
     * @return true 更新成功
     * @return false 更新失败
     */
    bool updateConfig(const WebServerConfig& config);
    
    /**
     * @brief 检查服务器是否运行
     * @return true 正在运行
     * @return false 未运行
     */
    bool isRunning();
    
    /**
     * @brief 获取服务器URL
     * @return String 服务器URL
     */
    String getServerURL();
    
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
     * @return WebServerManager& 单例引用
     */
    static WebServerManager& getInstance();
    
    /**
     * @brief 获取默认配置
     * @return WebServerConfig 默认配置
     */
    static WebServerConfig getDefaultConfig();

private:
    WebServer* server;               ///< HTTP服务器实例
    WebServerStatus status;          ///< 当前状态
    WebServerConfig config;          ///< 服务器配置
    WebServerStats stats;            ///< 统计信息
    String lastError;                ///< 最后的错误信息
    bool debugMode;                  ///< 调试模式
    bool initialized;                ///< 是否已初始化
    unsigned long startTime;         ///< 启动时间
    
    /**
     * @brief 设置路由处理器
     */
    void setupRoutes();
    
    /**
     * @brief 处理根路径请求
     */
    void handleRoot();
    
    /**
     * @brief 处理API请求
     */
    void handleAPI();
    
    /**
     * @brief 处理系统状态请求
     */
    void handleSystemStatus();
    
    /**
     * @brief 处理WiFi配置请求
     */
    void handleWiFiConfig();
    
    /**
     * @brief 处理数据库状态请求
     */
    void handleDatabaseStatus();
    
    /**
     * @brief 处理404错误
     */
    void handleNotFound();
    
    /**
     * @brief 发送JSON响应
     * @param json JSON字符串
     * @param httpCode HTTP状态码
     */
    void sendJSONResponse(const String& json, int httpCode = 200);
    
    /**
     * @brief 发送HTML响应
     * @param html HTML字符串
     * @param httpCode HTTP状态码
     */
    void sendHTMLResponse(const String& html, int httpCode = 200);
    
    /**
     * @brief 发送错误响应
     * @param message 错误信息
     * @param httpCode HTTP状态码
     */
    void sendErrorResponse(const String& message, int httpCode = 500);
    
    /**
     * @brief 检查认证
     * @return true 认证通过
     * @return false 认证失败
     */
    bool checkAuthentication();
    
    /**
     * @brief 添加CORS头
     */
    void addCORSHeaders();
    
    /**
     * @brief 记录请求
     * @param path 请求路径
     */
    void logRequest(const String& path);
    
    /**
     * @brief 生成主页HTML
     * @return String HTML内容
     */
    String generateHomePage();
    
    /**
     * @brief 生成系统状态JSON
     * @return String JSON内容
     */
    String generateSystemStatusJSON();
    
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
     * @brief 更新统计信息
     */
    void updateStats();
};

#endif // WEB_SERVER_H