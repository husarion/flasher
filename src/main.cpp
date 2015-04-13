#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#include "timeutil.h"
#include "Flasher.h"
#include "HardFlasher.h"
#include "SoftFlasher.h"

#include <vector>
using namespace std;

int doHelp = 0;
int doSoft = 0, doHard = 0, doUnprotect = 0, doProtect = 0, doFlash = 0, doDump = 0;

#define BEGIN_CHECK_USAGE() do {
#define END_CHECK_USAGE() usage(argv); return 1; } while (0);
#define CHECK_USAGE(x) if(x) break;

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
	fprintf(stderr, "       %s <other options>\n", argv[0]);
	fprintf(stderr, "\r\n");
	fprintf(stderr, "other options:\r\n");
	fprintf(stderr, "       --protect        protects bootloader against\n");
	fprintf(stderr, "                        unintended modifications (only valid with --hard)\n");
	fprintf(stderr, "       --unprotect      unprotects bootloader against\n");
	fprintf(stderr, "                        unintended modifications (only valid with --hard)\n");
	fprintf(stderr, "       --dump           dumps device info (only valid with --hard)\n");
}

void callback(uint32_t cur, uint32_t total)
{
	int width = 30;
	int ratio = cur * width / total;
	int id = cur / 2000;
	
	if (cur == -1)
	{
		printf("\rProgramming device... ");
		int toClear = width + 25;
		for (int i = 0; i < toClear; i++)
			putchar(' ');
		printf("\rProgramming device... ");
		return;
	}
	
	static int lastID = -1;
	if (id == lastID)
		return;
	lastID = id;
	
	printf("\rProgramming device... [");
	for (int i = 0; i < width; i++)
	{
		if (i <= ratio)
			putchar('=');
		else
			putchar(' ');
	}
	printf("] %4d kB / %4d kB %3d%%", cur / 1024, total / 1024, cur * 100 / total);
	fflush(stdout);
}

int main(int argc, char** argv)
{
	int speed = 460800;
	const char* device = 0;
	int res;
	
	setvbuf(stdout, NULL, _IONBF, 0);
	
	static struct option long_options[] =
	{
		{ "soft",  no_argument,       &doSoft, 1 },
		{ "hard",  no_argument,       &doHard, 1 },
		
		{ "test",  no_argument,  &doUnprotect, 1 },
		{ "flash",  no_argument,  &doFlash, 1 },
		{ "unprotect",  no_argument,  &doUnprotect, 1 },
		{ "protect",  no_argument,    &doProtect, 1 },
		{ "dump",  no_argument,    &doDump, 1 },
		
		{ "device", required_argument, 0, 'd' },
		{ "speed",  required_argument, 0, 's' },

		{ "usage",  no_argument,    &doHelp, 1 },
		{ "help",  no_argument,    &doHelp, 1 },
		
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
	
	doHard = !doSoft;
	doFlash = !doProtect && !doUnprotect && !doDump;
	if (doHelp || doFlash + doProtect + doUnprotect > 1)
	{
		usage(argv);
		return 1;
	}

	const char* filePath = 0;
	if (optind < argc)
		filePath = argv[optind];
		
	BEGIN_CHECK_USAGE();
	CHECK_USAGE(doHard && doFlash && filePath);
	CHECK_USAGE(doHard && doProtect);
	CHECK_USAGE(doHard && doUnprotect);
	CHECK_USAGE(doHard && doDump);
	CHECK_USAGE(doSoft && doFlash && device && filePath);
	END_CHECK_USAGE();
	
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
	if (doFlash)
	{
		res = flasher->load(filePath);
		if (res != 0)
		{
			printf("unable to load hex file\n");
			return 1;
		}
	}
	
	res = flasher->init();
	if (res != 0)
	{
		printf("unable to initialize flasher\n");
		return 1;
	}
	
	while (true)
	{
		res = flasher->start();
		if (res == 0)
		{
			uint32_t startTime = TimeUtilGetSystemTimeMs();
			
			if (doProtect)
			{
				printf("Protecting device... ");
				res = flasher->protect();
			}
			else if (doUnprotect)
			{
				printf("Unprotecting device... ");
				res = flasher->unprotect();
			}
			else if (doDump)
			{
				printf("Dumping info...\r\n");
				res = flasher->dump();
			}
			else if (doFlash)
			{
				printf("Erasing device... ");
				res = flasher->erase();
				if (res == 0)
				{
					// printf("Erasing done\n");
				}
				else
				{
					printf("\n");
					continue;
				}
				
				printf("Programming device... ");
				res = flasher->flash();
				if (res == 0)
				{
					// printf("Programming done\n");
				}
				else
				{
					printf("\n");
					continue;
				}
				
				printf("Reseting device... ");
				res = flasher->reset();
				if (res == 0)
				{
					// printf("Reseting done\n");
				}
				else
				{
					printf("\n");
					continue;
				}
				
				uint32_t endTime = TimeUtilGetSystemTimeMs();
				float time = endTime - startTime;
				float avg = flasher->getHexFile().totalLength / (time / 1000.0f) / 1024.0f;
				
				printf("==== Summary ====\nTime: %d ms\nSpeed: %.2f KBps (%d bps)\n", endTime - startTime, avg, (int)(avg * 8.0f * 1024.0f));
			}
			break;
		}
		else if (res == -2)
		{
			break;
		}
		// else
		// {
		// printf("unable to start flashing\n");
		// return 1;
		// }
	}
	
	flasher->close();
	
	return 0;
}







