#include "string.h"
#include "local_ctrl.h"
#include "stdio.h"
#include "log.h"
#include "mmio.h"
// #include "firmware.h"
// #include "main.h"
#include "clci_api.h"
#include "mailbox_sys.h"
// #include "error.h"
// #include "sys_timer.h"
// #include "clci_link.h"

static volatile clci_mcu_status g_clci_mcu_status;

inline static void init_sync_msg_reg(void)
{
	mmio_write_32(CLCI_MCU_LOCAL_CTRL_MSG3, 0);
}
/*
 * clci_init_firmware()
 * if SUPPORT_DOORBELL_ISR is defined, the doorbell must
 * be initialized by calling dbInit() with clci_doorbell_isr() before clci_init() is called,
 */

int clci_init_firmware()
{
	mailbox_sys_init();

#if 0
	struct clci_config_t cfg;
	clci_cfg_init(&cfg);
	memcpy((char *)CLCI_CONFOG_LOAD_ADDRESS, &cfg, sizeof(struct clci_config_t));
	copy_firmware();

	firmware_bootreg_set();

	firmware_run();
#endif
	return 0;
}
int clci_status_get(void *status)
{
	if (status == NULL)
		return 0;
	memcpy((char *)status, (char *)&g_clci_mcu_status, sizeof(g_clci_mcu_status));

	return 1;
}
