#ifndef __M2SEMI_PLATFORM_H__
#define __M2SEMI_PLATFORM_H__

#include <stdint.h>
#include <local_ctrl.h>
#include <config.h>
#include <log.h>

#define SWERV_EL2			1
#define M2SEMI_SWERV_EL2		1

/* Ext Interrupt */
#define PIC_IRQ_NUM			8

#define PIC_IRQ_MIN			0
#define PIC_IRQ_MAX			8

#define PLATFORM_IRQS			PIC_IRQ_MAX

#define IRQ_STACK_SIZE			400

/* PIC base address */
#if D_BORAD_CL2305
#define PSP_PIC_BASE_ADDRESS		0xf00c0000
#else
#define PSP_PIC_BASE_ADDRESS		0xc00c0000
#endif
/* Specified RAM address for generation of external interrupts */

#define EXT_INTS_GENERATION_REG_ADDRESS	CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU

/* System clock */
#define CLOCK_RATE			25000000

#define UART0_BASEADDR			CLCI_MCU_UART0_BASE
#define UART1_BASEADDR			CLCI_MCU_UART1_BASE

/*doorbell*/
#define DB_INTERRUPT_ID			5

#define DB_INTERRUPT_TYPE		PSP_EXT_INT_LEVEL_TRIG_TYPE
#define DB_INTERRUPT_LEVEL		PSP_EXT_INT_ACTIVE_HIGH

#define DB_INTERRUPT_PRIO		10

/*watchdog*/
#define WDT_TIMEOUT_VAL			11 //about 6s

#define WDT_INTERRUPT_ID		4
#define WDT_INTERRUPT_TYPE		PSP_EXT_INT_LEVEL_TRIG_TYPE
#define WDT_INTERRUPT_LEVEL		PSP_EXT_INT_ACTIVE_HIGH
#define WDT_INTERRUPT_PRIO		9

/*clci*/
#define CLCI_ERR_INTERRUPT_ID		1

#define CLCI_ERR_INTERRUPT_TYPE		PSP_EXT_INT_EDGE_TRIG_TYPE
#define CLCI_ERR_INTERRUPT_LEVEL	PSP_EXT_INT_ACTIVE_HIGH

#define CLCI_ERR_INTERRUPT_PRIO		10


uint32_t get_cur_clock_rating(void);
void platform_clci_api_init(void);

#endif
