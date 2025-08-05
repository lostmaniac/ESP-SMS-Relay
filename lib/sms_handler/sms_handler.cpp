#include "sms_handler.h"
#include "Arduino.h"

// 引用外部声明的串口对象
extern HardwareSerial simSerial;

void SmsHandler::processLine(const String& line) {
    if (line.startsWith("+CMTI:")) {
        Serial.println("收到新短信通知，准备读取...");
        int commaIndex = line.lastIndexOf(',');
        if (commaIndex != -1) {
            String indexStr = line.substring(commaIndex + 1);
            indexStr.trim();
            readMessage(indexStr.toInt());
        }
    }
}

void SmsHandler::processMessageBlock(const String& block) {
    PDU pdu;
    if (!pdu.decodePDU(block.c_str())) {
        Serial.println("PDU解码失败。");
        return;
    }

    int* concatInfo = pdu.getConcatInfo();
    if (concatInfo && concatInfo[0] != 0) {
        // 这是一个长短信分片
        unsigned short refNum = concatInfo[0];
        unsigned char partNum = concatInfo[1];
        unsigned char totalParts = concatInfo[2];

        Serial.printf("收到长短信分片，消息引用: %d，分片序号: %d/%d\n", refNum, partNum, totalParts);

        // 存储完整的PDU，而不仅仅是文本部分，以便后续正确拼接
        smsCache[refNum].totalParts = totalParts;
        smsCache[refNum].parts[partNum] = block; // 存储原始PDU

        // 检查是否已收到所有分片
        if (smsCache[refNum].parts.size() == totalParts) {
            assembleAndProcessSms(refNum);
        }
    } else {
        // 这是一个单条短信
        Serial.println("收到单条短信:");
        Serial.print("  发件人: ");
        Serial.println(pdu.getSender());
        Serial.print("  接收时间: ");
        Serial.println(formatTimestamp(pdu.getTimeStamp()));
        Serial.print("  消息内容: ");
        Serial.println(pdu.getText());
        Serial.println("----------");
    }
}

void SmsHandler::assembleAndProcessSms(uint8_t refNum) {
    Serial.printf("正在拼接消息, 引用号: %d...\n", refNum);
    String fullMessage = "";
    String sender = "";
    String timestamp = "";
    auto& sms = smsCache[refNum];

    // 按顺序拼接所有分片的用户数据部分
    for (int i = 1; i <= sms.totalParts; ++i) {
        PDU pduPart;
        if (pduPart.decodePDU(sms.parts[i].c_str())) {
            fullMessage += pduPart.getText();
            // 从第一个分片获取发送人和时间戳信息
            if (i == 1) {
                sender = pduPart.getSender();
                timestamp = pduPart.getTimeStamp();
            }
        } else {
            Serial.printf("解码分片 %d 失败，跳过此分片。\n", i);
        }
    }

    Serial.println("收到完整长短信:");
    Serial.print("  发件人: ");
    Serial.println(sender);
    Serial.print("  接收时间: ");
    Serial.println(formatTimestamp(timestamp));
    Serial.print("  消息内容: ");
    Serial.println(fullMessage);
    Serial.println("----------");

    // 清理此消息的缓存
    smsCache.erase(refNum);

    // 发送确认
    simSerial.println("AT+CNMA");
}

void SmsHandler::readMessage(int messageIndex) {
    Serial.print("正在读取短信，索引: ");
    Serial.println(messageIndex);
    simSerial.print("AT+CMGR=" + String(messageIndex) + "\r\n");
}

/**
 * @brief 将PDU时间戳转换为可读的日期时间格式
 * @param pduTimestamp PDU格式的时间戳字符串 (YYMMDDhhmmss)
 * @return 格式化的日期时间字符串 (YYYY-MM-DD HH:mm:ss)
 */
String SmsHandler::formatTimestamp(const String& pduTimestamp) {
    // PDU时间戳格式: YYMMDDhhmmss (12位数字)
    if (pduTimestamp.length() < 12) {
        return "时间格式错误";
    }
    
    // 提取各个时间组件
    String year = pduTimestamp.substring(0, 2);
    String month = pduTimestamp.substring(2, 4);
    String day = pduTimestamp.substring(4, 6);
    String hour = pduTimestamp.substring(6, 8);
    String minute = pduTimestamp.substring(8, 10);
    String second = pduTimestamp.substring(10, 12);
    
    // 转换年份 (假设20xx年)
    int yearInt = year.toInt();
    if (yearInt >= 0 && yearInt <= 99) {
        yearInt += 2000;
    }
    
    // 格式化为可读格式: YYYY-MM-DD HH:mm:ss
    String formattedTime = String(yearInt) + "-" + 
                          (month.length() == 1 ? "0" + month : month) + "-" +
                          (day.length() == 1 ? "0" + day : day) + " " +
                          (hour.length() == 1 ? "0" + hour : hour) + ":" +
                          (minute.length() == 1 ? "0" + minute : minute) + ":" +
                          (second.length() == 1 ? "0" + second : second);
    
    return formattedTime;
}