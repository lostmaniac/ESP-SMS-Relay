#ifndef CONFIG_H
#define CONFIG_H

// SIMCom module UART configuration
#define SIM_SERIAL_NUM 2
#define SIM_BAUD_RATE 115200
#define SIM_RX_PIN 17      // ESP32 receives, connects to module's TXD
#define SIM_TX_PIN 18      // ESP32 sends, connects to module's RXD
#define RI_PIN 40        // Ring Indicator
#define DTR_PIN 45       // Data Terminal Ready

#endif // CONFIG_H