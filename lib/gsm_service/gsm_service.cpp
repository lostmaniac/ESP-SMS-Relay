/**
 * @file gsm_service.cpp
 * @brief GSM基础服务实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "gsm_service.h"
#include "config_manager.h"
#include "log_manager.h"
#include "../../include/config.h"
#include "../../include/constants.h"
#include <Arduino.h>
#include <time.h>

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
    
    // 显示串口配置信息
    Serial.printf("串口配置: 波特率=%d, RX引脚=%d, TX引脚=%d\n", SIM_BAUD_RATE, SIM_RX_PIN, SIM_TX_PIN);
    
    // 清空串口缓冲区
    clearSerialBuffer();
    
    // 等待模块准备就绪（增加等待时间）
    Serial.println("等待GSM模块启动...");
    vTaskDelay(DEFAULT_GSM_INIT_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    // 多次尝试检查模块响应
    bool moduleResponding = false;
    for (int attempt = 1; attempt <= 5; attempt++) {
        Serial.printf("第%d次尝试连接GSM模块...\n", attempt);
        
        // 彻底清空缓冲区
        clearSerialBuffer();
        
        // 发送简单的AT命令，使用改进的发送方法
        simSerial.print("AT\r\n");
        simSerial.flush();
        Serial.println("发送: AT");
        
        // 等待模块处理
        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        // 等待响应
        String response = waitForResponse(DEFAULT_GSM_INIT_TIMEOUT_MS); // 增加超时时间
        Serial.printf("收到响应: '%s'\n", response.c_str());
        
        if (response.indexOf("OK") != -1) {
            moduleResponding = true;
            Serial.println("✓ GSM模块响应正常");
            break;
        }
        
        // 如果没有响应，等待后重试
        if (attempt < 5) {
            Serial.println("模块无响应，等待3秒后重试...");
            vTaskDelay(DEFAULT_AT_COMMAND_TIMEOUT_MS / portTICK_PERIOD_MS);
        }
    }
    
    if (!moduleResponding) {
        Serial.println("⚠️ 模块无响应，尝试硬件复位...");
        
        // 尝试硬件复位（如果DTR引脚已连接）
        if (DTR_PIN != -1) {
            pinMode(DTR_PIN, OUTPUT);
            digitalWrite(DTR_PIN, HIGH);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            digitalWrite(DTR_PIN, LOW);
            vTaskDelay(DEFAULT_AT_COMMAND_TIMEOUT_MS / portTICK_PERIOD_MS);
            
            // 复位后再次尝试通信
            clearSerialBuffer();
            simSerial.print("AT\r\n");
            simSerial.flush();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            
            String resetResponse = waitForResponse(DEFAULT_GSM_INIT_TIMEOUT_MS);
            if (resetResponse.indexOf("OK") != -1) {
                Serial.println("✓ 硬件复位后模块响应正常");
                moduleResponding = true;
            } else {
                Serial.println("❌ 硬件复位后模块仍无响应");
            }
        }
        
        if (!moduleResponding) {
            setError("模块无响应，请检查接线和电源");
            moduleStatus = GSM_MODULE_ERROR;
            return false;
        }
    }
    
    // 简化初始化流程，只保留基本功能
    LogManager& logger = LogManager::getInstance();
    
    // 保持回显开启，确保通信稳定
    Serial.println("保持AT命令回显开启，确保通信稳定");
    
    // 等待模块稳定
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 设置短信PDU模式（用于PDU解码）
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (sendAtCommand("AT+CMGF=0", "OK", DEFAULT_GSM_INIT_TIMEOUT_MS)) {
        Serial.println("✓ 短信PDU模式已设置");
    } else {
        Serial.println("⚠️ 短信PDU模式设置失败");
    }
    
    // 配置短信通知模式
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (sendAtCommand("AT+CNMI=2,2,0,0,0", "OK", DEFAULT_GSM_INIT_TIMEOUT_MS)) {
        Serial.println("✓ 短信通知模式已配置");
    } else {
        Serial.println("⚠️ 短信通知模式配置失败");
    }
    
    // 获取短信中心号码
    vTaskDelay(500 / portTICK_PERIOD_MS);
    smsCenterNumber = getSmsCenterNumber();
    if (smsCenterNumber.length() > 0) {
        Serial.printf("✓ 短信中心号码: %s\n", smsCenterNumber.c_str());
    } else {
        Serial.println("⚠️ 无法获取短信中心号码");
    }
    
    // 启用电话功能
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (sendAtCommand("AT+CLIP=1", "OK", DEFAULT_GSM_INIT_TIMEOUT_MS)) {
        Serial.println("✓ 来电显示已启用");
    } else {
        Serial.println("⚠️ 来电显示启用失败");
    }
    
    // 启用网络功能
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (sendAtCommand("AT+CGATT=1", "OK", 8000)) {
        Serial.println("✓ 网络附着已启用");
    } else {
        Serial.println("⚠️ 网络附着启用失败");
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
    // 清空缓冲区
    clearSerialBuffer();
    
    // 发送命令前等待，确保模块准备就绪
    vTaskDelay(200 / portTICK_PERIOD_MS);
    
    // 发送命令
    simSerial.print(command);
    simSerial.print("\r\n");
    simSerial.flush(); // 确保数据发送完毕
    Serial.printf("发送AT命令: %s\n", command.c_str());
    
    // 发送命令后等待，让模块开始处理
    vTaskDelay(300 / portTICK_PERIOD_MS);
    
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
    // 清空缓冲区
    clearSerialBuffer();
    
    // 发送命令前等待，确保模块准备就绪
    vTaskDelay(200 / portTICK_PERIOD_MS);
    
    // 发送命令
    simSerial.print(command);
    simSerial.print("\r\n");
    simSerial.flush(); // 确保数据发送完毕
    Serial.printf("发送AT命令: %s\n", command.c_str());
    
    // 发送命令后等待，让模块开始处理
    vTaskDelay(300 / portTICK_PERIOD_MS);
    
    return waitForResponse(timeout);
}

/**
 * @brief 检查模块是否在线
 * @return true 模块在线
 * @return false 模块离线
 */
