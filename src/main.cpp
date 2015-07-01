#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#include "TRoboCOREHeader.h"
#include "timeutil.h"
#include "Flasher.h"
#include "HardFlasher.h"
#include "SoftFlasher.h"
#include "utils.h"
#include "console.h"
#include "signal.h"

#ifdef EMBED_BOOTLOADERS
#include "bootloaders.h"
#endif

#include <vector>
using namespace std;

int doHelp = 0;
int doSoft = 0, doHard = 0, doUnprotect = 0, doProtect = 0, doFlash = 0,
    doDump = 0, doDumpEEPROM = 0, doRegister = 0, doSetup = 0, doFlashBootloader = 0,
    doTest = 0;
int regType = -1;
int doConsole = 0;

#define BEGIN_CHECK_USAGE() int found = 0; do {
#define END_CHECK_USAGE() if (found != 1) { if (found > 1) warn1(); else warn2(); usage(argv); return 1; } } while (0);
#define CHECK_USAGE(x) if(x) { found++; }
#define CHECK_USAGE_NO_INC(x) if(x && found == 0) { found++; }

uint32_t getTicks()
{
	timeval tv;
	gettimeofday(&tv, 0);
	uint32_t val = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return val;
}

void warn1()
{
	printf("only one action is allowed\r\n\r\n");
}
void warn2()
{
	printf("invalid action\r\n\r\n");
}
void usage(char** argv)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Flashing RoboCORE:\n");
	fprintf(stderr, "  %s [--hard] [--speed speed] file.hex\n", argv[0]);
	fprintf(stderr, "  %s --soft --device /dev/ttyUSB0 file.hex\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "Flashing bootloader:\n");
	fprintf(stderr, "  %s --flash-bootloader\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "Other commands:\n");
	fprintf(stderr, "  %s <other options>\n", argv[0]);
	fprintf(stderr, "\r\n");
	fprintf(stderr, "other options:\r\n");
	fprintf(stderr, "       --help, --usage  prints this message\n");
	fprintf(stderr, "       --test           tests connection to RoboCORE\n");
	fprintf(stderr, "       --setup          setup option bits\n");
	fprintf(stderr, "       --protect        protects bootloader against\n");
	fprintf(stderr, "                        unintended modifications (only valid with --hard)\n");
	fprintf(stderr, "       --unprotect      unprotects bootloader against\n");
	fprintf(stderr, "                        unintended modifications (only valid with --hard)\n");
	fprintf(stderr, "       --dump           dumps device info (only valid with --hard)\n");
	fprintf(stderr, "       --dump-eeprom    dumps emulated EEPROM content (only valid with --hard)\n");
	fprintf(stderr, "       --debug          show debug messages\n");
}

void callback(uint32_t cur, uint32_t total)
{
	int width = 30;
	int ratio = cur * width / total;
	int id = cur / 2000;

	if (cur == (uint32_t) - 1)
	{
		LOG_NICE("\rProgramming device... ");
		int toClear = width + 25;
		for (int i = 0; i < toClear; i++)
			LOG_NICE(" ");
		LOG_NICE("\rProgramming device... ");
		return;
	}

	static int lastID = -1;
	if (id == lastID)
		return;
	lastID = id;

	LOG_NICE("\rProgramming device... [");
	for (int i = 0; i < width; i++)
	{
		if (i <= ratio)
			LOG_NICE("=");
		else
			LOG_NICE(" ");
	}
	LOG_NICE("] %4d kB / %4d kB %3d%%", cur / 1024, total / 1024, cur * 100 / total);
	LOG_DEBUG("uploading %4d kB / %4d kB %3d%%", cur / 1024, total / 1024, cur * 100 / total);
	fflush(stdout);
}

static void sigHandler(int)
{
	uart_close();
	exit(0);
}

void decodeKey(const char* source, char* target)
{
	if (strlen(source) != 32)
	{
		printf("invalid key length (%s)\n", source);
		exit(1);
	}
	for (int i = 0; i < 16; i ++)
	{
		char byte[3];
		byte[0] = source[i * 2];
		byte[1] = source[i * 2 + 1];
		byte[2] = 0;
		int val;
		int read = sscanf(byte, "%x", &val);
		if (read != 1 || !isxdigit(byte[0]) || !isxdigit(byte[1]))
		{
			printf("invalid key");
			exit(1);
		}
		target[i] = (char)val;
	}
}

int main(int argc, char** argv)
{
	int speed = -1;
	const char* device = nullptr;
	int res;
	int regId = -1;
	int headerId = -1;
	uint32_t regVer = 0xffffffff;
	char robocoreKey[16];
	bool hasKey = false;

	setvbuf(stdout, NULL, _IONBF, 0);
	signal(SIGINT, sigHandler);

	static struct option long_options[] =
	{
		{ "soft",       no_argument,       &doSoft,   1 },
		{ "hard",       no_argument,       &doHard,   1 },

		{ "test",       no_argument,       &doTest ,  1 },
		{ "flash",      no_argument,       &doFlash,  1 },
#ifdef EMBED_BOOTLOADERS
		{ "flash-bootloader", no_argument, &doFlashBootloader, 1 },
#endif
		{ "unprotect",  no_argument,       &doUnprotect, 1 },
		{ "protect",    no_argument,       &doProtect,   1 },
		{ "dump",       no_argument,       &doDump,      1 },
		{ "dump-eeprom", no_argument,       &doDumpEEPROM, 1 },
		{ "setup",      no_argument,       &doSetup,     1 },

		// "hidden" options for registration
		{ "register",   no_argument,       &doRegister,  1 },

		{ "id",         required_argument, 0,       'i' },
		{ "header-id",  required_argument, 0,       'H' },
		{ "robocore-key", required_argument, 0,      'k' },
		{ "version",    required_argument, 0,       'v' },
		{ "big",        no_argument,       &regType,  2 },
		{ "pro",        no_argument,       &regType,  3 },

		{ "device",     required_argument, 0,       'd' },
		{ "speed",      required_argument, 0,       's' },

		{ "usage",      no_argument,       &doHelp,   1 },
		{ "help",       no_argument,       &doHelp,   1 },

		{ "console",    no_argument,       &doConsole, 1 },
		{ "debug",      no_argument,       &log_debug, 1 },

		{ 0, 0, 0, 0 }
	};

	for (;;)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "hs:d:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c)
		{
		case 'h':
			doHelp = 1;
			break;
		case 'd':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			if (speed == 0)
				speed = -1;
			break;
		case 'i':
			regId = atoi(optarg);
			if (regId < 0)
			{
				printf("invalid id\r\n");
				exit(1);
			}
			break;
		case 'H':
			headerId = atoi(optarg);
			if (headerId < 0 || headerId > 4)
			{
				printf("invalid header id\r\n");
				exit(1);
			}
			break;
		case 'k':
		{
			decodeKey(optarg, robocoreKey);
			hasKey = true;
			break;
		}
		case 'v':
		{
			vector<string> parts = splitString(optarg, ".");
			int a, b, c, d;
			if (parts.size() == 3)
			{
				a = atoi(parts[0].c_str());
				b = atoi(parts[1].c_str());
				c = atoi(parts[2].c_str());
				d = 0;
			}
			else if (parts.size() == 4)
			{
				a = atoi(parts[0].c_str());
				b = atoi(parts[1].c_str());
				c = atoi(parts[2].c_str());
				d = atoi(parts[3].c_str());
			}
			else
			{
				printf("invalid version (part count: %d)\r\n", (int)parts.size());
				exit(1);
			}
			if (a < 0 || b < 0 || c < 0 || d < 0)
			{
				printf("invalid version (bad numbers)\r\n");
				exit(1);
			}
			if (a + b + c + d == 0)
			{
				printf("invalid version (all zero)\r\n");
				exit(1);
			}

			makeVersion(regVer, a, b, c, d);
		}
		break;
		}
	}

	doHard = !doSoft;
	doFlash = !doProtect && !doUnprotect && !doDump && !doDumpEEPROM && !doRegister &&
	          !doSetup && !doFlashBootloader && !doTest;
	if (doHelp)
	{
		usage(argv);
		return 1;
	}
	if (doFlash + doProtect + doUnprotect + doDump + doDumpEEPROM + doRegister + doSetup + doFlashBootloader + doTest > 1)
	{
		warn1();
		usage(argv);
		return 1;
	}

	const char* filePath = 0;
	if (optind < argc)
		filePath = argv[optind];

	if (doFlash && doConsole && !filePath)
		doFlash = 0;

	BEGIN_CHECK_USAGE();
	CHECK_USAGE(doHard && doTest);
	CHECK_USAGE(doHard && doFlash && filePath);
	CHECK_USAGE(doHard && doProtect);
	CHECK_USAGE(doHard && doUnprotect);
	CHECK_USAGE(doHard && doDump);
	CHECK_USAGE(doHard && doDumpEEPROM);
	CHECK_USAGE(doHard && doRegister && regId != -1 && regVer != 0xffffffff && regType != -1
	            && headerId != -1 && hasKey);
	CHECK_USAGE(doHard && doSetup);
	CHECK_USAGE(doHard && doFlashBootloader);
	CHECK_USAGE(doSoft && doFlash && device && filePath);
	CHECK_USAGE_NO_INC(doConsole);
	END_CHECK_USAGE();

	int openBootloader = doTest || doFlash || doProtect || doUnprotect || doDump || doDumpEEPROM || doRegister || doSetup || doFlashBootloader;

	if (openBootloader)
	{
		Flasher *flasher = 0;
		if (doHard)
		{
			flasher = new HardFlasher();
			int s = speed;
			if (s == -1)
				s = 460800;
			flasher->setBaudrate(s);
		}
		if (doSoft)
		{
			flasher = new SoftFlasher();
			flasher->setBaudrate(460800);
			flasher->setDevice(device);
		}
		flasher->setCallback(&callback);
		if (doFlash)
		{
			LOG_DEBUG("loading file...");
			res = flasher->load(filePath);
			if (res != 0)
			{
				LOG("unable to load hex file");
				return 1;
			}
		}

		res = flasher->init();
		if (res != 0)
		{
			LOG("unable to initialize flasher");
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
					LOG_NICE("Protecting bootloader... ");
					LOG_DEBUG("protecting bootloader...");
					res = flasher->protect();
				}
				else if (doUnprotect)
				{
					LOG_NICE("Unprotecting bootloader... ");
					LOG_DEBUG("unprotecting bootloader...");
					res = flasher->unprotect();
				}
				else if (doDump)
				{
					LOG_NICE("Dumping info...\r\n");
					LOG_DEBUG("dumping info...");
					res = flasher->dump();
				}
				else if (doDumpEEPROM)
				{
					LOG_NICE("Dumping info...\r\n");
					LOG_DEBUG("dumping info...");
					res = flasher->dumpEmulatedEEPROM();
				}
				else if (doSetup)
				{
					LOG_NICE("Checking configuration... ");
					LOG_DEBUG("checking configuration...");
					res = flasher->setup();
				}
				else if (doRegister)
				{
					HardFlasher *hf = dynamic_cast<HardFlasher*>(flasher);

					TRoboCOREHeader oldHeader;
					hf->readHeader(oldHeader, headerId);

					if (!oldHeader.isClear())
					{
						LOG_NICE("Already registered\r\n");
						LOG_DEBUG("already registered");
						exit(1);
					}

					LOG_NICE("Registering...\r\n");
					LOG_DEBUG("registering...");
					TRoboCOREHeader h;
					h.headerVersion = 0x02;
					h.type = regType;
					h.version = regVer;
					h.id = regId;
					h.key[0] = 0x01;
					memcpy(h.key + 1, robocoreKey, 16);
					uint16_t crc = crc16_calc((uint8_t*)robocoreKey, 16);
					memcpy(h.key + 17, &crc, 2);
					res = hf->writeHeader(h, headerId);
				}
				else if (doFlash)
				{
					if (doHard)
					{
						LOG_NICE("Checking configuration... ");
						LOG_DEBUG("checking configuration...");
						res = flasher->setup();
						if (res != 0)
						{
							continue;
						}
					}

					LOG_NICE("Erasing device... ");
					LOG_DEBUG("erasing device...");
					res = flasher->erase();
					if (res != 0)
					{
						printf("\n");
						continue;
					}

					LOG_NICE("Programming device... ");
					LOG_DEBUG("programming device...");
					res = flasher->flash();
					if (res != 0)
					{
						printf("\n");
						continue;
					}

					LOG_NICE("Reseting device... ");
					LOG_DEBUG("reseting device...");
					res = flasher->reset();
					if (res != 0)
					{
						printf("\n");
						continue;
					}

					uint32_t endTime = TimeUtilGetSystemTimeMs();
					float time = endTime - startTime;
					float avg = flasher->getHexFile().totalLength / (time / 1000.0f) / 1024.0f;

					LOG_NICE("==== Summary ====\nTime: %d ms\nSpeed: %.2f KBps (%d bps)\n", endTime - startTime, avg, (int)(avg * 8.0f * 1024.0f));
				}
#ifdef EMBED_BOOTLOADERS
				else if (doFlashBootloader)
				{
					static int stage = 0;

					if (stage == 0)
					{
						LOG_NICE("Checking version... ");
						LOG_DEBUG("checking version... ");
						TRoboCOREHeader h;
						HardFlasher *hf = (HardFlasher*)flasher;
						res = hf->readHeader(h);
						if (h.isClear())
						{
							LOG_NICE("unable, device is unregistered, register it first.\r\n");
							LOG_DEBUG("unable, device is unregistered, register it first.");
							break;
						}
						else if (!h.isValid())
						{
							LOG_NICE("header is invalid, unable to recognize device.\r\n");
							LOG_DEBUG("header is invalid, unable to recognize device.");
							break;
						}

						LOG_NICE("OK\r\n");
						LOG_DEBUG("OK");
						char buf[50];
						int a, b, c, d;
						parseVersion(h.version, a, b, c, d);
						switch (h.type)
						{
						case 1: sprintf(buf, "bootloader_%d_%d_%d_mini", a, b, c); break;
						case 2: sprintf(buf, "bootloader_%d_%d_%d_big", a, b, c); break;
						case 3: sprintf(buf, "bootloader_%d_%d_%d_pro", a, b, c); break;
						default:
							LOG_NICE("Unsupported version\r\n");
							LOG_DEBUG("unsupported version");
							break;
						}

						bool found = false;
						TBootloaderData* ptr = bootloaders;
						while (ptr->name)
						{
							if (strcmp(ptr->name, buf) == 0)
							{
								found = true;
								break;
							}
							ptr++;
						}

						if (!found)
						{
							LOG_NICE("Bootloader not found\r\n");
							LOG_DEBUG("bootloader not found");
							break;
						}

						LOG_NICE("Bootloader found\r\n");
						LOG_DEBUG("bootloader found");

						flasher->loadData(ptr->data);

						LOG_NICE("Checking configuration... ");
						LOG_DEBUG("checking configuration...");
						res = flasher->setup();
						if (res != 0)
						{
							continue;
						}

						LOG_NICE("Unprotecting bootloader... ");
						LOG_DEBUG("unprotecting bootloader...");
						res = flasher->unprotect();
						if (res != 0)
						{
							continue;
						}

						stage = 1; // device must be reseted after unprotecting
						LOG_NICE("Resetting device\r\n");
						LOG_DEBUG("resetting device");
						continue;
					}
					else if (stage == 1)
					{
						LOG_NICE("Erasing device... ");
						LOG_DEBUG("erasing device...");
						res = flasher->erase();
						if (res != 0)
						{
							printf("\n");
							continue;
						}

						LOG_NICE("Programming device... ");
						LOG_DEBUG("programming device...");
						res = flasher->flash();
						if (res != 0)
						{
							printf("\n");
							continue;
						}

						LOG_NICE("Protecting bootloader... ");
						LOG_DEBUG("protecting bootloader...");
						res = flasher->protect();
						if (res != 0)
						{
							printf("\n");
							continue;
						}

						LOG_NICE("Reseting device... ");
						LOG_DEBUG("reseting device...");
						res = flasher->reset();
						if (res != 0)
						{
							printf("\n");
							continue;
						}

						// printf("searching for bootloader %s", buf);
						// printf("%s\n", buf);
					}
				}
#endif
				break;
			}
			else if (res == -2)
			{
				// printf("\r\n");
				break;
			}
		}

		flasher->cleanup();
	}

	if (doConsole)
	{
		int s = speed;
		if (s == -1)
			s = 460800;
		return runConsole(s);
	}

	return 0;
}
