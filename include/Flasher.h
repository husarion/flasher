#ifndef __FLASHER_H__
#define __FLASHER_H__

#include <string>

#include "ihex.h"

using namespace std;

typedef void (*ProgressCallback)(uint32_t current, uint32_t total);

class Flasher
{
public:
	Flasher() : m_callback(0) { }

	int load(const string& path);

	void setDevice(const string& device) { m_device = device; }
	void setBaudrate(int baudrate) { m_baudrate = baudrate; }
	void setCallback(ProgressCallback callback) { m_callback = callback; }

	THexFile& getHexFile() { return m_hexFile; }

	virtual int init() = 0;
	virtual int open() = 0;
	virtual int close() = 0;
	virtual int start() = 0;
	virtual int erase() = 0;
	virtual int flash() = 0;
	virtual int reset() = 0;

	virtual int protect() = 0;
	virtual int unprotect() = 0;

protected:
	THexFile m_hexFile;
	string m_device;
	int m_baudrate;
	ProgressCallback m_callback;
};

#endif
