#ifndef UART_DISPATCHER_H
#define UART_DISPATCHER_H

#include <Arduino.h>

class UartDispatcher {
public:
    void process(const String& data);

private:
    String messageBuffer;
    bool isBuffering = false;
};

#endif // UART_DISPATCHER_H