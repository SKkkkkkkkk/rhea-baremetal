#ifndef _EFUSE_REGS_H__
#define _EFUSE_REGS_H__

#include "common.h"
typedef struct _EFUSE_REGS {
	union { /* 0x00 */
		struct {
			__IOM uint32_t
				efuse_mode : 4; /* 3:0 bit 0:自动读，1：自动烧写，2：读SPACC-key， 3：读PKA-key */
			// __IOM uint32_t : 1; /* 3 bit */
			__IO uint32_t efuse_rsb_st : 1; /* 4 bit */
			__IOM uint32_t efuse_pd : 1; /* 5 bit */
			__IOM uint32_t : 10;
			__IOM uint32_t
				efuse_addr : 11; /* 26 : 16 bit efuse地址，写操作使用[26:16]，对应读操作时使用[20:16], */
			__IOM uint32_t : 4; /* 30: 27 bit */
			__IOM uint32_t efuse_soft_rstn : 1; /* 31 bit 软复位，使用时需要置高 */
		};
		__IOM uint32_t ALL; /* 0x00 */
	} EFUSE_MOD_U;

	__IOM uint32_t EFUSE_DOUT_TEST; /* 0x04 */
	__IOM uint32_t EFUSE_DOUT; /* 0x08 */

	union { /* 0x0c */
		struct {
			__IOM uint32_t efuse_int_irq_mask : 1; /* 0 bit */
			__IOM uint32_t efuse_int_pka_mask : 1; /* 1 bit */
			__IOM uint32_t efuse_int_spacc_mask : 1; /* 2 bit */
			__IOM uint32_t : 29; /* 31 : 3 */
		};
		__IOM uint32_t ALL;
	} EFUSE_INT_MASK_U;

	union { /* 0x10 */
		struct {
			__IO uint32_t efuse_int_irq_st : 1; /* 0 bit 自动烧写/读完成中断 */
			__IO uint32_t efuse_int_pka_st : 1; /* 1 bit 初始化PKA 密钥完成中断 */
			__IO uint32_t efuse_int_spacc_st : 1; /* 2 bit 初始化SPACC完成中断 */
			__IO uint32_t
				efuse_int_error_st : 1; /* 3 bit 非法读取SPACC和PKA key信息 ；烧写不可烧写地址。不可mask */
			__IO uint32_t : 27; /* 30 : 4 */
			__IO uint32_t
				efuse_busy : 1; /* 31 bit 置高时不能操作efuse，可以通过soft_rst 强行中断efuse操作，但可能会造成不可控影响 */
		};
		__IO uint32_t ALL;
	} EFUSE_INT_STATUS_U;

	union { /* 0x14 */
		struct {
			__IOM uint32_t efuse_int_irq_clr : 1; /* 0 bit */
			__IOM uint32_t efuse_int_pka_clr : 1; /* 1 bit */
			__IOM uint32_t efuse_int_spacc_clr : 1; /* 2 bit */
			__IOM uint32_t efuse_int_error_clr : 1; /* 3 bit */
			__IOM uint32_t : 28; /* 30 : 4 */
		};
		__IOM uint32_t ALL;
	} EFUSE_INT_CLEAR_U;

	RESERVED(0[2], uint32_t)

	union { /* 0x20 */
		struct {
			__IOM uint32_t
				efuse_pka_memsel : 2; /* 1:0 bit MEM_SELECT: 选择写入ram 00：A_ram 01：B_ram 10：C_ram 11：D_ram */
			__IOM uint32_t : 6;
			__IOM uint32_t efuse_pka_num : 5; /* 12:8 bit number:读取的数据量，单位4byte， 最大32Byte。Real=value+1 */
			__IOM uint32_t : 3;
			__IOM uint32_t efuse_pka_memaddr : 8; /* 23:16 bit PKAMEM_BASE_ADDR:7位目的地址 */
			__IOM uint32_t : 8; /* 31 : 24 */
		};
		__IOM uint32_t ALL;
	} EFUSE_PKA_MEM_U;

	__IOM uint32_t EFUSE_START; /* 0x24 先进行配置，最后start置1. */
	RESERVED(1[2], uint32_t)

	union { /* 0x30 */
		struct {
			__IOM uint32_t t_pgm_max : 10; /* 0x30 9:0 */
			__IOM uint32_t t_w_ps_up : 10; /* 0x30 19:10 烧写模式下，PS上拉的有效间隔，real = value + 1 （下同）*/
			__IOM uint32_t t_w_ps_down : 10; /* 0x30 29:20 烧写模式下，PS下拉的有效间隔 */
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG0_U;

	union { /* 0x34 */
		struct {
			__IOM uint32_t t_w_csb_up : 10; /* 0x34 9:0 烧写模式下，CSB上拉的有效间隔*/
			__IOM uint32_t t_w_csb_down : 10; /* 0x34 19:10 烧写模式下，CSB下拉的有效间隔*/
			__IOM uint32_t t_w_pgenb_up : 10; /* 0x34 29:20 烧写模式下，PGEND上拉的有效间隔*/
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG1_U;

	union { /* 0x38 */
		struct {
			__IOM uint32_t t_w_pgenb_down : 10; /* 0x38 9:0 烧写模式下，PGEND下拉的有效间隔 */
			__IOM uint32_t t_w_addr_up : 10; /* 0x38 19:10 烧写模式下，ADDR 上拉的有效间隔 */
			__IOM uint32_t t_w_addr_down : 10; /* 0x38 29:20 烧写模式下，ADDR 下拉的有效间隔 */
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG2_U;

	union { /* 0x3c */
		struct {
			__IOM uint32_t t_w_strobe_up : 10; /* 0x3c 9:0 烧写模式下， STROBE 上拉的有效间隔 */
			__IOM uint32_t t_w_strobe_down : 10; /* 0x3c 19:10 烧写模式下， STROBE 下拉的有效间隔 */
			__IOM uint32_t t_rd_max : 10; /* 0x3c 29:20 */
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG3_U;

	union { /* 0x40 */
		struct {
			__IOM uint32_t t_r_csb_up : 10; /* 0x40 9:0 读模式下， CSB 上拉的有效间隔 */
			__IOM uint32_t t_r_csb_down : 10; /* 0x40 19:10 读模式下， CSB 下拉的有效间隔 */
			__IOM uint32_t t_r_load_up : 10; /* 0x40 29:20 读模式下， load 上拉的有效间隔 */
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG4_U;

	union { /* 0x44 */
		struct {
			__IOM uint32_t t_r_load_down : 10; /* 0x44 9:0 读模式下， LOAD 下拉的有效间隔 */
			__IOM uint32_t t_r_addr_up : 10; /* 0x44 19:10 读模式下， ADDR 上拉的有效间隔 */
			__IOM uint32_t t_r_addr_down : 10; /* 0x44 29:20 读模式下， ADDR 下拉的有效间隔 */
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG5_U;

	union { /* 0x48 */
		struct {
			__IOM uint32_t t_r_strobe_up : 10; /* 0x48 9:0 读模式下， STROBE 上拉的有效间隔 */
			__IOM uint32_t t_r_strobe_down : 10; /* 0x48 19:10 读模式下， STROBE 下拉的有效间隔 */
			__IOM uint32_t t_r_q : 10; /* 0x48 29:20 */
		};
		__IOM uint32_t ALL;
	} EFUSE_TIMIMG_REG6_U;
	__IOM uint32_t EFUSE_T_LD_CSB; /* 0x64 读模式下，LOAD无效到CSB无效的间隔 */
} efuse_reg;

#ifndef EFUSE_BASE
#define EFUSE_BASE (0x10230000UL) /*!< (EFUSE     ) Base Address */
#endif
#define EFUSE ((efuse_reg *)EFUSE_BASE)
#endif
