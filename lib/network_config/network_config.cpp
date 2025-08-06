/**
 * @file network_config.cpp
 * @brief 网络配置管理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "network_config.h"
#include "../at_command_handler/at_command_handler.h"
#include <Arduino.h>

/**
 * @brief 构造函数
 */
NetworkConfig::NetworkConfig() {
    configStatus = NETWORK_CONFIG_NOT_STARTED;
    debugMode = false;
    initialized = false;
    resetConfigResult();
}

/**
 * @brief 析构函数
 */
NetworkConfig::~NetworkConfig() {
    // 析构函数实现
}

/**
 * @brief 初始化网络配置管理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool NetworkConfig::initialize() {
    if (initialized) {
        return true;
    }
    
    debugPrint("初始化网络配置管理器...");
    
    // 检查GSM服务是否可用
    GsmService& gsmService = GsmService::getInstance();
    if (!gsmService.isModuleOnline()) {
        setError("GSM模块未在线");
        return false;
    }
    
    initialized = true;
    debugPrint("网络配置管理器初始化成功");
    return true;
}

/**
 * @brief 执行自动网络配置
 * @return NetworkConfigResult 配置结果
 */
NetworkConfigResult NetworkConfig::autoConfigureNetwork() {
    debugPrint("开始自动网络配置...");
    configStatus = NETWORK_CONFIG_IN_PROGRESS;
    resetConfigResult();
    
    // 获取IMSI号码
    String imsi = getImsiNumber();
    if (imsi.length() == 0) {
        setError("无法获取IMSI号码");
        lastResult.status = NETWORK_CONFIG_FAILED;
        lastResult.errorMessage = lastError;
        configStatus = NETWORK_CONFIG_FAILED;
        return lastResult;
    }
    
    lastResult.imsi = imsi;
    debugPrint("获取到IMSI: " + imsi);
    
    // 识别运营商
    CarrierConfig& carrierConfig = CarrierConfig::getInstance();
    CarrierType carrierType = carrierConfig.identifyCarrier(imsi);
    
    if (carrierType == CARRIER_UNKNOWN) {
        setError("无法识别运营商类型");
        lastResult.status = NETWORK_CONFIG_FAILED;
        lastResult.errorMessage = lastError;
        configStatus = NETWORK_CONFIG_FAILED;
        return lastResult;
    }
    
    // 获取运营商信息
    CarrierInfo carrierInfo = carrierConfig.getCarrierInfo(carrierType);
    currentCarrierInfo = carrierInfo;
    lastResult.carrierType = carrierType;
    lastResult.carrierName = carrierInfo.name;
    lastResult.apnConfig = carrierInfo.apnConfig;
    lastResult.smsCenterNumber = carrierInfo.smsCenterNumber;
    
    debugPrint("识别运营商: " + carrierInfo.name);
    debugPrint("APN: " + carrierInfo.apnConfig.apn);
    
    // 配置APN
    if (!configureApn(carrierInfo.apnConfig)) {
        setError("APN配置失败");
        lastResult.status = NETWORK_CONFIG_FAILED;
        lastResult.errorMessage = lastError;
        configStatus = NETWORK_CONFIG_FAILED;
        return lastResult;
    }
    
    // 配置短信中心号码（仅在当前号码为空或不同时设置）
    if (carrierInfo.smsCenterNumber.length() > 0) {
        GsmService& gsmService = GsmService::getInstance();
        String currentSca = gsmService.smsCenterNumber;
        if (currentSca.length() == 0 || currentSca != carrierInfo.smsCenterNumber) {
            if (!configureSmsCenterNumber(carrierInfo.smsCenterNumber)) {
                debugPrint("警告: 短信中心号码配置失败，但继续执行");
            }
        } else {
            debugPrint("短信中心号码已正确配置: " + currentSca);
        }
    }
    
    // 验证网络配置
    if (!validateNetworkConfig()) {
        setError("网络配置验证失败");
        lastResult.status = NETWORK_CONFIG_FAILED;
        lastResult.errorMessage = lastError;
        configStatus = NETWORK_CONFIG_FAILED;
        return lastResult;
    }
    
    lastResult.status = NETWORK_CONFIG_SUCCESS;
    configStatus = NETWORK_CONFIG_SUCCESS;
    debugPrint("自动网络配置完成");
    
    return lastResult;
}

