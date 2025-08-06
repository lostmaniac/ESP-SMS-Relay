/**
 * @file gsm_service.cpp
 * @brief GSM基础服务实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "gsm_service.h"
#include "config_manager.h"
#include <Arduino.h>

// 外部串口对象
extern HardwareSerial simSerial;

/**
 * @brief 构造函数
 */
GsmService::GsmService() : 
    moduleStatus(GSM_MODULE_OFFLINE),
    lastError(""),
    smsCenterNumber(""),
    initialized(false) {
}

/**
 * @brief 析构函数
 */
GsmService::~GsmService() {
    // 清理资源
}

/**
 * @brief 获取单例实例
 * @return GsmService& 单例引用
 */
GsmService& GsmService::getInstance() {
    static GsmService instance;
    return instance;
}

/**
 * @brief 初始化GSM服务
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool GsmService::initialize() {
    if (initialized) {
        return true;
    }
    
    Serial.println("正在初始化GSM服务...");
    
    // 获取配置
    ConfigManager& configManager = ConfigManager::getInstance();
    GsmConfig gsmConfig = configManager.getGsmConfig();
    
    moduleStatus = GSM_MODULE_INITIALIZING;
    
    // 等待模块准备就绪
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 检查模块是否响应
    if (!isModuleOnline()) {
        setError("模块无响应，请检查接线和电源");
        moduleStatus = GSM_MODULE_ERROR;
        return false;
    }
    
    // 关闭回显，简化响应解析
    if (!sendAtCommand("ATE0", "OK", gsmConfig.commandTimeout)) {
        Serial.println("关闭回显失败，可能影响响应解析。");
    } else {
        Serial.println("已关闭模块回显。");
    }
    
    // 检查SIM卡状态
    if (!isSimCardReady()) {
        setError("SIM卡未就绪");
        moduleStatus = GSM_MODULE_ERROR;
        return false;
    }
    
    // 等待网络注册（使用配置的超时时间）
    if (!waitForNetworkRegistration(gsmConfig.initTimeout)) {
        setError("网络注册失败，请检查SIM卡和天线");
        moduleStatus = GSM_MODULE_ERROR;
        return false;
    }
    
    // 配置短信通知
    if (!configureSmsNotification()) {
        Serial.println("配置短信通知失败，但不影响基础功能。");
    }
    
    // 获取短信中心号码（仅获取一次，后续模块可复用）
    smsCenterNumber = getSmsCenterNumber();
    if (smsCenterNumber.length() > 0) {
        Serial.printf("成功获取短信中心号码: %s\n", smsCenterNumber.c_str());
    } else {
        Serial.println("警告: 无法获取短信中心号码，短信功能可能受影响。");
    }
    
    moduleStatus = GSM_MODULE_ONLINE;
    initialized = true;
    Serial.println("GSM服务初始化完成。");
    return true;
}

/**
 * @brief 发送AT命令并等待响应
 * @param command AT命令
 * @param expectedResponse 期望的响应
 * @param timeout 超时时间（毫秒）
 * @return true 命令执行成功
 * @return false 命令执行失败
 */
bool GsmService::sendAtCommand(const String& command, const String& expectedResponse, unsigned long timeout) {
    clearSerialBuffer();
    
    // 发送命令
    simSerial.println(command);
    Serial.printf("发送AT命令: %s\n", command.c_str());
    
    String response = waitForResponse(timeout);
    
    if (response.indexOf(expectedResponse) != -1) {
        Serial.printf("AT命令成功，响应: %s\n", response.c_str());
        return true;
    }
    
    Serial.printf("AT命令失败，超时或响应不匹配。收到: %s\n", response.c_str());
    setError("AT命令失败: " + command + ", 响应: " + response);
    return false;
}

/**
 * @brief 发送AT命令并获取完整响应
 * @param command AT命令
 * @param timeout 超时时间（毫秒）
 * @return String 响应内容
 */
String GsmService::sendAtCommandWithResponse(const String& command, unsigned long timeout) {
    clearSerialBuffer();
    
    // 发送命令
    simSerial.println(command);
    Serial.printf("发送AT命令: %s\n", command.c_str());
    
    return waitForResponse(timeout);
}

/**
 * @brief 检查模块是否在线
 * @return true 模块在线
 * @return false 模块离线
 */
