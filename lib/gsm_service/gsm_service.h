/**
 * @file gsm_service.h
 * @brief GSM基础服务模块 - 提供GSM模块的基础通信和状态管理功能
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. GSM模块的基础AT命令通信
 * 2. 网络注册状态管理
 * 3. 模块状态监控
 * 4. 基础配置管理
 */

#ifndef GSM_SERVICE_H
#define GSM_SERVICE_H

#include <Arduino.h>

/**
 * @enum GsmNetworkStatus
 * @brief GSM网络状态枚举
 */
enum GsmNetworkStatus {
    GSM_NETWORK_NOT_REGISTERED,     ///< 未注册
    GSM_NETWORK_REGISTERED_HOME,    ///< 注册到本地网络
    GSM_NETWORK_SEARCHING,          ///< 正在搜索网络
    GSM_NETWORK_REGISTRATION_DENIED,///< 注册被拒绝
    GSM_NETWORK_UNKNOWN,            ///< 未知状态
    GSM_NETWORK_REGISTERED_ROAMING  ///< 注册到漫游网络
};

/**
 * @enum GsmModuleStatus
 * @brief GSM模块状态枚举
 */
enum GsmModuleStatus {
    GSM_MODULE_OFFLINE,     ///< 模块离线
    GSM_MODULE_ONLINE,      ///< 模块在线
    GSM_MODULE_ERROR,       ///< 模块错误
    GSM_MODULE_INITIALIZING ///< 模块初始化中
};

/**
 * @class GsmService
 * @brief GSM基础服务类
 * 
 * 提供GSM模块的基础通信功能和状态管理
 */
class GsmService {
public:
    /**
     * @brief 构造函数
     */
    GsmService();
    
    /**
     * @brief 析构函数
     */
    ~GsmService();
    
    /**
     * @brief 初始化GSM服务
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 发送AT命令并等待响应
     * @param command AT命令
     * @param expectedResponse 期望的响应
     * @param timeout 超时时间（毫秒）
     * @return true 命令执行成功
     * @return false 命令执行失败
     */
    bool sendAtCommand(const String& command, const String& expectedResponse, unsigned long timeout = 3000);
    
    /**
     * @brief 发送AT命令并获取完整响应
     * @param command AT命令
     * @param timeout 超时时间（毫秒）
     * @return String 响应内容
     */
    String sendAtCommandWithResponse(const String& command, unsigned long timeout = 3000);
    
    /**
     * @brief 检查模块是否在线
     * @return true 模块在线
     * @return false 模块离线
     */
    bool isModuleOnline();
    
    /**
     * @brief 获取网络注册状态
     * @return GsmNetworkStatus 网络状态
     */
    GsmNetworkStatus getNetworkStatus();
    
    /**
     * @brief 等待网络注册
     * @param timeout 超时时间(ms)
     * @return true 注册成功
     * @return false 注册失败
     */
    bool waitForNetworkRegistration(unsigned long timeout = 30000);
    
    /**
     * @brief 获取信号强度
     * @return int 信号强度值（-1表示获取失败）
     */
    int getSignalStrength();
    
    /**
     * @brief 获取SIM卡状态
     * @return true SIM卡就绪
     * @return false SIM卡未就绪
     */
    bool isSimCardReady();
    
    /**
     * @brief 获取IMSI号码
     * @return String IMSI号码，失败返回空字符串
     */
    String getImsi();
    
    /**
     * @brief 获取短信中心号码
     * @return String 短信中心号码，失败返回空字符串
     */
    String getSmsCenterNumber();
    
    /**
     * @brief 设置短信中心号码
     * @param scaNumber 短信中心号码
     * @return true 设置成功
     * @return false 设置失败
     */
    bool setSmsCenterNumber(const String& scaNumber);
    
    /**
     * @brief 配置短信通知模式
     * @return true 配置成功
     * @return false 配置失败
     */
    bool configureSmsNotification();
    
    /**
     * @brief 获取模块状态
     * @return GsmModuleStatus 模块状态
     */
    GsmModuleStatus getModuleStatus();
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 重置模块
     * @return true 重置成功
     * @return false 重置失败
     */
    bool resetModule();
    
    /**
     * @brief 清空串口缓冲区
     */
    void clearSerialBuffer();
    
    /**
     * @brief 获取单例实例
     * @return GsmService& 单例引用
     */
    static GsmService& getInstance();
    
    // 公共成员变量（供其他模块访问）
    String smsCenterNumber;        ///< 短信中心号码（缓存，避免重复获取）

private:
    GsmModuleStatus moduleStatus;   ///< 模块状态
    String lastError;              ///< 最后的错误信息
    bool initialized;              ///< 是否已初始化
    
    /**
     * @brief 解析网络注册状态响应
     * @param response AT+CREG?的响应
     * @return GsmNetworkStatus 网络状态
     */
    GsmNetworkStatus parseNetworkStatus(const String& response);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 等待串口响应
     * @param timeout 超时时间（毫秒）
     * @return String 响应内容
     */
    String waitForResponse(unsigned long timeout);
};

#endif // GSM_SERVICE_H