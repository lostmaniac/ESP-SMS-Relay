/**
 * @file carrier_config.h
 * @brief 运营商识别和APN配置模块 - 根据IMSI自动识别运营商并配置APN
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 根据IMSI前缀识别运营商类型
 * 2. 自动配置对应运营商的APN参数
 * 3. 提供运营商相关的网络配置信息
 */

#ifndef CARRIER_CONFIG_H
#define CARRIER_CONFIG_H

#include <Arduino.h>

/**
 * @enum CarrierType
 * @brief 运营商类型枚举
 */
enum CarrierType {
    CARRIER_UNKNOWN,    ///< 未知运营商
    CARRIER_CHINA_MOBILE,   ///< 中国移动
    CARRIER_CHINA_UNICOM,   ///< 中国联通
    CARRIER_CHINA_TELECOM   ///< 中国电信
};

/**
 * @struct ApnConfig
 * @brief APN配置结构体
 */
struct ApnConfig {
    String apn;         ///< APN名称
    String username;    ///< 用户名
    String password;    ///< 密码
    String authType;    ///< 认证类型
};

/**
 * @struct CarrierInfo
 * @brief 运营商信息结构体
 */
struct CarrierInfo {
    CarrierType type;           ///< 运营商类型
    String name;                ///< 运营商名称
    ApnConfig apnConfig;        ///< APN配置
    String smsCenterNumber;     ///< 短信中心号码
};

/**
 * @class CarrierConfig
 * @brief 运营商配置类
 * 
 * 提供运营商识别和APN自动配置功能
 */
class CarrierConfig {
public:
    /**
     * @brief 构造函数
     */
    CarrierConfig();
    
    /**
     * @brief 析构函数
     */
    ~CarrierConfig();
    
    /**
     * @brief 根据IMSI识别运营商
     * @param imsi IMSI号码
     * @return CarrierType 运营商类型
     */
    CarrierType identifyCarrier(const String& imsi);
    
    /**
     * @brief 获取运营商信息
     * @param carrierType 运营商类型
     * @return CarrierInfo 运营商信息
     */
    CarrierInfo getCarrierInfo(CarrierType carrierType);
    
    /**
     * @brief 获取运营商名称
     * @param carrierType 运营商类型
     * @return String 运营商名称
     */
    String getCarrierName(CarrierType carrierType);
    
    /**
     * @brief 检查IMSI是否有效
     * @param imsi IMSI号码
     * @return true IMSI有效
     * @return false IMSI无效
     */
    bool isValidImsi(const String& imsi);
    
    /**
     * @brief 获取单例实例
     * @return CarrierConfig& 单例引用
     */
    static CarrierConfig& getInstance();

private:
    /**
     * @brief 初始化运营商配置数据
     */
    void initializeCarrierData();
    
    /**
     * @brief 检查IMSI前缀是否匹配运营商
     * @param imsi IMSI号码
     * @param prefixes 运营商前缀数组
     * @param count 前缀数量
     * @return true 匹配
     * @return false 不匹配
     */
    bool matchesCarrierPrefix(const String& imsi, const String* prefixes, int count);
};

#endif // CARRIER_CONFIG_H