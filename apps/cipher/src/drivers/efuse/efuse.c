#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "efuse.h"
#include "systimer.h"
#include "efuse_regs.h"

#define EFUSE_SPACE_ADDR 64
#define EFUSE_SKEY_BASE 12
#define EFUSE_PKEY_BASE 20

static bool g_efuse_pka_attach = false;

static void software_reset_enble(bool is_disable)
{
	// uint32_t tmp = EFUSE->EFUSE_MOD_U.ALL;
	if (is_disable) { /* 解复位 */
		EFUSE->EFUSE_MOD_U.efuse_soft_rstn = 1;
		while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
			;
	} else {
		EFUSE->EFUSE_MOD_U.efuse_soft_rstn = 0;
	}
}

#if 0
static void set_timing_reg(void)
{
	// EFUSE->EFUSE_T_RD = 10;
	// EFUSE->EFUSE_T_SQ = 10;
	// EFUSE->EFUSE_T_HR_LD = 2;
}
#endif
static int efuse_burn_byate(uint32_t data, uint32_t addr)
{
	uint32_t offnum = 0;
	printf(">>> data = 0x%x addr = %d\n", data, addr);
	for (offnum = 0; offnum < 32; offnum++) {
		if ((data >> offnum) & 0x1) {
			while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
				;
			EFUSE->EFUSE_MOD_U.efuse_mode = EFUSE_MODE_AUTOWR;
			EFUSE->EFUSE_MOD_U.efuse_addr = addr + (offnum << 6); /* offnum * 64 */
			EFUSE->EFUSE_INT_MASK_U.efuse_int_irq_mask = 1;
			EFUSE->EFUSE_START = 0xabcdef98;
			while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
				;
		}
	}
	return 0;
}

static void efuse_rd_spacc_key(void)
{
	while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
		;
	EFUSE->EFUSE_MOD_U.efuse_mode = EFUSE_MODE_SKEYRD;
	EFUSE->EFUSE_MOD_U.efuse_addr = EFUSE_SKEY_BASE;
	EFUSE->EFUSE_INT_MASK_U.efuse_int_irq_mask = 1;
	EFUSE->EFUSE_START = 1;
	while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
		;
}

static void efuse_rd_pka_key(efuse_pka_memsel_type memsel, uint32_t memaddr, uint32_t memsizebyte)
{
	software_reset_enble(1);
	while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
		;
	// EFUSE_SET_EFUSE_MOD(3); // auto read
	EFUSE->EFUSE_MOD_U.efuse_mode = EFUSE_MODE_PKEYRD;
	// EFUSE_SET_EFUSE_ADDR(9); // the-first-addr is 9
	EFUSE->EFUSE_MOD_U.efuse_addr = EFUSE_PKEY_BASE;
	// EFUSE_SET_EFUSE_INT_IRQ_MASK(1); // mask interrupt
	EFUSE->EFUSE_INT_MASK_U.efuse_int_irq_mask = 1;
	// EFUSE_SET_EFUSE_PKA_MEMSEL(memsel);
	EFUSE->EFUSE_PKA_MEM_U.efuse_pka_memsel = memsel;
	EFUSE->EFUSE_PKA_MEM_U.efuse_pka_memaddr = memaddr;
	// EFUSE_SET_EFUSE_PKA_MEMADDR(memaddr);
	// EFUSE_SET_EFUSE_PKA_BYTECNT(memsizebyte);
	EFUSE->EFUSE_PKA_MEM_U.efuse_pka_num = (memsizebyte / 4) - 1;
	EFUSE->EFUSE_START = 0xabcdef98;
	while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy)
		;
}

