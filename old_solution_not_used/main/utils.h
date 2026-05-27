#ifndef UTILS_H
#define UTILS_H

#include "driver/uart.h"

uint8_t readUntil(uart_port_t uart, char* buf, uint8_t len, TickType_t timeoutms, const char* terminators = "\x0A\x0D");
void IRAM_ATTR delayUsbyNops(uint32_t us);
//char* strtrim (char* s);


#endif
