#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "chip.h"
#include "xmodem.h"
#include "dw_apb_gpio.h"
#include "nor_flash.h"
#include "nand_flash.h"

// Error codes
#define XMODEM_ERROR_NO_FLASH		(XMODEM_ERROR_USER_BEGIN-0) // -6
#define XMODEM_ERROR_NAND_ERASE		(XMODEM_ERROR_USER_BEGIN-1) // -7
#define XMODEM_ERROR_NAND_PROGRAM	(XMODEM_ERROR_USER_BEGIN-2) // -8

/******************/
#define FLASH_OFFSET (512*1024U)
#define FIP_MAX_SIZE 	 (1024*1024U)
int program_nor(unsigned char *buf, int buflen)
{
	assert((FLASH_OFFSET & 0xfff) == 0);

	static uint32_t size = 0;

	if(buf == NULL && buflen == 0) // ! end of transmission signal
		return XMODEM_ERROR_OK;

	if((size & 0xfff) == 0)
		flash_sector_erase(BOOTSPI_ID, FLASH_OFFSET + size);
	flash_write_dma(BOOTSPI_ID, FLASH_OFFSET + size, buf, buflen);
	size += buflen;

	return XMODEM_ERROR_OK;
}

int program_nand(unsigned char *buf, int buflen)
{
    assert((FLASH_OFFSET & (NAND_BLOCK_SIZE-1)) == 0);

    static uint32_t pos = 0;
    static uint8_t page_buf[NAND_PAGE_SIZE];
    uint32_t page_buf_pos = pos & (NAND_PAGE_SIZE-1);

    // End of transmission signal
    if(buf == NULL && buflen == 0)
    {
        if(page_buf_pos != 0)
            if(nand_flash_page_program(BOOTSPI_ID, FLASH_OFFSET + pos - page_buf_pos, page_buf) != 0)
                return XMODEM_ERROR_NAND_PROGRAM;
        return XMODEM_ERROR_OK;
    }

    // Align to block size, erase block
    if((pos & (NAND_BLOCK_SIZE-1)) == 0)
        if(nand_flash_erase(BOOTSPI_ID, FLASH_OFFSET + pos) != 0)
            return XMODEM_ERROR_NAND_ERASE;

    // Handle buffer that may exceed the remaining space in the page buffer
    while(buflen > 0)
    {
        uint32_t copy_len = NAND_PAGE_SIZE - page_buf_pos;
        if(copy_len > buflen)
            copy_len = buflen;

        memcpy(page_buf + page_buf_pos, buf, copy_len);

        // Align to page size, program page
		if((page_buf_pos + copy_len) == NAND_PAGE_SIZE)
		{
			if(nand_flash_page_program(BOOTSPI_ID, FLASH_OFFSET + pos - page_buf_pos, page_buf) != 0)
            	return XMODEM_ERROR_NAND_PROGRAM;
		}

		buflen -= copy_len;
		buf += copy_len;
        pos += copy_len;
		page_buf_pos = pos & (NAND_PAGE_SIZE-1);
    }

    return XMODEM_ERROR_OK;
}

int program_flash(unsigned char *buf, int buflen)
{
	static bool is_init = false;
	static bool is_nand = false;
	if (!is_init) {
		flash_init(BOOTSPI_ID, 10, 3, UNKNOWN_FLASH);
		is_init = true;
		uint8_t flash_id[3];
		flash_read_id(BOOTSPI_ID, flash_id, 3);
		if(flash_id[1]==0xff || flash_id[2]==0xff || flash_id[1]==0x00 || flash_id[2]==0x00)
			return XMODEM_ERROR_NO_FLASH;
		if(flash_id[0]==0xff || flash_id[0]==0x00)
		{
			nand_flash_reset(BOOTSPI_ID);
			nand_set_feature(BOOTSPI_ID, FEATURE_REG_NAND_BLKLOCK_REG_ADDR, 0x00); // disable block lock
			is_nand = true;
		}
		else
			is_nand = false;
	}

	return is_nand ? program_nand(buf, buflen) : program_nor(buf, buflen);
}


/******************/
#define FWU_KEY_PIN 1
#define BYPSECURE_KEY_PIN 	 2
#define FIP_STORAGE_KEY_PIN0 4

enum boot_device {
	BOOT_DEVICE_EMMC = 0,
	BOOT_DEVICE_BOOTSPI = 1,
	BOOT_DEVICE_NONE = 2
};
static enum boot_device boot_dev = BOOT_DEVICE_NONE;

enum boot_device get_boot_dev(void)
{
	pinmux_select(PORTB, FIP_STORAGE_KEY_PIN0, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTB,
		.pin = FIP_STORAGE_KEY_PIN0,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode
	};
	gpio_init(&gpio_init_config);

	uint8_t pin_status = \
	(uint8_t)gpio_read_pin(PORTB, FIP_STORAGE_KEY_PIN0);

	return ((pin_status&1) == 0) ? BOOT_DEVICE_EMMC : BOOT_DEVICE_BOOTSPI;
}

static action_t program_table[] = {
	[BOOT_DEVICE_EMMC]		= program_flash, // TODO: Currently using program_flash instead
	[BOOT_DEVICE_BOOTSPI]	= program_flash,
};

int main(void)
{
	int st;
	
	// Get boot device
	boot_dev = get_boot_dev();
	if (boot_dev == BOOT_DEVICE_NONE) {
		printf("FWU_SRAM: Boot Device detection failed, Check GPIO_PROGSEL\n");
		while(1) asm volatile("wfi");
	}
	printf("FWU_SRAM: Boot Device: %s\n", (boot_dev == BOOT_DEVICE_BOOTSPI) ? "BOOTSPI" : "EMMC");

	// Xmodem receive FIP image
	// ! You cannot use uart(e.g., printf) while xmodem is running.
	printf("FWU_SRAM: Receiving FIP image...\n");
	while( (st = xmodemReceiveWithAction(program_table[boot_dev], FIP_MAX_SIZE)) < 0)
		printf("FWU_SRAM: Xmodem receive error: status: %d, retrying...\n", st);
	printf("FWU_SRAM: Xmodem successfully received %d bytes\n", st);

	// Reset the system
	printf("FWU_SRAM: System is about to restart...\n");
	system_reset();
	UNREACHABLE();
}