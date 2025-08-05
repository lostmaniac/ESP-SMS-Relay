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
        Serial.print("  时间戳: ");
        Serial.println(pdu.getTimeStamp());
        Serial.print("  消息内容: ");
        Serial.println(pdu.getText());
        Serial.println("----------");
    }
}

void SmsHandler::assembleAndProcessSms(uint8_t refNum) {
    Serial.printf("正在拼接消息, 引用号: %d...\n", refNum);
    String fullMessage = "";
    auto& sms = smsCache[refNum];

    // 按顺序拼接所有分片的用户数据部分
    for (int i = 1; i <= sms.totalParts; ++i) {
        PDU pduPart;
        if (pduPart.decodePDU(sms.parts[i].c_str())) {
            fullMessage += pduPart.getText();
        } else {
            Serial.printf("解码分片 %d 失败，跳过此分片。\n", i);
        }
    }

    Serial.println("完整消息内容:");
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