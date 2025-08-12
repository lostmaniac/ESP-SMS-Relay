#ifndef UART_DISPATCHER_H
#define UART_DISPATCHER_H

#include <Arduino.h>

/**
 * @class UartDispatcher
 * @brief UART数据调度器 - 处理串口数据并避免与CLI冲突
 * 
 * 负责处理来自GSM模块的串口数据，包括SMS消息和AT命令响应。
 * 当CLI运行时，会抑制原始数据输出以避免干扰用户交互。
 */
class UartDispatcher {
public:
    /**
     * @brief 处理串口数据
     * @param data 接收到的串口数据
     */
    void process(const String& data);
    
    /**
     * @brief 设置是否抑制原始数据输出
     * @param suppress true表示抑制输出，false表示正常输出
     */
    void setSuppressOutput(bool suppress);
    
    /**
     * @brief 检查是否正在抑制输出
     * @return bool 当前是否抑制输出
     */
    bool isOutputSuppressed() const;
    
    /**
     * @brief 检查是否正在缓冲PDU数据
     * @return bool 当前是否正在缓冲PDU数据
     */
    bool isBufferingPDU() const;

private:
    String messageBuffer;           ///< 消息缓冲区
    bool isBuffering = false;       ///< 是否正在缓冲PDU数据
    bool suppressOutput = false;    ///< 是否抑制原始数据输出
};

#endif // UART_DISPATCHER_H