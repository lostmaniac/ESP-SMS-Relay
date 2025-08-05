#ifndef SMS_HANDLER_H
#define SMS_HANDLER_H

#include <Arduino.h>
#include <pdulib.h>

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

    // 使用map来缓存不同参考号的长短信
    std::map<uint8_t, ConcatenatedSms> smsCache;
};

#endif // SMS_HANDLER_H