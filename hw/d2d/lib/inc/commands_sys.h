#ifndef __COMMANDS_SYS_H__
#define __COMMANDS_SYS_H__

#include <stdint.h>

int32_t cmd_clci_get_reg(int32_t die, uint32_t addr, uint32_t *data);

int32_t cmd_clci_set_reg(int32_t die, uint32_t addr, uint32_t data);

int32_t cmd_clci_ram_firmware_go(int32_t die, uint32_t boot_addr);

int32_t cmd_clci_update_clk_bandrate(int32_t die, int16_t sys_clk, int16_t bandrate);

int32_t cmd_clci_clk_counter(int16_t die, int16_t lane, uint32_t *txfreq, uint32_t *rxfreq);

int32_t cmd_clci_delayline_tracking(int32_t die, uint32_t temp);

int32_t cmd_clci_common(int32_t die, uint8_t cmd);

int32_t cmd_clci_common_noack(int32_t die, uint8_t cmd);

int32_t clci_relink();

int32_t clci_relink_2();

int32_t clci_link_status();

int32_t clci_bist_loop(int32_t loop_type);

int32_t clci_enable_steam_mode(void);

void clci_dump(int id);

#endif
