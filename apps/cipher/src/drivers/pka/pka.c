#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "systimer.h"
#include "pka.h"
#include "pka_fw.h"
#include "pka_regs.h"
#include "efuse.h"
#include "cipher_debug.h"

#define writel(d, a) *(volatile uint32_t *)(uintptr_t)(a) = (d)
#define readl(a) (*(volatile uint32_t *)(uintptr_t)(a))

#if 0
#define udelay(x)                                                                                                      \
	do {                                                                                                           \
		unsigned xx = x;                                                                                       \
		while (xx--) {                                                                                         \
			asm volatile("nop");                                                                           \
		}                                                                                                      \
	} while (0)
#endif

#define PKA_CTRL_GO (1 << 31)
#define PKA_CTRL_STOP_RQST (1 << 27)
#define PKA_CTRL_M521_MODE (0xff << 16)
#define PKA_CTRL_BASE_RADIX (0x7 << 8)
#define PKA_CTRL_BASE_RADIX_256BIT (0x2 << 8)
#define PKA_CTRL_BASE_RADIX_512BIT (0x3 << 8)
#define PKA_CTRL_BASE_RADIX_1024BIT (0x4 << 8)
#define PKA_CTRL_BASE_RADIX_2048BIT (0x5 << 8)
#define PKA_CTRL_BASE_RADIX_4096BIT (0x6 << 8)

#define PKA_STAT_DONE (1 << 30)

#define PKA_RTNCODE_BUSY (1 << 31)
#define PKA_RTNCODE_ZERO (1 << 28)
#define PKA_RTNCODE_IRQ (1 << 30)
#define PKA_RTNCODE_DPA_EN (1 << 27)
#define PKA_RTNCODE_STOP_REASON (0xff << 16)
#define PKA_RTNCODE_NOR_STOP (0 << 16)
#define PKA_RTNCODE_STOP_INV_OPCODE (1 << 16)
#define PKA_RTNCODE_STOP_STACK_UNDERFLOW (2 << 16)
#define PKA_RTNCODE_STOP_STACK_OVERFLOW (3 << 16)
#define PKA_RTNCODE_STOP_WDG (4 << 16)
#define PKA_RTNCODE_STOP_HOST_REQ (5 << 16)
#define PKA_RTNCODE_STOP_MEM_PORT_COLLIS (8 << 16)

#define PKA_IRQ_EN_IE (1 << 30)

typedef enum {
	PKA_FWEP_MODMULT = 0xa, /* Modular Multiplication :  Calculate x * y mod m */
	PKA_FWEP_MODADD = 0xb, /* Modular Addition :  Calculate x + y mod m */
	PKA_FWEP_MODSUB = 0xc, /* Modular Subtraction : Calculate x - y mod m */
	PKA_FWEP_MODDIV = 0xd, /* Modular Division : Calculate y/x mod m */
	PKA_FWEP_MODINV = 0xe, /* Modular Inversion : Calculate 1/x mod m */
	PKA_FWEP_REDUCE = 0xf, /* Modular Reduction : Calculate x mod m */
	PKA_FWEP_CALC_MP = 0x10,
	PKA_FWEP_CALC_R_INV =
		0x11, /* Montgomery r^-1 :  r_inv = 1/r mod m ; mp = ((r * r-1)-1)/m; r_sqr = (r * r) mod m */
	PKA_FWEP_CALC_R_SQR = 0x12, /* Montgomery r^2 */
	PKA_FWEP_BIT_SERIAL_MOD = 0x13, /* Modular Reduction : Calculate y/x mod m */
	PKA_FWEP_BIT_SERIAL_MOD_DP = 0x14, /* Modular Reduction - Double Precision : Calculate y/x mod m */
	PKA_FWEP_MULT = 0x15, /* Modular Reduction :  Calculate y/x mod m */
	PKA_FWEP_MODEXP = 0x16, /* Modular Exponentiation :  Calculate xy mod m */
	PKA_FWEP_CRT_KEY_SETUP = 0x17, /* Chinese Remainder Theorem key setup : Calculate CRT key setup coefficients */
	PKA_FWEP_CRT = 0x18, /* Modular Exponentiation : Calculate xy mod m */
	PKA_FWEP_PMULT = 0x19, /* ECC Point Multiplication :  Calculate Q = kP */
	PKA_FWEP_PDBL = 0x1a, /* ECC Point Double :  Calculate Q = 2P */
	PKA_FWEP_PDBL_STD_PRJ = 0x1b,
	PKA_FWEP_PADD = 0x1c, /* ECC Point Addition :  Calculate R = P + Q */
	PKA_FWEP_PADD_STD_PRJ = 0x1d,
	PKA_FWEP_PVER = 0x1e, /* ECC Point Verification :  Calculate y2 == x3 + ax + b mod p */
	PKA_FWEP_STD_PRJ_TO_AFFINE = 0x1f,
	PKA_FWEP_IS_P_EQUAL_Q = 0x20,
	PKA_FWEP_IS_P_REFLECT_Q = 0x21,
	PKA_FWEP_IS_A_M3 = 0x22,
	PKA_FWEP_SHAMIR = 0x23, /* ECC Shamir's Trick : Calculate R = kP + lQ */
	PKA_FWEP_BUTT
} pka_fw_entry_point;

