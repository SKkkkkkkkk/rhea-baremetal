#ifndef _CLCI_API_H
#define _CLCI_API_H
// #include "main.h"
#include <stdint.h>

//#define SUPPORT_DOORBELL_ISR

#define CLCI_INIT_SYNC_TIME_MAX	1 //second

#define CLCI_INIT_LINK_TIME_MAX	10 //seconds

#define SYNC_FLAG_VALUE		0xd4e1dc57

typedef struct _clci_mcu_status {
	// e_sys_status boot_status;
	uint8_t err_code;
	uint8_t sys_status;
	uint8_t clci_status;
} clci_mcu_status;

void clci_handle_db_cmd(uint32_t val);
int clci_init_firmware();
void clci_doorbell_isr();
void clci_doorbell_poll();
int clci_status_get(void *status);
int clci_operate(uint8_t *in_buff, uint8_t in_len, uint8_t *out_buff, uint8_t out_len);

#endif
