/**
 * @file at_command_handler.cpp
 * @brief AT命令处理器实现
 * @author ESP-SMS-Relay Project
 * @date 2024
 */

#include "at_command_handler.h"
#include "../../include/constants.h"
#include <Arduino.h>

// 外部串口对象
extern HardwareSerial simSerial;

/**
 * @brief 构造函数
 * @param serial 串口对象引用
 */
AtCommandHandler::AtCommandHandler(HardwareSerial& serial) 
    : serialPort(serial), lastError(""), debugMode(false), initialized(false),
      totalCommands(0), successfulCommands(0), failedCommands(0), 
      timeoutCommands(0), lastDiagnosticTime(0), lastFailedCommand(""), 
      lastFailedResponse("") {
}

/**
 * @brief 析构函数
 */
AtCommandHandler::~AtCommandHandler() {
    // 清理资源
}

/**
 * @brief 获取单例实例
 * @return AtCommandHandler& 单例引用
 */
AtCommandHandler& AtCommandHandler::getInstance() {
    // 注意：这个方法现在不应该被使用，因为构造函数需要串口参数
    // 保留此方法是为了向后兼容，但会导致编译错误
    // 应该使用带参数的构造函数创建实例
    static AtCommandHandler* instance = nullptr;
    if (!instance) {
        // 这里需要外部提供串口实例，暂时使用全局simSerial
        extern HardwareSerial simSerial;
        instance = new AtCommandHandler(simSerial);
    }
    return *instance;
}

/**
 * @brief 初始化AT命令处理器
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool AtCommandHandler::initialize() {
    if (initialized) {
        return true;
    }
    
    debugPrint("正在初始化AT命令处理器...");
    
    // 检查串口是否可用
    if (!serialPort) {
        setError("串口未初始化");
        return false;
    }
    
    // 清空缓冲区
    clearBuffer();
    
    // 测试基本AT命令
    AtResponse response = sendCommand("AT", "OK", 1000);
    if (response.result != AT_RESULT_SUCCESS) {
        setError("AT命令测试失败: " + response.response);
        return false;
    }
    
    initialized = true;
    debugPrint("AT命令处理器初始化完成");
    return true;
}

/**
 * @brief 发送AT命令并等待响应
 * @param command AT命令
 * @param expectedResponse 期望的响应
 * @param timeout 超时时间（毫秒）
 * @param retries 重试次数
 * @return AtResponse 命令执行结果
 */
AtResponse AtCommandHandler::sendCommand(const String& command, 
                                        const String& expectedResponse, 
                                        unsigned long timeout,
                                        int retries) {
    AtResponse response;
    response.result = AT_RESULT_ERROR;
    response.response = "";
    
    unsigned long startTime = millis();
    totalCommands++; // 统计总命令数
    
    for (int attempt = 0; attempt <= retries; attempt++) {
        if (attempt > 0) {
            debugPrint("重试命令: " + command + " (第" + String(attempt) + "次)");
            vTaskDelay(500 / portTICK_PERIOD_MS); // 重试间隔
        }
        
        clearBuffer();
        
        // 发送命令
        serialPort.println(command);
        serialPort.flush();
        debugPrint("发送AT命令: " + command);
        
        // 读取响应
        String rawResponse = readResponse(timeout);
        response.response = rawResponse;
        
        // 检查响应
        if (rawResponse.indexOf(expectedResponse) != -1) {
            response.result = AT_RESULT_SUCCESS;
            successfulCommands++; // 统计成功命令数
            debugPrint("命令执行成功，响应: " + rawResponse);
            break;
        } else if (rawResponse.indexOf("ERROR") != -1) {
            response.result = AT_RESULT_ERROR;
            debugPrint("命令执行错误，响应: " + rawResponse);
        } else if (rawResponse.length() == 0) {
            response.result = AT_RESULT_TIMEOUT;
            timeoutCommands++; // 统计超时命令数
            debugPrint("命令执行超时");
        } else {
            response.result = AT_RESULT_INVALID;
            debugPrint("命令响应无效，响应: " + rawResponse);
        }
    }
    
    response.duration = millis() - startTime;
    
    // 统计失败命令并记录详细信息
    if (response.result != AT_RESULT_SUCCESS) {
        failedCommands++;
        lastFailedCommand = command;
        lastFailedResponse = response.response;
        setError("AT命令失败: " + command + ", 响应: " + response.response);
    }
    
    return response;
}

