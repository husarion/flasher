#include "myFTDI.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <libusb.h>
#include <ftdi.h>
#include <ftdi_i.h>

#include "utils.h"
#include "timeutil.h"

#define BOOT0  0
#define RST    1
#define EDISON 3

uint32_t getTicks();

static uint8_t vals;
static int speed;
static ftdi_context* ftdi = 0;

int setPin(int pin, int value)
{
	vals &= ~(1 << pin);
	vals |= (1 << (pin + 4)) | (value << pin);
	const char *name;
	switch (pin)
	{
	case BOOT0: name = "BOOT0"; break;
	case RST: name = "RST"; break;
	case EDISON: name = "EDISON"; break;
	}
	LOG_DEBUG("setting pin %s to %d (values 0x%02x)", name, value, vals);
	return ftdi_set_bitmode(ftdi, vals, BITMODE_CBUS);
}

bool uart_open(int speed, bool showErrors)
{
	int ret;

	if (ftdi)
	{
		uart_close();
	}
	ftdi = ftdi_new();

	LOG_DEBUG("opening ftdi");
	if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6015)) < 0)
	{
		if (showErrors)
			fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		ftdi_free(ftdi);
		ftdi = 0;
		return 0;
	}

	ftdi->usb_read_timeout = 1000;
	ftdi->usb_write_timeout = 1000;
	libusb_set_auto_detach_kernel_driver(ftdi->usb_dev, 1);
	ftdi_read_data_set_chunksize(ftdi, 4096);
	::speed = speed;

	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, NONE);
	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);

	ftdi_disable_bitbang(ftdi);
#ifdef WIN32
	ftdi_set_baudrate(ftdi, speed * 4);
#else
	ftdi_set_baudrate(ftdi, speed);
#endif

	return ftdi;
}

bool uart_open_with_config(int speed, const gpio_config_t& config, bool showErrors)
{
	bool res = uart_open(speed, showErrors);
	if (res)
	{
		LOG_NICE(" OK\r\n");
		LOG_NICE("Checking settings... ");
		int r = uart_set_gpio_config(config);
		if (r)
		{
			uart_close();
			LOG_NICE(" FTDI settings changed, RoboCORE must be replugged to take changes into account.\r\n");
			LOG_NICE("Unplug RoboCORE.");
			bool restarted = false;
			for (;;)
			{
				if (!restarted)
				{
					res = uart_open(speed, showErrors);
					if (!res)
					{
						restarted = true;
						LOG_NICE(" OK, plug it again.");
					}
					else
					{
						uart_close();
					}
				}
				else
				{
					res = uart_open(speed, showErrors);
					if (res)
						break;
				}
				TimeUtilDelayMs(10);
				static int cnt = 0;
				if (cnt++ == 15)
				{
					LOG_NICE(".");
					cnt = 0;
				}
			}
			LOG_NICE(" OK\r\n");
		}
		else
		{
			LOG_NICE(" OK\r\n");
		}
		return 0;
	}
	else
	{
		return -1;
	}
}

int uart_set_gpio_config(const gpio_config_t& config)
{
	LOG_DEBUG("checking gpio config");
	ftdi_read_eeprom(ftdi);
	ftdi_eeprom_decode(ftdi, 0);
	int p1 = ftdi->eeprom->cbus_function[0];
	int p2 = ftdi->eeprom->cbus_function[1];
	int p3 = ftdi->eeprom->cbus_function[2];
	int p4 = ftdi->eeprom->cbus_function[3];
	if (p1 != config.cbus0 || p2 != config.cbus1 || p3 != config.cbus2 || p4 != config.cbus3)
	{
		ftdi->eeprom->cbus_function[0] = config.cbus0;
		ftdi->eeprom->cbus_function[1] = config.cbus1;
		ftdi->eeprom->cbus_function[2] = config.cbus2;
		ftdi->eeprom->cbus_function[3] = config.cbus3;
		ftdi_eeprom_build(ftdi);
		ftdi_write_eeprom(ftdi);
		return 1;
	}
	return 0;
}
int uart_reset_boot()
{
	LOG_DEBUG("resetting to bootloader mode...");
	setPin(BOOT0, 1);
	setPin(EDISON, 0);
	usleep(10000);
	setPin(RST, 1);
	usleep(100000);
	setPin(RST, 0);
	usleep(100000);

	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, EVEN);
	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);

	ftdi_disable_bitbang(ftdi);
#ifdef WIN32
	ftdi_set_baudrate(ftdi, speed * 4);
#else
	ftdi_set_baudrate(ftdi, speed);
#endif
	return 0;
}
int uart_switch_to_edison(bool resetSTM)
{
	LOG_DEBUG("setting pins for Edison mode...");
	gpio_config_t config;
	config.cbus0 = IOMODE;
	config.cbus1 = IOMODE;
	config.cbus2 = KEEP_AWAKE;
	config.cbus3 = DRIVE_1;
	int res = uart_open_with_config(115200, config, false);
	if (res)
		return -1;

	setPin(BOOT0, 0);
	setPin(RST, resetSTM ? 1 : 0);

	uart_close();

	return 0;
}
int uart_switch_to_stm32()
{
	LOG_DEBUG("setting pins for STM32 mode...");
	gpio_config_t config;
	config.cbus0 = IOMODE;
	config.cbus1 = IOMODE;
	config.cbus2 = KEEP_AWAKE;
	config.cbus3 = DRIVE_0;
	int res = uart_open_with_config(115200, config, false);
	if (res)
		return -1;

	setPin(BOOT0, 0);
	setPin(RST, 0);

	uart_close();

	return 0;
}
bool uart_is_opened()
{
	return ftdi != 0;
}
int uart_tx(const void* data, int len)
{
	uint8_t* _data = (uint8_t*)data;
	while (len)
	{
		int written = ftdi_write_data(ftdi, _data, len);
		if (written < 0)
			return -1;
		len -= written;
		_data += written;
	}
	return 0;
}
int uart_rx_any(void* data, int len)
{
	uint8_t* _data = (uint8_t*)data;

	int rread = ftdi_read_data(ftdi, _data, len);
	if (rread < 0)
		return -1;

	return rread;
}
int uart_rx(void* data, int len, uint32_t timeout_ms)
{
	uint8_t* _data = (uint8_t*)data;
	int r = 0;
	uint32_t start = getTicks();

	while (len && getTicks() - start < timeout_ms)
	{
		int rread = ftdi_read_data(ftdi, _data, len);
		if (rread < 0)
			return -1;

		len -= rread;
		_data += rread;
		r += rread;
	}
	return r;
}
void uart_reset_normal()
{
	LOG_DEBUG("resetting to normal mode...");
	setPin(BOOT0, 0);
	setPin(EDISON, 0);
	usleep(10000);
	setPin(RST, 1);
	usleep(100000);
	setPin(RST, 0);

	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, NONE);
	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);

	ftdi_disable_bitbang(ftdi);
#ifdef WIN32
	ftdi_set_baudrate(ftdi, speed * 4);
#else
	ftdi_set_baudrate(ftdi, speed);
#endif
}
void uart_close()
{
	int ret;

	if (!ftdi)
		return;

	LOG_DEBUG("closing ftdi...");
	// libusb_attach_kernel_driver(ftdi->usb_dev, ftdi->interface);
	if ((ret = ftdi_usb_close(ftdi)) < 0)
		fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));

	ftdi_free(ftdi);

	ftdi = 0;
}
