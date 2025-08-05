#include "uart_monitor.h"
#include "../include/config.h"
#include "uart_dispatcher.h"

extern HardwareSerial simSerial;
UartDispatcher dispatcher;

void uart_monitor_task(void *pvParameters) {
  String buffer = "";
  while (1) {
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