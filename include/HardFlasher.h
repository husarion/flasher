#ifndef __HARDFLASHER_H__
#define __HARDFLASHER_H__

#include "Flasher.h"

#include "devices.h"
#include "myFTDI.h"
#include "TRoboCOREHeader.h"

class HardFlasher : public Flasher
{
public:
	int init();
	int start(bool initBootloader = true);
	int erase();
	int flash();
	int reset();
	int cleanup(bool reset = true);

	int protect();
	int unprotect();
	int dump();
	int dumpEmulatedEEPROM();
	int eraseEmulatedEEPROM();
	int setup();

	int readHeader(TRoboCOREHeader& header);
	int writeHeader(TRoboCOREHeader& header);

	int switchToEdison();
	int switchToSTM32();

private:
	stm32_dev_t m_dev;

	int open();
	int close(bool reset);

	// commands
	int getVersion();
	int getCommand();
	int getID();
	int readMemory(uint32_t addr, void* data, int len);
	int writeMemory(uint32_t addr, const void* data, int len);
	int erasePages(const vector<int>& pages);

	// misc
	void dumpOptionBytes();

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
