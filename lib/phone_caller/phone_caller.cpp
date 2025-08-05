/**
 * @file phone_caller.cpp
 * @brief 电话拨打模块实现 - 提供拨打电话、等待和挂断功能
 * @author ESP-SMS-Relay Project
 * @version 1.0.0
 * @date 2024
 * 
 * 该模块实现了完整的电话拨打功能，包括：
 * - 拨打电话
 * - 等待指定时间
 * - 挂断电话
 * - 错误处理
 */

#include "phone_caller.h"
#include <HardwareSerial.h>

// 外部串口对象声明
extern HardwareSerial simSerial;

/**
 * @brief 构造函数
 */
PhoneCaller::PhoneCaller() {
    last_error_ = "";
}

/**
 * @brief 析构函数
 */
PhoneCaller::~PhoneCaller() {
    // 清理资源
}

/**
 * @brief 拨打电话
 * @param phone_number 电话号码（支持国际格式，如+8610086）
 * @return PhoneCallResult 拨打结果
 */
PhoneCallResult PhoneCaller::makeCall(const String& phone_number) {
    // 验证电话号码格式
    if (!validatePhoneNumber(phone_number)) {
        last_error_ = "Invalid phone number format";
        return CALL_ERROR_INVALID_NUMBER;
    }
    
    // 检查网络状态
    if (!isNetworkReady()) {
        last_error_ = "Network not ready";
        return CALL_ERROR_NETWORK_NOT_READY;
    }
    
    // 构造拨打电话的AT命令
    String call_command = "ATD" + phone_number + ";";
    
    Serial.println("[PhoneCaller] 拨打电话: " + phone_number);
    Serial.println("[PhoneCaller] 发送命令: " + call_command);
    
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送拨打命令
    simSerial.println(call_command);
    
    // 等待拨打响应
    unsigned long start_time = millis();
    String response = "";
    bool call_initiated = false;
    
    while (millis() - start_time < 15000) { // 15秒超时
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
            Serial.print(c); // 实时打印响应
            
            // 检查拨打成功的响应
            if (response.indexOf("OK") != -1) {
                call_initiated = true;
                break;
            }
            
            // 检查错误响应
            if (response.indexOf("ERROR") != -1 || 
                response.indexOf("BUSY") != -1 ||
                response.indexOf("NO ANSWER") != -1 ||
                response.indexOf("NO CARRIER") != -1) {
                Serial.println("\n[PhoneCaller] 拨打失败: " + response);
                last_error_ = "Call failed: " + response;
                return CALL_ERROR_AT_COMMAND_FAILED;
            }
        }
        delay(10);
    }
    
    if (call_initiated) {
        Serial.println("\n[PhoneCaller] 电话拨打命令发送成功");
        return CALL_SUCCESS;
    } else {
        Serial.println("\n[PhoneCaller] 拨打电话超时");
        last_error_ = "Call timeout";
        return CALL_ERROR_AT_COMMAND_FAILED;
    }
}

/**
 * @brief 拨打电话并等待指定时间后挂断
 * @param phone_number 电话号码
 * @param wait_seconds 等待时间（秒）
 * @return PhoneCallResult 拨打结果
 */
PhoneCallResult PhoneCaller::makeCallAndWait(const String& phone_number, int wait_seconds) {
    Serial.println("[PhoneCaller] 开始拨打电话并等待测试...");
    
    // 拨打电话
    PhoneCallResult result = makeCall(phone_number);
    if (result != CALL_SUCCESS) {
        Serial.println("[PhoneCaller] 拨打电话失败，错误码: " + String(result));
        return result;
    }
    
    Serial.println("[PhoneCaller] 电话拨打成功，开始等待 5 秒...");
    
    // 等待5秒，每秒显示倒计时
    for (int i = 5; i > 0; i--) {
        Serial.println("[PhoneCaller] 倒计时: " + String(i) + " 秒");
        delay(1000);
    }
    
    Serial.println("[PhoneCaller] 等待时间结束，准备挂断电话");
    
    // 挂断电话
    if (!hangupCall()) {
        Serial.println("[PhoneCaller] 挂断电话失败: " + last_error_);
        return CALL_ERROR_HANGUP_FAILED;
    }
    
    Serial.println("[PhoneCaller] 电话拨打测试完成");
    return CALL_SUCCESS;
}

/**
 * @brief 挂断电话
 * @return true 挂断成功
 * @return false 挂断失败
 */
