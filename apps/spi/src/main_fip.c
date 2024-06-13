#include <stdio.h>
#include <string.h>
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
int main()
{
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

	memset(wbuf, 0x5a, FIP_MAX_SIZE);
	flash_init(BOOTSPI_ID, 50, 3, UNKNOWN_FLASH);
	
	uint32_t i = 1;
	while(1)
	{
	flash_read_id(BOOTSPI_ID, rbuf, 3);
	printf("No.%u flash_id: 0x%02x%02x%02x, ", i, rbuf[0], rbuf[1], rbuf[2]);


	for(uint32_t i = 0;i<(FIP_MAX_SIZE/4096);i++)
		flash_sector_erase(BOOTSPI_ID, (NOR_FIP_OFFSET&0xfffff000)+4096*i);

	flash_write(BOOTSPI_ID, NOR_FIP_OFFSET, wbuf, FIP_MAX_SIZE);

	memset(rbuf, 0xa5, FIP_MAX_SIZE);
	flash_read_quad_dma(BOOTSPI_ID, NOR_FIP_OFFSET, rbuf, FIP_MAX_SIZE);

	if(memcmp(wbuf, rbuf, FIP_MAX_SIZE)==0)
		printf("nor flash 128KB rw test pass\n\r");
	else
		printf("nor flash 128KB rw test fail\n\r");
	// return 0;
	if(i++ >= 5)
		break;
	}

	printf("nor flash test done\n\r");
}