/**
 * @brief 手动配置网络
 * @param carrierType 运营商类型
 * @return NetworkConfigResult 配置结果
 */
NetworkConfigResult NetworkConfig::configureNetwork(CarrierType carrierType) {
    debugPrint("开始手动网络配置...");
    configStatus = NETWORK_CONFIG_IN_PROGRESS;
    resetConfigResult();
    
    // 获取运营商信息
    CarrierConfig& carrierConfig = CarrierConfig::getInstance();
    CarrierInfo carrierInfo = carrierConfig.getCarrierInfo(carrierType);
    currentCarrierInfo = carrierInfo;
    lastResult.carrierType = carrierType;
    lastResult.carrierName = carrierInfo.name;
    lastResult.apnConfig = carrierInfo.apnConfig;
    lastResult.smsCenterNumber = carrierInfo.smsCenterNumber;
    
    debugPrint("配置运营商: " + carrierInfo.name);
    
    // 配置APN
    if (!configureApn(carrierInfo.apnConfig)) {
        setError("APN配置失败");
        lastResult.status = NETWORK_CONFIG_FAILED;
        lastResult.errorMessage = lastError;
        configStatus = NETWORK_CONFIG_FAILED;
        return lastResult;
    }
    
    // 配置短信中心号码（仅在当前号码为空或不同时设置）
    if (carrierInfo.smsCenterNumber.length() > 0) {
        GsmService& gsmService = GsmService::getInstance();
        String currentSca = gsmService.smsCenterNumber;
        if (currentSca.length() == 0 || currentSca != carrierInfo.smsCenterNumber) {
            if (!configureSmsCenterNumber(carrierInfo.smsCenterNumber)) {
                debugPrint("警告: 短信中心号码配置失败，但继续执行");
            }
        } else {
            debugPrint("短信中心号码已正确配置: " + currentSca);
        }
    }
    
    // 验证网络配置
    if (!validateNetworkConfig()) {
        setError("网络配置验证失败");
        lastResult.status = NETWORK_CONFIG_FAILED;
        lastResult.errorMessage = lastError;
        configStatus = NETWORK_CONFIG_FAILED;
        return lastResult;
    }
    
    lastResult.status = NETWORK_CONFIG_SUCCESS;
    configStatus = NETWORK_CONFIG_SUCCESS;
    debugPrint("手动网络配置完成");
    
    return lastResult;
}

/**
 * @brief 获取当前配置状态
 * @return NetworkConfigStatus 配置状态
 */
NetworkConfigStatus NetworkConfig::getConfigStatus() {
    return configStatus;
}

/**
 * @brief 获取最后的配置结果
 * @return NetworkConfigResult 配置结果
 */
NetworkConfigResult NetworkConfig::getLastConfigResult() {
    return lastResult;
}

/**
 * @brief 重新配置网络
 * @return NetworkConfigResult 配置结果
 */
NetworkConfigResult NetworkConfig::reconfigureNetwork() {
    debugPrint("重新配置网络...");
    return autoConfigureNetwork();
}

/**
 * @brief 检查网络连接状态
 * @return true 网络已连接
 * @return false 网络未连接
 */
bool NetworkConfig::isNetworkReady() {
    GsmService& gsmService = GsmService::getInstance();
    GsmNetworkStatus networkStatus = gsmService.getNetworkStatus();
    
    return (networkStatus == GSM_NETWORK_REGISTERED_HOME || 
            networkStatus == GSM_NETWORK_REGISTERED_ROAMING);
}

/**
 * @brief 获取当前运营商信息
 * @return CarrierInfo 运营商信息
 */
