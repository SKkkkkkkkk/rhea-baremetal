#ifndef __DW_APB_GPIO_REGS_H__
#define __DW_APB_GPIO_REGS_H__

#include "common.h"
typedef struct _DW_APB_GPIO {
	__IOM uint32_t SWPORTA_DR;
	__IOM uint32_t SWPORTA_DDR;
	__IOM uint32_t SWPORTA_CTL;
	__IOM uint32_t SWPORTB_DR;
	__IOM uint32_t SWPORTB_DDR;
	__IOM uint32_t SWPORTB_CTL;
	__IOM uint32_t SWPORTC_DR;
	__IOM uint32_t SWPORTC_DDR;
	__IOM uint32_t SWPORTC_CTL;
	__IOM uint32_t SWPORTD_DR;
	__IOM uint32_t SWPORTD_DDR;
	__IOM uint32_t SWPORTD_CTL;
	__IOM uint32_t INTEN;
	__IOM uint32_t INTMASK;
	__IOM uint32_t INTTYPE_LEVEL;
	__IOM uint32_t INT_POLARITY;
	__IOM uint32_t INTSTATUS;
	__IOM uint32_t RAW_INTSTATUS;
	__IOM uint32_t DEBOUNCE;
	__IOM uint32_t PORTA_EOI;
	__IOM uint32_t EXT_PORTA;
	__IOM uint32_t EXT_PORTB;
	__IOM uint32_t EXT_PORTC;
	__IOM uint32_t EXT_PORTD;
	__IOM uint32_t LS_SYNC;
	__IOM uint32_t ID_CODE;
	__IOM uint32_t INT_BOTHEDGE;
	__IOM uint32_t VER_ID_CODE;
	__IOM uint32_t CONFIG_REG2;
	__IOM uint32_t CONFIG_REG1;
} DW_APB_GPIO_TypeDef;

#endif