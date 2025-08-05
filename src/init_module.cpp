#include "init_module.h"
#include <Arduino.h>
#include "uart_monitor.h"
#include "sms_sender.h"

extern HardwareSerial simSerial;

// 全局短信发送器实例
SmsSender* g_sms_sender = nullptr;

/**
 * @brief 获取短信服务中心号码 (SCA)
 * 
 * @return String 短信中心号码，如果失败则返回空字符串
 */
String getScaAddress() {
    simSerial.println("AT+CSCA?");
    String response = "";
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) { // 3秒超时
        if (simSerial.available()) {
            response += (char)simSerial.read();
        }
        // 检查是否收到了完整的响应
        if (response.indexOf("+CSCA:") != -1 && response.indexOf("OK") != -1) {
            // 解析出号码
            int firstQuote = response.indexOf('"');
            int secondQuote = response.indexOf('"', firstQuote + 1);
            if (firstQuote != -1 && secondQuote != -1) {
                String sca = response.substring(firstQuote + 1, secondQuote);
                Serial.printf("成功获取短信中心号码: %s\n", sca.c_str());
                // 清空串口缓冲区，防止旧数据干扰
                while(simSerial.available()) simSerial.read();
                return sca;
            }
        }
    }
    Serial.println("获取短信中心号码失败。");
    return "";
}

/**
 * @brief 发送测试短信
 * 
 * 使用SmsSender类进行文本模式和PDU模式短信发送测试
 */
void sendTestSms(const String& scaAddress)
{
    if (scaAddress.length() == 0) {
        Serial.println("错误: 未能获取到短信中心号码，无法发送短信。");
        return;
    }

    // 创建短信发送器实例
    if (!g_sms_sender) {
        g_sms_sender = new SmsSender(200); // 200字节缓冲区
        if (!g_sms_sender) {
            Serial.println("错误: 无法创建短信发送器实例");
            return;
        }
    }
    
    // 初始化短信发送器
    if (!g_sms_sender->initialize(scaAddress)) {
        Serial.printf("短信发送器初始化失败: %s\n", g_sms_sender->getLastError().c_str());
        return;
    }
    
    Serial.println("=== 开始短信发送测试 ===");
    
    // 1. 发送文本模式测试短信（纯英文数字）
    Serial.println("\n--- 测试1: 文本模式短信 ---");
    SmsSendResult textResult = g_sms_sender->sendTextSms("+8610086", "YE");
    
    switch (textResult) {
        case SMS_SUCCESS:
            Serial.println("文本模式短信发送成功！");
            break;
        case SMS_ERROR_NETWORK_NOT_READY:
            Serial.println("文本模式短信发送失败: 网络未就绪");
            break;
        case SMS_ERROR_SCA_NOT_SET:
            Serial.println("文本模式短信发送失败: 短信中心号码未设置");
            break;
        case SMS_ERROR_ENCODE_FAILED:
            Serial.println("文本模式短信发送失败: 编码失败");
            break;
        case SMS_ERROR_AT_COMMAND_FAILED:
            Serial.println("文本模式短信发送失败: AT命令执行失败");
            break;
        case SMS_ERROR_SEND_TIMEOUT:
            Serial.println("文本模式短信发送失败: 发送超时");
            break;
        case SMS_ERROR_INVALID_PARAMETER:
            Serial.println("文本模式短信发送失败: 参数无效");
            break;
        default:
            Serial.println("文本模式短信发送失败: 未知错误");
            break;
    }
    
    if (textResult != SMS_SUCCESS) {
        Serial.printf("文本模式详细错误信息: %s\n", g_sms_sender->getLastError().c_str());
    }
    Serial.println("\n=== 短信发送测试完成 ===");
}

/**
 * @brief 发送简单AT命令并等待响应
 * @param command AT命令
 * @param expected_response 期望的响应
 * @param timeout 超时时间（毫秒）
 * @return true 命令执行成功
 * @return false 命令执行失败
 */
bool sendSimpleAtCommand(const String& command, const String& expected_response, unsigned long timeout) {
    // 清空串口缓冲区
    while (simSerial.available()) {
        simSerial.read();
    }
    
    // 发送命令
    simSerial.println(command);
    Serial.printf("发送AT命令: %s\n", command.c_str());
    
    unsigned long start_time = millis();
    String response = "";
    
    while (millis() - start_time < timeout) {
        if (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
        
        if (response.indexOf(expected_response) != -1) {
            Serial.printf("AT命令成功，响应: %s\n", response.c_str());
            return true;
        }
        
        vTaskDelay(1);
    }
    
    Serial.printf("AT命令失败，超时或响应不匹配。收到: %s\n", response.c_str());
    return false;
}

/**
 * @brief 模块初始化任务
 * @param pvParameters 任务参数
 */
void init_module_task(void *pvParameters) {
    // 等待模块准备就绪
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    Serial.println("正在初始化模块...");

    // 检查模块是否响应
    if (sendSimpleAtCommand("AT", "OK", 3000)) {
        Serial.println("模块响应正常。");

        // 关闭回显，简化响应解析
        if (!sendSimpleAtCommand("ATE0", "OK", 3000)) {
            Serial.println("关闭回显失败，可能影响响应解析。");
        } else {
            Serial.println("已关闭模块回显。");
        }
    } else {
        Serial.println("模块无响应，请检查接线和电源。");
        vTaskDelete(NULL); // 退出任务
        return;
    }

    // 检查网络注册状态
    Serial.println("正在检查网络注册状态...");
    unsigned long startTime = millis();
    bool registered = false;
    while (millis() - startTime < 30000) { // 等待最多30秒
        if (sendSimpleAtCommand("AT+CREG?", "+CREG: 0,1", 3000) || 
            sendSimpleAtCommand("AT+CREG?", "+CREG: 0,5", 3000)) {
            Serial.println("网络已注册。");
            registered = true;
            break;
        }
        Serial.println("网络未注册，等待2秒后重试...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (!registered) {
        Serial.println("网络注册失败，请检查SIM卡和天线。");
        vTaskDelete(NULL);
        return;
    }

    // 配置新短信通知为URC
    if (sendSimpleAtCommand("AT+CNMI=2,2,0,0,0", "OK", 3000)) {
        Serial.println("新短信通知已配置。");
    } else {
        Serial.println("配置新短信通知失败。");
    }

    Serial.println("模块基础初始化完成。");

    // 获取短信中心号码
    String scaAddress = getScaAddress();

    // 发送测试短信
    sendTestSms(scaAddress);

    Serial.println("正在启动串口监听任务...");

    xTaskCreate(
        uart_monitor_task,   // Task function
        "UartMonitorTask",   // Task name
        10000,               // Stack size (bytes)
        NULL,                // Parameter
        1,                   // Priority
        NULL                 // Task handle
    );

    // 初始化完成后删除此任务
    vTaskDelete(NULL);
}