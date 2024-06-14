#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nor_flash.h"
#include "memmap.h"


#define APRAM_SIZE	  (512*1024U)
#define AP_DRAM_SIZE  (384*1024*1024U)

#define NOR_FIP_OFFSET (1024*1024U)
#define FIP_MAX_SIZE   (128*1024U)

extern char _all_end;
#define WBUF_ADDR (&_all_end)
#define RBUF_ADDR (WBUF_ADDR + FIP_MAX_SIZE)
// uint8_t* wbuf = (uint8_t*)WBUF_ADDR;
// uint8_t* rbuf = (uint8_t*)RBUF_ADDR;

uint8_t wbuf[FIP_MAX_SIZE];
uint8_t rbuf[FIP_MAX_SIZE];

bool golden_pattern_check()
{
	for(uint32_t i = 0;i<FIP_MAX_SIZE;i++)
		if(wbuf[i] != 0x5a)
			return false;
	return true;
}

int main()
{
	assert((NOR_FIP_OFFSET&0xfff) == 0);
	// if(wbuf >= (uint8_t*)AP_DRAM_BASE)
	// {
	// 	if((rbuf+FIP_MAX_SIZE) >= (uint8_t*)(AP_DRAM_BASE + AP_DRAM_SIZE))
	// 	{
	// 		printf("rbuf out of range\n\r");
	// 		return -1;
	// 	}
	// }
	// else
	// {
	// 	if((rbuf+FIP_MAX_SIZE) >= (uint8_t*)(APRAM_BASE + APRAM_SIZE))
	// 	{
	// 		printf("rbuf out of range\n\r");
	// 		return -1;
	// 	}
	// }

	// write golden pattern
	memset(wbuf, 0x5a, FIP_MAX_SIZE);

	flash_init(BOOTSPI_ID, 50, 3, UNKNOWN_FLASH);
	
	uint32_t i = 1;
	uint8_t flash_id[3];
	while(1)
	{
		flash_read_id(BOOTSPI_ID, flash_id, 3);

		for(uint32_t i = 0;i<((FIP_MAX_SIZE/4096)+1);i++)
			flash_sector_erase(BOOTSPI_ID, NOR_FIP_OFFSET+(4096*i));

		// check wbuf before write
		// for(uint32_t i = 0;i<FIP_MAX_SIZE;i++)
		// 	if(wbuf[i] != 0x5a)
		// 		printf("wbuf[%u] != 0x5a\n\r", i);

		flash_write(BOOTSPI_ID, NOR_FIP_OFFSET, wbuf, FIP_MAX_SIZE);

		memset(rbuf, 0xa5, FIP_MAX_SIZE);
		flash_read_quad_dma(BOOTSPI_ID, NOR_FIP_OFFSET, rbuf, FIP_MAX_SIZE);

		// check wbuf after read
		// for(uint32_t i = 0;i<FIP_MAX_SIZE;i++)
		// 	if(wbuf[i] != 0x5a)
		// 		printf("after flash read, wbuf[%u] != 0x5a\n\r", i);

		if(golden_pattern_check() == false)
			printf("after flash read, wbuf != golden pattern\n\r");
		else
			printf("after flash read, wbuf == golden pattern\n\r");

		printf("No.%u flash_id: 0x%02x%02x%02x, ", i, flash_id[0], flash_id[1], flash_id[2]);
		if(memcmp(wbuf, rbuf, FIP_MAX_SIZE)==0)
			printf("nor flash rw test pass, size %u\n\r", FIP_MAX_SIZE);
		else
			printf("nor flash rw test fail, size %u\n\r", FIP_MAX_SIZE);

		if(i++ >= 10)
			break;
	}

	printf("nor flash test done\n\r");
}