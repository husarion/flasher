#ifndef __SOFTFLASHER_H__
#define __SOFTFLASHER_H__

#include "Flasher.h"

class SoftFlasher : public Flasher
{
public:
	int init();
	int start();
	int erase();
	int flash();
	int reset();

	int protect() { return -1; }
	int unprotect() { return -1; }
	int dump() { return -1; }
	
private:
	int open();
	int close();
};

#endif
