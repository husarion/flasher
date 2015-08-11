#ifndef __HARDFLASHER_H__
#define __HARDFLASHER_H__

#include <string>

using namespace std;

#include "devices.h"
#include "myFTDI.h"
#include "TRoboCOREHeader.h"
#include "ihex.h"

typedef void (*ProgressCallback)(uint32_t current, uint32_t total);

class HardFlasher
{
public:
	int load(const string& path);
	int loadData(const char* data);

	void setDevice(const string& device) { m_device = device; }
	void setBaudrate(int baudrate) { m_baudrate = baudrate; }
	void setCallback(ProgressCallback callback) { m_callback = callback; }

	THexFile& getHexFile() { return m_hexFile; }

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

	int readHeader(TRoboCOREHeader& header, int headerId = 0);
	int writeHeader(TRoboCOREHeader& header, int headerId = 0);

private:
	stm32_dev_t m_dev;
	THexFile m_hexFile;
	string m_device;
	int m_baudrate;
	ProgressCallback m_callback;

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
