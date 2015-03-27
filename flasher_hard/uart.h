#ifndef UART_H
#define UART_H

#include <stdint.h>

typedef void* SerialHandle;

SerialHandle uart_open(int speed);
void uart_setspeed(SerialHandle handle, int speed);
int uart_tx(SerialHandle handle, const void* data, int len);
int uart_rx(SerialHandle handle, void* data, int len, uint32_t timeout_ms);
void uart_close(SerialHandle handle);

#endif
