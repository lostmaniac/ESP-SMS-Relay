/**
 * @file carrier_config.cpp
 * @brief 运营商识别和APN配置模块实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "carrier_config.h"
#include <Arduino.h>

// 中国移动IMSI前缀
static const String CHINA_MOBILE_PREFIXES[] = {
    "46000", "46002", "46004", "46007", "46008"
};
static const int CHINA_MOBILE_PREFIX_COUNT = 5;

// 中国联通IMSI前缀
static const String CHINA_UNICOM_PREFIXES[] = {
    "46001", "46006", "46009"
};
static const int CHINA_UNICOM_PREFIX_COUNT = 3;

// 中国电信IMSI前缀
static const String CHINA_TELECOM_PREFIXES[] = {
    "46003", "46005", "46011"
};
static const int CHINA_TELECOM_PREFIX_COUNT = 3;

/**
 * @brief 构造函数
 */
CarrierConfig::CarrierConfig() {
    initializeCarrierData();
}

/**
 * @brief 析构函数
 */
CarrierConfig::~CarrierConfig() {
    // 析构函数实现
}

/**
 * @brief 根据IMSI识别运营商
 * @param imsi IMSI号码
 * @return CarrierType 运营商类型
 */
CarrierType CarrierConfig::identifyCarrier(const String& imsi) {
    if (!isValidImsi(imsi)) {
        Serial.println("无效的IMSI号码");
        return CARRIER_UNKNOWN;
    }
    
    // 检查中国移动
    if (matchesCarrierPrefix(imsi, CHINA_MOBILE_PREFIXES, CHINA_MOBILE_PREFIX_COUNT)) {
        Serial.println("识别为中国移动");
        return CARRIER_CHINA_MOBILE;
    }
    
    // 检查中国联通
    if (matchesCarrierPrefix(imsi, CHINA_UNICOM_PREFIXES, CHINA_UNICOM_PREFIX_COUNT)) {
        Serial.println("识别为中国联通");
        return CARRIER_CHINA_UNICOM;
    }
    
    // 检查中国电信
    if (matchesCarrierPrefix(imsi, CHINA_TELECOM_PREFIXES, CHINA_TELECOM_PREFIX_COUNT)) {
        Serial.println("识别为中国电信");
        return CARRIER_CHINA_TELECOM;
    }
    
    Serial.println("未知运营商");
    return CARRIER_UNKNOWN;
}

/**
 * @brief 获取运营商信息
 * @param carrierType 运营商类型
 * @return CarrierInfo 运营商信息
 */
CarrierInfo CarrierConfig::getCarrierInfo(CarrierType carrierType) {
    CarrierInfo info;
    info.type = carrierType;
    
    switch (carrierType) {
        case CARRIER_CHINA_MOBILE:
            info.name = "中国移动";
            info.apnConfig.apn = "cmnet";
            info.apnConfig.username = "";
            info.apnConfig.password = "";
            info.apnConfig.authType = "NONE";
            info.smsCenterNumber = "+8613800100500";
            break;
            
        case CARRIER_CHINA_UNICOM:
            info.name = "中国联通";
            info.apnConfig.apn = "3gnet";
            info.apnConfig.username = "";
            info.apnConfig.password = "";
            info.apnConfig.authType = "NONE";
            info.smsCenterNumber = "+8613010112500";
            break;
            
        case CARRIER_CHINA_TELECOM:
            info.name = "中国电信";
            info.apnConfig.apn = "ctnet";
            info.apnConfig.username = "ctnet@mycdma.cn";
            info.apnConfig.password = "vnet.mobi";
            info.apnConfig.authType = "PAP";
            info.smsCenterNumber = "+8613800100500";
            break;
            
        default:
            info.name = "未知运营商";
            info.apnConfig.apn = "";
            info.apnConfig.username = "";
            info.apnConfig.password = "";
            info.apnConfig.authType = "NONE";
            info.smsCenterNumber = "";
            break;
    }
    
    return info;
}

/**
 * @brief 根据IMSI获取运营商信息
 * @param imsi IMSI号码
 * @return CarrierInfo 运营商信息
 */
CarrierInfo CarrierConfig::getCarrierInfoByImsi(const String& imsi) {
    CarrierType carrierType = identifyCarrier(imsi);
    return getCarrierInfo(carrierType);
}

/**
 * @brief 获取运营商名称
 * @param carrierType 运营商类型
 * @return String 运营商名称
 */
String CarrierConfig::getCarrierName(CarrierType carrierType) {
    CarrierInfo info = getCarrierInfo(carrierType);
    return info.name;
}

/**
 * @brief 获取APN配置
 * @param carrierType 运营商类型
 * @return ApnConfig APN配置
 */
ApnConfig CarrierConfig::getApnConfig(CarrierType carrierType) {
    CarrierInfo info = getCarrierInfo(carrierType);
    return info.apnConfig;
}

/**
 * @brief 获取短信中心号码
 * @param carrierType 运营商类型
 * @return String 短信中心号码
 */
String CarrierConfig::getSmsCenterNumber(CarrierType carrierType) {
    CarrierInfo info = getCarrierInfo(carrierType);
    return info.smsCenterNumber;
}

/**
 * @brief 检查IMSI是否有效
 * @param imsi IMSI号码
 * @return true IMSI有效
 * @return false IMSI无效
 */
bool CarrierConfig::isValidImsi(const String& imsi) {
    // IMSI应该是15位数字
    if (imsi.length() != 15) {
        return false;
    }
    
    // 检查是否全为数字
    for (int i = 0; i < imsi.length(); i++) {
        if (!isdigit(imsi.charAt(i))) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 获取单例实例
 * @return CarrierConfig& 单例引用
 */
CarrierConfig& CarrierConfig::getInstance() {
    static CarrierConfig instance;
    return instance;
}

/**
 * @brief 初始化运营商配置数据
 */
void CarrierConfig::initializeCarrierData() {
    Serial.println("运营商配置模块初始化完成");
}

/**
 * @brief 检查IMSI前缀是否匹配运营商
 * @param imsi IMSI号码
 * @param prefixes 运营商前缀数组
 * @param count 前缀数量
 * @return true 匹配
 * @return false 不匹配
 */
bool CarrierConfig::matchesCarrierPrefix(const String& imsi, const String* prefixes, int count) {
    for (int i = 0; i < count; i++) {
        if (imsi.startsWith(prefixes[i])) {
            Serial.printf("IMSI %s 匹配前缀 %s\n", imsi.c_str(), prefixes[i].c_str());
            return true;
        }
    }
    return false;
}