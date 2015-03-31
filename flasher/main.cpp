#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

// #include "proto.h"
// #include "uart.h"
// #include "devices.h"
// #include "cmds.h"
// #include "ihex.h"

#include "Flasher.h"
#include "HardFlasher.h"
#include "SoftFlasher.h"

#include <vector>
using namespace std;

int doSoft = 0, doHard = 1;

uint32_t getTicks()
{
	timeval tv;
	gettimeofday(&tv, 0);
	uint32_t val = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return val;
}

void usage(char** argv)
{
	fprintf(stderr, "Usage: %s [--hard] [--speed speed] file.hex\n", argv[0]);
	fprintf(stderr, "       %s --soft --device /dev/ttyUSB0 file.hex\n", argv[0]);
}

void callback(uint32_t cur, uint32_t total)
{
	printf("%d %d\r\n", cur, total);
}

int main(int argc, char** argv)
{
	int speed = 460800;
	const char* device = 0;
	
	static struct option long_options[] =
	{
		{ "soft",  no_argument,       &doSoft, 1 },
		{ "hard",  no_argument,       &doHard, 1 },
		
		{ "device", required_argument, 0, 'd' },
		{ "speed",  required_argument, 0, 's' },
		
		{ 0, 0, 0, 0 }
	};
	
	for (;;)
	{
		int option_index = 0;
		char *p;
		int c = getopt_long(argc, argv, "s:d:", long_options, &option_index);
		if (c == -1)
			break;
			
		switch (c)
		{
		case 'd':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			if (speed == 0)
				speed = 460800;
			break;
		}
	}
	
	const char* filePath = 0;
	if (optind < argc)
		filePath = argv[optind];
		
	if (!filePath || (doSoft && !device))
	{
		usage(argv);
		return 1;
	}
	
	Flasher *flasher = 0;
	if (doHard)
	{
		flasher = new HardFlasher();
		flasher->setBaudrate(speed);
	}
	if (doSoft)
	{
		flasher = new SoftFlasher();
		flasher->setBaudrate(921600);
		flasher->setDevice(device);
	}
	flasher->setCallback(&callback);
	int res;
	res = flasher->load(filePath);
	if(res != 0)
	{
		fprintf(stderr, "unable to load hex file\n");
		return 1;
	}
	
	res = flasher->init();
	if(res != 0)
	{
		fprintf(stderr, "unable to initialize flasher\n");
		return 1;
	}

try_flash:
	res = flasher->start();
	if (res == 0)
	{
		res = flasher->erase();
		if (res == 0)
		{
			printf("Erasing done\n");
		}
		else
		{
			goto try_flash;
		}

		res = flasher->flash();
		if (res == 0)
		{
			printf("Programming done\n");
		}
		else
		{
			goto try_flash;
		}
	}
	else
	{
		fprintf(stderr, "unable to start flashing\n");
		return 1;
	}
	
	flasher->close();
	
	return 0;
}