bool GsmService::isModuleOnline() {
    return sendAtCommand("AT", "OK", 3000);
}

/**
 * @brief 获取网络注册状态
 * @return GsmNetworkStatus 网络状态
 */
GsmNetworkStatus GsmService::getNetworkStatus() {
    String response = sendAtCommandWithResponse("AT+CREG?", 3000);
    return parseNetworkStatus(response);
}

/**
 * @brief 等待网络注册
 * @param timeout 超时时间(ms)
 * @return true 注册成功
 * @return false 注册失败
 */
bool GsmService::waitForNetworkRegistration(unsigned long timeout) {
    Serial.println("等待网络注册...");
    
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        // 获取当前网络状态
        GsmNetworkStatus status = getNetworkStatus();
        
        // 检查是否已注册（本地网络或漫游网络）
        if (status == GSM_NETWORK_REGISTERED_HOME || status == GSM_NETWORK_REGISTERED_ROAMING) {
            Serial.println("网络注册成功");
            return true;
        }
        
        // 输出当前状态信息
        switch (status) {
            case GSM_NETWORK_NOT_REGISTERED:
                Serial.print("[未注册]");
                break;
            case GSM_NETWORK_SEARCHING:
                Serial.print("[搜索中]");
                break;
            case GSM_NETWORK_REGISTRATION_DENIED:
                Serial.print("[注册被拒绝]");
                break;
            default:
                Serial.print("[未知状态]");
                break;
        }
        
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    Serial.println("\n网络注册超时");
    return false;
}

/**
 * @brief 获取信号强度
 * @return int 信号强度值（-1表示获取失败）
 */
int GsmService::getSignalStrength() {
    String response = sendAtCommandWithResponse("AT+CSQ", 3000);
    
    // 解析响应 +CSQ: <rssi>,<ber>
    int csqIndex = response.indexOf("+CSQ:");
    if (csqIndex != -1) {
        int commaIndex = response.indexOf(',', csqIndex);
        if (commaIndex != -1) {
            String rssiStr = response.substring(csqIndex + 6, commaIndex);
            rssiStr.trim();
            int rssi = rssiStr.toInt();
            
            // RSSI值范围: 0-31 (99表示未知)
            if (rssi >= 0 && rssi <= 31) {
                return rssi;
            }
        }
    }
    
    return -1; // 获取失败
}

/**
 * @brief 获取SIM卡状态
 * @return true SIM卡就绪
 * @return false SIM卡未就绪
 */
bool GsmService::isSimCardReady() {
    String response = sendAtCommandWithResponse("AT+CPIN?", 3000);
    return response.indexOf("+CPIN: READY") != -1;
}

/**
 * @brief 获取IMSI号码
 * @return String IMSI号码，失败返回空字符串
 */
String GsmService::getImsi() {
    String response = sendAtCommandWithResponse("AT+CIMI", 5000);
    
    // 查找IMSI号码（通常是15位数字）
    int startIndex = -1;
    for (int i = 0; i < response.length(); i++) {
        if (isdigit(response.charAt(i))) {
            startIndex = i;
            break;
        }
    }
    
    if (startIndex != -1) {
        String imsi = "";
        for (int i = startIndex; i < response.length() && isdigit(response.charAt(i)); i++) {
            imsi += response.charAt(i);
        }
        
        // IMSI应该是15位数字
        if (imsi.length() == 15) {
            Serial.printf("成功获取IMSI: %s\n", imsi.c_str());
            return imsi;
        }
    }
    
    Serial.println("获取IMSI失败。");
    return "";
}

/**
 * @brief 获取短信中心号码
 * @return String 短信中心号码，失败返回空字符串
 */
String GsmService::getSmsCenterNumber() {
    String response = sendAtCommandWithResponse("AT+CSCA?", 3000);
    
    // 解析响应 +CSCA: "+8613800100500",145
    int csca_index = response.indexOf("+CSCA:");
    if (csca_index != -1) {
        int firstQuote = response.indexOf('"', csca_index);
        int secondQuote = response.indexOf('"', firstQuote + 1);
        if (firstQuote != -1 && secondQuote != -1) {
            String sca = response.substring(firstQuote + 1, secondQuote);
            Serial.printf("成功获取短信中心号码: %s\n", sca.c_str());
            return sca;
        }
    }
    
    Serial.println("获取短信中心号码失败。");
    return "";
}

