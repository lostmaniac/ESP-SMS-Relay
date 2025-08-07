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
        String sender = pdu.getSender();
        String content = pdu.getText();
        String timestamp = pdu.getTimeStamp();
        
        Serial.println("收到单条短信:");
        Serial.print("  发件人: ");
        Serial.println(sender);
        Serial.print("  接收时间: ");
        Serial.println(formatTimestamp(timestamp));
        Serial.print("  消息内容: ");
        Serial.println(content);
        Serial.println("----------");
        
        // 处理完整短信（存储到数据库并转发）
    processSmsComplete(sender, content, timestamp);
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
    
    // 处理完整短信（存储到数据库并转发）
    processSmsComplete(sender, fullMessage, timestamp);

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

/**
 * @brief 处理完整的短信（存储到数据库并转发）
 * @param sender 发送方号码
 * @param content 短信内容
 * @param timestamp 接收时间戳
 */
void SmsHandler::processSmsComplete(const String& sender, const String& content, const String& timestamp) {
    Serial.println("开始处理完整短信...");
    
    // 存储到数据库
    int recordId = storeSmsToDatabase(sender, content, timestamp);
    if (recordId > 0) {
        Serial.printf("短信已存储到数据库，记录ID: %d\n", recordId);
        
        // 转发短信
        if (forwardSms(sender, content, timestamp, recordId)) {
            Serial.println("短信转发成功");
        } else {
            Serial.println("警告: 短信转发失败");
        }
    } else {
        Serial.println("警告: 短信存储到数据库失败");
        
        // 即使存储失败，也尝试转发（使用-1作为记录ID）
        if (forwardSms(sender, content, timestamp, -1)) {
            Serial.println("短信转发成功（未存储到数据库）");
        } else {
            Serial.println("警告: 短信转发失败");
        }
    }
}

/**
 * @brief 存储短信到数据库
 * @param sender 发送方号码
 * @param content 短信内容
 * @param timestamp 接收时间戳
 * @return int 记录ID，-1表示失败
 */
int SmsHandler::storeSmsToDatabase(const String& sender, const String& content, const String& timestamp) {
    DatabaseManager& dbManager = DatabaseManager::getInstance();
    
    // 检查数据库是否就绪
    if (!dbManager.isReady()) {
        Serial.println("数据库未就绪，无法存储短信");
        return -1;
    }
    
    // 创建短信记录
    SMSRecord record;
    record.fromNumber = sender;
    record.content = content;
    record.receivedAt = time(nullptr); // 使用当前时间戳
    
    // 添加到数据库
    int recordId = dbManager.addSMSRecord(record);
    if (recordId > 0) {
        Serial.printf("短信记录已添加到数据库，ID: %d\n", recordId);
    } else {
        Serial.println("添加短信记录到数据库失败: " + dbManager.getLastError());
    }
    
    return recordId;
}

/**
 * @brief 推送短信到配置的转发目标
 * @param sender 发送方号码
 * @param content 短信内容
 * @param timestamp 接收时间戳
 * @param smsRecordId 短信记录ID
 * @return true 推送成功
 * @return false 推送失败
 */
bool SmsHandler::forwardSms(const String& sender, const String& content, const String& timestamp, int smsRecordId) {
    PushManager& pushManager = PushManager::getInstance();
    
    // 检查推送管理器是否已初始化
    if (!pushManager.initialize()) {
        Serial.println("推送管理器初始化失败: " + pushManager.getLastError());
        return false;
    }
    
    // 构建推送上下文
    PushContext context;
    context.sender = sender;
    context.content = content;
    context.timestamp = timestamp;
    context.smsRecordId = smsRecordId;
    
    Serial.println("正在处理短信转发...");
    Serial.println("发送方: " + sender);
    Serial.println("内容: " + content.substring(0, 50) + (content.length() > 50 ? "..." : ""));
    
    // 处理短信转发
    PushResult result = pushManager.processSmsForward(context);
    
    // 处理结果
    switch (result) {
        case PUSH_SUCCESS:
            Serial.println("✅ 短信转发成功");
            return true;
            
        case PUSH_NO_RULE:
            Serial.println("ℹ️ 没有匹配的转发规则，跳过转发");
            return true; // 没有规则不算失败
            
        case PUSH_RULE_DISABLED:
            Serial.println("ℹ️ 转发规则已禁用，跳过转发");
            return true; // 规则禁用不算失败
            
        case PUSH_CONFIG_ERROR:
            Serial.println("❌ 转发配置错误: " + pushManager.getLastError());
            return false;
            
        case PUSH_NETWORK_ERROR:
            Serial.println("❌ 网络错误: " + pushManager.getLastError());
            return false;
            
        case PUSH_FAILED:
        default:
            Serial.println("❌ 短信转发失败: " + pushManager.getLastError());
            return false;
    }
}