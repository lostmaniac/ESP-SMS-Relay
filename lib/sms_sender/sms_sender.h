/**
 * @file sms_sender.h
 * @brief 短信发送模块 - 使用pdulib库进行PDU编码和短信发送
 * @author ESP-SMS-Relay Project
 * @version 1.0.0
 * @date 2024
 * 
 * 该模块提供了完整的短信发送功能，包括：
 * - PDU编码（使用pdulib库）
 * - AT命令处理
 * - 错误处理和重试机制
 * - 内存管理
 */

#ifndef SMS_SENDER_H
#define SMS_SENDER_H

#include <Arduino.h>
#include <pdulib.h>

/**
 * @brief 短信发送结果枚举
 */
enum SmsSendResult {
    SMS_SUCCESS,                    ///< 发送成功
    SMS_ERROR_NETWORK_NOT_READY,   ///< 网络未就绪
    SMS_ERROR_SCA_NOT_SET,          ///< 短信中心号码未设置
    SMS_ERROR_ENCODE_FAILED,        ///< PDU编码失败
    SMS_ERROR_AT_COMMAND_FAILED,    ///< AT命令执行失败
    SMS_ERROR_SEND_TIMEOUT,         ///< 发送超时
    SMS_ERROR_INVALID_PARAMETER     ///< 参数无效
};

/**
 * @brief 短信发送器类
 * 
 * 该类封装了完整的短信发送功能，使用pdulib库进行PDU编码，
 * 提供简洁的API接口用于发送短信。
 */
class SmsSender {
public:
    /**
     * @brief 构造函数
     * @param buffer_size PDU编码缓冲区大小，默认200字节
     */
    SmsSender(int buffer_size = 200);
    
    /**
     * @brief 析构函数
     */
    ~SmsSender();
    
    /**
     * @brief 初始化短信发送器
     * @param sca_number 短信中心号码
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize(const String& sca_number);
    
    /**
     * @brief 发送短信
     * @param recipient 接收方号码（支持国际格式，如+8610086）
     * @param message 短信内容（UTF-8编码）
     * @return SmsSendResult 发送结果
     */
    SmsSendResult sendSms(const String& recipient, const String& message);
    
    /**
     * @brief 发送文本模式短信（仅用于启动时测试）
     * @param recipient 接收方号码
     * @param message 短信内容（纯英文数字）
     * @return SmsSendResult 发送结果
     */
    SmsSendResult sendTextSms(const String& recipient, const String& message);
    
    /**
     * @brief 获取最后一次错误的详细信息
     * @return String 错误描述
     */
    String getLastError() const;
    
    /**
     * @brief 检查网络状态
     * @return true 网络已注册
     * @return false 网络未注册
     */
    bool isNetworkReady();
    
    /**
     * @brief 设置短信中心号码
     * @param sca_number 短信中心号码
     */
    void setScaNumber(const String& sca_number);
    
private:
    PDU* pdu_encoder_;              ///< PDU编码器实例
    String sca_number_;             ///< 短信中心号码
    String last_error_;             ///< 最后一次错误信息
    bool initialized_;              ///< 初始化状态
    
    /**
     * @brief 发送AT命令并等待响应
     * @param command AT命令
     * @param expected_response 期望的响应
     * @param timeout 超时时间（毫秒）
     * @param wait_for_prompt 是否等待'>'提示符
     * @return true 命令执行成功
     * @return false 命令执行失败
     */
    bool sendAtCommand(const String& command, const String& expected_response, 
                      unsigned long timeout, bool wait_for_prompt = false);
    
    /**
     * @brief 发送PDU数据
     * @param pdu_data PDU数据字符串
     * @param tpdu_length TPDU长度
     * @return true 发送成功
     * @return false 发送失败
     */
    bool sendPduData(const char* pdu_data, int tpdu_length);
    
    /**
     * @brief 验证手机号码格式
     * @param phone_number 手机号码
     * @return true 格式正确
     * @return false 格式错误
     */
    bool validatePhoneNumber(const String& phone_number);
    
    /**
     * @brief 检测消息是否为纯英文数字（适合文本模式）
     * @param message 短信内容
     * @return true 纯英文数字
     * @return false 包含中文或特殊字符
     */
    bool isSimpleTextMessage(const String& message);
    
    /**
     * @brief 发送文本模式数据
     * @param recipient 接收方号码
     * @param message 短信内容
     * @return true 发送成功
     * @return false 发送失败
     */
    bool sendTextData(const String& recipient, const String& message);
    
    /**
     * @brief 清理资源
     */
    void cleanup();
};

#endif // SMS_SENDER_H