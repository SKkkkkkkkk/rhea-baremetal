#ifndef __COMMANDS_SYS_H__
#define __COMMANDS_SYS_H__

#include <stdint.h>

int32_t cmd_clci_get_reg(int32_t die, uint32_t addr, uint32_t *data);

int32_t cmd_clci_set_reg(int32_t die, uint32_t addr, uint32_t data);

int32_t cmd_clci_ram_firmware_go(int32_t die, uint32_t boot_addr);

int32_t cmd_clci_update_clk_bandrate(int32_t die, int16_t sys_clk, int16_t bandrate);

int32_t cmd_clci_clk_counter(int16_t die, int16_t lane, uint32_t *txfreq, uint32_t *rxfreq);

int32_t cmd_clci_transfer_temp(int32_t die, int32_t temp);

int32_t cmd_clci_common(int32_t die, uint8_t cmd);

int32_t clci_relink();

int32_t clci_relink_2();

int32_t clci_link_status();

#endif