/**
 * @brief 设置短信中心号码
 * @param scaNumber 短信中心号码
 * @return true 设置成功
 * @return false 设置失败
 */
bool GsmService::setSmsCenterNumber(const String& scaNumber) {
    String command = "AT+CSCA=\"" + scaNumber + "\",145";
    if (sendAtCommand(command, "OK", 3000)) {
        smsCenterNumber = scaNumber;
        return true;
    }
    return false;
}

/**
 * @brief 配置短信通知模式
 * @return true 配置成功
 * @return false 配置失败
 */
bool GsmService::configureSmsNotification() {
    // 配置新短信通知为URC模式
    if (sendAtCommand("AT+CNMI=2,2,0,0,0", "OK", 3000)) {
        Serial.println("新短信通知已配置。");
        return true;
    } else {
        Serial.println("配置新短信通知失败。");
        return false;
    }
}

/**
 * @brief 获取模块状态
 * @return GsmModuleStatus 模块状态
 */
GsmModuleStatus GsmService::getModuleStatus() {
    return moduleStatus;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String GsmService::getLastError() {
    return lastError;
}

/**
 * @brief 重置模块
 * @return true 重置成功
 * @return false 重置失败
 */
bool GsmService::resetModule() {
    Serial.println("正在重置GSM模块...");
    
    if (sendAtCommand("AT+CFUN=1,1", "OK", 10000)) {
        // 等待模块重启
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        
        // 重新初始化
        initialized = false;
        return initialize();
    }
    
    return false;
}

/**
 * @brief 清空串口缓冲区
 */
void GsmService::clearSerialBuffer() {
    while (simSerial.available()) {
        simSerial.read();
    }
}

/**
 * @brief 解析网络注册状态响应
 * @param response AT+CREG?的响应
 * @return GsmNetworkStatus 网络状态
 */
GsmNetworkStatus GsmService::parseNetworkStatus(const String& response) {
    // 解析响应 +CREG: <n>,<stat>[,<lac>,<ci>]
    // 支持基本格式和扩展格式
    int cregIndex = response.indexOf("+CREG:");
    if (cregIndex != -1) {
        // 查找第一个逗号（分隔n和stat）
        int firstCommaIndex = response.indexOf(',', cregIndex);
        if (firstCommaIndex != -1) {
            // 查找第二个逗号（分隔stat和lac，如果存在）
            int secondCommaIndex = response.indexOf(',', firstCommaIndex + 1);
            
            // 确定状态值的结束位置
            int statusEnd;
            if (secondCommaIndex != -1) {
                // 扩展格式：+CREG: <n>,<stat>,<lac>,<ci>
                statusEnd = secondCommaIndex;
            } else {
                // 基本格式：+CREG: <n>,<stat>
                statusEnd = response.indexOf('\n', firstCommaIndex);
                if (statusEnd == -1) statusEnd = response.length();
            }
            
            // 提取状态值
            String statusStr = response.substring(firstCommaIndex + 1, statusEnd);
            statusStr.trim();
            int status = statusStr.toInt();
            
            // 调试输出
            Serial.printf("解析CREG响应: %s, 状态值: %d\n", response.c_str(), status);
            
            switch (status) {
                case 0: return GSM_NETWORK_NOT_REGISTERED;
                case 1: return GSM_NETWORK_REGISTERED_HOME;
                case 2: return GSM_NETWORK_SEARCHING;
                case 3: return GSM_NETWORK_REGISTRATION_DENIED;
                case 5: return GSM_NETWORK_REGISTERED_ROAMING;
                default: return GSM_NETWORK_UNKNOWN;
            }
        }
    }
    
    return GSM_NETWORK_UNKNOWN;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void GsmService::setError(const String& error) {
    lastError = error;
    Serial.printf("GSM服务错误: %s\n", error.c_str());
}

/**
 * @brief 等待串口响应
 * @param timeout 超时时间（毫秒）
 * @return String 响应内容
 */
String GsmService::waitForResponse(unsigned long timeout) {
    unsigned long startTime = millis();
    String response = "";
    
    while (millis() - startTime < timeout) {
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
        vTaskDelay(1);
    }
    
    return response;
}