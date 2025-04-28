#ifndef _CLCI_API_H
#define _CLCI_API_H

//#define SUPPORT_DOORBELL_ISR

/*
 * clci_init_firmware()
 * Initialization clci firmware before accessing the clci device
 */
int clci_init_firmware();
/* set the base address of the clci device
 * This API should be invoked before access the clci device
 * ctrl_reg_base:
 * 		the base address of the clci device
 */
void clci_device_reg_base_set(uint64_t clci_reg_base);

#endif
