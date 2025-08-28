#include "uart_monitor.h"
#include "../include/config.h"
#include "../../include/constants.h"
#include "uart_dispatcher.h"
#include "terminal_manager.h"
#include "esp_task_wdt.h"

extern HardwareSerial simSerial;
UartDispatcher dispatcher;

void uart_monitor_task(void *pvParameters) {
  String buffer = "";
  bool atCommandMode = false;
  unsigned long lastAtCommandTime = 0;
  const unsigned long AT_COMMAND_TIMEOUT = DEFAULT_AT_COMMAND_TIMEOUT_MS; // AT命令超时
  
  while (1) {
    // 检查CLI状态
    TerminalManager& terminalManager = TerminalManager::getInstance();
    bool cliRunning = terminalManager.isCLIRunning();
    
    // 检查是否有AT命令正在执行（通过检测最近是否有AT命令发送）
    unsigned long currentTime = millis();
    if (atCommandMode && (currentTime - lastAtCommandTime > AT_COMMAND_TIMEOUT)) {
      atCommandMode = false; // AT命令超时，退出AT命令模式
    }
    
    if (simSerial.available()) {
        String newData = simSerial.readString();
        
        // 检测是否是AT命令响应（包含"AT"开头的命令回显）
        if (newData.indexOf("AT") != -1 && cliRunning) {
          atCommandMode = true;
          lastAtCommandTime = currentTime;
        }
        
        buffer += newData;
        
        // 当CLI运行且处于AT命令模式时，抑制输出但仍然处理数据
        if (cliRunning && atCommandMode) {
          dispatcher.setSuppressOutput(true);
        } else {
          dispatcher.setSuppressOutput(false);
        }
        
        int newlineIndex;
        while ((newlineIndex = buffer.indexOf('\n')) != -1) {
            String line = buffer.substring(0, newlineIndex + 1);
            buffer.remove(0, newlineIndex + 1);
            
            // 检查是否是短信相关的URC（+CMT:, +CMTI:等）
            String trimmedLine = line;
            trimmedLine.trim();
            
            // 短信相关的URC始终需要处理，即使在CLI模式下
             if (trimmedLine.startsWith("+CMT:") || 
                 trimmedLine.startsWith("+CMTI:") || 
                 trimmedLine.startsWith("+CDSI:") ||
                 trimmedLine.startsWith("+CBM:") ||
                 (dispatcher.isBufferingPDU() && trimmedLine.length() > 10)) { // PDU数据行
              // 强制处理短信相关数据，不受CLI状态影响
              dispatcher.setSuppressOutput(false);
              dispatcher.process(line);
            } else if (!cliRunning || !atCommandMode) {
              // 非AT命令模式或CLI未运行时，正常处理所有数据
              dispatcher.process(line);
            }
            // 在AT命令模式下，非短信相关的数据会被忽略，避免干扰CLI
        }
    }
    
    // 重置看门狗，防止任务超时
    esp_task_wdt_reset();
    
    vTaskDelay(50 / portTICK_PERIOD_MS); // 增加延时以允许更多数据到达缓冲区
  }
}