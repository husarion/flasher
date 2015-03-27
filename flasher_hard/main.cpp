#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#include "proto.h"
#include "uart.h"
#include "devices.h"
#include "cmds.h"

#include <vector>
using namespace std;

uint8_t firmwareData[1024 * 1024];
uint32_t firmwareLen;

// device info
stm32_dev_t dev;

int doTest = 0;
int doErase = 0;
int doFlash = 0;

uint32_t getTicks()
{
	timeval tv;
	gettimeofday(&tv, 0);
	uint32_t val = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return val;
}

int start()
{
	printf("sending init...\n");
	uart_tx(handle, "\x7f", 1);
	
	int res = uart_read_ack_nack_fast();
	printf("res %x\r\n", res);
	if (res == ACK || res == NACK)
	{
		printf("device answered with 0x%02x\n", res);
		
		if (getVersion())
			return -1;
		if (getCommand())
			return -1;
		if (getID())
			return -1;
			
		return 0;
	}
	return -1;
}
int programm()
{
	uint32_t curAddr = 0x08008000;
	uint32_t sent = 0;
	
	uint32_t startTime = getTicks();
	
	while (sent < firmwareLen)
	{
		printf("sending page of addr: 0x%08x...\n", curAddr);
		uart_send_cmd(0x31);
		
		int res = uart_read_ack_nack();
		if (res == ACK)
		{
			uint32_t tmp = ((curAddr & 0x000000ff) << 24) | ((curAddr & 0x0000ff00) << 8) |
			               ((curAddr & 0x00ff0000) >> 8) | ((curAddr & 0xff000000) >> 24);
			uart_write_data_checksum((char*)&tmp, 4);
			
			res = uart_read_ack_nack();
			if (res == ACK)
			{
				int left = firmwareLen - sent;
				if (left > 256) left = 256;
				
				printf("to send: %d\n", left);
				
				char buf[1000];
				
				memcpy(buf + 1, firmwareData + sent, left);
				
				buf[0] = left - 1;
				
				uart_write_data_checksum(buf, left + 1);
				
				res = uart_read_ack_nack();
				printf("ack: 0x%02x\n", res);
				
				if (res == ACK)
				{
					sent += left;
					curAddr += left;
					printf("ok 0x%08x %d of %d\n", curAddr, sent, firmwareLen);
				}
				else
				{
					return -1;
				}
			}
		}
		else
		{
			return -1;
		}
	}
	
	uint32_t endTime = getTicks();
	float time = endTime - startTime;
	float avg = firmwareLen / (time / 1000.0f) / 1024.0f;
	
	printf("time: %d ms avg speed: %.2f KBps (%d bps)\n", endTime - startTime, avg, (int)(avg * 8.0f * 1024.0f));
	
	return 0;
}
int run(uint32_t addr)
{
	printf("go to 0x%08x...\n", addr);
	uart_send_cmd(0x21);
	
	int res = uart_read_ack_nack();
	if (res == ACK)
	{
		uint32_t tmp = ((addr & 0x000000ff) << 24) | ((addr & 0x0000ff00) << 8) |
		               ((addr & 0x00ff0000) >> 8) | ((addr & 0xff000000) >> 24);
		uart_write_data_checksum((char*)&tmp, 4);
		
		res = uart_read_ack_nack();
		if (res == ACK)
		{
			printf("acked 1\n");
			return 0;
		}
		else
		{
			return -1;
		}
		
		res = uart_read_ack_nack();
		if (res == ACK)
		{
			printf("acked 2\n");
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int main(int argc, char** argv)
{
	int speed = 460800;

	static struct option long_options[] =
	{
		{ "test",  no_argument,       &doTest,  1 },
		{ "flash", no_argument,       &doFlash, 1 },
		
		{ "speed", required_argument, 0, 's' },
		
		{ 0, 0, 0, 0 }
	};
	
	for (;;)
	{
		int option_index = 0;
		char *p;
		int c = getopt_long(argc, argv, "tfs:", long_options, &option_index);
		if (c == -1)
			break;
			
		switch (c)
		{
		case 't':
			doTest = 1;
			break;
		case 'f':
			doFlash = 1;
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
		
	if (!doTest && !doFlash)
	{
		printf("no command specified, assuming test\n");
		doTest = 1;
	}
	
	if (doFlash)
	{
		if (filePath)
		{
			FILE *file = fopen(filePath, "rb");
			firmwareLen = fread(firmwareData, 1, sizeof(firmwareData), file);
			fclose(file);
			
			while ((firmwareLen % 4) != 0)
			{
				firmwareData[firmwareLen] = 0xff;
				firmwareLen++;
			}
		}
		else
		{
			printf("bin file path must be provided\n");
			return 1;
		}
	}
	
	for (;;)
	{
		if (handle)
			uart_close(handle);
		printf("opening device...\n");
		handle = uart_open(speed);
		if (!handle)
		{
			perror("unable to open device");
			return 1;
		}
		int res;
		
		res = start();
		if (res == 0)
		{
			if (doTest)
				break;
				
			res = 0;
			if (doFlash)
			{
				res = erase();
				if (res == 0)
				{
					res = programm();
					if (res == 0)
					{
						printf("Programming done\n");
					}
				}
			}
		}
		usleep(100 * 1000);
	}
	
	if (handle)
		uart_close(handle);
		
	return 0;
}