CarrierInfo NetworkConfig::getCurrentCarrierInfo() {
    return currentCarrierInfo;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String NetworkConfig::getLastError() {
    return lastError;
}

/**
 * @brief 设置调试模式
 * @param enabled 是否启用调试
 */
void NetworkConfig::setDebugMode(bool enabled) {
    debugMode = enabled;
}

/**
 * @brief 获取单例实例
 * @return NetworkConfig& 单例引用
 */
NetworkConfig& NetworkConfig::getInstance() {
    static NetworkConfig instance;
    return instance;
}

/**
 * @brief 获取IMSI号码
 * @return String IMSI号码
 */
String NetworkConfig::getImsiNumber() {
    GsmService& gsmService = GsmService::getInstance();
    return gsmService.getImsi();
}

/**
 * @brief 配置APN
 * @param apnConfig APN配置
 * @return true 配置成功
 * @return false 配置失败
 */
bool NetworkConfig::configureApn(const ApnConfig& apnConfig) {
    debugPrint("配置APN: " + apnConfig.apn);
    
    // 获取HTTP客户端实例
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    GsmService& gsmService = GsmService::getInstance();
    HttpClient httpClient(atHandler, gsmService);
    
    // 配置并激活APN
    bool result = httpClient.configureAndActivateApn(
        apnConfig.apn, 
        apnConfig.username, 
        apnConfig.password
    );
    
    if (!result) {
        setError("APN配置失败: " + httpClient.getLastError());
        return false;
    }
    
    debugPrint("APN配置成功");
    return true;
}

/**
 * @brief 配置短信中心号码
 * @param smsCenterNumber 短信中心号码
 * @return true 配置成功
 * @return false 配置失败
 */
bool NetworkConfig::configureSmsCenterNumber(const String& smsCenterNumber) {
    debugPrint("配置短信中心号码: " + smsCenterNumber);
    
    GsmService& gsmService = GsmService::getInstance();
    bool result = gsmService.setSmsCenterNumber(smsCenterNumber);
    
    if (!result) {
        setError("短信中心号码配置失败: " + gsmService.getLastError());
        return false;
    }
    
    // 更新GSM服务中的缓存
    gsmService.smsCenterNumber = smsCenterNumber;
    debugPrint("短信中心号码配置成功");
    return true;
}

/**
 * @brief 验证网络配置
 * @return true 配置有效
 * @return false 配置无效
 */
bool NetworkConfig::validateNetworkConfig() {
    debugPrint("验证网络配置...");
    
    // 检查网络注册状态
    if (!isNetworkReady()) {
        setError("网络未注册");
        return false;
    }
    
    // 检查PDP上下文状态
    AtCommandHandler& atHandler = AtCommandHandler::getInstance();
    GsmService& gsmService = GsmService::getInstance();
    HttpClient httpClient(atHandler, gsmService);
    
    if (!httpClient.isPdpContextActive()) {
        setError("PDP上下文未激活");
        return false;
    }
    
    debugPrint("网络配置验证成功");
    return true;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void NetworkConfig::setError(const String& error) {
    lastError = error;
    if (debugMode) {
        Serial.printf("[网络配置] 错误: %s\n", error.c_str());
    }
}

/**
 * @brief 打印调试信息
 * @param message 调试信息
 */
void NetworkConfig::debugPrint(const String& message) {
    if (debugMode) {
        Serial.printf("[网络配置] %s\n", message.c_str());
    }
}

/**
 * @brief 重置配置结果
 */
void NetworkConfig::resetConfigResult() {
    lastResult.status = NETWORK_CONFIG_NOT_STARTED;
    lastResult.carrierType = CARRIER_UNKNOWN;
    lastResult.carrierName = "";
    lastResult.imsi = "";
    lastResult.apnConfig.apn = "";
    lastResult.apnConfig.username = "";
    lastResult.apnConfig.password = "";
    lastResult.apnConfig.authType = "";
    lastResult.smsCenterNumber = "";
    lastResult.errorMessage = "";
}