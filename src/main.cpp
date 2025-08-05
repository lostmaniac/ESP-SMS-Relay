/**
 * @file main.cpp
 * @brief ESP32 SMS 中继系统主程序
 * @author ESP-SMS-Relay Project
 * @date 2024
 * 
 * 该文件是系统的入口点，负责:
 * 1. 硬件初始化
 * 2. 启动系统初始化任务
 * 3. 主循环管理
 */

#include <Arduino.h>
#include "system_init.h"
#include "uart_monitor.h"
#include "config.h"

// 定义硬件串口
HardwareSerial simSerial(SIM_SERIAL_NUM); // 使用配置的串口号

/**
 * @brief 系统设置函数
 * 
 * 负责基础硬件初始化和启动系统初始化任务
 */
void setup() {
    // 初始化串口
    Serial.begin(115200);
    simSerial.begin(SIM_BAUD_RATE, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN); // 使用配置的引脚
    
    // 等待串口稳定
    delay(1000);
    Serial.println("\n=== ESP32 SMS Relay System ===");
    Serial.println("硬件初始化完成，启动系统...");
    
    // 创建系统初始化任务
    xTaskCreate(
        system_init_task,     // 任务函数
        "SystemInitTask",     // 任务名称
        8192,                 // 堆栈大小
        NULL,                 // 任务参数
        1,                    // 任务优先级
        NULL                  // 任务句柄
    );
}

/**
 * @brief 主循环函数
 * 
 * 系统运行后的主循环，负责系统状态监控
 */
void loop() {
    // 获取系统状态
    SystemInit& systemInit = SystemInit::getInstance();
    SystemStatus status = systemInit.getSystemStatus();
    
    // 如果系统出错，尝试重启
    if (status == SYSTEM_ERROR) {
        Serial.println("检测到系统错误，尝试重启...");
        systemInit.restart();
    }
    
    // 主循环保持空闲，所有工作由任务处理
    vTaskDelay(5000 / portTICK_PERIOD_MS); // 每5秒检查一次
}