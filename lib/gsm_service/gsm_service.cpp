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
    
    // 显示串口配置信息
    Serial.printf("串口配置: 波特率=%d, RX引脚=%d, TX引脚=%d\n", SIM_BAUD_RATE, SIM_RX_PIN, SIM_TX_PIN);
    
    // 清空串口缓冲区
    clearSerialBuffer();
    
    // 等待模块准备就绪（增加等待时间）
    Serial.println("等待GSM模块启动...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
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
        String response = waitForResponse(5000); // 增加超时时间
        Serial.printf("收到响应: '%s'\n", response.c_str());
        
        if (response.indexOf("OK") != -1) {
            moduleResponding = true;
            Serial.println("✓ GSM模块响应正常");
            break;
        }
        
        // 如果没有响应，等待后重试
        if (attempt < 5) {
            Serial.println("模块无响应，等待3秒后重试...");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
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
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            
            // 复位后再次尝试通信
            clearSerialBuffer();
            simSerial.print("AT\r\n");
            simSerial.flush();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            
            String resetResponse = waitForResponse(5000);
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
    if (sendAtCommand("AT+CMGF=0", "OK", 5000)) {
        Serial.println("✓ 短信PDU模式已设置");
    } else {
        Serial.println("⚠️ 短信PDU模式设置失败");
    }
    
    // 配置短信通知模式
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (sendAtCommand("AT+CNMI=2,2,0,0,0", "OK", 5000)) {
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
    if (sendAtCommand("AT+CLIP=1", "OK", 5000)) {
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