/**
 * @brief 发送AT命令并获取完整响应
 * @param command AT命令
 * @param timeout 超时时间（毫秒）
 * @return AtResponse 命令执行结果
 */
AtResponse AtCommandHandler::sendCommandWithFullResponse(const String& command, 
                                                        unsigned long timeout) {
    AtResponse response;
    response.result = AT_RESULT_SUCCESS;
    
    unsigned long startTime = millis();
    
    clearBuffer();
    
    // 发送命令
    serialPort.println(command);
    serialPort.flush();
    debugPrint("发送AT命令: " + command);
    
    // 读取完整响应
    response.response = readResponse(timeout);
    response.duration = millis() - startTime;
    
    if (response.response.length() == 0) {
        response.result = AT_RESULT_TIMEOUT;
        setError("命令超时: " + command);
    }
    
    debugPrint("完整响应: " + response.response);
    return response;
}

/**
 * @brief 发送原始数据
 * @param data 要发送的数据
 * @param timeout 超时时间（毫秒）
 * @return AtResponse 执行结果
 */
AtResponse AtCommandHandler::sendRawData(const String& data, unsigned long timeout) {
    AtResponse response;
    response.result = AT_RESULT_SUCCESS;
    
    unsigned long startTime = millis();
    
    // 发送原始数据（不添加换行符）
    serialPort.print(data);
    serialPort.flush();
    debugPrint("发送原始数据: " + data);
    
    // 读取响应
    response.response = readResponse(timeout);
    response.duration = millis() - startTime;
    
    if (response.response.length() == 0) {
        response.result = AT_RESULT_TIMEOUT;
        setError("发送数据超时");
    }
    
    return response;
}

/**
 * @brief 等待特定响应
 * @param expectedResponse 期望的响应
 * @param timeout 超时时间（毫秒）
 * @return AtResponse 执行结果
 */
AtResponse AtCommandHandler::waitForResponse(const String& expectedResponse, 
                                            unsigned long timeout) {
    AtResponse response;
    response.result = AT_RESULT_TIMEOUT;
    
    unsigned long startTime = millis();
    String rawResponse = "";
    unsigned long lastCharTime = startTime;
    bool hasData = false;
    
    debugPrint("等待响应: " + expectedResponse);
    
    // 优化的等待逻辑：一旦收到期望响应就立即返回
    while (millis() - startTime < timeout) {
        if (serialPort.available()) {
            char c = serialPort.read();
            rawResponse += c;
            lastCharTime = millis();
            hasData = true;
            
            // 检查是否收到期望响应
            if (rawResponse.indexOf(expectedResponse) != -1) {
                // 等待一小段时间确保完整响应
                delay(50);
                // 读取剩余数据
                while (serialPort.available()) {
                    rawResponse += (char)serialPort.read();
                }
                response.result = AT_RESULT_SUCCESS;
                debugPrint("收到期望响应: " + rawResponse);
                break;
            }
        } else if (hasData && (millis() - lastCharTime > 200)) {
            // 如果已经有数据且200ms内没有新数据，检查是否包含期望响应
            if (rawResponse.indexOf(expectedResponse) != -1) {
                response.result = AT_RESULT_SUCCESS;
                debugPrint("收到期望响应: " + rawResponse);
                break;
            }
        }
        vTaskDelay(1); // 让出CPU时间
    }
    
    response.response = rawResponse;
    response.duration = millis() - startTime;
    
    if (response.result != AT_RESULT_SUCCESS) {
        setError("未收到期望响应: " + expectedResponse + ", 实际响应: " + rawResponse);
    }
    
    return response;
}

/**
 * @brief 清空串口缓冲区
 */
void AtCommandHandler::clearBuffer() {
    while (serialPort.available()) {
        serialPort.read();
    }
}

/**
 * @brief 检查设备是否响应
 * @return true 设备响应正常
 * @return false 设备无响应
 */
