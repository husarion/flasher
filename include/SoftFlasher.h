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
	
private:
	int open();
	int close();
};

#endif
