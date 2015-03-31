#ifndef __IHEX_H__
#define __IHEX_H__

#include <stdint.h>

#include <string>
#include <vector>

class TPart
{
public:
	uint32_t startAddr;
	std::vector<uint8_t> data;
	
	uint32_t getStartAddr();
	uint32_t getEndAddr();
	bool hasAddr(uint32_t addr);
	uint32_t getLen();
};

class THexFile
{
public:
	THexFile();
	~THexFile();
	
	int load(const std::string& path);
	
	int totalLength;
	
	std::vector<TPart*> parts;
	
private:
	TPart* findPart(uint32_t addr);
};


#endif