static bool g_is_init = false;
static pka_base_modul_adapter g_pka_adp = { 0 };

static void udelay(uint64_t us)
{
	systimer_delay(us, IN_US);
	// uint64_t cnt = us * 100;
	// bool floop = true;
	// while (floop)
	// {
	// 	cnt--;
	// 	if (cnt == 0) {
	// 		floop = false;
	// 	}
	// }

}

static pka_reg *__pka_get_reg_base(void)
{
	if (g_is_init != true) {
		cipher_debug_err("pka not init\n");
		return NULL;
	}
	return PKA;
}

static void __pka_load_data(uint32_t start_addr, uint32_t *value, uint32_t len)
{
	for (int i = 0; i < len; i++) {
		writel(value[i], start_addr + i * 4);
	}
}

static void __pka_get_data(uint32_t start_Addr, uint32_t *result, uint32_t len)
{
	int value;
	for (int i = 0; i < len; i++) {
		value = readl(start_Addr + i * 4);
		result[i] = value;
	}
}

static int __pka_run(int start_code, pka_reg *reg_ptr, pka_base_radix_type radix_type)
{
	uint32_t ctrl = 0;
	uint32_t stat;
	/* clear residula flag : should be set to 0 prior to starting new PKA operations
       unless a function specificallyrequests otherwise.*/
	writel(0x0, &reg_ptr->FLAGS);

	/* clear the stack pointer :  set to 0 prior to starting new PKA operations.*/
	writel(0x0, &reg_ptr->STACK_PNTR);

	/* 0 = No Byte Lane Swap (little endian) */
	writel(0x0, &reg_ptr->CONFIG);

	writel(start_code, &reg_ptr->ENTRY_PNT);

	if (radix_type == PKA_BASE_RADIX_256BIT) {
		ctrl |= PKA_CTRL_BASE_RADIX_256BIT;
	} else if (radix_type == PKA_BASE_RADIX_512BIT) {
		ctrl |= PKA_CTRL_BASE_RADIX_512BIT;
	} else if (radix_type == PKA_BASE_RADIX_1024BIT) {
		ctrl |= PKA_CTRL_BASE_RADIX_1024BIT;
	} else if (radix_type == PKA_BASE_RADIX_2048BIT) {
		ctrl |= PKA_CTRL_BASE_RADIX_2048BIT;
	} else if (radix_type == PKA_BASE_RADIX_4096BIT) {
		ctrl |= PKA_CTRL_BASE_RADIX_4096BIT;
	} else {
		cipher_parem_invalid();
		return -1;
	}

	ctrl |= PKA_CTRL_GO;

	/* run pka */
	writel(ctrl, &reg_ptr->CTRL);
	uint32_t cnt = 0;

	while (1) {
		stat = readl(&reg_ptr->STAT);
		if ((stat & PKA_STAT_DONE) != 0) { /* clear interrupt and stop pka */
			writel(PKA_STAT_DONE, &reg_ptr->STAT);
			break;
		}
		udelay(100);
		cnt++;
		if (cnt > 20000000) {
			cipher_debug_err("error: run pka timeout !\n");
			return -1;
		}
	}

	writel(0x0, &reg_ptr->STACK_PNTR);
	uint32_t ret_code = readl(&reg_ptr->RTN_CODE);
	if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_NOR_STOP) {
		return 0;
	} else if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_STOP_INV_OPCODE) {
		cipher_debug_err("error: return code Invalid op-code !\n");
	} else if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_STOP_STACK_UNDERFLOW) {
		cipher_debug_err("error: return code Stack underflow !\n");
	} else if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_STOP_STACK_OVERFLOW) {
		cipher_debug_err("error: return code Stack overflow !\n");
	} else if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_STOP_WDG) {
		cipher_debug_err("error: return code Watchdog !\n");
	} else if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_STOP_MEM_PORT_COLLIS) {
		cipher_debug_err("error: return code Memory port collision !\n");
	} else if ((ret_code & PKA_RTNCODE_STOP_REASON) == PKA_RTNCODE_STOP_HOST_REQ) {
		cipher_debug_err("error: return code Host request !\n");
	} else {
		cipher_debug_err("error: stop return code = 0x%x\n", (ret_code & PKA_RTNCODE_STOP_REASON) >> 16);
	}

	return -1;
}

