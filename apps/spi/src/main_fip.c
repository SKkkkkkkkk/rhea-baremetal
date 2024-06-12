#include <stdio.h>
#include <string.h>
#include "nor_flash.h"

#define NOR_FIP_OFFSET (1024*1024U)
#define FIP_MAX_SIZE   (128*1024U)

#define WBUF_ADDR 0x42000000U
#define RBUF_ADDR (WBUF_ADDR + FIP_MAX_SIZE)
uint8_t* wbuf = (uint8_t*)WBUF_ADDR;
uint8_t* rbuf = (uint8_t*)RBUF_ADDR;

int main()
{
	flash_init(BOOTSPI_ID, 50, 3, UNKNOWN_FLASH);
	
	uint32_t i = 1;
	while(1)
	{
	flash_read_id(BOOTSPI_ID, rbuf, 3);
	printf("flash_id: 0x%02x%02x%02x\n\r", rbuf[0], rbuf[1], rbuf[2]);


	for(uint32_t i = 0;i<(FIP_MAX_SIZE/4096);i++)
		flash_sector_erase(BOOTSPI_ID, (NOR_FIP_OFFSET&0xfffff000)+4096*i);

	flash_write(BOOTSPI_ID, NOR_FIP_OFFSET, wbuf, FIP_MAX_SIZE);

	flash_read(BOOTSPI_ID, NOR_FIP_OFFSET, rbuf, FIP_MAX_SIZE);

	if(memcmp(wbuf, rbuf, FIP_MAX_SIZE)==0)
		printf("nor flash rw test pass\n\r");
	else
		printf("nor flash rw test fail\n\r");
	// return 0;
	if(i++ > 20)
		break;
	}
}