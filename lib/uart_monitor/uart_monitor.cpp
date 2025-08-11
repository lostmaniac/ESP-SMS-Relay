#include "uart_monitor.h"
#include "../include/config.h"
#include "uart_dispatcher.h"
#include "terminal_manager.h"

extern HardwareSerial simSerial;
UartDispatcher dispatcher;

void uart_monitor_task(void *pvParameters) {
  String buffer = "";
  while (1) {
    // 检查CLI状态并设置输出抑制
    TerminalManager& terminalManager = TerminalManager::getInstance();
    bool cliRunning = terminalManager.isCLIRunning();
    dispatcher.setSuppressOutput(cliRunning);
    
    if (simSerial.available()) {
        buffer += simSerial.readString();
        int newlineIndex;
        while ((newlineIndex = buffer.indexOf('\n')) != -1) {
            String line = buffer.substring(0, newlineIndex + 1);
            buffer.remove(0, newlineIndex + 1);
            dispatcher.process(line);
        }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // 增加延时以允许更多数据到达缓冲区
  }
}