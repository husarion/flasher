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

#define TIMEOUT (1000)

uint32_t SWAP32(uint32_t v)
{
	return ((v & 0x000000ff) << 24) | ((v & 0x0000ff00) << 8) |
	       ((v & 0x00ff0000) >> 8) | ((v & 0xff000000) >> 24);
}

int HardFlasher::init()
{
	m_serial = 0;
	return 0;
}
int HardFlasher::open()
{
	m_serial = uart_open(m_baudrate, false);
	if (m_serial)
	{
		printf(" connected\r\n");
		printf("Checking settings... ");
		int r = uart_check_gpio(m_serial);
		if (r)
		{
			uart_close(m_serial);
			m_serial = 0;
			printf(" FTDI settings changed, RoboCORE must be replugged to take changes into account.\r\n");
			printf("Unplug RoboCORE.");
			bool restarted = false;
			for (;;)
			{
				if (!restarted)
				{
					m_serial = uart_open(m_baudrate, false);
					if (m_serial == 0)
					{
						restarted = true;
						printf(" OK, plug it again.");
					}
					else
					{
						uart_close(m_serial);
						m_serial = 0;
					}
				}
				else
				{
					m_serial = uart_open(m_baudrate, false);
					if (m_serial)
						break;
				}
				TimeUtilDelayMs(10);
				static int cnt = 0;
				if (cnt++ == 15)
				{
					printf(".");
					cnt = 0;
				}
			}
			printf(" connected\r\n");
		}
		else
		{
			printf(" OK\r\n");
		}
		return 0;
	}
	else
	{
		return -1;
	}
}
int HardFlasher::close()
{
	if (m_serial)
	{
		uart_reset_normal(m_serial);
		uart_close(m_serial);
		m_serial = 0;
	}
	return 0;
}
int HardFlasher::start()
{
	int lastMsg = -1;
	printf("Connecting to RoboCORE...");
	
	for (;;)
	{
		if (open())
		{
			if (lastMsg == -1)
			{
				lastMsg = 0;
				printf(" plug in RoboCORE..");
			}
			else
			{
				printf(".");
			}
			TimeUtilDelayMs(200);
		}
		else
		{
			break;
		}
	}
	
	printf("Connecting to bootloader..");
	for (;;)
	{
		if (uart_reset(m_serial))
		{
			printf(" failed\r\n");
			return -1;
		}
		
		if (uart_tx(m_serial, "\x7f", 1) == -1)
		{
			printf(" failed\r\n");
			return -1;
		}
		
		int res = uart_read_ack_nack_fast();
		// printf("res 0x%02x\r\n", (unsigned char)res);
		if (res == ACK || res == NACK)
		{
			// printf("connected\n");
			// printf("device answered with 0x%02x\n", (unsigned char)res);
			
			// if (getVersion())
			// return -1;
			printf(" ");
			if (getCommand())
				return -1;
			// if (getID())
			// return -1;
			
			printf("OK\n");
			break;
		}
		else
		{
			printf(".");
		}
	}
	return 0;
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
	
	// printf(">>> get command\n");
	
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
	// printf("Bootloader version: 0x%02x = v%d.%d\n", m_dev.bootVersion, m_dev.bootVersion >> 4, m_dev.bootVersion & 0x0f);
	// printf("Get command    is 0x%02x\n", m_dev.cmds[GET]);
	// printf("Get version    is 0x%02x\n", m_dev.cmds[GET_VERSION]);
	// printf("Get ID         is 0x%02x\n", m_dev.cmds[GET_ID]);
	// printf("Read memory    is 0x%02x\n", m_dev.cmds[READ_MEMORY]);
	// printf("Go             is 0x%02x\n", m_dev.cmds[GO]);
	// printf("Write memory   is 0x%02x\n", m_dev.cmds[WRITE_MEMORY]);
	// printf("Erase command  is 0x%02x\n", m_dev.cmds[ERASE]);
	// printf("Write Protect  is 0x%02x\n", m_dev.cmds[WRITE_PROTECT]);
	// printf("Write Unpotect is 0x%02x\n", m_dev.cmds[WRITE_UNPROTECT]);
	// printf("Read Protect   is 0x%02x\n", m_dev.cmds[READOUT_PROTECT]);
	// printf("Read Unprotect is 0x%02x\n", m_dev.cmds[READOUT_UNPROTECT]);
	
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
		// printf("Erasing device with Extended Erase command\n");
		
		uart_send_cmd(0x44);
		res = uart_read_ack_nack();
		if (res != ACK)
		{
			printf("ERROR\n");
			// printf("Erase NACK'ed\n");
			return -1;
		}
		
		// printf("Erase ACK'ed\n");
		
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
		// printf("Pages to erase: ", pagesToEraseCnt);
		
		uint8_t data[2 + pagesToEraseCnt * 2];
		data[0] = (pagesToEraseCnt - 1) >> 8;
		data[1] = (pagesToEraseCnt - 1) & 0xff;
		int idx = 0;
		for (map<int, int>::iterator it = pages.begin(); it != pages.end(); it++)
		{
			int pageNr = it->first;
			// printf("%d ", pageNr);
			data[2 + idx * 2] = pageNr >> 8;
			data[2 + idx * 2 + 1] = pageNr & 0xff;
			idx++;
		}
		// printf("\n");
		
		uart_write_data_checksum(data, sizeof(data));
		
		res = uart_read_ack_nack(40000);
		if (res == ACK)
		{
			printf("OK\n");
			// printf("Mass Erase ACK'ed\n");
			return 0;
		}
		else if (res == NACK)
		{
			printf("ERROR\n");
			// printf("Mass Erase NACK'ed\n");
			return -1;
		}
		else
		{
			printf("ERROR\n");
			// printf("Timeout\n");
			return -1;
		}
	}
	else if (m_dev.cmds[ERASE] == 0x43)
	{
		printf("ERROR\n");
		return -1; // not supported
	}
	else
	{
		printf("ERROR (unknown command)\n");
		return -2;
	}
}
int HardFlasher::flash()
{
	char buf[256 + 1];
	
	uint32_t sent = 0;
	
	for (unsigned int i = 0; i < m_hexFile.parts.size(); i++)
	{
		TPart* part = m_hexFile.parts[i];
		
		uint32_t curAddr = part->getStartAddr();
		uint8_t* data = part->data.data();
		
		while (curAddr <= part->getEndAddr())
		{
			int len = part->getEndAddr() - curAddr + 1;
			if (len > 256) len = 256;
			
			// printf("writing 0x%08x len: %d...\n", curAddr, len);
			
			uart_send_cmd(0x31);
			
			int res = uart_read_ack_nack();
			if (res != ACK)
			{
				if (m_callback)
					m_callback(-1, -1);
				printf("ERROR\n");
				return -1;
			}
			uint32_t tmp = SWAP32(curAddr);
			uart_write_data_checksum((char*)&tmp, 4);
			
			res = uart_read_ack_nack();
			if (res != ACK)
			{
				if (m_callback)
					m_callback(-1, -1);
				printf("ERROR\n");
				return -1;
			}
			buf[0] = len - 1;
			memcpy(buf + 1, data, len);
			
			uart_write_data_checksum(buf, len + 1);
			res = uart_read_ack_nack();
			
			if (res != ACK)
			{
				if (m_callback)
					m_callback(-1, -1);
				printf("ERROR\n");
				return -1;
			}
			
			sent += len;
			curAddr += len;
			data += len;
			
			if (m_callback)
				m_callback(sent, m_hexFile.totalLength);
		}
	}
	if (m_callback)
		m_callback(-1, -1);
	printf("OK\n");
	
	return 0;
}
int HardFlasher::reset()
{
	close();
	printf("OK\n");
	return 0;
}

int HardFlasher::uart_send_cmd(uint8_t cmd)
{
	uint8_t buf[] = { cmd, (uint8_t)~cmd };
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
