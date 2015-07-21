#ifndef __SOFTFLASHER_H__
#define __SOFTFLASHER_H__

#include "Flasher.h"

class SoftFlasher : public Flasher
{
public:
	int init();
	int start(bool initBootloader = true);
	int erase();
	int flash();
	int reset();
	int cleanup(bool reset = true) { return 0; }

	int protect() { return -1; }
	int unprotect() { return -1; }
	int dump() { return -1; }
	int dumpEmulatedEEPROM() { return -1; }
	int setup() { return -1; }

private:
	int open();
	int close();
};

#endif
