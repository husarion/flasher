/*
 * cmds.cpp
 * Copyright (C) 2014 Krystian Dużyński <krystian.duzynski@gmail.com>
 */

#include "cmds.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "proto.h"
#include "uart.h"
#include "devices.h"
#include "ihex.h"

#include <algorithm>
#include <map>

using namespace std;

extern stm32_dev_t dev;

extern IHexFile hexFile;

int getVersion()
{
	int res;
	
	printf(">>> get version\n");
	
	uart_send_cmd(0x01);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get version: NACK(1)\n");
		return -1;
	}
	
	dev.version = uart_read_byte();
	dev.option1 = uart_read_byte();
	dev.option2 = uart_read_byte();
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get version: NACK(2)\n");
		return -1;
	}
	
	printf("Device version: 0x%02x Option1: 0x%02x Option2: 0x%02x\n", dev.version, dev.option1, dev.option2);
	
	return 0;
}
int getCommand()
{
	int res;
	
	printf(">>> get command\n");
	
	uart_send_cmd(0x00);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get command: NACK(1)\n");
		return -1;
	}
	
	int len = uart_read_byte() + 1;
	// printf ("getcmd len: %d\n", len);
	
	char d[50];
	memset(d, 0, 50);
	uart_read_data(d, len);
	dev.bootVersion = d[0];
	
	for (int i = 0; i < 11; i++)
		dev.cmds[i] = d[i + 1];
	printf("Bootloader version: 0x%02x = v%d.%d\n", dev.bootVersion, dev.bootVersion >> 4, dev.bootVersion & 0x0f);
	printf("Get command    is 0x%02x\n", dev.cmds[GET]);
	printf("Get version    is 0x%02x\n", dev.cmds[GET_VERSION]);
	printf("Get ID         is 0x%02x\n", dev.cmds[GET_ID]);
	printf("Read memory    is 0x%02x\n", dev.cmds[READ_MEMORY]);
	printf("Go             is 0x%02x\n", dev.cmds[GO]);
	printf("Write memory   is 0x%02x\n", dev.cmds[WRITE_MEMORY]);
	printf("Erase command  is 0x%02x\n", dev.cmds[ERASE]);
	printf("Write Protect  is 0x%02x\n", dev.cmds[WRITE_PROTECT]);
	printf("Write Unpotect is 0x%02x\n", dev.cmds[WRITE_UNPROTECT]);
	printf("Read Protect   is 0x%02x\n", dev.cmds[READOUT_PROTECT]);
	printf("Read Unprotect is 0x%02x\n", dev.cmds[READOUT_UNPROTECT]);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get cmd: NACK(2)\n");
		return -1;
	}
	
	return 0;
}
int getID()
{
	int res;
	
	printf(">>> get id\n");
	
	uart_send_cmd(dev.cmds[GET_ID]);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get id: NACK(1)\n");
		return -1;
	}
	
	int len = uart_read_byte() + 1;
	printf("get id: length: %d\n", len);
	
	char d[50];
	memset(d, 0, 50);
	uart_read_data(d, len);
	
	dev.id = (d[0] << 8) | d[1];
	
	printf("Device ID: 0x%04x\n", dev.id);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get id: NACK(2)\n");
		return -1;
	}
	
	return 0;
}
int readoutUnprotect()
{
	int res;
	
	uart_send_cmd(0x92);
	
	res = uart_read_ack_nack(2000);
	if (res != ACK)
	{
		printf("Readout Unprotect command first NACK'ed\n");
		return -1;
	}
	
	res = uart_read_ack_nack(2000);
	if (res != ACK)
	{
		printf("Readout Unprotect command second NACK'ed\n");
		return -1;
	}
	
	printf("Readout Unprotect succeed\n");
	
	usleep(1000 * 1000);
	
	return 0;
}
int readoutProtect()
{
	int res;
	
	uart_send_cmd(0x82);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("Readout Protect command first NACK'ed\n");
		return -1;
	}
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("Readout Protect command second NACK'ed\n");
		return -1;
	}
	
	printf("Readout Protect succeed\n");
	
	return 0;
}
int writeUnprotect()
{
	int res;
	
	uart_send_cmd(0x73);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("Write Unprotect command first NACK'ed\n");
		return -1;
	}
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("Write Unprotect command second NACK'ed\n");
		return -1;
	}
	
	printf("Write Unprotect succeed\n");
	
	usleep(1000 * 1000);
	
	return 0;
}
int getPageSize(int page)
{
	if (page <= 3)
		return 16 * 1024;
	if (page == 4)
		return 64 * 1024;
	else
		return 128 * 1024;
}
int erase()
{
	int res;
	
	if (dev.cmds[ERASE] == 0x44)
	{
		printf("Erasing device with Extended Erase command\n");
		
		uart_send_cmd(0x44);
		res = uart_read_ack_nack();
		if (res != ACK)
		{
			printf("Erase NACK'ed\n");
			return -1;
		}
		
		printf("Erase ACK'ed\n");
		
		map<int, int> pages;
		
		// TODO: optimize
		for (int i = 0; i < hexFile.parts.size(); i++)
		{
			TPart* part = hexFile.parts[i];
			for (uint32_t addr = part->getStartAddr(); addr <= part->getEndAddr(); addr += 256)
			{
				for (int j = 0; j < flashPages; j++)
				{
					if (addr >= flashLayout[j].sector_start && addr <= flashLayout[j].sector_start + flashLayout[j].sector_size - 1)
					{
						pages[flashLayout[j].sector_num] = 1;
					}
				}
			}
		}
		
		int pagesToEraseCnt = pages.size();
		printf("pages to erase %d\r\n", pagesToEraseCnt);
		
		uint8_t data[2 + pagesToEraseCnt * 2];
		data[0] = (pagesToEraseCnt - 1) >> 8;
		data[1] = (pagesToEraseCnt - 1) & 0xff;
		int idx = 0;
		for (map<int, int>::iterator it = pages.begin(); it != pages.end(); it++)
		{
			int pageNr = it->first;
			printf("page %d\r\n", pageNr);
			data[2 + idx * 2] = pageNr >> 8;
			data[2 + idx * 2 + 1] = pageNr & 0xff;
			idx++;
		}
		
		uart_write_data_checksum(data, sizeof(data));
		
		res = uart_read_ack_nack(40000);
		if (res == ACK)
		{
			printf("Mass Erase ACK'ed\n");
			return 0;
		}
		else if (res == NACK)
		{
			printf("Mass Erase NACK'ed\n");
			return -1;
		}
		else
		{
			printf("Timeout\n");
			return -1;
		}
	}
	else if (dev.cmds[ERASE] == 0x43)
	{
		return -1;
		// printf("erasing device with standard Erase command\n");
		
		// uart_send_cmd(0x43);
		// int res = uart_read_ack_nack();
		// if (res == ACK)
		// {
		// if (excludedPages.size() == 0)
		// {
		// uart_send_cmd(0xff);
		// }
		// else
		// {
		// int pagesToEraseCnt = 128;
		// uint16_t pages = pagesToEraseCnt - 1 - excludedPages.size();
		
		// uint8_t data[1 + pages + 1];
		// int idx = 1;
		// data[0] = pages;
		// for (uint16_t i = 0; i < pagesToEraseCnt; i++)
		// if (find(excludedPages.begin(), excludedPages.end(), i) == excludedPages.end())
		// data[idx++] = i;
		
		// uart_write_data_checksum(data, sizeof(data));
		// }
		
// retry:
		// res = uart_read_ack_nack();
		// printf("res: 0x%x\n", res);
		// if (res != ACK && res != NACK)
		// goto retry;
		// if (res == NACK)
		// {
		// printf("device NACKed\n");
		// return -1;
		// }
		// printf("device erased\n");
		// return 0;
		// }
		// else
		// {
		// printf("NACK received 0x%02x\n", res);
		// return -1;
		// }
	}
	else
	{
		printf("unknown Erase command\n");
		return -1;
	}
}