static uint32_t efuse_rd_auto_to_apb(uint32_t addr)
{
	software_reset_enble(1);
	uint32_t dout = 0;
	// EFUSE_SET_EFUSE_MOD(3); // auto read
	while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy);
	EFUSE->EFUSE_MOD_U.efuse_mode = EFUSE_MODE_AUTORD;
	// EFUSE_SET_EFUSE_ADDR(9); // the-first-addr is 9
	EFUSE->EFUSE_MOD_U.efuse_addr = addr;
	// EFUSE_SET_EFUSE_INT_IRQ_MASK(1); // mask interrupt
	EFUSE->EFUSE_START = 1;
	while (EFUSE->EFUSE_INT_STATUS_U.efuse_busy);
	systimer_delay(10, IN_US);
	dout = EFUSE->EFUSE_DOUT;

	return dout;
}

int efuse_init(void)
{
	software_reset_enble(0);
	software_reset_enble(1);
	// set_timing_reg();
	return 0;
}

int efuse_burn_key(uint32_t *key, uint32_t key_len, uint32_t aes_or_ecc)
{
	if (key == NULL || key_len > EFUSE_SPACE_ADDR) {
		return -1;
	}
	uint32_t i;
	int ret = -1;
	for (i = 0; i < key_len / sizeof(uint32_t); i++) {
		if (aes_or_ecc == 0) {
			ret = efuse_burn_byate(key[i], EFUSE_SKEY_BASE + i);
		} else if (aes_or_ecc == 1) {
			ret = efuse_burn_byate(key[i], EFUSE_PKEY_BASE + i);
		} else {
			return -1;
		}
		if (ret != 0) {
			return -1;
		}
	}

	// for (i = 0; i < key_len / sizeof(uint32_t); i++ ) {
	// 	efuse_rd_auto_to_apb(EFUSE_PKEY_BASE + i);
	// }
	return 0;
}

int efuse_private_aes_key(void)
{
	g_efuse_pka_attach = false;
	efuse_rd_spacc_key();
	return 0;
}

int efuse_private_ecc_key(void)
{
	g_efuse_pka_attach = true;
	efuse_rd_pka_key(EFUSE_PKA_RAM_A, 0, 32); /* 256 bit key */
	return 0;
}

int efuse_pka_get_key(efuse_pka_memsel_type memsel, uint32_t memaddr, uint32_t bytcnt)
{
	if (g_efuse_pka_attach != true) {
		return -1;
	}

	efuse_rd_pka_key(memsel, memaddr, bytcnt);

	return 0;
}

int efuse_auto_read_data(uint32_t addr, uint32_t *data, uint32_t len)
{
	if (addr + (len / 4) > 29) {
//		return -1;
	}
	uint32_t i  = 0;
	// int ret;
	for (i = 0; i < (len / 4); i ++) {
		data[i] = efuse_rd_auto_to_apb(addr + i);
		printf("addr = %d data = 0x%x\n", addr + i, data[i]);
	}

	return 0;
}

int efuse_auto_write_data(uint32_t addr, uint32_t *data, uint32_t len)
{
	if (addr < 17 || addr > 29 || addr + (len / 4) > 29) {
//		return -1;
	}
	uint32_t i  = 0;
	int ret;
	for (i = 0; i < (len / 4); i ++) {
		ret = efuse_burn_byate(data[i], addr + i);
		if (ret != 0) {
			return -1;
		}
	}

	return 0;
}

int efuse_read_and_write_protect(uint32_t start_addr, uint32_t len, bool write)
{
	if (start_addr + len >= 60) {
		return -1;
	}

	uint32_t h_data = 0;
	uint32_t l_data = 0;
	uint32_t i = 0;
	uint32_t h_addr = 61, l_addr = 60;

	for (i = 0; i < len; i++) {
        if (start_addr + i >= 32) {
            h_data = 1 << (start_addr + i - 32);
        } else {
            l_data = 1 << (start_addr + i);
        }
	}

	if (write == true) {
		h_addr = 63;
		l_addr = 62;
	}
	return (efuse_burn_byate(h_data, h_addr) || efuse_burn_byate(l_data, l_addr));
}

void efuse_deinit(void)
{
	g_efuse_pka_attach = false;
	software_reset_enble(0);
}
