#ifndef __COMMANDS_COMMON_H__
#define __COMMANDS_COMMON_H__

#include <stdint.h>
#include <local_ctrl.h>

#define SYS_CLCI_CTRL_MSG  CLCI_MCU_LOCAL_CTRL_MSG3
#define SYS_CLCI_CMD_ARG_P CLCI_MCU_LOCAL_CTRL_MSG0

enum {
	CMD_RESP_NULL,
	CMD_RESP_SUCCESS,
	CMD_RESP_FAIL,
	CMD_GET_REG,
	CMD_SET_REG,
	CMD_BOOTADDR_GO,
	CMD_UPDATE_SYS_CLK_BAUDRATE,
	CMD_IDDQ,
	CMD_PLL_CLK_CNT,
	CMD_BIST,
	CMD_APHY_INIT,
	CMD_SPHY_INIT,
	CMD_APHY_PLL_INIT,
	CMD_BITLOCK,
	CMD_PCSLOCK,
	CMD_MAC_INIT,
	CMD_CLCI_LINK,
	CMD_RESET,
	CMD_DELAYLINE_TRACKING
};

struct cmd_common_t {
	uint32_t addr;
	uint32_t data;
	uint32_t res;
};

void clci_set_resp();

void clci_clr_resp();

void clci_set_req();

int clci_get_req();

void clci_clr_req();

void clci_set_cmd(uint8_t cmd);

uint32_t clci_get_cmd();

int clci_is_resp_busy();

uint32_t clci_req(uint8_t cmd);

void clci_resp(uint8_t cmd);

void clci_set_status(int32_t status);

void clci_set_error(int32_t error);

#endif
