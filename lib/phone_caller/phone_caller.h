/**
 * @file phone_caller.h
 * @brief 电话拨打模块 - 提供拨打电话、等待和挂断功能
 * @author ESP-SMS-Relay Project
 * @version 1.0.0
 * @date 2024
 * 
 * 该模块提供了完整的电话拨打功能，包括：
 * - 拨打电话
 * - 等待指定时间
 * - 挂断电话
 * - 错误处理
 */

#ifndef PHONE_CALLER_H
#define PHONE_CALLER_H

#include <Arduino.h>

/**
 * @brief 电话拨打结果枚举
 */
enum PhoneCallResult {
    CALL_SUCCESS,                   ///< 拨打成功
    CALL_ERROR_NETWORK_NOT_READY,  ///< 网络未就绪
    CALL_ERROR_INVALID_NUMBER,      ///< 号码格式无效
    CALL_ERROR_AT_COMMAND_FAILED,   ///< AT命令执行失败
    CALL_ERROR_CALL_TIMEOUT,        ///< 拨打超时
    CALL_ERROR_HANGUP_FAILED        ///< 挂断失败
};

/**
 * @brief 电话拨打器类
 * 
 * 该类封装了完整的电话拨打功能，提供简洁的API接口用于拨打电话。
 */
class PhoneCaller {
public:
    /**
     * @brief 构造函数
     */
    PhoneCaller();
    
    /**
     * @brief 析构函数
     */
    ~PhoneCaller();
    
    /**
     * @brief 拨打电话
     * @param phone_number 电话号码（支持国际格式，如+8610086）
     * @return PhoneCallResult 拨打结果
     */
    PhoneCallResult makeCall(const String& phone_number);
    
    /**
     * @brief 拨打电话并等待指定时间后挂断
     * @param phone_number 电话号码
     * @param wait_seconds 等待时间（秒）
     * @return PhoneCallResult 拨打结果
     */
    PhoneCallResult makeCallAndWait(const String& phone_number, int wait_seconds);
    
    /**
     * @brief 挂断电话
     * @return true 挂断成功
     * @return false 挂断失败
     */
    bool hangupCall();
    
    /**
     * @brief 检查网络状态
     * @return true 网络已注册
     * @return false 网络未注册
     */
    bool isNetworkReady();
    
    /**
     * @brief 获取最后一次错误的详细信息
     * @return String 错误描述
     */
    String getLastError() const;
    
private:
    String last_error_;             ///< 最后一次错误信息
    
    /**
     * @brief 发送AT命令并等待响应
     * @param command AT命令
     * @param expected_response 期望的响应
     * @param timeout 超时时间（毫秒）
     * @return true 命令执行成功
     * @return false 命令执行失败
     */
    bool sendAtCommand(const String& command, const String& expected_response, unsigned long timeout);
    
    /**
     * @brief 验证电话号码格式
     * @param phone_number 电话号码
     * @return true 格式正确
     * @return false 格式错误
     */
    bool validatePhoneNumber(const String& phone_number);
};

#endif // PHONE_CALLER_H