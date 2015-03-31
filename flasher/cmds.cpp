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

extern THexFile hexFile;


int hard_readoutUnprotect()
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
int hard_readoutProtect()
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
int hard_writeUnprotect()
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
