#include "HardFlasher.h"

#include <stdio.h>
#include <string.h>

#include <vector>
#include <map>

using namespace std;

#include "myFTDI.h"
#include "timeutil.h"

#define ACK 0x79
#define NACK 0x1f

uint32_t SWAP32(uint32_t v)
{
	return ((v & 0x000000ff) << 24) | ((v & 0x0000ff00) << 8) |
	       ((v & 0x00ff0000) >> 8) | ((v & 0xff000000) >> 24);
}

int HardFlasher::init()
{
	m_serial = 0;
}
int HardFlasher::open()
{
	printf("opening device...\n");
	m_serial = uart_open(m_baudrate);
}
int HardFlasher::close()
{
	if (m_serial)
		uart_close(m_serial);
}
int HardFlasher::start()
{
retry:
	close();
	open();

	printf("sending init...\n");
	uart_tx(m_serial, "\x7f", 1);
	
	int res = uart_read_ack_nack_fast();
	printf("res 0x%02x\r\n", (unsigned char)res);
	if (res == ACK || res == NACK)
	{
		printf("device answered with 0x%02x\n", (unsigned char)res);
		
		if (getVersion())
			return -1;
		if (getCommand())
			return -1;
		if (getID())
			return -1;
			
		return 0;
	}
	goto retry;
}
int HardFlasher::getVersion()
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
	
	m_dev.version = uart_read_byte();
	m_dev.option1 = uart_read_byte();
	m_dev.option2 = uart_read_byte();
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get version: NACK(2)\n");
		return -1;
	}
	
	printf("Device version: 0x%02x Option1: 0x%02x Option2: 0x%02x\n", m_dev.version, m_dev.option1, m_dev.option2);
	
	return 0;
}
int HardFlasher::getCommand()
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
	m_dev.bootVersion = d[0];
	
	for (int i = 0; i < 11; i++)
		m_dev.cmds[i] = d[i + 1];
	printf("Bootloader version: 0x%02x = v%d.%d\n", m_dev.bootVersion, m_dev.bootVersion >> 4, m_dev.bootVersion & 0x0f);
	printf("Get command    is 0x%02x\n", m_dev.cmds[GET]);
	printf("Get version    is 0x%02x\n", m_dev.cmds[GET_VERSION]);
	printf("Get ID         is 0x%02x\n", m_dev.cmds[GET_ID]);
	printf("Read memory    is 0x%02x\n", m_dev.cmds[READ_MEMORY]);
	printf("Go             is 0x%02x\n", m_dev.cmds[GO]);
	printf("Write memory   is 0x%02x\n", m_dev.cmds[WRITE_MEMORY]);
	printf("Erase command  is 0x%02x\n", m_dev.cmds[ERASE]);
	printf("Write Protect  is 0x%02x\n", m_dev.cmds[WRITE_PROTECT]);
	printf("Write Unpotect is 0x%02x\n", m_dev.cmds[WRITE_UNPROTECT]);
	printf("Read Protect   is 0x%02x\n", m_dev.cmds[READOUT_PROTECT]);
	printf("Read Unprotect is 0x%02x\n", m_dev.cmds[READOUT_UNPROTECT]);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get cmd: NACK(2)\n");
		return -1;
	}
	
	return 0;
}
int HardFlasher::getID()
{
	int res;
	
	printf(">>> get id\n");
	
	uart_send_cmd(m_dev.cmds[GET_ID]);
	
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
	
	m_dev.id = (d[0] << 8) | d[1];
	
	printf("Device ID: 0x%04x\n", m_dev.id);
	
	res = uart_read_ack_nack();
	if (res != ACK)
	{
		printf("get id: NACK(2)\n");
		return -1;
	}
	
	return 0;
}
int HardFlasher::erase()
{
	int res;
	
	if (m_dev.cmds[ERASE] == 0x44)
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
		for (int i = 0; i < m_hexFile.parts.size(); i++)
		{
			TPart* part = m_hexFile.parts[i];
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
	else if (m_dev.cmds[ERASE] == 0x43)
	{
		return -1; // not supported
	}
	else
	{
		printf("unknown Erase command\n");
		return -1;
	}
}
int HardFlasher::flash()
{
	char buf[256 + 1];
	
	uint32_t curAddr = 0x08008000;
	uint32_t sent = 0;
	
	uint32_t startTime = TimeUtilGetSystemTimeMs();
	
	for (int i = 0; i < m_hexFile.parts.size(); i++)
	{
		TPart* part = m_hexFile.parts[i];
		
		uint32_t curAddr = part->getStartAddr();
		uint8_t* data = part->data.data();
		
		while (curAddr < part->getEndAddr())
		{
			int len = part->getEndAddr() - curAddr;
			if (len > 256) len = 256;
			
			printf("writing 0x%08x len: %d...\n", curAddr, len);
			
			uart_send_cmd(0x31);
			
			int res = uart_read_ack_nack();
			if (res == ACK)
			{
				uint32_t tmp = SWAP32(curAddr);
				uart_write_data_checksum((char*)&tmp, 4);
				
				res = uart_read_ack_nack();
				if (res == ACK)
				{
					printf("to send: %d\n", len);
					
					buf[0] = len - 1;
					memcpy(buf + 1, data, len);
					
					uart_write_data_checksum(buf, len + 1);
					res = uart_read_ack_nack();
					
					if (res == ACK)
					{
						sent += len;
						curAddr += len;
						data += len;
						printf("ok 0x%08x %d of %d\n", curAddr, sent, m_hexFile.totalLength);
					}
					else
					{
						printf("failed\n");
						return -1;
					}
				}
			}
			else
			{
				return -1;
			}
		}
	}
	
	uint32_t endTime = TimeUtilGetSystemTimeMs();
	float time = endTime - startTime;
	float avg = m_hexFile.totalLength / (time / 1000.0f) / 1024.0f;
	
	printf("time: %d ms avg speed: %.2f KBps (%d bps)\n", endTime - startTime, avg, (int)(avg * 8.0f * 1024.0f));
	
	return 0;
}

#define TIMEOUT (1000)


int HardFlasher::uart_send_cmd(uint8_t cmd)
{
	char buf[] = { cmd, ~cmd };
	return uart_tx(m_serial, buf, 2);
}

int HardFlasher::uart_read_ack_nack()
{
	char buf[1];
	int r = uart_rx(m_serial, buf, 1, TIMEOUT);
	if (r == -1) return -1;
	return buf[0];
}
int HardFlasher::uart_read_ack_nack(int timeout)
{
	char buf[1];
	int r = uart_rx(m_serial, buf, 1, timeout);
	if (r == -1) return -1;
	return buf[0];
}
int HardFlasher::uart_read_ack_nack_fast()
{
	char buf[1];
	int r = uart_rx(m_serial, buf, 1, 100);
	if (r == -1) return -1;
	return buf[0];
}

int HardFlasher::uart_read_byte()
{
	char b;
	int r = uart_rx(m_serial, &b, 1, TIMEOUT);
	return r == -1 ? -1 : b;
}
int HardFlasher::uart_read_data(void* data, int len)
{
	uint8_t* _data = (uint8_t*)data;
	int r = uart_rx(m_serial, _data, len, TIMEOUT * len);
	return r == -1 ? -1 : r;
}

int HardFlasher::uart_write_data_checksum(const void* data, int len)
{
	const uint8_t* _data = (uint8_t*)data;
	char chk = _data[0];
	for (int i = 1; i < len; i++)
		chk ^= _data[i];
	int w = uart_tx(m_serial, _data, len);
	if (w == -1) return -1;
	w = uart_tx(m_serial, &chk, 1);
	return w == -1 ? -1 : len;
}
int HardFlasher::uart_write_data(const void* data, int len)
{
	const uint8_t* _data = (uint8_t*)data;
	int w = uart_tx(m_serial, _data, len);
	return w == -1 ? -1 : len;
}
int HardFlasher::uart_write_byte(char data)
{
	int w = uart_tx(m_serial, &data, 1);
	return w == -1 ? -1 : 1;
}
