#ifndef __H_MYFTDI__
#define __H_MYFTDI__

#include <stdint.h>

const int IOMODE = 8;
const int KEEP_AWAKE = 21;
const int DRIVE_0 = 6;
const int DRIVE_1 = 7;

struct gpio_config_t
{
	int cbus0, cbus1, cbus2, cbus3;
};

bool uart_open(int speed, bool showErrors = true);
bool uart_open_with_config(int speed, const gpio_config_t& config, bool showErrors);
int uart_set_gpio_config(const gpio_config_t& config);
int uart_reset_boot();
int uart_switch_to_edison();
int uart_switch_to_stm32();
bool uart_is_opened();
void uart_reset_normal();
void uart_setspeed(int speed);
int uart_tx(const void* data, int len);
int uart_rx_any(void* data, int len);
int uart_rx(void* data, int len, uint32_t timeout_ms);
void uart_close();

#endif
