#ifndef __CHIP_H__
#define __CHIP_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "common.h"
#include "memmap.h"

#define CHIP_ID 			(REG32(SYSCTRL_CFG_BASE + 0xd00) & 0xFFFF)
#define IS_ASIC				((REG32(SYSCTRL_CFG_BASE + 0xd00)>>16U) & 0x1)
#define TAPEOUT_ID			((REG32(SYSCTRL_CFG_BASE + 0xd00)>>24U) & 0xFF)
#define MILESTONE_ID 		(REG32(SYSCTRL_CFG_BASE + 0xd04) & 0xFF)
#define CHIP_VERSION_MAJOR	((REG32(SYSCTRL_CFG_BASE + 0xd04)>>8U) & 0xFF)
#define CHIP_VERSION_MINOR	((REG32(SYSCTRL_CFG_BASE + 0xd04)>>16U) & 0xFF)
#define AP_SYS_VERSION		(REG32(SYSCTRL_CFG_BASE + 0xd10) & 0xFFFF)
#define TILE_SYS_VERSION	(REG32(SYSCTRL_CFG_BASE + 0xd14) & 0xFFFF)
#define D2D_SYS_VERSION		(REG32(SYSCTRL_CFG_BASE + 0xd18) & 0xFFFF)
#define C2C_SYS_VERSION		(REG32(SYSCTRL_CFG_BASE + 0xd1c) & 0xFFFF)

#ifdef __cplusplus
}
#endif

#endif // __CHIP_H__