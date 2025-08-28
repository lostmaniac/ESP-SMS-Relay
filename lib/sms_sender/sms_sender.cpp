/**
 * @file sms_sender.cpp
 * @brief 短信发送模块实现 - 使用pdulib库进行PDU编码和短信发送
 * @author ESP-SMS-Relay Project
 * @version 1.0.0
 * @date 2024
 */

#include "sms_sender.h"
#include "constants.h"
#include "esp_task_wdt.h"
#include <HardwareSerial.h>

// 引用外部声明的串口对象
extern HardwareSerial simSerial;

/**
 * @brief 构造函数
 * @param buffer_size PDU编码缓冲区大小
 */
SmsSender::SmsSender(int buffer_size) 
    : pdu_encoder_(nullptr), initialized_(false) {
    // 创建PDU编码器实例
    pdu_encoder_ = new PDU(buffer_size);
    if (!pdu_encoder_) {
        last_error_ = "内存分配失败：无法创建PDU编码器";
    }
}

/**
 * @brief 析构函数
 */
SmsSender::~SmsSender() {
    cleanup();
}

/**
 * @brief 初始化短信发送器
 * @param sca_number 短信中心号码
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool SmsSender::initialize(const String& sca_number) {
    if (!pdu_encoder_) {
        last_error_ = "PDU编码器未创建";
        return false;
    }
    
    if (sca_number.length() == 0) {
        last_error_ = "短信中心号码不能为空";
        return false;
    }
    
    // 设置短信中心号码
    sca_number_ = sca_number;
    pdu_encoder_->setSCAnumber(sca_number_.c_str());
    
    // 检查网络状态
    if (!isNetworkReady()) {
        last_error_ = "网络未就绪";
        return false;
    }
    
    // 设置PDU模式
    if (!sendAtCommand("AT+CMGF=0", "OK", 3000)) {
        last_error_ = "设置PDU模式失败";
        return false;
    }
    
    initialized_ = true;
    last_error_ = "";
    return true;
}

/**
 * @brief 发送短信
 * @param recipient 接收方号码
 * @param message 短信内容
 * @return SmsSendResult 发送结果
 */
SmsSendResult SmsSender::sendSms(const String& recipient, const String& message) {
    if (!initialized_) {
        last_error_ = "短信发送器未初始化";
        return SMS_ERROR_SCA_NOT_SET;
    }
    
    if (!pdu_encoder_) {
        last_error_ = "PDU编码器不可用";
        return SMS_ERROR_ENCODE_FAILED;
    }
    
    // 验证参数
    if (recipient.length() == 0 || message.length() == 0) {
        last_error_ = "接收方号码或短信内容不能为空";
        return SMS_ERROR_INVALID_PARAMETER;
    }
    
    if (!validatePhoneNumber(recipient)) {
        last_error_ = "接收方号码格式无效";
        return SMS_ERROR_INVALID_PARAMETER;
    }
    
    // 检查网络状态
    if (!isNetworkReady()) {
        last_error_ = "网络未就绪";
        return SMS_ERROR_NETWORK_NOT_READY;
    }
    
    // 开始发送短信
    
    // 进行PDU编码
    int tpdu_length = pdu_encoder_->encodePDU(recipient.c_str(), message.c_str());
    
    if (tpdu_length < 0) {
        // 处理编码错误
        switch (tpdu_length) {
            case PDU::UCS2_TOO_LONG:
                last_error_ = "UCS2消息过长";
                break;
            case PDU::GSM7_TOO_LONG:
                last_error_ = "GSM7消息过长";
                break;
            case PDU::WORK_BUFFER_TOO_SMALL:
                last_error_ = "工作缓冲区太小";
                break;
            case PDU::ADDRESS_FORMAT:
                last_error_ = "地址格式错误";
                break;
            case PDU::MULTIPART_NUMBERS:
                last_error_ = "多部分消息编号错误";
                break;
            case PDU::ALPHABET_8BIT_NOT_SUPPORTED:
                last_error_ = "不支持8位字母表";
                break;
            default:
                last_error_ = "PDU编码失败，未知错误";
                break;
        }
        return SMS_ERROR_ENCODE_FAILED;
    }
    
    // 获取编码后的PDU数据
    const char* pdu_data = pdu_encoder_->getSMS();
    if (!pdu_data) {
        last_error_ = "获取PDU数据失败";
        return SMS_ERROR_ENCODE_FAILED;
    }
    
    // PDU编码成功
    
    // 发送PDU数据
    if (!sendPduData(pdu_data, tpdu_length)) {
        return SMS_ERROR_SEND_TIMEOUT;
    }
    
    last_error_ = "";
    return SMS_SUCCESS;
}

/**
 * @brief 发送文本模式短信（仅用于启动时测试）
 * @param recipient 接收方号码
 * @param message 短信内容（纯英文数字）
 * @return SmsSendResult 发送结果
 */
