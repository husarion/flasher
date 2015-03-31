#include "ihex.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>

using namespace std;

uint32_t strhex2int(string str)
{
	uint32_t val;
	stringstream ss;
	ss << str;
	ss >> hex >> val;
	return val;
}

int str2int(string str)
{
	int val;
	stringstream ss;
	ss << str;
	ss >> val;
	return val;
}

THexFile::THexFile()
{
}
THexFile::~THexFile()
{
}
#include <unistd.h>

int THexFile::load(const std::string& path)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return false;
	uint32_t maxAddr = 0;
	uint32_t extAddr2 = 0, extAddr4 = 0;
	char lineData[128];
	totalLength = 0;
	// fseek(f, 0, SEEK_SET);
	while (fgets(lineData, sizeof(lineData), f) != NULL)
	{
		string line = lineData;
		uint32_t len = strhex2int(line.substr(1, 2));
		uint32_t addr = strhex2int(line.substr(3, 4));
		// printf("addr 0x%08x\r\n", addr);
		// return 0;
		int type = str2int(line.substr(7, 2));
		switch (type)
		{
		case 0:
		{
			uint32_t dstAddr = extAddr4 + extAddr2 + addr;
			// printf("finding 0x%08x\r\n", dstAddr);
			TPart *p = findPart(dstAddr);
			// printf("got part 0x%08x cur len: %d\r\n", p->startAddr, p->data.size());
			// usleep(10000);
			totalLength += len;
			for (int i = 0; i < len; i++)
			{
				uint32_t dstAddr = extAddr4 + extAddr2 + addr + i;
				uint8_t byte = strhex2int(line.substr(9 + i * 2, 2));
				p->data.push_back(byte);
				// printf("0x%08x\r\n", dstAddr);
				// data[dstAddr] = byte;
			}
		}
		break;
		case 1:
			break;
		case 2:
			extAddr2 = strhex2int(line.substr(9, 4)) * 16;
			printf("extAddr2 = 0x%08x\r\n", extAddr2);
			break;
		case 3:
			break;
		case 4:
			extAddr4 = strhex2int(line.substr(9, 4)) << 16;
			printf("extAddr4 = 0x%08x\r\n", extAddr4);
			break;
		case 5:
			break;
		}
	}
	fclose(f);
	return true;
}

TPart* THexFile::findPart(uint32_t addr)
{
	for (uint32_t i = 0; i < parts.size(); i++)
	{
		TPart *p = parts[i];
		if (p->hasAddr(addr - 1))
			return p;
	}
	TPart *p = new TPart();
	p->startAddr = addr;
	parts.push_back(p);
	printf("adding part 0x%08x\r\n", addr);
	return p;
}

uint32_t TPart::getStartAddr()
{
	return startAddr;
}
uint32_t TPart::getEndAddr()
{
	return startAddr + data.size() - 1;
}
bool TPart::hasAddr(uint32_t addr)
{
	return addr >= getStartAddr() && addr <= getEndAddr();
}
uint32_t TPart::getLen()
{
	return data.size();
}
