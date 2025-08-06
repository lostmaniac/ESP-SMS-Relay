#include "sms_handler.h"
#include "Arduino.h"

// 引用外部声明的串口对象
extern HardwareSerial simSerial;

// 企业微信机器人webhook地址
const String SmsHandler::WECHAT_WEBHOOK_URL = "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=753ca375-1904-4bcf-928f-817941b15f36";

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
        
        // 处理完整短信（存储到数据库并推送到企业微信）
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
    
    // 处理完整短信（存储到数据库并推送到企业微信）
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
 * @brief 处理完整的短信（存储到数据库并推送到企业微信）
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
    } else {
        Serial.println("警告: 短信存储到数据库失败");
    }
    
    // 推送到企业微信
    if (pushToWechatBot(sender, content, timestamp)) {
        Serial.println("短信已推送到企业微信机器人");
    } else {
        Serial.println("警告: 推送到企业微信机器人失败");
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
 * @brief 推送短信到企业微信机器人
 * @param sender 发送方号码
 * @param content 短信内容
 * @param timestamp 接收时间戳
 * @return true 推送成功
 * @return false 推送失败
 */
bool SmsHandler::pushToWechatBot(const String& sender, const String& content, const String& timestamp) {
    HttpClient& httpClient = HttpClient::getInstance();
    
    // 检查HTTP客户端是否已初始化
    if (!httpClient.initialize()) {
        Serial.println("HTTP客户端初始化失败: " + httpClient.getLastError());
        return false;
    }
    
    // 构建企业微信消息体（JSON格式）
    String messageBody = "{\"msgtype\":\"text\",\"text\":{\"content\":\"📱 收到新短信\\n\\n";
    messageBody += "📞 发送方: " + sender + "\\n";
    messageBody += "🕐 时间: " + formatTimestamp(timestamp) + "\\n";
    messageBody += "📄 内容: " + content + "\"}}";
    
    // 设置请求头
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    
    Serial.println("正在推送到企业微信机器人...");
    Serial.println("请求体: " + messageBody);
    
    // 发送POST请求
    HttpResponse response = httpClient.post(WECHAT_WEBHOOK_URL, messageBody, headers, 30000);
    
    // 简化的响应处理逻辑 - 只检查HTTP状态码
    Serial.printf("HTTP响应 - 状态码: %d, 错误码: %d\n", response.statusCode, response.error);
    Serial.println("响应内容: " + response.body);
    
    // 根据AT命令+HTTPACTION响应，只需检查状态码是否为200
    if (response.statusCode == 200) {
        Serial.println("✅ 企业微信推送成功（状态码200）");
        return true;
    } else {
        Serial.printf("❌ 企业微信推送失败，状态码: %d, 错误码: %d\n", response.statusCode, response.error);
        Serial.println("HTTP错误: " + httpClient.getLastError());
        return false;
    }
}