SmsSendResult SmsSender::sendTextSms(const String& recipient, const String& message) {
    if (!initialized_) {
        last_error_ = "短信发送器未初始化";
        return SMS_ERROR_SCA_NOT_SET;
    }
    
    // 验证参数
    if (recipient.length() == 0 || message.length() == 0) {
        last_error_ = "接收方号码或短信内容不能为空";
        return SMS_ERROR_INVALID_PARAMETER;
    }
    
    if (!validatePhoneNumber(recipient)) {
        last_error_ = "接收方号码格式无效";
        return SMS_ERROR_INVALID_PARAMETER;
    }
    
    // 检查消息是否适合文本模式
    if (!isSimpleTextMessage(message)) {
        last_error_ = "消息包含非ASCII字符，不适合文本模式";
        return SMS_ERROR_INVALID_PARAMETER;
    }
    
    // 检查网络状态
    if (!isNetworkReady()) {
        last_error_ = "网络未就绪";
        return SMS_ERROR_NETWORK_NOT_READY;
    }
    
    // 切换到文本模式
    if (!sendAtCommand("AT+CMGF=1", "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS)) {
        last_error_ = "切换到文本模式失败";
        return SMS_ERROR_SEND_TIMEOUT;
    }
    
    vTaskDelay(DEFAULT_AT_COMMAND_TIMEOUT_MS);
    
    // 发送文本模式短信
    bool text_send_success = sendTextData(recipient, message);
    
    // 切换回PDU模式
    if (!sendAtCommand("AT+CMGF=0", "OK", DEFAULT_AT_COMMAND_TIMEOUT_MS)) {
        // 切换回PDU模式失败，但短信可能已发送成功
    }
    
    if (!text_send_success) {
        return SMS_ERROR_SEND_TIMEOUT;
    }
    
    last_error_ = "";
    return SMS_SUCCESS;
}

/**
 * @brief 获取最后一次错误的详细信息
 * @return String 错误描述
 */
String SmsSender::getLastError() const {
    return last_error_;
}

/**
 * @brief 检查网络是否就绪
 * @return true 网络已注册
 * @return false 网络未注册
 */
bool SmsSender::isNetworkReady() {
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送AT+CREG?命令
    simSerial.println("AT+CREG?");
    
    unsigned long start_time = millis();
    String response = "";
    
    // 等待响应
    while (millis() - start_time < 3000) {
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
        
        // 检查是否收到完整响应
        if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1) {
            break;
        }
        
        vTaskDelay(1);
    }
    
    // 解析CREG响应
    int cregIndex = response.indexOf("+CREG:");
    if (cregIndex == -1) {
        return false;
    }
    
    // 查找状态值（第二个逗号后的数字）
    int firstCommaIndex = response.indexOf(',', cregIndex);
    if (firstCommaIndex == -1) {
        return false;
    }
    
    int secondCommaIndex = response.indexOf(',', firstCommaIndex + 1);
    int statusStart, statusEnd;
    
    if (secondCommaIndex != -1) {
        // 扩展格式: +CREG: <n>,<stat>,<lac>,<ci>
        statusStart = firstCommaIndex + 1;
        statusEnd = secondCommaIndex;
    } else {
        // 基本格式: +CREG: <n>,<stat>
        statusStart = firstCommaIndex + 1;
        statusEnd = response.indexOf('\r', statusStart);
        if (statusEnd == -1) {
            statusEnd = response.indexOf('\n', statusStart);
        }
        if (statusEnd == -1) {
            statusEnd = response.length();
        }
    }
    
    // 提取状态值
    String statusStr = response.substring(statusStart, statusEnd);
    statusStr.trim();
    int status = statusStr.toInt();
    
    // 状态1表示本地网络注册，状态5表示漫游网络注册
    return (status == 1 || status == 5);
}

/**
 * @brief 设置短信中心号码
 * @param sca_number 短信中心号码
 */
void SmsSender::setScaNumber(const String& sca_number) {
    sca_number_ = sca_number;
    if (pdu_encoder_) {
        pdu_encoder_->setSCAnumber(sca_number_.c_str());
    }
}

/**
 * @brief 发送AT命令并等待响应
 * @param command AT命令
 * @param expected_response 期望的响应
 * @param timeout 超时时间（毫秒）
 * @param wait_for_prompt 是否等待'>'提示符
 * @return true 命令执行成功
 * @return false 命令执行失败
 */
