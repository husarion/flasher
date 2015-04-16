#ifndef __DEVICES_H__
#define __DEVICES_H__

#include <stdint.h>

enum ECmd
{
	GET, GET_VERSION, GET_ID, READ_MEMORY, GO, WRITE_MEMORY,
	ERASE, WRITE_PROTECT, WRITE_UNPROTECT, READOUT_PROTECT, READOUT_UNPROTECT
};

struct stm32_dev_info_t;

struct stm32_dev_t
{
	uint16_t id;
	uint8_t version, option1, option2, bootVersion;
	uint8_t cmds[11];
	stm32_dev_info_t *info;
};

typedef struct 
{
  uint32_t sector_start;
  uint32_t sector_size;
  uint32_t sector_num;
} tFlashSector;

extern const tFlashSector flashLayout[];
extern const int flashPages;

const uint32_t OPTION_BYTE_1 = 0x1fffc000;
const uint32_t OPTION_BYTE_2 = 0x1fffc008;

const uint32_t OTP_START = 0x1fff7800;
const uint32_t OTP_LOCK_START = 0x1fff7a00;

void parseVersion(uint32_t version, int& a, int& b, int& c, int& d);

#endif