static void pka_fw_init(void)
{
	static uint32_t fw_code[PKA_FW_SIZE / 4] = PKA_FW_CODE;
	pka_reg *reg_ptr = __pka_get_reg_base();
	int i = 0;
	if (reg_ptr != 0) {
		for (i = 0; i < PKA_FW_SIZE / 4; i++) {
			writel(fw_code[i], &reg_ptr->FW_MEM[i]);
		}
	}
}

static void pka_irq_en(void)
{
	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr != NULL) {
		writel(0x0, &reg_ptr->STACK_PNTR);
		writel(PKA_IRQ_EN_IE, &reg_ptr->IRQ_EN);
		/* 0 = No Byte Lane Swap (little endian) */
		writel(0x0, &reg_ptr->CONFIG);
	}
}

static int __pka_check_zero(uint32_t *x, uint32_t len)
{
	int i = 0;
	for (i = 0; i < len / sizeof(int); i++) {
		if (x[i] != 0) {
			return 0;
		}
	}

	return -1;
}

static int __pka_montgomery_rinv(uint32_t *m, uint32_t *r_inv, int len, pka_base_radix_type radix_type)
{
	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_CALC_R_INV, reg_ptr, radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), r_inv, len / sizeof(int)); /* C0 */
	return 0;
}

static int __pka_montgomery_mp(uint32_t *r_inv, uint32_t *m, uint32_t *mp, uint32_t len, pka_base_radix_type radix_type)
{
	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), r_inv, len / sizeof(int)); /* C0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_CALC_MP, reg_ptr, radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[(len / sizeof(int)) * 1]), mp, len / sizeof(int)); /* D1 */
	return 0;
}

static int __pka_montgomery_rsqr(uint32_t *r_inv, uint32_t *m, uint32_t *r_sqr, uint32_t len,
				 pka_base_radix_type radix_type)
{
	if (((len * 8) != 256) && ((len * 8) != 512) && ((len * 8) != 1024) && ((len * 8) != 2048)) {
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), r_inv, len / sizeof(int)); /* C0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_CALC_R_SQR, reg_ptr, radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[(len / sizeof(int)) * 3]), r_sqr, len / sizeof(int)); /* D3 */
	return 0;
}

#define PKA_BANK_MAX_LENGTH 512

