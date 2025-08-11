#include "sms_handler.h"
#include "Arduino.h"
#include "log_manager.h"

// 引用外部声明的串口对象
extern HardwareSerial simSerial;

void SmsHandler::processLine(const String& line) {
    LogManager& logger = LogManager::getInstance();
    
    if (line.startsWith("+CMTI:")) {
        logger.logInfo(LOG_MODULE_SMS, "收到新短信通知，准备读取...");
        int commaIndex = line.lastIndexOf(',');
        if (commaIndex != -1) {
            String indexStr = line.substring(commaIndex + 1);
            indexStr.trim();
            readMessage(indexStr.toInt());
        }
    }
    // 处理+CMT格式的直接短信通知（当前配置使用的格式）
    else if (line.startsWith("+CMT:")) {
        logger.logInfo(LOG_MODULE_SMS, "📱 收到新短信通知 (+CMT格式)");
        // +CMT格式的短信通知，PDU数据在下一行
        // 这里不需要特殊处理，uart_dispatcher会处理PDU数据
    }
}

void SmsHandler::processMessageBlock(const String& block) {
    LogManager& logger = LogManager::getInstance();
    
    PDU pdu;
    if (!pdu.decodePDU(block.c_str())) {
        logger.logError(LOG_MODULE_SMS, "PDU解码失败");
        return;
    }

    int* concatInfo = pdu.getConcatInfo();
    if (concatInfo && concatInfo[0] != 0) {
        // 这是一个长短信分片
        unsigned short refNum = concatInfo[0];
        unsigned char partNum = concatInfo[1];
        unsigned char totalParts = concatInfo[2];

        logger.logInfo(LOG_MODULE_SMS, "收到长短信分片，消息引用: " + String(refNum) + "，分片序号: " + String(partNum) + "/" + String(totalParts));

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
        
        // 输出短信接收日志
        logger.printSeparator("收到新短信");
        logger.logInfo(LOG_MODULE_SMS, "📞 发送方: " + sender);
        logger.logInfo(LOG_MODULE_SMS, "📝 内容: " + content);
        logger.logInfo(LOG_MODULE_SMS, "🕐 时间: " + timestamp);
        logger.printSeparator();
        
        // 处理完整短信（存储到数据库并转发）
        processSmsComplete(sender, content, timestamp);
    }
}

void SmsHandler::assembleAndProcessSms(uint8_t refNum) {
    // 拼接长短信
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
        }
    }

    // 输出长短信拼接完成日志
    LogManager& logger = LogManager::getInstance();
    logger.printSeparator("长短信拼接完成");
    logger.logInfo(LOG_MODULE_SMS, "📞 发送方: " + sender);
    logger.logInfo(LOG_MODULE_SMS, "📝 完整内容: " + fullMessage);
    logger.logInfo(LOG_MODULE_SMS, "🕐 时间: " + timestamp);
    logger.printSeparator();
    
    // 处理完整短信（存储到数据库并转发）
    processSmsComplete(sender, fullMessage, timestamp);

    // 清理此消息的缓存
    smsCache.erase(refNum);

    // 发送确认
    simSerial.println("AT+CNMA");
}

void SmsHandler::readMessage(int messageIndex) {
    // 读取短信索引: messageIndex
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
    LogManager& logger = LogManager::getInstance();
    logger.logInfo(LOG_MODULE_SMS, "🔄 开始处理短信...");
    
    // 存储到数据库
    int recordId = storeSmsToDatabase(sender, content, timestamp);
    if (recordId > 0) {
        logger.logInfo(LOG_MODULE_SMS, "💾 短信已存储到数据库，记录ID: " + String(recordId));
        forwardSms(sender, content, timestamp, recordId);
    } else {
        logger.logError(LOG_MODULE_SMS, "❌ 短信存储到数据库失败，仍尝试转发");
        forwardSms(sender, content, timestamp, -1);
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
        return -1;
    }
    
    // 创建短信记录
    SMSRecord record;
    record.fromNumber = sender;
    record.content = content;
    record.receivedAt = time(nullptr); // 使用当前时间戳
    
    // 添加到数据库
    return dbManager.addSMSRecord(record);
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
        return false;
    }
    
    // 构建推送上下文
    PushContext context;
    context.sender = sender;
    context.content = content;
    context.timestamp = timestamp;
    context.smsRecordId = smsRecordId;
    
    // 处理短信转发
    
    // 处理短信转发
    PushResult result = pushManager.processSmsForward(context);
    
    // 处理结果
    LogManager& logger = LogManager::getInstance();
    switch (result) {
        case PUSH_SUCCESS:
            logger.logInfo(LOG_MODULE_SMS, "✅ 短信转发成功");
            return true;
            
        case PUSH_NO_RULE:
            logger.logInfo(LOG_MODULE_SMS, "ℹ️ 没有匹配的转发规则，跳过转发");
            return true; // 没有规则不算失败
            
        case PUSH_RULE_DISABLED:
            logger.logInfo(LOG_MODULE_SMS, "ℹ️ 转发规则已禁用，跳过转发");
            return true; // 规则禁用不算失败
            
        case PUSH_CONFIG_ERROR:
            logger.logError(LOG_MODULE_SMS, "❌ 转发配置错误: " + pushManager.getLastError());
            return false;
            
        case PUSH_NETWORK_ERROR:
            logger.logError(LOG_MODULE_SMS, "❌ 网络错误: " + pushManager.getLastError());
            return false;
            
        case PUSH_FAILED:
        default:
            logger.logError(LOG_MODULE_SMS, "❌ 短信转发失败: " + pushManager.getLastError());
            return false;
    }
}