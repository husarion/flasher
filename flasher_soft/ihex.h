/*
 * Copyright (C) 2015 Krystian Dużyński <krystian.duzynski@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

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

class IHexFile
{
public:
	IHexFile();
	~IHexFile();
	
	int load(const std::string& path);
	
	uint8_t *data;
	int length;
	
	std::vector<TPart*> parts;
	
private:
	TPart* findPart(uint32_t addr);
};


#endif