static int pka_montgomery_operator_calc(uint32_t *m, uint32_t len, pka_base_radix_type radix_type)
{
	int ret;
	static uint32_t r_inv[PKA_BANK_MAX_LENGTH / 4] = { 0 }, mp[PKA_BANK_MAX_LENGTH / 4] = { 0 },
			 r_sqr[PKA_BANK_MAX_LENGTH / 4] = { 0 };
	memset(r_inv, 0, sizeof(r_inv));
	memset(mp, 0, sizeof(mp));
	memset(r_sqr, 0, sizeof(r_sqr));

	if (__pka_check_zero(m, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	ret = __pka_montgomery_rinv(m, r_inv, len, radix_type);
	if (ret != 0) {
		cipher_function_err(__pka_montgomery_rinv);
		return -1;
	}

	ret = __pka_montgomery_mp(r_inv, m, mp, len, radix_type);
	if (ret != 0) {
		cipher_function_err(__pka_montgomery_mp);
		return -1;
	}

	ret = __pka_montgomery_rsqr(r_inv, m, r_sqr, len, radix_type);
	if (ret != 0) {
		cipher_function_err(__pka_montgomery_rsqr);
		return -1;
	}

	return 0;
}

static uint32_t pka_get_len(pka_base_radix_type radix_type)
{
	int len = 0;
	switch (radix_type) {
	case PKA_BASE_RADIX_256BIT:
		len = 32;
		break;
	case PKA_BASE_RADIX_512BIT:
		len = 64;
		break;
	case PKA_BASE_RADIX_1024BIT:
		len = 128;
		break;
	case PKA_BASE_RADIX_2048BIT:
		len = 256;
		break;
	case PKA_BASE_RADIX_4096BIT:
		len = 512;
		break;
	default:
		return 0;
	}

	return len;
}

static int __pka_check_len(pka_base_radix_type radix_type, uint32_t len)
{
	switch (radix_type) {
	case PKA_BASE_RADIX_256BIT:
		if ((len * 8) != 256) {
			return -1;
		}
		break;
	case PKA_BASE_RADIX_512BIT:
		if ((len * 8) != 512) {
			return -1;
		}
		break;
	case PKA_BASE_RADIX_1024BIT:
		if ((len * 8) != 1024) {
			return -1;
		}
		break;
	case PKA_BASE_RADIX_2048BIT:
		if ((len * 8) != 2048) {
			return -1;
		}
		break;
	case PKA_BASE_RADIX_4096BIT:
		if ((len * 8) != 4096) {
			return -1;
		}

		break;
	default:
		return -1;
		break;
	}

	return 0;
}

/* c = x•y mod m provided (0 <= x < m) and (0 <= y < m) */
static int pka_modul_mult(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c, uint32_t len)
{
	if (adp == NULL || y == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (x == NULL) {
		if (!((adp->is_sec_key == 1) && (adp->is_mult_key == 1))) {
			cipher_pointer_invalid();
			return -1;
		}
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		cipher_debug_err("check len error!\n");
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}
	int ret;

	ret = pka_montgomery_operator_calc(m, len, adp->radix_type);
	if (ret != 0) {
		cipher_function_err(pka_montgomery_operator_calc);
		return -1;
	}

	if ((adp->is_sec_key == 1) && (adp->is_mult_key == 1)) {
		ret = efuse_pka_get_key(EFUSE_PKA_RAM_A, 0, len); /* A0 */
		if (ret != 0) {
			cipher_function_err(efuse_pka_get_key);
			return -1;
		}
	} else {
		__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), x, len / sizeof(int)); /* A0 */
	}
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_B[0]), y, len / sizeof(int)); /* B0 */

	if (__pka_run(PKA_FWEP_MODMULT, reg_ptr, adp->radix_type) != 0) {
		cipher_function_err(__pka_run);
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), c, len / sizeof(int)); /* A0 */
	return 0;
}

/* c = x + y mod m provided (0 <= x < 2m) and (0 <= y < 2m). */
static int pka_modul_add(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c, uint32_t len)
{
	if (adp == NULL || x == NULL || y == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), x, len / sizeof(int)); /* A0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_B[0]), y, len / sizeof(int)); /* B0 */

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_MODADD, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), c, len / sizeof(int)); /* A0 */

	return 0;
}

/* c = x - y mod m provided (0 <= x < 2m) and (0 <= y < 2m). */
static int pka_modul_sub(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c, uint32_t len)
{
	if (adp == NULL || x == NULL || y == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), x, len / sizeof(int)); /* A0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_B[0]), y, len / sizeof(int)); /* B0 */

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_MODSUB, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), c, len / sizeof(int)); /* A0 */

	return 0;
}