bool SmsSender::sendAtCommand(const String& command, const String& expected_response, 
                             unsigned long timeout, bool wait_for_prompt) {
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送命令
    if (command.length() > 0) {
        simSerial.println(command);
    }
    
    unsigned long start_time = millis();
    String response = "";
    
    if (wait_for_prompt) {
        // 等待'>'提示符
        while (millis() - start_time < timeout) {
            // 重置看门狗，防止长时间等待导致超时
            esp_task_wdt_reset();
            
            if (simSerial.available()) {
                char c = simSerial.read();
                if (c == '>') {
                    return true;
                }
            }
            vTaskDelay(1);
        }
    } else {
        // 等待期望的响应
        while (millis() - start_time < timeout) {
            // 重置看门狗，防止长时间等待导致超时
            esp_task_wdt_reset();
            
            if (simSerial.available()) {
                char c = simSerial.read();
                response += c;
            }
            
            if (expected_response.length() == 0 || response.indexOf(expected_response) != -1) {
                return true;
            }
            
            vTaskDelay(1);
        }
    }
    
    last_error_ = "AT命令执行失败: " + command;
    return false;
}

/**
 * @brief 发送PDU数据
 * @param pdu_data PDU数据字符串
 * @param tpdu_length TPDU长度
 * @return true 发送成功
 * @return false 发送失败
 */
bool SmsSender::sendPduData(const char* pdu_data, int tpdu_length) {
    // 构造AT+CMGS命令
    String cmgs_command = "AT+CMGS=" + String(tpdu_length);
    
    // 发送AT+CMGS命令，等待'>'提示符
    if (!sendAtCommand(cmgs_command, "", DEFAULT_AT_COMMAND_TIMEOUT_MS, true)) {
        last_error_ = "发送AT+CMGS命令失败";
        return false;
    }
    
    // pdulib的getSMS()方法返回的PDU数据已经包含了Ctrl+Z结束符
    // 直接发送完整的PDU数据
    simSerial.print(pdu_data);
    
    // 等待发送结果
    unsigned long start_time = millis();
    String response = "";
    
    while (millis() - start_time < DEFAULT_SMS_SEND_TIMEOUT_MS) { // SMS发送超时
        // 重置看门狗，防止长时间等待导致超时
        esp_task_wdt_reset();
        
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
        
        // 检查成功响应
        if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
            return true;
        }
        
        // 检查错误响应
        if (response.indexOf("ERROR") != -1) {
            last_error_ = "PDU发送失败: " + response;
            return false;
        }
        
        vTaskDelay(10);
    }
    
    last_error_ = "PDU发送超时";
    return false;
}

/**
 * @brief 验证手机号码格式
 * @param phone_number 手机号码
 * @return true 格式正确
 * @return false 格式错误
 */
bool SmsSender::validatePhoneNumber(const String& phone_number) {
    if (phone_number.length() < 7 || phone_number.length() > 20) {
        return false;
    }
    
    // 检查是否以+开头（国际格式）
    int start_index = 0;
    if (phone_number.charAt(0) == '+') {
        start_index = 1;
    }
    
    // 检查剩余字符是否都是数字
    for (int i = start_index; i < phone_number.length(); i++) {
        if (!isdigit(phone_number.charAt(i))) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 检测消息是否为纯英文数字（适合文本模式）
 * @param message 短信内容
 * @return true 纯英文数字
 * @return false 包含中文或特殊字符
 */
bool SmsSender::isSimpleTextMessage(const String& message) {
    for (int i = 0; i < message.length(); i++) {
        char c = message.charAt(i);
        // 只允许ASCII字符：字母、数字、空格和基本标点
        if (c < 32 || c > 126) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 发送文本模式数据
 * @param recipient 接收方号码
 * @param message 短信内容
 * @return true 发送成功
 * @return false 发送失败
 */
bool SmsSender::sendTextData(const String& recipient, const String& message) {
    // 构造AT+CMGS命令（文本模式）
    String cmgs_command = "AT+CMGS=\"" + recipient + "\"";
    
    // 发送AT+CMGS命令，等待'>'提示符
    if (!sendAtCommand(cmgs_command, "", DEFAULT_AT_COMMAND_TIMEOUT_MS, true)) {
        last_error_ = "发送AT+CMGS命令失败（文本模式）";
        return false;
    }
    
    // 发送短信内容
    simSerial.print(message);
    simSerial.write(0x1A); // Ctrl+Z结束符
    
    // 等待发送结果
    unsigned long start_time = millis();
    String response = "";
    bool send_success = false;
    
    while (millis() - start_time < DEFAULT_SMS_SEND_TIMEOUT_MS) { // SMS发送超时
        // 重置看门狗，防止长时间等待导致超时
        esp_task_wdt_reset();
        
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
            
            // 检查成功响应
            if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
                send_success = true;
                break;
            }
            
            // 检查错误响应
            if (response.indexOf("ERROR") != -1 || response.indexOf("+CMS ERROR") != -1) {
                break;
            }
        }
        vTaskDelay(10);
    }
    
    if (send_success) {
        return true;
    } else {
        last_error_ = "文本模式发送超时或失败: " + response;
        return false;
    }
}

/**
 * @brief 清理资源
 */
void SmsSender::cleanup() {
    if (pdu_encoder_) {
        delete pdu_encoder_;
        pdu_encoder_ = nullptr;
    }
    initialized_ = false;
}