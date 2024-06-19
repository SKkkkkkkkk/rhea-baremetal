#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "chip.h"
#include "xmodem.h"
#include "nor_flash.h"

#define NOR_FLASH_OFFSET (512*1024U)
#define FIP_MAX_SIZE 	 (1024*1024U)
void program_norflash(unsigned char *buf, int buflen)
{
	assert((NOR_FLASH_OFFSET & 0xfff) == 0);

	static uint32_t size = 0;
	if((size & 0xfff) == 0)
		flash_sector_erase(BOOTSPI_ID, NOR_FLASH_OFFSET + size);
	flash_write_dma(BOOTSPI_ID, NOR_FLASH_OFFSET + size, buf, buflen);
	size += buflen;
}

int main(void)
{
	int st;
	flash_init(BOOTSPI_ID, 20, 3, UNKNOWN_FLASH);

	printf("FWU_SRAM: Receiving FIP image...\n");
	while( (st = xmodemReceiveWithAction(program_norflash, FIP_MAX_SIZE)) < 0)
		printf("FWU_SRAM: Xmodem receive error: status: %d, retrying...\n", st);
	printf("FWU_SRAM: Xmodem successfully received %d bytes\n", st);
	printf("FWU_SRAM: System is about to restart...\n");
	system_reset();
    return 0;
}