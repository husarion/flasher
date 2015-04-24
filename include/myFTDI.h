#ifndef __H_MYFTDI__
#define __H_MYFTDI__

#include <stdint.h>

bool uart_open(int speed, bool showErrors = true);
int uart_check_gpio();
int uart_reset();
bool uart_isOpened();
void uart_reset_normal();
void uart_setspeed(int speed);
int uart_tx(const void* data, int len);
int uart_rx_any(void* data, int len);
int uart_rx(void* data, int len, uint32_t timeout_ms);
void uart_close();

#endif