bool AtCommandHandler::isDeviceResponding() {
    AtResponse response = sendCommand("AT", "OK", 3000, 2);
    return response.result == AT_RESULT_SUCCESS;
}

/**
 * @brief 获取最后的错误信息
 * @return String 错误信息
 */
String AtCommandHandler::getLastError() {
    return lastError;
}

/**
 * @brief 设置调试模式
 * @param enabled 是否启用调试
 */
void AtCommandHandler::setDebugMode(bool enabled) {
    debugMode = enabled;
}

/**
 * @brief 读取串口响应
 * @param timeout 超时时间（毫秒）
 * @return String 响应内容
 */
String AtCommandHandler::readResponse(unsigned long timeout) {
    unsigned long startTime = millis();
    String response = "";
    unsigned long lastCharTime = startTime;
    bool hasData = false;
    
    while (millis() - startTime < timeout) {
        if (serialPort.available()) {
            char c = serialPort.read();
            response += c;
            lastCharTime = millis();
            hasData = true;
            
            // 检查是否收到完整响应（以OK、ERROR或特定响应结束）
            if (response.indexOf("OK\r\n") != -1 || 
                response.indexOf("ERROR\r\n") != -1 ||
                response.indexOf("+HTTPACTION:") != -1) {
                // 等待一小段时间确保没有更多数据
                delay(50);
                // 读取剩余数据
                while (serialPort.available()) {
                    response += (char)serialPort.read();
                }
                break;
            }
        } else if (hasData && (millis() - lastCharTime > 100)) {
            // 如果已经有数据且100ms内没有新数据，认为响应完成
            break;
        }
        vTaskDelay(1); // 让出CPU时间
    }
    
    response.trim();
    debugPrint("收到响应: " + response);
    return response;
}

/**
 * @brief 设置错误信息
 * @param error 错误信息
 */
void AtCommandHandler::setError(const String& error) {
    lastError = error;
    if (debugMode) {
        Serial.printf("AT命令处理器错误: %s\n", error.c_str());
    }
}

/**
 * @brief 打印调试信息
 * @param message 调试信息
 */
void AtCommandHandler::debugPrint(const String& message) {
    if (debugMode) {
        Serial.printf("[AT] %s\n", message.c_str());
    }
}

/**
 * @brief 执行AT命令诊断
 * @return String 诊断结果报告
 */
String AtCommandHandler::performDiagnostic() {
    lastDiagnosticTime = millis();
    String report = "\n=== AT命令处理器诊断报告 ===\n";
    
    // 基本状态检查
    report += "初始化状态: " + String(initialized ? "已初始化" : "未初始化") + "\n";
    report += "调试模式: " + String(debugMode ? "开启" : "关闭") + "\n";
    
    // 设备响应检查
    bool deviceResponding = isDeviceResponding();
    report += "设备响应状态: " + String(deviceResponding ? "正常" : "异常") + "\n";
    
    // 错误统计信息
    report += getErrorStatistics();
    
    // 设备状态详细检查
    report += checkDeviceStatus();
    
    // 最后错误信息
    if (lastError.length() > 0) {
        report += "\n最后错误: " + lastError + "\n";
    }
    
    // 最后失败的命令分析
    if (lastFailedCommand.length() > 0) {
        report += "\n最后失败命令分析:\n";
        report += analyzeCommandError(lastFailedCommand, lastFailedResponse);
    }
    
    report += "\n=== 诊断完成 ===\n";
    return report;
}

/**
 * @brief 分析AT命令错误
 * @param command 失败的命令
 * @param response 错误响应
 * @return String 错误分析结果
 */