bool PhoneCaller::hangupCall() {
    Serial.println("[PhoneCaller] 开始挂断电话...");
    
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送AT+CHUP挂断命令
    simSerial.println("AT+CHUP");
    
    // 等待响应
    unsigned long start_time = millis();
    String response = "";
    
    while (millis() - start_time < 10000) { // 10秒超时
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
            
            // 检查成功响应
            if (response.indexOf("OK") != -1 ||
                response.indexOf("NO CARRIER") != -1 ||
                response.indexOf("VOICE CALL: END") != -1 ||
                response.indexOf("ERROR") != -1) {
                break;
            }
        }
        delay(10);
    }
    
    // 检查响应
    if (response.indexOf("OK") != -1 ||
        response.indexOf("NO CARRIER") != -1 ||
        response.indexOf("VOICE CALL: END") != -1) {
        Serial.println("\n[PhoneCaller] 电话挂断成功");
        return true;
    } else if (response.indexOf("ERROR") != -1) {
        Serial.println("[PhoneCaller] 挂断命令失败");
        last_error_ = "挂断失败";
        return false;
    } else {
        Serial.println("[PhoneCaller] 挂断命令超时");
        last_error_ = "挂断超时";
        return false;
    }
}

/**
 * @brief 检查通话状态
 * @return true 无通话进行中
 * @return false 有通话进行中
 */
bool PhoneCaller::checkCallStatus() {
    Serial.println("[PhoneCaller] 检查通话状态...");
    
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送查询通话状态命令
    simSerial.println("AT+CLCC");
    
    unsigned long start_time = millis();
    String response = "";
    
    while (millis() - start_time < 5000) {
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
            
            if (response.indexOf("OK") != -1) {
                // 如果响应中包含+CLCC:，说明有通话进行中
                if (response.indexOf("+CLCC:") != -1) {
                    Serial.println("[PhoneCaller] 检测到通话仍在进行中");
                    return false;
                } else {
                    Serial.println("[PhoneCaller] 确认无通话进行中");
                    return true;
                }
            }
        }
        delay(10);
    }
    
    Serial.println("[PhoneCaller] 通话状态检查超时，假设已挂断");
    return true;
}

/**
 * @brief 检查网络状态
 * @return true 网络已注册
 * @return false 网络未注册
 */
bool PhoneCaller::isNetworkReady() {
    // 发送网络注册状态查询命令
    simSerial.println("AT+CREG?");
    
    unsigned long start_time = millis();
    String response = "";
    
    // 等待响应，超时时间5秒
    while (millis() - start_time < 5000) {
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
            
            // 检查是否收到完整响应
            if (response.indexOf("OK") != -1) {
                // 检查网络注册状态
                // +CREG: 0,1 或 +CREG: 0,5 表示已注册
                if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
                    return true;
                }
                break;
            }
        }
        delay(10);
    }
    
    return false;
}

/**
 * @brief 获取最后一次错误的详细信息
 * @return String 错误描述
 */
String PhoneCaller::getLastError() const {
    return last_error_;
}

/**
 * @brief 发送AT命令并等待响应
 * @param command AT命令
 * @param expected_response 期望的响应
 * @param timeout 超时时间（毫秒）
 * @return true 命令执行成功
 * @return false 命令执行失败
 */
bool PhoneCaller::sendAtCommand(const String& command, const String& expected_response, unsigned long timeout) {
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送AT命令
    simSerial.println(command);
    Serial.println("[PhoneCaller] 发送命令: " + command);
    
    unsigned long start_time = millis();
    String response = "";
    
    // 等待响应
    while (millis() - start_time < timeout) {
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
            
            // 检查是否收到期望的响应
            if (response.indexOf(expected_response) != -1) {
                Serial.println("[PhoneCaller] 收到响应: " + response.substring(response.length() - 50));
                return true;
            }
            
            // 检查是否收到错误响应
            if (response.indexOf("ERROR") != -1) {
                Serial.println("[PhoneCaller] 命令执行失败: " + response);
                return false;
            }
        }
        delay(10);
    }
    
    Serial.println("[PhoneCaller] 命令超时: " + command);
    return false;
}

/**
 * @brief 验证电话号码格式
 * @param phone_number 电话号码
 * @return true 格式正确
 * @return false 格式错误
 */
bool PhoneCaller::validatePhoneNumber(const String& phone_number) {
    // 检查号码长度（最少7位，最多15位）
    if (phone_number.length() < 7 || phone_number.length() > 15) {
        return false;
    }
    
    // 检查是否以+开头（国际格式）或直接是数字
    if (phone_number.charAt(0) == '+') {
        // 国际格式，检查后续是否都是数字
        for (int i = 1; i < phone_number.length(); i++) {
            if (!isDigit(phone_number.charAt(i))) {
                return false;
            }
        }
    } else {
        // 本地格式，检查是否都是数字
        for (int i = 0; i < phone_number.length(); i++) {
            if (!isDigit(phone_number.charAt(i))) {
                return false;
            }
        }
    }
    
    return true;
}