#include "TRoboCOREHeader.h"

#include "utils.h"

bool TRoboCOREHeader::isClear()
{
	uint8_t* ptr = (uint8_t*)this;
	for (int i = 0; i < sizeof(TRoboCOREHeader); i++)
		if (*ptr++ != 0xff)
			return false;
	return true;
}
bool TRoboCOREHeader::isValid()
{
	if (headerVersion == 0x01)
	{
		return true;
	}
	else if (headerVersion == 0x02)
	{
		uint16_t crc = crc16_calc((uint8_t*)this, sizeof(TRoboCOREHeader) - 2);
		return crc == checksum;
	}
	return false;
}
void TRoboCOREHeader::calcChecksum()
{
	checksum = crc16_calc((uint8_t*)this, sizeof(TRoboCOREHeader) - 2);
}
