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
	
	bool isClear()
	{
		uint8_t* ptr = (uint8_t*)this;
		for (int i = 0; i < sizeof(TRoboCOREHeader); i++)
			if (*ptr++ != 0xff)
				return false;
		return true;
	}
};
#pragma pack()

#endif
