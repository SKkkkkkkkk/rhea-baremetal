#include <stdio.h>
#include "nor_flash.h"
int main()
{
	void nor_flash_test(spi_id_t spi_id, flash_model_t flash_model);
	nor_flash_test(BOOTSPI_ID, UNKNOWN_FLASH);
	return 0;
}