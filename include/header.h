#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdint.h>

#pragma pack(1)
class TRoboCOREHeader
{
public:
	uint8_t headerVersion, type;
	uint32_t version, id;
	uint8_t key[19];
	uint16_t checksum;
	
	bool isClear();
	bool isValid();

	void calcChecksum();
};
#pragma pack()

#endif
