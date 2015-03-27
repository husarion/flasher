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

#include <algorithm>

using namespace std;

extern stm32_dev_t dev;

extern uint32_t firmwareLen;

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
		
		// if (excludedPages.size() == 0)
		// {
		// uart_write_data_checksum("\xff\xff", 2);
		// }
		// else
		{
		
			uint32_t curAddr = 0;
			int startPage=-1;
			int endPage;
			// firmwareLen=
			for (int i = 0; i < 256; i++)
			{
				uint32_t start = curAddr;
				uint32_t end = start + getPageSize(i);
				printf("s 0x%08x e 0x%08x 0x%08x\r\n", start, end, getPageSize(i));
				curAddr += getPageSize(i);
				
				if (startPage==-1 && start >= 0x00008000)
				{
					startPage = i;
				}
				if (end > firmwareLen + 0x00008000)
				{
					endPage = i;
					break;
				}
			}
			
			printf("sp %d ep %d\r\n", startPage, endPage);
			// exit(0);
			
			// int startPage = 2;
			int pagesToEraseCnt = endPage - startPage + 1;//(firmwareLen + pageSize - 1) / pageSize;
			printf("pages %d\r\n", pagesToEraseCnt);
			uint16_t pages = pagesToEraseCnt - 1;
			
			uint8_t data[2 + (pages + 1) * 2];
			data[0] = pages >> 8;
			data[1] = pages & 0xff;
			int idx = 1;
			for (uint16_t i = 0; i < pagesToEraseCnt; i++)
			{
				int pageNr = startPage + i;
				printf("page %d\r\n", pageNr);
				data[2 + idx * 2] = pageNr >> 8;
				data[2 + idx * 2 + 1] = pageNr & 0xff;
				idx++;
			}
			
			uart_write_data_checksum(data, sizeof(data));
		}
		
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