String AtCommandHandler::analyzeCommandError(const String& command, const String& response) {
    String analysis = "命令: " + command + "\n";
    analysis += "响应: " + response + "\n";
    
    // 分析常见错误类型
    if (response.indexOf("ERROR") != -1) {
        analysis += "错误类型: AT命令执行错误\n";
        
        // 具体错误代码分析
        if (response.indexOf("+CME ERROR") != -1) {
            analysis += "详细分析: GSM模块内部错误\n";
            if (response.indexOf("3") != -1) {
                analysis += "建议: 操作不被允许，检查模块状态\n";
            } else if (response.indexOf("4") != -1) {
                analysis += "建议: 操作不支持，检查命令格式\n";
            } else if (response.indexOf("14") != -1) {
                analysis += "建议: SIM卡错误，检查SIM卡状态\n";
            }
        } else if (response.indexOf("+CMS ERROR") != -1) {
            analysis += "详细分析: SMS相关错误\n";
            analysis += "建议: 检查SMS服务状态和参数设置\n";
        }
    } else if (response.length() == 0) {
        analysis += "错误类型: 命令超时\n";
        analysis += "详细分析: 模块无响应或响应超时\n";
        analysis += "建议: 检查串口连接、波特率设置或模块电源\n";
    } else if (response.indexOf("BUSY") != -1) {
        analysis += "错误类型: 模块忙碌\n";
        analysis += "详细分析: 模块正在处理其他操作\n";
        analysis += "建议: 等待当前操作完成后重试\n";
    } else {
        analysis += "错误类型: 响应格式异常\n";
        analysis += "详细分析: 收到意外的响应内容\n";
        analysis += "建议: 检查命令格式和模块固件版本\n";
    }
    
    return analysis;
}

/**
 * @brief 检查设备状态
 * @return String 设备状态报告
 */
String AtCommandHandler::checkDeviceStatus() {
    String status = "\n--- 设备状态检查 ---\n";
    
    // 检查基本AT响应
    AtResponse atResponse = sendCommand("AT", "OK", 1000, 0);
    status += "基本AT响应: " + String(atResponse.result == AT_RESULT_SUCCESS ? "正常" : "异常") + "\n";
    
    // 检查信号强度
    AtResponse signalResponse = sendCommand("AT+CSQ", "OK", 2000, 0);
    if (signalResponse.result == AT_RESULT_SUCCESS) {
        status += "信号强度查询: 成功\n";
        status += "信号响应: " + signalResponse.response + "\n";
    } else {
        status += "信号强度查询: 失败\n";
    }
    
    // 检查网络注册状态
    AtResponse networkResponse = sendCommand("AT+CREG?", "OK", 2000, 0);
    if (networkResponse.result == AT_RESULT_SUCCESS) {
        status += "网络注册查询: 成功\n";
        status += "网络状态: " + networkResponse.response + "\n";
    } else {
        status += "网络注册查询: 失败\n";
    }
    
    // 检查SIM卡状态
    AtResponse simResponse = sendCommand("AT+CPIN?", "OK", 2000, 0);
    if (simResponse.result == AT_RESULT_SUCCESS) {
        status += "SIM卡状态查询: 成功\n";
        status += "SIM卡状态: " + simResponse.response + "\n";
    } else {
        status += "SIM卡状态查询: 失败\n";
    }
    
    return status;
}

/**
 * @brief 获取详细的错误统计信息
 * @return String 错误统计报告
 */
String AtCommandHandler::getErrorStatistics() {
    String stats = "\n--- 命令执行统计 ---\n";
    stats += "总命令数: " + String(totalCommands) + "\n";
    stats += "成功命令数: " + String(successfulCommands) + "\n";
    stats += "失败命令数: " + String(failedCommands) + "\n";
    stats += "超时命令数: " + String(timeoutCommands) + "\n";
    
    if (totalCommands > 0) {
        float successRate = (float)successfulCommands / totalCommands * 100.0;
        float failureRate = (float)failedCommands / totalCommands * 100.0;
        float timeoutRate = (float)timeoutCommands / totalCommands * 100.0;
        
        stats += "成功率: " + String(successRate, 1) + "%\n";
        stats += "失败率: " + String(failureRate, 1) + "%\n";
        stats += "超时率: " + String(timeoutRate, 1) + "%\n";
        
        // 健康状态评估
        if (successRate >= 95.0) {
            stats += "健康状态: 优秀\n";
        } else if (successRate >= 85.0) {
            stats += "健康状态: 良好\n";
        } else if (successRate >= 70.0) {
            stats += "健康状态: 一般\n";
        } else {
            stats += "健康状态: 异常\n";
        }
    } else {
        stats += "健康状态: 无数据\n";
    }
    
    return stats;
}