/* c = x mod m provided (0 <= x < 2m). */
static int pka_modul_reduction(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *m, uint32_t *c, uint32_t len)
{
	if (adp == NULL || x == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), x, len / sizeof(int)); /* C0 */

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_REDUCE, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), c, len / sizeof(int)); /* A0 */

	return 0;
}

/* c = y / x mod m provided (0 < x < m) and (0 < y < m) */
static int pka_modul_division(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *y, uint32_t *m, uint32_t *c,
			      uint32_t len)
{
	if (adp == NULL || x == NULL || y == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_zero(x, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), y, len / sizeof(int)); /* C0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), x, len / sizeof(int)); /* A0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_MODDIV, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), c, len / sizeof(int)); /* C0 */

	return 0;
}

/* c = 1/x mod m provided (0 <= x < m). */
static int pka_modul_inversion(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *m, uint32_t *c, uint32_t len)
{
	if (adp == NULL || x == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_zero(x, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), x, len / sizeof(int)); /* A0 */

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_MODINV, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), c, len / sizeof(int)); /* C0 */

	return 0;
}

/* c = a * b */
static int pka_non_modul_muilt(pka_base_modul_adapter *adp, uint32_t *a, uint32_t *b, uint32_t *c_lo, uint32_t *c_hi,
			       uint32_t len)
{
	if (adp == NULL || a == NULL || b == NULL || c_hi == NULL || c_lo == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_A[0]), a, len / sizeof(int)); /* A0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_B[0]), b, len / sizeof(int)); /* B0 */

	if (__pka_run(PKA_FWEP_MULT, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), c_lo, len / sizeof(int));
	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[(len / sizeof(int)) * 1]), c_hi, len / sizeof(int)); /* C1 */
	return 0;
}

/* c = x mod m provided (0 < x < 2^ceil(log2(m))) */
static int pka_bit_serial_modul_reduction(pka_base_modul_adapter *adp, uint32_t *x, uint32_t *m, uint32_t *c,
					  uint32_t len)
{
	if (adp == NULL || x == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), x, len / sizeof(int)); /* C0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_BIT_SERIAL_MOD, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), c, len / sizeof(int)); /* C0 */
	return 0;
}

/* c = x mod m provided (0 < x < 2^2*ceil(log2(m))) */
static int pka_double_bit_serial_modul_reduction(pka_base_modul_adapter *adp, uint32_t *x_lo, uint32_t *x_hi,
						 uint32_t *m, uint32_t *c, uint32_t len)
{
	if (adp == NULL || x_lo == NULL || x_hi == NULL || m == NULL || c == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	if (__pka_check_len(adp->radix_type, len) != 0) {
		cipher_parem_invalid();
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), x_lo, len / sizeof(int)); /* C0 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[(len / sizeof(int)) * 1]), x_hi, len / sizeof(int)); /* C1 */
	__pka_load_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_D[0]), m, len / sizeof(int)); /* D0 */

	if (__pka_run(PKA_FWEP_BIT_SERIAL_MOD, reg_ptr, adp->radix_type) != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)(&reg_ptr->BANKS_C[0]), c, len / sizeof(int)); /* C0 */
	return 0;
}

