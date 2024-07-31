#ifndef __APE1210_SPACC_REGS_H__
#define __APE1210_SPACC_REGS_H__

#include "common.h"
typedef struct _SPACC_REGS {
	__IOM uint32_t IRQ_EN; /* 0x00 */
	__IOM uint32_t IRQ_STAT; /* 0x04 */
	__IOM uint32_t IRQ_CTRL; /* 0x08 */
	__IM uint32_t FIFO_STAT; /* 0x0c */
	__IOM uint32_t SDMA_BRST_SZ; /* 0x10 */
	RESERVED(0 [3], uint32_t) /* 0x14 ~ 0x1c */
	__IOM uint32_t SRC_PTR; /* 0x20 */
	__IOM uint32_t DST_PTR; /* 0x24 */
	__IOM uint32_t OFFSET; /* 0x28 */
	__IOM uint32_t PRE_AAD_LEN; /* 0x2c */
	__IOM uint32_t POST_AAD_LEN; /* 0x30 */
	__IOM uint32_t PROC_LEN; /* 0x34 */
	__IOM uint32_t ICV_LEN; /* 0x38 */
	__IOM uint32_t ICV_OFFSET; /* 0x3c */
	__IOM uint32_t IV_OFFSET; /* 0x40 */
	__IOM uint32_t SW_CTRL; /* 0x44 */
	__IOM uint32_t AUX_INFO; /* 0x48 */
	__IOM uint32_t CTRL; /* 0x4c */
	__OM uint32_t STAT_POP; /* 0x50 */
	__IM uint32_t STATUS; /* 0x54 */
	RESERVED(1 [10], uint32_t) /* 0x58 ~ 0x7c */
	__IOM uint32_t STAT_WD_CTRL; /* 0x80 */
	RESERVED(2 [31], uint32_t) /* 0x84 ~ 0x9c */
	__IOM uint32_t KEY_SZ; /* 0x100 */
	RESERVED(3 [15], uint32_t) /* 0x104 ~ 0x13c */
	__OM uint32_t VSPACC_RQST; /* 0x140 */
	__IM uint32_t VSPACC_ALLOC; /* 0x144 */
	__IOM uint32_t VSPACC_PRIORITY; /* 0x148 */
	RESERVED(4 [1], uint32_t) /* 0x14c */
	__IOM uint32_t VSPACC_RC4_KEY_REQ; /* 0x150 */
	__IM uint32_t VSPACC_RC4_KEY_GNT; /* 0x154 */
	RESERVED(5 [10], uint32_t) /* 0x158 ~ 0x17c */
	__IM uint32_t VERSION; /* 0x180 */
	__IM uint32_t VERSION_EXT; /* 0x184 */
	RESERVED(6 [2], uint32_t) /* 0x188 ~ 0x18c */
	__IM uint32_t VERSION_EXT_2; /* 0x190 */
	RESERVED(7 [11], uint32_t) /* 0x194 ~ 0x1bc */
	__IOM uint32_t SECURE_CTRL; /* 0x1c0 */
	__OM uint32_t SECURE_CTX_RELEASE; /* 0x1c4 */
	RESERVED(8 [3982], uint32_t) /* 0x1c8 ~ 0x3fc */
	__IOM uint32_t CIPH_KEY[36]; /* 0x4000 : [key][iv/ctr/y0][xts key] */
	RESERVED(9 [4060], uint32_t) /* 0x4050 ~ 0x7FFC CIPH_KEY RESERVED */
	__IOM uint32_t HASH_KEY[32]; /* 0x8000 */
	RESERVED(10 [992], uint32_t) /* 0x8080 ~ 0x8ffc HASH_KEY RESERVED */
	RESERVED(11 [0x5c00], uint32_t) /* 0x9000 ~ 0x1fffc */
	__IOM uint32_t RC4_CTX[0x4000]; /* 0X20000 */
} spacc_reg;

#ifndef DWC_SPACC_BASE
#define DWC_SPACC_BASE (0x10200000) /*!< (SPACC     ) Base Address */
#endif
#define __M3VIEW_FST_SRC_ADDR 0x47600000
#define __M3VIEW_FST_SRC_SIZE 0x00010000 /* 64K */
#define __M3VIEW_FST_DST_ADDR 0x47610000
#define __M3VIEW_FST_SRC_SIZE 0x00010000 /* 64K */

#define SPACC ((spacc_reg *)DWC_SPACC_BASE)



#endif
