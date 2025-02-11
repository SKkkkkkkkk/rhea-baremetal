#ifndef __LOCAL_CTRL_H__
#define __LOCAL_CTRL_H__

#include <reg_map.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint32_t vender_id; //0x0
	uint32_t reserv0; //0x4
	uint32_t device_id; //0x8
	uint32_t reserv1; //0xc
	uint32_t status0; //0x10
	uint32_t reserv3; //0x14
	uint32_t reserv4; //0x18
	uint32_t reserv5; //0x1c
	uint32_t status1; //0x20
	uint32_t reserv6; //0x24
	uint32_t reserv7; //0x28
	uint32_t reserv8; //0x2c
	uint32_t boot_sel; //0x30
	uint32_t reserv9; //0x34
	uint32_t reserv10; //0x38
	uint32_t reserv11; //0x3c
	uint32_t ctrl0; //0x40
	uint32_t addr_remap; //0x44
	uint32_t reserv12; //0x48
	uint32_t reserv13; //0x4c
	uint32_t msg0; //0x50
	uint32_t msg1; //0x54
	uint32_t msg2; //0x58
	uint32_t msg3; //0x5c
	uint32_t cpu_reboot_addr; //0x60
	uint32_t nmi_addr; //0x64
	uint32_t db_mcu; //0x68
	uint32_t db_soc; //0x6c
	uint32_t chiplet_id; //0x70
} mcu_local_reg_st;

extern uint64_t local_ctrl_base;

#define LOCAT_CTRL_REG_ADDR(MEMBER) ((uint64_t)local_ctrl_base + offsetof(mcu_local_reg_st, MEMBER))

#define CLCI_MCU_LOCAL_CTRL_CTRL0    (LOCAT_CTRL_REG_ADDR(ctrl0))
#define CLCI_MCU_LOCAL_CTRL_MSG0     (LOCAT_CTRL_REG_ADDR(msg0))
#define CLCI_MCU_LOCAL_CTRL_MSG1     (LOCAT_CTRL_REG_ADDR(msg1))
#define CLCI_MCU_LOCAL_CTRL_MSG2     (LOCAT_CTRL_REG_ADDR(msg2))
#define CLCI_MCU_LOCAL_CTRL_MSG3     (LOCAT_CTRL_REG_ADDR(msg3))
#define CLCI_MCU_LOCAL_CTRL_NMI_ADDR (LOCAT_CTRL_REG_ADDR(nmi_addr))

#define CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_MCU (LOCAT_CTRL_REG_ADDR(db_mcu))
#define CLCI_MCU_LOCAL_CTRL_DOORBELL_TO_SOC (LOCAT_CTRL_REG_ADDR(db_soc))

#define CLCI_MCU_CPU_BOOT_ADDR (LOCAT_CTRL_REG_ADDR(cpu_reboot_addr))
#define CLCI_MCU_CPU_CHIPLET_ID (LOCAT_CTRL_REG_ADDR(chiplet_id))

#define BIT_CPU_SOFT_RSTN			(1)
#define BIT_CPU_RUN_REQ				(1 << 1)
#define BIT_CPU_HALT_REG			(1 << 2)
#define BIT_BIU_RSTN				(1 << 3)
#define BIT_WDT_RSTN				(1 << 4)
#define BIT_TEST_NMI				(1 << 8)
#define BIT_TEST_SOFT_INT			(1 << 9)

void local_ctrl_base_set(uint64_t reg_base);
#endif
