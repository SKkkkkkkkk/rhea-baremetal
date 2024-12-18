/*
	these APIs are used to control CLCI module.
	Some features need to be combined sequentially with the following APIs.
	This file do the same thing as commands_sys.c
*/
#include <commands_sys.h>
#include <commands_common.h>

int32_t cmd_clci_get_reg(int32_t die, uint32_t addr, uint32_t *data)
{
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->addr = addr;
	item->res = die;
	*data = item->data;
	return clci_req(CMD_GET_REG);
}

int32_t cmd_clci_set_reg(int32_t die, uint32_t addr, uint32_t data)
{
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->addr = addr;
	item->data = data;
	item->res = die;
	return clci_req(CMD_SET_REG);
}

int32_t cmd_clci_ram_firmware_go(int32_t die, uint32_t boot_addr)
{
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->addr = boot_addr;
	item->res = die;
	return clci_req(CMD_BOOTADDR_GO);
}

int32_t cmd_clci_update_clk_bandrate(int32_t die, int16_t sys_clk, int16_t bandrate)
{
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->res = die;
	item->data = (sys_clk << 20) | bandrate;
	return clci_req(CMD_UPDATE_SYS_CLK_BAUDRATE);
}

int32_t cmd_clci_clk_counter(int16_t die, int16_t lane, uint32_t *txfreq, uint32_t *rxfreq)
{
	const int default_div = 4;
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->addr = (die << 16) | lane;
	uint32_t ret = clci_req(CMD_PLL_CLK_CNT);
	*txfreq = 1024 * default_div * ((item->data >> 16) & 0xffff) / 64;
	*rxfreq = 1024 * default_div * (item->data & 0xffff) / 64;
	return ret;
}
int32_t cmd_clci_common(int32_t die, uint8_t cmd)
{
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->res = die;
	return clci_req(cmd);
}

int32_t cmd_clci_delayline_tracking(int32_t die, uint32_t temp)
{
	struct cmd_common_t *item = (struct cmd_common_t *)SYS_CLCI_CMD_ARG_P;
	item->res = die;
	item->data = temp;
	return clci_req(CMD_DELAYLINE_TRACKING);
}