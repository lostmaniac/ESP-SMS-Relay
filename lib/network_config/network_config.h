/**
 * @file network_config.h
 * @brief 网络配置管理器 - 集成运营商识别和APN自动配置功能
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 在系统启动时自动识别SIM卡运营商
 * 2. 根据运营商自动配置APN
 * 3. 配置短信中心号码
 * 4. 管理网络连接状态
 */

#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <Arduino.h>
#include "../carrier_config/carrier_config.h"
#include "../gsm_service/gsm_service.h"
#include "../http_client/http_client.h"

/**
 * @enum NetworkConfigStatus
 * @brief 网络配置状态枚举
 */
enum NetworkConfigStatus {
    NETWORK_CONFIG_NOT_STARTED,     ///< 未开始配置
    NETWORK_CONFIG_IN_PROGRESS,     ///< 配置进行中
    NETWORK_CONFIG_SUCCESS,         ///< 配置成功
    NETWORK_CONFIG_FAILED           ///< 配置失败
};

/**
 * @struct NetworkConfigResult
 * @brief 网络配置结果结构体
 */
struct NetworkConfigResult {
    NetworkConfigStatus status;     ///< 配置状态
    CarrierType carrierType;        ///< 识别的运营商类型
    String carrierName;             ///< 运营商名称
    String imsi;                    ///< IMSI号码
    ApnConfig apnConfig;            ///< 使用的APN配置
    String smsCenterNumber;         ///< 配置的短信中心号码
    String errorMessage;            ///< 错误信息（如果有）
};

/**
 * @class NetworkConfig
 * @brief 网络配置管理器类
 * 
 * 提供自动运营商识别和网络配置功能
 */
class NetworkConfig {
public:
    /**
     * @brief 构造函数
     */
    NetworkConfig();
    
    /**
     * @brief 析构函数
     */
    ~NetworkConfig();
    
    /**
     * @brief 初始化网络配置管理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 执行自动网络配置
     * @return NetworkConfigResult 配置结果
     */
    NetworkConfigResult autoConfigureNetwork();
    
    /**
     * @brief 手动配置网络
     * @param carrierType 运营商类型
     * @return NetworkConfigResult 配置结果
     */
    NetworkConfigResult configureNetwork(CarrierType carrierType);
    
    /**
     * @brief 获取当前配置状态
     * @return NetworkConfigStatus 配置状态
     */
    NetworkConfigStatus getConfigStatus();
    
    /**
     * @brief 获取最后的配置结果
     * @return NetworkConfigResult 配置结果
     */
    NetworkConfigResult getLastConfigResult();
    
    /**
     * @brief 重新配置网络
     * @return NetworkConfigResult 配置结果
     */
    NetworkConfigResult reconfigureNetwork();
    
    /**
     * @brief 检查网络连接状态
     * @return true 网络已连接
     * @return false 网络未连接
     */
    bool isNetworkReady();
    
    /**
     * @brief 获取当前运营商信息
     * @return CarrierInfo 运营商信息
     */
    CarrierInfo getCurrentCarrierInfo();
    
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
     * @return NetworkConfig& 单例引用
     */
    static NetworkConfig& getInstance();

private:
    NetworkConfigStatus configStatus;      ///< 当前配置状态
    NetworkConfigResult lastResult;        ///< 最后的配置结果
    CarrierInfo currentCarrierInfo;        ///< 当前运营商信息
    String lastError;                      ///< 最后的错误信息
    bool debugMode;                        ///< 调试模式
    bool initialized;                      ///< 是否已初始化
    
    /**
     * @brief 获取IMSI号码
     * @return String IMSI号码
     */
    String getImsiNumber();
    
    /**
     * @brief 配置APN
     * @param apnConfig APN配置
     * @return true 配置成功
     * @return false 配置失败
     */
    bool configureApn(const ApnConfig& apnConfig);
    
    /**
     * @brief 配置短信中心号码
     * @param smsCenterNumber 短信中心号码
     * @return true 配置成功
     * @return false 配置失败
     */
    bool configureSmsCenterNumber(const String& smsCenterNumber);
    
    /**
     * @brief 验证网络配置
     * @return true 配置有效
     * @return false 配置无效
     */
    bool validateNetworkConfig();
    
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
     * @brief 重置配置结果
     */
    void resetConfigResult();
};

#endif // NETWORK_CONFIG_H