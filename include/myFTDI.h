#ifndef __H_MYFTDI__
#define __H_MYFTDI__

#include <stdint.h>

typedef void* SerialHandle;

SerialHandle uart_open(int speed, bool showErrors = true);
int uart_check_gpio(SerialHandle handle);
int uart_reset(SerialHandle handle);
void uart_reset_normal(SerialHandle handle);
void uart_setspeed(SerialHandle handle, int speed);
int uart_tx(SerialHandle handle, const void* data, int len);
int uart_rx(SerialHandle handle, void* data, int len, uint32_t timeout_ms);
void uart_close(SerialHandle handle);

#endif
