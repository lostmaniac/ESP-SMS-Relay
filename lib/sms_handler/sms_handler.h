#ifndef SMS_HANDLER_H
#define SMS_HANDLER_H

#include <Arduino.h>
#include <pdulib.h>
#include "../database_manager/database_manager.h"
#include "../push_manager/push_manager.h"

#include <map>

// 用于存储分段短信的结构体
struct ConcatenatedSms {
    int totalParts; // 短信总部分数
    std::map<int, String> parts; // 已接收的部分，key是部分编号，value是PDU内容
};

class SmsHandler {
public:
    void processLine(const String& line);
    void processMessageBlock(const String& block);

private:
    void readMessage(int messageIndex);
    void assembleAndProcessSms(uint8_t refNum);
    
    /**
     * @brief 将PDU时间戳转换为可读的日期时间格式
     * @param pduTimestamp PDU格式的时间戳字符串 (YYMMDDhhmmss)
     * @return 格式化的日期时间字符串 (YYYY-MM-DD HH:mm:ss)
     */
    String formatTimestamp(const String& pduTimestamp);
    
    /**
     * @brief 处理完整的短信（存储到数据库并推送到企业微信）
     * @param sender 发送方号码
     * @param content 短信内容
     * @param timestamp 接收时间戳
     */
    void processSmsComplete(const String& sender, const String& content, const String& timestamp);
    
    /**
     * @brief 存储短信到数据库
     * @param sender 发送方号码
     * @param content 短信内容
     * @param timestamp 接收时间戳
     * @return int 记录ID，-1表示失败
     */
    int storeSmsToDatabase(const String& sender, const String& content, const String& timestamp);
    
    /**
     * @brief 推送短信到配置的转发目标
     * @param sender 发送方号码
     * @param content 短信内容
     * @param timestamp 接收时间戳
     * @param smsRecordId 短信记录ID
     * @return true 推送成功
     * @return false 推送失败
     */
    bool forwardSms(const String& sender, const String& content, const String& timestamp, int smsRecordId);

    // 使用map来缓存不同参考号的长短信
    std::map<uint8_t, ConcatenatedSms> smsCache;
};

#endif // SMS_HANDLER_H