bool GsmService::isModuleOnline() {
    return sendAtCommand("AT", "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS);
}

/**
 * @brief 获取网络注册状态
 * @return GsmNetworkStatus 网络状态
 */
GsmNetworkStatus GsmService::getNetworkStatus() {
    String response = sendAtCommandWithResponse("AT+CREG?", DEFAULT_AT_COMMAND_TIMEOUT_MS);
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
    String response = sendAtCommandWithResponse("AT+CSQ", DEFAULT_AT_COMMAND_TIMEOUT_MS);
    
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
    String response = sendAtCommandWithResponse("AT+CPIN?", DEFAULT_AT_COMMAND_TIMEOUT_MS);
    return response.indexOf("+CPIN: READY") != -1;
}

/**
 * @brief 获取IMSI号码
 * @return String IMSI号码，失败返回空字符串
 */
String GsmService::getImsi() {
    String response = sendAtCommandWithResponse("AT+CIMI", DEFAULT_GSM_INIT_TIMEOUT_MS);
    
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
    String response = sendAtCommandWithResponse("AT+CSCA?", DEFAULT_AT_COMMAND_TIMEOUT_MS);
    
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
    if (sendAtCommand(command, "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS)) {
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
    if (sendAtCommand("AT+CNMI=2,2,0,0,0", "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS)) {
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
    
    if (sendAtCommand("AT+CFUN=1,1", "OK", DEFAULT_GSM_INIT_TIMEOUT_MS)) {
        // 等待模块重启
        vTaskDelay(DEFAULT_GSM_INIT_TIMEOUT_MS / portTICK_PERIOD_MS);
        
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
    // 更彻底的缓冲区清理
    unsigned long startTime = millis();
    int bytesCleared = 0;
    
    // 清理接收缓冲区
    while (simSerial.available() && (millis() - startTime < 1000)) {
        simSerial.read();
        bytesCleared++;
        vTaskDelay(1);
    }
    
    // 确保发送缓冲区也被清空
    simSerial.flush();
    
    // 额外等待，确保模块处理完毕
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    if (bytesCleared > 0) {
        Serial.printf("清理了 %d 字节的缓冲区数据\n", bytesCleared);
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
    String line = "";
    bool dataReceived = false;
    
    Serial.printf("等待响应，超时时间: %lu ms\n", timeout);
    
    while (millis() - startTime < timeout) {
        if (simSerial.available()) {
            if (!dataReceived) {
                dataReceived = true;
                Serial.println("开始接收数据...");
            }
            
            char c = simSerial.read();
            response += c;
            
            // 简化调试输出，只显示可打印字符
            if (c >= 32 && c <= 126) {
                Serial.printf("%c", c);
            } else if (c == '\n') {
                Serial.print("\n");
            }
            
            // 处理每一行
            if (c == '\n') {
                line.trim();
                
                // 检查是否收到结束标志
                if (line == "OK" || line == "ERROR" || 
                    line.startsWith("ERROR:") || line.startsWith("+CME ERROR:") || 
                    line.startsWith("+CMS ERROR:")) {
                    // 收到结束标志，立即返回
                    Serial.printf("\n收到结束标志: %s\n", line.c_str());
                    return response;
                }
                
                // 对于某些命令，特定的响应行可以作为结束标志（关闭回显后可能不返回OK）
                if (line.startsWith("+CPIN:") || line.startsWith("+CREG:") || 
                    line.startsWith("+CSQ:") || line.startsWith("+CIMI")) {
                    Serial.printf("\n收到有效响应: %s\n", line.c_str());
                    // 等待一小段时间看是否还有OK，如果没有就直接返回
                    unsigned long waitStart = millis();
                    bool foundOK = false;
                    while (millis() - waitStart < 500) { // 等待500ms
                        if (simSerial.available()) {
                            char c = simSerial.read();
                            response += c;
                            if (c == '\n') {
                                String tempLine = "";
                                // 从response末尾向前找到最后一行
                                int lastLF = response.lastIndexOf('\n', response.length() - 2);
                                if (lastLF != -1) {
                                    tempLine = response.substring(lastLF + 1);
                                    tempLine.trim();
                                    if (tempLine == "OK") {
                                        foundOK = true;
                                        break;
                                    }
                                }
                            }
                        }
                        vTaskDelay(1);
                    }
                    if (foundOK) {
                        Serial.println("\n收到OK确认");
                    } else {
                        Serial.println("\n未收到OK，但有效响应已接收");
                    }
                    return response;
                }
                
                // 重置行缓冲
                line = "";
            } else if (c != '\r') {
                // 忽略回车符，只处理换行符
                line += c;
            }
        }
        vTaskDelay(1);
    }
    
    if (!dataReceived) {
        Serial.println("超时：未收到任何数据");
    } else {
        Serial.println("\n超时：数据接收不完整");
    }
    
    return response;
}

/**
 * @brief 获取网络时间
 * @return String 网络时间字符串，格式为"YY/MM/DD,HH:MM:SS+TZ"，失败返回空字符串
 */
String GsmService::getNetworkTime() {
    Serial.println("正在获取网络时间...");
    
    // 发送AT+CCLK?命令获取时钟
    String response = sendAtCommandWithResponse("AT+CCLK?", DEFAULT_GSM_INIT_TIMEOUT_MS);
    
    if (response.length() == 0) {
        setError("获取网络时间超时");
        return "";
    }
    
    // 解析响应: +CCLK: "YY/MM/DD,HH:MM:SS+TZ"
    int cclkIndex = response.indexOf("+CCLK:");
    if (cclkIndex != -1) {
        int firstQuote = response.indexOf('"', cclkIndex);
        if (firstQuote != -1) {
            int secondQuote = response.indexOf('"', firstQuote + 1);
            if (secondQuote != -1) {
                String timeStr = response.substring(firstQuote + 1, secondQuote);
                Serial.printf("获取到网络时间: %s\n", timeStr.c_str());
                return timeStr;
            }
        }
    }
    
    setError("解析网络时间响应失败: " + response);
    return "";
}

/**
 * @brief 获取Unix时间戳
 * @return unsigned long Unix时间戳，失败返回0
 */
unsigned long GsmService::getUnixTimestamp() {
    String networkTime = getNetworkTime();
    if (networkTime.length() == 0) {
        return 0;
    }
    
    // 解析时间字符串: "YY/MM/DD,HH:MM:SS+TZ"
    // 格式示例: "24/12/20,10:30:45+32"
    
    int commaIndex = networkTime.indexOf(',');
    if (commaIndex == -1) {
        setError("时间格式错误: " + networkTime);
        return 0;
    }
    
    String dateStr = networkTime.substring(0, commaIndex);  // "YY/MM/DD"
    String timeStr = networkTime.substring(commaIndex + 1); // "HH:MM:SS+TZ"
    
    // 解析日期部分
    int firstSlash = dateStr.indexOf('/');
    int secondSlash = dateStr.indexOf('/', firstSlash + 1);
    if (firstSlash == -1 || secondSlash == -1) {
        setError("日期格式错误: " + dateStr);
        return 0;
    }
    
    int year = dateStr.substring(0, firstSlash).toInt() + 2000; // YY -> YYYY
    int month = dateStr.substring(firstSlash + 1, secondSlash).toInt();
    int day = dateStr.substring(secondSlash + 1).toInt();
    
    // 解析时间部分和时区
    int plusIndex = timeStr.indexOf('+');
    int minusIndex = timeStr.indexOf('-');
    int tzIndex = (plusIndex != -1) ? plusIndex : minusIndex;
    
    String pureTimeStr = (tzIndex != -1) ? timeStr.substring(0, tzIndex) : timeStr;
    
    // 解析时区偏移（单位：15分钟）
    int timezoneOffset = 0; // 以秒为单位
    if (tzIndex != -1) {
        String tzStr = timeStr.substring(tzIndex + 1);
        int tzValue = tzStr.toInt();
        // 时区值是以15分钟为单位的，例如+32表示+8小时（32*15分钟=480分钟=8小时）
        timezoneOffset = tzValue * 15 * 60; // 转换为秒
        if (timeStr.charAt(tzIndex) == '-') {
            timezoneOffset = -timezoneOffset;
        }
        Serial.printf("解析时区偏移: %s -> %d秒 (%.1f小时)\n", 
                     timeStr.substring(tzIndex).c_str(), 
                     timezoneOffset, 
                     timezoneOffset / 3600.0);
    }
    
    int firstColon = pureTimeStr.indexOf(':');
    int secondColon = pureTimeStr.indexOf(':', firstColon + 1);
    if (firstColon == -1 || secondColon == -1) {
        setError("时间格式错误: " + pureTimeStr);
        return 0;
    }
    
    int hour = pureTimeStr.substring(0, firstColon).toInt();
    int minute = pureTimeStr.substring(firstColon + 1, secondColon).toInt();
    int second = pureTimeStr.substring(secondColon + 1).toInt();
    
    // 使用C标准库的时间处理函数
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    
    // 设置时间结构体
    timeinfo.tm_year = year - 1900;  // tm_year是从1900年开始的年数
    timeinfo.tm_mon = month - 1;     // tm_mon是0-11（0表示1月）
    timeinfo.tm_mday = day;          // 日期
    timeinfo.tm_hour = hour;         // 小时
    timeinfo.tm_min = minute;        // 分钟
    timeinfo.tm_sec = second;        // 秒
    timeinfo.tm_isdst = -1;          // 让系统自动判断夏令时
    
    // 网络时间是本地时间，需要转换为UTC时间戳
    // 首先减去时区偏移得到UTC时间
    timeinfo.tm_hour -= (timezoneOffset / 3600);
    timeinfo.tm_min -= ((timezoneOffset % 3600) / 60);
    
    // 处理小时和分钟的溢出
    if (timeinfo.tm_min < 0) {
        timeinfo.tm_min += 60;
        timeinfo.tm_hour -= 1;
    }
    if (timeinfo.tm_hour < 0) {
        timeinfo.tm_hour += 24;
        timeinfo.tm_mday -= 1;
        if (timeinfo.tm_mday <= 0) {
            timeinfo.tm_mon -= 1;
            if (timeinfo.tm_mon < 0) {
                timeinfo.tm_mon = 11;
                timeinfo.tm_year -= 1;
            }
            // 简化处理：假设每月30天（实际应用中可以更精确）
            timeinfo.tm_mday = 30;
        }
    }
    
    // 使用timegm转换为UTC时间戳（如果可用），否则使用mktime并手动调整
    #ifdef __GLIBC__
        time_t utcTimestamp = timegm(&timeinfo);
    #else
        // ESP32环境下使用mktime，但设置为UTC时间
        time_t utcTimestamp = mktime(&timeinfo);
        // 由于我们已经调整了时区，这里直接使用结果
    #endif
    
    if (utcTimestamp == -1) {
        setError("时间转换失败");
        return 0;
    }
    
    Serial.printf("解析时间: %04d-%02d-%02d %02d:%02d:%02d\n", 
                 year, month, day, hour, minute, second);
    Serial.printf("UTC时间戳: %lu, 时区偏移: %d秒 (%.1f小时)\n", 
                 (unsigned long)utcTimestamp, timezoneOffset, timezoneOffset / 3600.0);
    Serial.printf("UTC时间戳: %lu\n", utcTimestamp);
    
    return utcTimestamp;
}