int pka_ecc_point_mult(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_base_radix_type radix_type)
{
	int ret;
	if (ecc_parm == NULL || Q == NULL || P == NULL || P->x == NULL || P->y == NULL || Q->x == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	uint32_t len = pka_get_len(radix_type);
	if (len == 0 || len != ecc_parm->len || len != P->len) {
		cipher_parem_invalid();
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		cipher_function_err(__pka_get_reg_base);
		return -1;
	}

	uint32_t tmp_len = len / sizeof(uint32_t);

	if (ecc_parm->a == NULL || ecc_parm->k == NULL || ecc_parm->p == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	ret = pka_montgomery_operator_calc(ecc_parm->p, len, radix_type);
	if (ret != 0) {
		cipher_function_err(pka_montgomery_operator_calc);
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], P->x, tmp_len); /* A2 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], P->y, tmp_len); /* B2 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 6], ecc_parm->a, tmp_len); /* A6 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_D[tmp_len * 7], ecc_parm->k, tmp_len); /* D7 */
	if (ecc_parm->w != NULL) {
		__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 7], ecc_parm->w, tmp_len); /* A7 */
	}

	ret = __pka_run(PKA_FWEP_PMULT, reg_ptr, radix_type);
	if (ret != 0) {
		cipher_function_err(__pka_run);
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], Q->x, tmp_len); /* A2 */

	if (Q->y != NULL) {
		__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], Q->y, tmp_len); /* B2 */
	}

	Q->len = len;
	return 0;
}

int pka_ecc_point_add(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_piont *R,
		      pka_base_radix_type radix_type)
{
	int ret;
	if (ecc_parm == NULL || P == NULL || P->x == NULL || P->y == NULL || Q == NULL || Q->x == NULL ||
	    Q->y == NULL || R == NULL || R->x == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	uint32_t len = pka_get_len(radix_type);
	if (len == 0 || len != ecc_parm->len || len != P->len || len != Q->len) {
		cipher_function_err(pka_get_len);
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		cipher_function_err(__pka_get_reg_base);
		return -1;
	}

	uint32_t tmp_len = len / sizeof(uint32_t);

	if (ecc_parm->p == NULL || ecc_parm->a == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	ret = pka_montgomery_operator_calc(ecc_parm->p, len, radix_type);
	if (ret != 0) {
		cipher_function_err(pka_montgomery_operator_calc);
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], P->x, tmp_len); /* A2 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], P->y, tmp_len); /* B2 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 3], Q->x, tmp_len); /* A3 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 3], Q->y, tmp_len); /* B3 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 6], ecc_parm->a, tmp_len); /* A6 */

	ret = __pka_run(PKA_FWEP_PADD, reg_ptr, radix_type);
	if (ret != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], R->x, tmp_len); /* A2 */
	if (R->y != NULL) {
		__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], R->y, tmp_len); /* B2 */
	}

	R->len = len;
	return 0;
}

int pka_ecc_point_double(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_base_radix_type radix_type)
{
	int ret;
	if (ecc_parm == NULL || Q == NULL || P == NULL || P->x == NULL || P->y == NULL || Q->x == NULL ||
	    Q->y == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	uint32_t len = pka_get_len(radix_type);
	if (len == 0 || len != ecc_parm->len || len != P->len) {
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		return -1;
	}

	uint32_t tmp_len = len / sizeof(uint32_t);

	if (ecc_parm->p == NULL || ecc_parm->a == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	ret = pka_montgomery_operator_calc(ecc_parm->p, len, radix_type);
	if (ret != 0) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 3], P->x, tmp_len); /* A3 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 3], P->y, tmp_len); /* B3 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 6], ecc_parm->a, tmp_len); /* A6 */

	ret = __pka_run(PKA_FWEP_PDBL, reg_ptr, radix_type);
	if (ret != 0) {
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], Q->x, tmp_len); /* A2 */
	__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], Q->y, tmp_len); /* B2 */

	Q->len = len;

	return 0;
}

