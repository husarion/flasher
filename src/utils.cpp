#include "utils.h"

uint16_t crc16_calc(uint8_t* data, int len)
{
	int i;

	uint16_t crc = 0;
	
	while (len--)
	{
		crc ^= *data++;
		for (i = 0; i < 8; ++i)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = (crc >> 1);
		}
	}
	
	return crc;
}
