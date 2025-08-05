#include <Arduino.h>
#include "init_module.h"
#include "uart_monitor.h"
#include "config.h"

HardwareSerial simSerial(SIM_SERIAL_NUM);
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void setup() {
  simSerial.begin(SIM_BAUD_RATE, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  Serial.begin(115200);

  xTaskCreate(
    init_module_task,    // Task function
    "InitModuleTask",    // Task name
    10000,               // Stack size (bytes)
    NULL,                // Parameter
    1,                   // Priority
    NULL                 // Task handle
  );

  // uart_monitor_task 将在 init_module_task 成功后启动

}

void loop() {
  // Empty. Tasks are running.
}