/*
	these APIs are used to control CLCI module.
	Some features need to be combined sequentially with the following APIs.
*/
#include <string.h>
#include <commands_sys.h>
#include <commands_common.h>
#include <mailbox_sys.h>
#include <mmio.h>

#include "delay.h"

#define MAILBOX_SYS_TIMEOUT_MS 5000
/*
	read register from local/remote die

input:
	die
		0 : local die
		1 : remote die
	addr
		register addr
	data
		register data
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_get_reg(int32_t die, uint32_t addr, uint32_t *data)
{
	uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	struct cmd_common_t item;

	item.addr = addr;
	item.res = die;
	in_buf[0] = CMD_GET_REG;
	memcpy(&in_buf[1], &item, sizeof(item));
	int32_t ret = mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX, MAILBOX_SYS_TIMEOUT_MS);
	if (ret == sizeof(struct cmd_common_t)) {
		struct cmd_common_t *info = (struct cmd_common_t *)out_buf;
		*data = info->data;
	}

	return ret;
}

/*
	update register value of local/remote die

input:
	die
		0 : local die
		1 : remote die
	addr
		register addr
	data
		register data
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_set_reg(int32_t die, uint32_t addr, uint32_t data)
{
	uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	struct cmd_common_t item;

	item.addr = addr;
	item.data = data;
	item.res = die;
	in_buf[0] = CMD_SET_REG;
	memcpy(&in_buf[1], &item, sizeof(item));
	return mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX, MAILBOX_SYS_TIMEOUT_MS);
}

/*
	run a new firmware on local/remote die

input:
	die
		0 : local die
		1 : remote die
	boot_addr
		start address of new firmware
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_ram_firmware_go(int32_t die, uint32_t boot_addr)
{
	if (die) {
		struct cmd_common_t item;
		uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
		uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };

		item.addr = boot_addr;
		item.res = die;
		in_buf[0] = CMD_BOOTADDR_GO;
		memcpy(&in_buf[1], &item, sizeof(item));
		return mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX,
					MAILBOX_SYS_TIMEOUT_MS);
	} else {
		mmio_write_32(CLCI_MCU_CPU_BOOT_ADDR, boot_addr);
		uint32_t val = mmio_read_32(CLCI_MCU_LOCAL_CTRL_CTRL0);
		mmio_write_32(CLCI_MCU_LOCAL_CTRL_CTRL0, val & (~1));
		mdelay(10);
		mmio_write_32(CLCI_MCU_LOCAL_CTRL_CTRL0, val | 1);
		return 0;
	}
}

/*
	update clci mcu sys_clk and UART bandrate

input:
	die
		0 : local die
		1 : remote die
	sys_clk
		clci mcu sys_clk
	bandrate
		UART bandrate, both debug UART and sideband UART
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_update_clk_bandrate(int32_t die, int16_t sys_clk, int16_t bandrate)
{
	struct cmd_common_t item;
	uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };

	item.res = die;
	item.data = (sys_clk << 20) | (bandrate);
	in_buf[0] = CMD_UPDATE_SYS_CLK_BAUDRATE;
	memcpy(&in_buf[1], &item, sizeof(item));
	return mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX, MAILBOX_SYS_TIMEOUT_MS);
}

/*
	get clci clk counter which is used to debug clci clk issues

input:
	die
		0 : local die
		1 : remote die
	lane
		lane id
	txfreq/rxfreq
		tx/rx freq of this lane
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_clk_counter(int16_t die, int16_t lane, uint32_t *txfreq, uint32_t *rxfreq)
{
	const int default_div = 4;
	struct cmd_common_t item;
	uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };

	item.data = (die << 8) | lane;
	in_buf[0] = CMD_PLL_CLK_CNT;
	memcpy(&in_buf[1], &item, sizeof(item));
	int32_t ret = mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX, MAILBOX_SYS_TIMEOUT_MS);
	if (ret == sizeof(struct cmd_common_t)) {
		struct cmd_common_t *info = (struct cmd_common_t *)out_buf;
		*txfreq = 1024 * default_div * ((info->data >> 16) & 0xffff) / 64;
		*rxfreq = 1024 * default_div * (info->data & 0xffff) / 64;
	}
	return ret;
}

/*
	run cmd function in firmware

input:
	die
		0 : local die
		1 : remote die
	cmd
		reference command name
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_common(int32_t die, uint8_t cmd)
{
	struct cmd_common_t item;
	uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };

	item.res = die;
	in_buf[0] = cmd;
	memcpy(&in_buf[1], &item, sizeof(item));
	return mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX, MAILBOX_SYS_TIMEOUT_MS + 5);
}

/*
	update temp to CLCI module

input:
	die
		0 : local die
		1 : remote die
	temp
		chip temp
return:
	<0	: fail and return error code
	>=0	: success and the length of this commamd execution
*/
int32_t cmd_clci_delayline_tracking(int32_t die, uint32_t temp)
{
	struct cmd_common_t item;
	uint8_t in_buf[MAILBOX_SYS_DATA_MAX] = { 0 };
	uint8_t out_buf[MAILBOX_SYS_DATA_MAX] = { 0 };

	item.data = temp;
	item.res = die;
	in_buf[0] = CMD_DELAYLINE_TRACKING;
	memcpy(&in_buf[1], &item, sizeof(item));
	return mailbox_sys_send(in_buf, 1 + sizeof(item), out_buf, MAILBOX_SYS_DATA_MAX, MAILBOX_SYS_TIMEOUT_MS);
}

/*
	a simple clci relink api

return:
	0	: success
	1 	: fail
*/
int32_t clci_relink()
{
	int32_t ret = 0;

	if ((ret = cmd_clci_common(2, CMD_RESET)) < 0)
		return ret;
	/*
		update clci_config info at 0x10017c00
		refer to clci_config_regs.txt
	*/
	if ((ret = cmd_clci_common(2, CMD_CLCI_LINK)) < 0)
		return ret;

	return 0;
}

/*
return:
	0	: success
	1 	: fail
*/
int32_t clci_relink_2()
{
	int32_t ret = 0;

	if ((ret = cmd_clci_common(2, CMD_RESET)) < 0)
		return ret;

	if ((ret = cmd_clci_common(2, CMD_APHY_PLL_INIT)) < 0)
		return ret;

	// if (clci_error_get())
	// 	return 1;

	if ((ret = cmd_clci_common(2, CMD_APHY_INIT)) < 0)
		return ret;

	if ((ret = cmd_clci_common(2, CMD_SPHY_INIT)) < 0)
		return ret;

	if ((ret = cmd_clci_common(2, CMD_BITLOCK)) < 0)
		return ret;

	// if (clci_error_get())
	// 	return 1;

	if ((ret = cmd_clci_common(2, CMD_PCSLOCK)) < 0)
		return ret;

	// if (clci_error_get())
	// 	return 1;

	if ((ret = cmd_clci_common(2, CMD_MAC_INIT)) < 0)
		return ret;

	return 0;
}

/*
return:
	0	: link success
	1	: link fail
*/
int32_t clci_link_status()
{
	uint32_t regdata_0 = 0;
	uint32_t regdata_1 = 0;

	cmd_clci_get_reg(1, 0x3003c, &regdata_0);
	cmd_clci_get_reg(0, 0x3003c, &regdata_1);

	if (((regdata_0 >> 12) & 0x1) && ((regdata_1 >> 12) & 1)) {
		return 0;
	}

	return 1;
}
