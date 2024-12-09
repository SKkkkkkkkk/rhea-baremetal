#ifndef __CLCI_ERROR_H__
#define __CLCI_ERROR_H__

#include <local_ctrl.h>

#define CLCI_DEV_ERROR_REG CLCI_MCU_LOCAL_CTRL_MSG1

#define SYS_ERROR_CLASS_SYS_EXCEPTION	 0
#define SYS_ERROR_CLASS_DEV_SIDEBAND	 1
#define SYS_ERROR_CLASS_DEV_WDT		 2
#define SYS_ERROR_CLASS_DEV_DOORBELL_COM 3
#define SYS_ERROR_CLASS_DEV_CLCI	 4

uint32_t clci_error_get(void);
void clci_irq_enable(int32_t dieid);
void clci_irq_disable(int32_t dieid);
void clci_irq_handler(int32_t dieid);

#endif
