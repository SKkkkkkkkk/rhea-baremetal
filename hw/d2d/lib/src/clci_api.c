#include <string.h>
#include "local_ctrl.h"
#include "stdio.h"
#include "log.h"
#include "mmio.h"
#include "clci_api.h"
#include "mailbox_sys.h"

inline static void init_sync_msg_reg(void)
{
	mmio_write_32(LOCAL_CTRL_REG_ADDR(msg3), 0);
}
/*
 * clci_init_firmware()
 * Initialization clci firmware before accessing the clci device
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
/* set the base address of the clci device
 * This API should be invoked before access the clci device
 * ctrl_reg_base:
 * 		the base address of the clci device
 */
void clci_device_reg_base_set(uint64_t clci_reg_base)
{
	local_ctrl_base_set(clci_reg_base);
	mailbox_reg_address_refresh();
}