int pka_ecc_piont_verification(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_base_radix_type radix_type)
{
	int ret;
	if (ecc_parm == NULL || P == NULL || P->x == NULL || P->y == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	uint32_t len = pka_get_len(radix_type);
	if (len == 0 || len != ecc_parm->len || len != P->len) {
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	uint32_t tmp_len = len / sizeof(uint32_t);

	if (ecc_parm->p == NULL || ecc_parm->b == NULL || ecc_parm->a == NULL) {
		cipher_pointer_invalid();
		return -1;
	}

	ret = pka_montgomery_operator_calc(ecc_parm->p, len, radix_type);
	if (ret != 0) {
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], P->x, tmp_len); /* A2 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], P->y, tmp_len); /* B2 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 6], ecc_parm->a, tmp_len); /* A6 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 7], ecc_parm->b, tmp_len); /* A7 */

	ret = __pka_run(PKA_FWEP_PVER, reg_ptr, radix_type);
	if (ret != 0) {
		return -1;
	}

	uint32_t result = readl(&reg_ptr->RTN_CODE);
	if ((result & PKA_RTNCODE_ZERO) != PKA_RTNCODE_ZERO) {
		cipher_debug_err("error: the P piont  is not on curve! \n ");
		return -1;
	}
	return 0;
}

int pka_ecc_shamirs_trick(pka_ecc_param_info *ecc_parm, pka_piont *P, pka_piont *Q, pka_piont *R,
			  pka_base_radix_type radix_type)
{
	int ret;
	if (ecc_parm == NULL || P == NULL || P->x == NULL || P->y == NULL || Q == NULL || Q->x == NULL ||
	    Q->y == NULL || R == NULL || R->x == NULL) {
		cipher_parem_invalid();
		return -1;
	}

	uint32_t len = pka_get_len(radix_type);
	if (len == 0 || len != ecc_parm->len || len != P->len || len != Q->len) {
		cipher_function_err(pka_get_len);
		return -1;
	}

	pka_reg *reg_ptr = __pka_get_reg_base();
	if (reg_ptr == NULL) {
		cipher_function_err(__pka_get_reg_base);
		return -1;
	}

	uint32_t tmp_len = len / sizeof(uint32_t);

	if (ecc_parm->p == NULL || ecc_parm->l == NULL || ecc_parm->a == NULL || ecc_parm->k == NULL) {
		cipher_parem_invalid();
		return -1;
	}

	ret = pka_montgomery_operator_calc(ecc_parm->p, len, radix_type);
	if (ret != 0) {
		cipher_function_err(pka_montgomery_operator_calc);
		return -1;
	}

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], P->x, tmp_len); /* A2 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], P->y, tmp_len); /* B2 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 3], Q->x, tmp_len); /* A3 */
	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 3], Q->y, tmp_len); /* B3 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 6], ecc_parm->a, tmp_len); /* A6 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 7], ecc_parm->k, tmp_len); /* A7 */

	__pka_load_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_D[tmp_len * 7], ecc_parm->l, tmp_len); /* D7 */

	ret = __pka_run(PKA_FWEP_SHAMIR, reg_ptr, radix_type);
	if (ret != 0) {
		cipher_function_err(__pka_run);
		return -1;
	}

	__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_A[tmp_len * 2], R->x, tmp_len); /* A2 */
	if (R->y != NULL) {
		__pka_get_data((uint32_t)(uintptr_t)&reg_ptr->BANKS_B[tmp_len * 2], R->y, tmp_len); /* B2 */
	}

	R->len = len;
	return 0;
}

int pka_init(void)
{
	if (g_is_init == true) {
		return 0;
	}

	g_is_init = true;
	pka_fw_init();

	pka_irq_en();
	return 0;
}

void pka_deinit(void)
{
	g_is_init = false;
}

pka_base_modul_adapter *pka_base_modul_adapter_init(pka_base_radix_type radix_type)
{
	if (g_is_init == false) {
		cipher_debug_err("error: pka no initialize! \n");
		return NULL;
	}

	pka_base_modul_adapter *pka_adp = &g_pka_adp;
	pka_adp->radix_type = radix_type;
	pka_adp->is_mult_key = 0;
	pka_adp->is_sec_key = 0;
	pka_adp->mult = pka_modul_mult;
	pka_adp->add = pka_modul_add;
	pka_adp->sub = pka_modul_sub;
	pka_adp->div = pka_modul_division;
	pka_adp->reduc = pka_modul_reduction;
	pka_adp->inv = pka_modul_inversion;
	pka_adp->non_mod_mult = pka_non_modul_muilt;
	pka_adp->bit_seri_reduc = pka_bit_serial_modul_reduction;
	pka_adp->double_bit_seri_reduc = pka_double_bit_serial_modul_reduction;

	return pka_adp;
}

void pka_base_mod_adapter_deinit(void)
{
	memset(&g_pka_adp, 0, sizeof(pka_base_modul_adapter));
}
