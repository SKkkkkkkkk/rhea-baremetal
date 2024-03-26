#include <stdio.h>
#include "nor_flash.h"
int main()
{
	void flash_fastest_read_test(spi_id_t spi_id, flash_model_t flash_model);
	void nor_flash_test(spi_id_t spi_id, flash_model_t flash_model);
	nor_flash_test(BOOTSPI_ID, UNKNOWN_FLASH);
	// flash_fastest_read_test(BOOTSPI_ID, GD25LQ64);
	return 0;
}