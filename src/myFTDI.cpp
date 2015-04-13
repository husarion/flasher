#include "myFTDI.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <ftdi.h>
#include <ftdi_i.h>

#define BOOT0 0
#define RST   1

uint32_t getTicks();

uint8_t vals;
int speed;
int setPin(ftdi_context* ftdi, int pin, int value)
{
	vals &= ~(1 << pin);
	vals |= (1 << (pin + 4)) | (value << pin);
	return ftdi_set_bitmode(ftdi, vals, BITMODE_CBUS);
}

SerialHandle uart_open(int speed, bool showErrors)
{
	ftdi_context *ftdi = ftdi_new();
	int ret;
	
	if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6015)) < 0)
	{
		if (showErrors)
			fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		ftdi_free(ftdi);
		return 0;
	}
	
	ftdi_read_data_set_chunksize(ftdi, 4096);
	::speed = speed;
	
	return ftdi;
}
int uart_check_gpio(SerialHandle handle)
{
	ftdi_context *ftdi = (ftdi_context*)handle;
	int r;
	r = ftdi_read_eeprom(ftdi);
	ftdi_eeprom_decode(ftdi, 0);
	int p1 = ftdi->eeprom->cbus_function[0];
	int p2 = ftdi->eeprom->cbus_function[1];
	const int IOMODE = 8;
	// const int IOMODE = 9;
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
int uart_reset(SerialHandle handle)
{
	ftdi_context *ftdi = (ftdi_context*)handle;
	setPin(ftdi, BOOT0, 1);
	usleep(10000);
	setPin(ftdi, RST, 1);
	usleep(100000);
	setPin(ftdi, RST, 0);
	usleep(500000);
	
	ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, EVEN);
	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
	
	ftdi_disable_bitbang(ftdi);
	ftdi_set_baudrate(ftdi, speed);
}
int uart_tx(SerialHandle handle, const void* data, int len)
{
	ftdi_context *ftdi = (ftdi_context*)handle;
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
int uart_rx(SerialHandle handle, void* data, int len, uint32_t timeout_ms)
{
	ftdi_context *ftdi = (ftdi_context*)handle;
	uint8_t* _data = (uint8_t*)data;
	int l = len;
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
void uart_reset_normal(SerialHandle handle)
{
	ftdi_context *ftdi = (ftdi_context*)handle;
	int ret;
	
	setPin(ftdi, BOOT0, 0);
	usleep(10000);
	setPin(ftdi, RST, 1);
	usleep(100000);
	setPin(ftdi, RST, 0);
	
	ftdi_disable_bitbang(ftdi);
}
void uart_close(SerialHandle handle)
{
	ftdi_context *ftdi = (ftdi_context*)handle;
	int ret;
	
	if ((ret = ftdi_usb_close(ftdi)) < 0)
		fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
		
	ftdi_free(ftdi);
}
