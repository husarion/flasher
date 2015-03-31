#ifndef __FLASHER_H__
#define __FLASHER_H__

#include <string>

#include "ihex.h"

using namespace std;

class Flasher
{
public:
	int load(const string& path);

	void setDevice(const string& device) { m_device = device; }
	void setBaudrate(int baudrate) { m_baudrate = baudrate; }

	virtual int init() = 0;
	virtual int open() = 0;
	virtual int close() = 0;
	virtual int start() = 0;
	virtual int erase() = 0;
	virtual int flash() = 0;

protected:
	THexFile m_hexFile;
	string m_device;
	int m_baudrate;
};

#endif
