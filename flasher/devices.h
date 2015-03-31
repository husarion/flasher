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
struct stm32_dev_info_t
{
	uint16_t id;
	uint32_t flashStart, flashEnd;
	uint32_t optStart, optEnd;
	int sectors;
};

typedef struct 
{
  uint32_t sector_start;
  uint32_t sector_size;
  uint32_t sector_num;
} tFlashSector;

extern const tFlashSector flashLayout[];
extern const int flashPages;

extern stm32_dev_info_t devices[];

stm32_dev_info_t* findDeviceInfo(const stm32_dev_t& device);

#endif
