#ifndef __APE1210_PKA_REGS_H__
#define __APE1210_PKA_REGS_H__

#include "common.h"
typedef struct _PKA_REGS {
	__IOM uint32_t CTRL; /* 0x00 */
	__IOM uint32_t ENTRY_PNT; /* 0x04 */
	__IM uint32_t RTN_CODE; /* 0x08 */
	__IM uint32_t BUILD_CONFIG; /* 0x0c */
	__IOM uint32_t STACK_PNTR; /* 0x10 */
	__IOM uint32_t INSTR_SINCE_GO; /* 0x14 */
	RESERVED(0, uint32_t) /* 0x18 */
	__IOM uint32_t CONFIG; /* 0x1c */
	__IOM uint32_t STAT; /* 0x20 */
	__IOM uint32_t FLAGS; /* 0x24 */
	__IOM uint32_t WATCHDOG; /* 0x28 */
	__IOM uint32_t CYCLES_SINCE_GO; /* 0x2c */
	__IOM uint32_t INDEX_I; /* 0x30 */
	__IOM uint32_t INDEX_J; /* 0x34 */
	__IOM uint32_t INDEX_K; /* 0x38 */
	__IOM uint32_t INDEX_L; /* 0x3c */
	__IOM uint32_t IRQ_EN; /* 0x40 */
	__IOM uint32_t JMP_PROB; /* 0x44 */
	__IOM uint32_t JMP_PROB_LFSR; /* 0x48 */
	RESERVED(1, uint32_t) /* 0x4c */
	__IOM uint32_t BANK_SW_A; /* 0x50 */
	__IOM uint32_t BANK_SW_B; /* 0x54 */
	__IOM uint32_t BANK_SW_C; /* 0x58 */
	__IOM uint32_t BANK_SW_D; /* 0x5c */
	RESERVED(3 [232], uint32_t) /* 0x60   ~ 0x3fc */

	/*PKA Wide Register Map Base Addresses*/
	union { /* 0x400  ~ 0x7fc */
		__IOM uint32_t BANKS_A[256]; /* 0x400  ~ 0x7fc */
		struct ECC256_BANKS {
			__IOM uint32_t _0[8]; /* A0:0x400; B0:0x800; C0:0xc00; D0:0x1000 */
			__IOM uint32_t _1[8]; /* A1:0x420; B1:0x820; C1:0xc20; D1:0x1020 */
			__IOM uint32_t _2[8]; /* A2:0x440; B2:0x840; C2:0xc40; D2:0x1040 */
			__IOM uint32_t _3[8]; /* A3:0x460; B3:0x860; C3:0xc60; D3:0x1060 */
			__IOM uint32_t _4[8]; /* A4:0x480; B4:0x880; C4:0xc80; D4:0x1080 */
			__IOM uint32_t _5[8]; /* A5:0x4a0; B5:0x8a0; C5:0xca0; D5:0x10a0 */
			__IOM uint32_t _6[8]; /* A6:0x4c0; B6:0x8c0; C6:0xcc0; D6:0x10c0 */
			__IOM uint32_t _7[8]; /* A7:0x4e0; B7:0x8e0; C7:0xce0; D7:0x10e0 */
			RESERVED(4 [192], uint32_t)
		} ECC256_BANKS_A;

		struct ECC512_BANKS {
			__IOM uint32_t _0[16]; /* A0:0x400; B0:0x800; C0:0xc00; D0:0x1000 */
			__IOM uint32_t _1[16]; /* A1:0x440; B1:0x840; C1:0xc40; D1:0x1040 */
			__IOM uint32_t _2[16]; /* A2:0x480; B2:0x880; C2:0xc80; D2:0x1080 */
			__IOM uint32_t _3[16]; /* A3:0x4c0; B3:0x8c0; C3:0xcc0; D3:0x10c0 */
			__IOM uint32_t _4[16]; /* A4:0x500; B4:0x900; C4:0xd00; D4:0x1100 */
			__IOM uint32_t _5[16]; /* A5:0x540; B5:0x940; C5:0xd40; D5:0x1140 */
			__IOM uint32_t _6[16]; /* A6:0x580; B6:0x980; C6:0xd80; D6:0x1180 */
			__IOM uint32_t _7[16]; /* A7:0x5c0; B7:0x9c0; C7:0xdc0; D7:0x11c0 */
			RESERVED(5 [128], uint32_t)
		} ECC512_BANKS_A;

		struct ECC1024_BANKS {
			__IOM uint32_t _0[32]; /* A0:0x400; B0:0x800; C0:0xc00; D0:0x1000 */
			__IOM uint32_t _1[32]; /* A1:0x480; B1:0x880; C1:0xc80; D1:0x1080 */
			__IOM uint32_t _2[32]; /* A2:0x500; B2:0x900; C2:0xd00; D2:0x1100 */
			__IOM uint32_t _3[32]; /* A3:0x580; B3:0x980; C3:0xd80; D3:0x1180 */
			__IOM uint32_t _4[32]; /* A4:0x600; B4:0xa00; C4:0xe00; D4:0x1200 */
			__IOM uint32_t _5[32]; /* A5:0x680; B5:0xa80; C5:0xe80; D5:0x1280 */
			__IOM uint32_t _6[32]; /* A6:0x700; B6:0xb00; C6:0xf00; D6:0x1300 */
			__IOM uint32_t _7[32]; /* A7:0x780; B7:0xb80; C7:0xf80; D7:0x1380 */
		} ECC1024_BANKS_A;

		struct RSA512_BANKS {
			__IOM uint32_t _0[16]; /* A0:0x400; B0:0x800; C0:0xc00; D0:0x1000 */
			__IOM uint32_t _1[16]; /* A1:0x440; B1:0x840; C1:0xc40; D1:0x1040 */
			__IOM uint32_t _2[16]; /* A2:0x480; B2:0x880; C2:0xc80; D2:0x1080 */
			__IOM uint32_t _3[16]; /* A3:0x4c0; B3:0x8c0; C3:0xcc0; D3:0x10c0 */
			RESERVED(6 [192], uint32_t)
		} RSA512_BANKS_A;

		struct RSA1024_BANKS {
			__IOM uint32_t _0[32]; /* A0:0x400; B0:0x800; C0:0xc00; D0:0x1000 */
			__IOM uint32_t _1[32]; /* A1:0x480; B1:0x880; C1:0xc80; D1:0x1080 */
			__IOM uint32_t _2[32]; /* A2:0x500; B2:0x900; C2:0xd00; D2:0x1100 */
			__IOM uint32_t _3[32]; /* A3:0x580; B3:0x980; C3:0xd80; D3:0x1180 */
			RESERVED(7 [128], uint32_t)
		} RSA1024_BANKS_A;

		struct RSA2048_BANKS {
			__IOM uint32_t _0[64];
			__IOM uint32_t _1[64];
			__IOM uint32_t _2[64];
			__IOM uint32_t _3[64];
		} RSA2048_BANKS_A;

		struct RSA4096_BANKS {
			__IOM uint32_t _0[128];
			__IOM uint32_t _1[128];
		} RSA4096_BANKS_A;
	};

	union { /* 0x800  ~ 0xbfc */
		__IOM uint32_t BANKS_B[256]; /* 0x800  ~ 0xbfc */
		struct ECC256_BANKS ECC256_BANKS_B;
		struct ECC512_BANKS ECC512_BANKS_B;
		struct ECC1024_BANKS ECC1024_BANKS_B;
		struct RSA512_BANKS RSA512_BANKS_B;
		struct RSA1024_BANKS RSA1024_BANKS_B;
		struct RSA2048_BANKS RSA2048_BANKS_B;
		struct RSA4096_BANKS RSA4096_BANKS_B;
	};

	union { /* 0xc00  ~ 0xffc */
		__IOM uint32_t BANKS_C[256]; /* 0xc00  ~ 0xffc */
		struct ECC256_BANKS ECC256_BANKS_C;
		struct ECC512_BANKS ECC512_BANKS_C;
		struct ECC1024_BANKS ECC1024_BANKS_C;
		struct RSA512_BANKS RSA512_BANKS_C;
		struct RSA1024_BANKS RSA1024_BANKS_C;
		struct RSA2048_BANKS RSA2048_BANKS_C;
		struct RSA4096_BANKS RSA4096_BANKS_C;
	};

	union { /* 0x1000 ~ 0x13fc */
		__IOM uint32_t BANKS_D[256]; /* 0x1000 ~ 0x13fc */
		struct ECC256_BANKS ECC256_BANKS_D;
		struct ECC512_BANKS ECC512_BANKS_D;
		struct ECC1024_BANKS ECC1024_BANKS_D;
		struct RSA512_BANKS RSA512_BANKS_D;
		struct RSA1024_BANKS RSA1024_BANKS_D;
		struct RSA2048_BANKS RSA2048_BANKS_D;
		struct RSA4096_BANKS RSA4096_BANKS_D;
	};

	RESERVED(8 [2816], uint32_t)
	__IOM uint32_t FW_MEM[4096]; /* 0x4000 */
} pka_reg;

#ifndef DWC_PKA_BASE
#define DWC_PKA_BASE (0x10220000) /*!< (PKA       ) Base Address */
#endif

#define PKA ((pka_reg *)DWC_PKA_BASE)
#endif
