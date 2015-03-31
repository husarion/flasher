#ifndef __HARDFLASHER_H__
#define __HARDFLASHER_H__

#include "Flasher.h"

#include "devices.h"
#include "myFTDI.h"

class HardFlasher : public Flasher
{
public:
	int init();
	int open();
	int close();
	int start();
	
private:
	stm32_dev_t m_dev;
	SerialHandle m_serial;
	
	// commands
	int getVersion();
	int getCommand();
	int getID();
	int erase();
	int flash();
	
	// low-level protocol
	int uart_send_cmd(uint8_t cmd);
	
	int uart_read_ack_nack();
	int uart_read_ack_nack(int timeout);
	int uart_read_ack_nack_fast();
	
	int uart_read_byte();
	int uart_read_data(void* data, int len);
	
	int uart_write_data_checksum(const void* data, int len);
	int uart_write_data(const void* data, int len);
	int uart_write_byte(char data);
};

#endif
