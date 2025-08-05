#include "uart_dispatcher.h"
#include "sms_handler.h"

SmsHandler smsHandler;

void UartDispatcher::process(const String& data) {
    // 原始输出
    Serial.print(data);

    String trimmedData = data;
    trimmedData.trim();

    // 检查是否是+CMT: URC的开始
    if (trimmedData.startsWith("+CMT:")) {
        isBuffering = true; // 准备接收下一行的PDU数据
        return; // 等待PDU数据行
    }

    // 如果我们正在等待PDU数据
    if (isBuffering) {
        // 并且当前行不为空（避免处理+CMT和PDU之间的空行）
        if (trimmedData.length() > 0) {
            smsHandler.processMessageBlock(trimmedData);
            isBuffering = false; // PDU处理完毕，重置状态
        }
    } else {
        // 如果不是在处理PDU，则按常规方式处理其他URC或响应
        smsHandler.processLine(trimmedData);
    }
    // 其他消息当前仅打印，不作进一步处理。
}