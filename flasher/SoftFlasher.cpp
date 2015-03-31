#include "SoftFlasher.h"

#include <stdio.h>
#include <string.h>

#include <vector>
#include <map>

using namespace std;

#include "xcpmaster.h"
#include "timeutil.h"

int SoftFlasher::init()
{
	return 0;
}
int SoftFlasher::open()
{
	if (!XcpMasterInit(m_device.c_str(), m_baudrate))
		return -1;
	return 0;
}
int SoftFlasher::close()
{
	XcpMasterDeinit();
	return 0;
}
int SoftFlasher::start()
{
	for (;;)
	{
		int res = open();
		if (res != 0)
			return -1;
			
		if (!XcpMasterConnect())
		{
			printf("Unable to connect to bootloader, reset your RoboCORE...\n");
		}
		else
		{
			printf("Connected\r\n");
			
			printf("Initializing programming session... ");
			if (!XcpMasterStartProgrammingSession())
			{
				printf("ERROR\n");
				XcpMasterDisconnect();
				XcpMasterDeinit();
				return -1;
			}
			printf("OK\n");
			break;
		}
	}
	return 0;
}
int SoftFlasher::erase()
{
	for (unsigned int i = 0; i < m_hexFile.parts.size(); i++)
	{
		TPart* part = m_hexFile.parts[i];
		printf("erasing 0x%08x len: %d\r\n", part->getStartAddr(), part->getLen());
		if (!XcpMasterClearMemory(part->getStartAddr(), part->getLen()))
		{
			printf("ERROR\n");
			XcpMasterDisconnect();
			XcpMasterDeinit();
			return -1;
		}
	}
	printf("OK\n");
	return 0;
}
int SoftFlasher::flash()
{
	uint32_t processedBytes = 0;
	
	printf("Programming data. Please wait...");
	for (unsigned int i = 0; i < m_hexFile.parts.size(); i++)
	{
		TPart* part = m_hexFile.parts[i];
		uint32_t curAddr = part->getStartAddr();
		uint8_t* data = part->data.data();
		
		while (curAddr <= part->getEndAddr())
		{
			int len = part->getEndAddr() - curAddr + 1;
			if (len > 255) len = 255;
			
			printf("writing 0x%08x len: %d\r\n", curAddr, len);
			if (!XcpMasterProgramData(curAddr, len, data))
			{
				printf("ERROR\n");
				XcpMasterDisconnect();
				XcpMasterDeinit();
				return -1;
			}
			
			data += len;
			curAddr += len;
			processedBytes += len;
			
			if (m_callback)
				m_callback(processedBytes, m_hexFile.totalLength);
		}
		
	}
	printf("OK\n");
	
	printf("Finishing programming session... ");
	if (!XcpMasterStopProgrammingSession())
	{
		printf("ERROR\n");
		XcpMasterDisconnect();
		XcpMasterDeinit();
		return -1;
	}
	printf("OK\n");
	
	printf("Performing software reset... ");
	if (!XcpMasterDisconnect())
	{
		printf("ERROR\n");
		XcpMasterDeinit();
		return -1;
	}
	printf("OK\n");
	
	return 0;
}
