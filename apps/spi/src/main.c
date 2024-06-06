#include <stdio.h>
#include <string.h>
#include "nor_flash.h"
#include "nand_flash.h"

int main()
{
	void flash_fastest_read_test(spi_id_t spi_id, flash_model_t flash_model, uint16_t clk_div);
	void nor_flash_test(spi_id_t spi_id, flash_model_t flash_model, uint16_t clk_div);
	void nand_test();
	nor_flash_test(BOOTSPI_ID, UNKNOWN_FLASH, 30);
	// flash_fastest_read_test(BOOTSPI_ID, UNKNOWN_FLASH, 30);
	// nand_test();
	return 0;
}

#define	FEATURE_REG_NAND_BLKLOCK_REG_ADDR 		 0xA0
#define	FEATURE_REG_NAND_CONFIGURATION_REG_ADDR  0xB0
#define	FEATURE_REG_NAND_STATUS_REG_ADDR 		 0xC0
#define	FEATURE_REG_NAND_DIE_SELECT_REC_ADDR	 0xD0

void nand_test()
{
	nand_flash_init(BOOTSPI_ID, 30, 3, UNKNOWN_NAND_FLASH);
	nand_flash_reset(BOOTSPI_ID);
	uint8_t flash_id[3];
	// while(1)
	// {
		nand_flash_read_id(BOOTSPI_ID, flash_id, 3);
		printf("flash_id: %x %x %x\n\r", flash_id[0], flash_id[1], flash_id[2]);
	// }

	uint8_t protection = nand_get_feature(BOOTSPI_ID, FEATURE_REG_NAND_BLKLOCK_REG_ADDR);
	uint8_t config = nand_get_feature(BOOTSPI_ID, FEATURE_REG_NAND_CONFIGURATION_REG_ADDR);
	uint8_t status = nand_get_feature(BOOTSPI_ID, FEATURE_REG_NAND_STATUS_REG_ADDR);
	uint8_t die_select = nand_get_feature(BOOTSPI_ID, FEATURE_REG_NAND_DIE_SELECT_REC_ADDR);

	printf("protection: 0x%x\n\r", protection);
	printf("config: 0x%x\n\r", config);
	printf("status: 0x%x\n\r", status);
	printf("die_select: 0x%x\n\r", die_select);

	nand_set_feature(BOOTSPI_ID, FEATURE_REG_NAND_BLKLOCK_REG_ADDR, 0x00); // disable block lock

	printf("protection: 0x%x\n\r", nand_get_feature(BOOTSPI_ID, FEATURE_REG_NAND_BLKLOCK_REG_ADDR));

#if 0
	uint8_t w_buf[NAND_PAGE_SIZE];
	uint8_t r_buf[NAND_PAGE_SIZE] = {0};
	for(int i=0; i<NAND_PAGE_SIZE; i++)
		w_buf[i] = i%256;

	#define TEST_ADDR 0
	nand_flash_erase(BOOTSPI_ID, TEST_ADDR); // erase 1 block(64 pages)
	nand_flash_page_program(BOOTSPI_ID, TEST_ADDR, w_buf);
	nand_flash_read(BOOTSPI_ID, TEST_ADDR, r_buf, NAND_PAGE_SIZE);

	if(memcmp(w_buf, r_buf, NAND_PAGE_SIZE)==0)
		printf("nand flash rw test pass\n\r");
	else
		printf("nand flash rw test fail\n\r");
#else
	#define TEST_ADDR (512*1024)
	#define TEST_SIZE (128*1024)
	uint8_t* w_buf = (uint8_t*)0xa0000000UL;
	uint8_t* r_buf = w_buf + TEST_SIZE;

	for(int i=0; i<TEST_SIZE/NAND_BLOCK_SIZE; i++)
		nand_flash_erase(BOOTSPI_ID, TEST_ADDR + i*NAND_BLOCK_SIZE);
	for(int i=0; i<TEST_SIZE/NAND_PAGE_SIZE; i++)
		nand_flash_page_program(BOOTSPI_ID, TEST_ADDR + i*NAND_PAGE_SIZE, w_buf + i*NAND_PAGE_SIZE);
	
	nand_flash_read(BOOTSPI_ID, TEST_ADDR, r_buf, TEST_SIZE);

	if(memcmp(w_buf, r_buf, TEST_SIZE)==0)
		printf("nand flash rw test pass\n\r");
	else
		printf("nand flash rw test fail\n\r");
#endif

}