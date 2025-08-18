/**
 * @file at_command_handler.h
 * @brief AT命令处理器 - 提供通用的AT命令处理功能
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该模块负责:
 * 1. 通用AT命令的发送和响应处理
 * 2. AT命令的超时管理
 * 3. 响应解析和错误处理
 * 4. 命令队列管理
 */

#ifndef AT_COMMAND_HANDLER_H
#define AT_COMMAND_HANDLER_H

#include <Arduino.h>
#include <queue>

/**
 * @enum AtCommandResult
 * @brief AT命令执行结果枚举
 */
enum AtCommandResult {
    AT_RESULT_SUCCESS,      ///< 命令执行成功
    AT_RESULT_TIMEOUT,      ///< 命令执行超时
    AT_RESULT_ERROR,        ///< 命令执行错误
    AT_RESULT_INVALID,      ///< 无效命令
    AT_RESULT_BUSY          ///< 设备忙碌
};

/**
 * @struct AtCommand
 * @brief AT命令结构体
 */
struct AtCommand {
    String command;             ///< AT命令字符串
    String expectedResponse;    ///< 期望的响应
    unsigned long timeout;      ///< 超时时间(ms)
    int retries;               ///< 重试次数
};

/**
 * @struct AtResponse
 * @brief AT命令响应结构体
 */
struct AtResponse {
    AtCommandResult result;     ///< 执行结果
    String response;           ///< 响应内容
    unsigned long duration;    ///< 执行时长(ms)
};

/**
 * @class AtCommandHandler
 * @brief AT命令处理器类
 * 
 * 提供通用的AT命令处理功能
 */
class AtCommandHandler {
public:
    /**
     * @brief 构造函数
     * @param serial 串口对象引用
     */
    AtCommandHandler(HardwareSerial& serial);
    
    /**
     * @brief 析构函数
     */
    ~AtCommandHandler();
    
    /**
     * @brief 初始化AT命令处理器
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 发送AT命令并等待响应
     * @param command AT命令
     * @param expectedResponse 期望的响应
     * @param timeout 超时时间（毫秒）
     * @param retries 重试次数
     * @return AtResponse 命令执行结果
     */
    AtResponse sendCommand(const String& command, 
                          const String& expectedResponse = "OK", 
                          unsigned long timeout = 3000,
                          int retries = 0);
    
    /**
     * @brief 发送AT命令并获取完整响应
     * @param command AT命令
     * @param timeout 超时时间（毫秒）
     * @return AtResponse 命令执行结果
     */
    AtResponse sendCommandWithFullResponse(const String& command, 
                                          unsigned long timeout = 3000);
    
    /**
     * @brief 发送原始数据
     * @param data 要发送的数据
     * @param timeout 超时时间（毫秒）
     * @return AtResponse 执行结果
     */
    AtResponse sendRawData(const String& data, unsigned long timeout = 3000);
    
    /**
     * @brief 等待特定响应
     * @param expectedResponse 期望的响应
     * @param timeout 超时时间（毫秒）
     * @return AtResponse 执行结果
     */
    AtResponse waitForResponse(const String& expectedResponse, 
                              unsigned long timeout = 3000);
    
    /**
     * @brief 清空串口缓冲区
     */
    void clearBuffer();
    
    /**
     * @brief 检查设备是否响应
     * @return true 设备响应正常
     * @return false 设备无响应
     */
    bool isDeviceResponding();
    
    /**
     * @brief 获取最后的错误信息
     * @return String 错误信息
     */
    String getLastError();
    
    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试
     */
    void setDebugMode(bool enabled);
    
    /**
     * @brief 获取单例实例
     * @return AtCommandHandler& 单例引用
     */
    static AtCommandHandler& getInstance();
    
    /**
     * @brief 执行AT命令诊断
     * @return String 诊断结果报告
     */
    String performDiagnostic();
    
    /**
     * @brief 分析AT命令错误
     * @param command 失败的命令
     * @param response 错误响应
     * @return String 错误分析结果
     */
    String analyzeCommandError(const String& command, const String& response);
    
    /**
     * @brief 检查设备状态
     * @return String 设备状态报告
     */
    String checkDeviceStatus();
    
    /**
     * @brief 获取详细的错误统计信息
     * @return String 错误统计报告
     */
    String getErrorStatistics();

private:
    HardwareSerial& serialPort; ///< 串口对象引用
    String lastError;           ///< 最后的错误信息
    bool debugMode;            ///< 调试模式
    bool initialized;          ///< 是否已初始化
    
    // 错误统计相关成员
    unsigned long totalCommands;     ///< 总命令数
    unsigned long successfulCommands; ///< 成功命令数
    unsigned long failedCommands;    ///< 失败命令数
    unsigned long timeoutCommands;   ///< 超时命令数
    unsigned long lastDiagnosticTime; ///< 上次诊断时间
    String lastFailedCommand;        ///< 最后失败的命令
    String lastFailedResponse;       ///< 最后失败的响应
    
    /**
     * @brief 读取串口响应
     * @param timeout 超时时间（毫秒）
     * @return String 响应内容
     */
    String readResponse(unsigned long timeout);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const String& error);
    
    /**
     * @brief 打印调试信息
     * @param message 调试信息
     */
    void debugPrint(const String& message);
};

#endif // AT_COMMAND_HANDLER_H