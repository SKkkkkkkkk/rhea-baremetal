#include "platform.h"
#include "local_ctrl.h"

uint64_t local_ctrl_base = CLCI_MCU_LOCAL_CTRL_ADDR;

void local_ctrl_base_set(uint64_t reg_base)
{
	local_ctrl_base = reg_base;
}

