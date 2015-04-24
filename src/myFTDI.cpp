#include "myFTDI.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <libusb.h>
#include <ftdi.h>
#include <ftdi_i.h>

#define BOOT0 0
#define RST   1

uint32_t getTicks();

static uint8_t vals;
static int speed;
static ftdi_context* ftdi = 0;

int setPin(int pin, int value)
{
	vals &= ~(1 << pin);
	vals |= (1 << (pin + 4)) | (value << pin);
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
	int r =ftdi_set_baudrate(ftdi, speed);
	printf("%d\r\n", r);
	
	return ftdi;
}
int uart_check_gpio()
{
	int r;
	r = ftdi_read_eeprom(ftdi);
	ftdi_eeprom_decode(ftdi, 0);
	int p1 = ftdi->eeprom->cbus_function[0];
	int p2 = ftdi->eeprom->cbus_function[1];
	const int IOMODE = 8;
	if (p1 != IOMODE || p2 != IOMODE)
	{
		ftdi->eeprom->cbus_function[0] = IOMODE;
		ftdi->eeprom->cbus_function[1] = IOMODE;
		ftdi_eeprom_build(ftdi);
		ftdi_write_eeprom(ftdi);
		return 1;
	}
	return 0;
}
int uart_reset()
{
	setPin(BOOT0, 1);
	usleep(10000);
	setPin(RST, 1);
	usleep(100000);
	setPin(RST, 0);
	usleep(100000);
	
	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, EVEN);
	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
	
	ftdi_disable_bitbang(ftdi);
	ftdi_set_baudrate(ftdi, speed);
}
bool uart_isOpened()
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
	int ret;
	
	setPin(BOOT0, 0);
	usleep(10000);
	setPin(RST, 1);
	usleep(100000);
	setPin(RST, 0);
	
	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, NONE);
	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
	
	ftdi_disable_bitbang(ftdi);
	ftdi_set_baudrate(ftdi, speed);
}
void uart_close()
{
	int ret;
	
	if (!ftdi)
		return;
		
	// libusb_attach_kernel_driver(ftdi->usb_dev, ftdi->interface);
	if ((ret = ftdi_usb_close(ftdi)) < 0)
		fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		
	ftdi_free(ftdi);
	
	ftdi = 0;
}
