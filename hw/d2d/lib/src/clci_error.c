#include <stdint.h>
#include <stdio.h>
#include "mmio.h"
#include "clci_error.h"
#include "commands_sys.h"
#include "log.h"
#include "clci_common.h"

/* Read the CLCI device error status
*  The CLCI device error status register 32-bit, each bit of this register
*  indicate a device error, which is defined in the clci_error.h
*
*  bit0: the clci device controler falls in an fatal exception
*  bit1: the clci device sideband module is failed, and do not work;
*  bit2: the clci device controler watch dog is timeout
*  bit3: the clci device controler mailbox communication using doorbell is failed, and do not work
*  bit4: the clci device controler clci link is failed, and do not work
*  bit5-bit31: unuseless
*/

#define CLCI_IRQ_EN0 0x100300ac
#define CLCI_IRQ_EN1 0x100300b4
#define CLCI_IRQ_EN2 0x100300bc
#define CLCI_IRQ_EN3 0x100300c4
#define CLCI_IRQ_EN4 0x100300cc
#define CLCI_IRQ_EN5 0x100300d4
#define CLCI_IRQ_EN6 0x100300dc
#define CLCI_IRQ_EN7 0x100300e4
#define CLCI_IRQ_EN8 0x100300ec
#define CLCI_IRQ_EN9 0x100300f4

#define CLCI_IRQ_STS0 0x100300a8
#define CLCI_IRQ_STS1 0x100300b0
#define CLCI_IRQ_STS2 0x100300b8
#define CLCI_IRQ_STS3 0x100300c0
#define CLCI_IRQ_STS4 0x100300c8
#define CLCI_IRQ_STS5 0x100300d0
#define CLCI_IRQ_STS6 0x100300d8
#define CLCI_IRQ_STS7 0x100300e0
#define CLCI_IRQ_STS8 0x100300e8
#define CLCI_IRQ_STS9 0x100300f0

uint32_t clci_module_base_addr;

uint32_t clci_error_get(void)
{
	return (mmio_read_32(CLCI_DEV_ERROR_REG));
}

/*
	Enable all clci interrupts
*/
void clci_irq_enable(int32_t dieid)
{
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN0, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN1, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN2, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN3, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN4, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN5, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN6, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN7, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN8, 0xffffffff);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN9, 0xffffffff);
}

/*
	Disable all clci interrupts
*/
void clci_irq_disable(int32_t dieid)
{
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN0, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN1, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN2, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN3, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN4, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN5, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN6, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN7, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN8, 0x0);
	cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_EN9, 0x0);
}

/*
	If interrupt comes, use this function to parse the interrupt source.
	Users can call this function to check CLCI status.

	dieid
		0 - local die
		1 - remote die
*/
void clci_irq_handler(int32_t dieid)
{
	uint32_t status = 0;

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS0, status);
	if (status & 0x1) {
		INFO(irq, "clci error: pcs_crc_err_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 0)));
	} else if (status & 0x2) {
		INFO(irq, "clci error: pcs_cmd_crc_err_int_sts");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 1)));
	} else if (status & 0x4) {
		INFO(irq, "clci error: pcs_retry_timeout_int_sts");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 2)));
	} else if (status & 0x8) {
		INFO(irq, "clci error: pcs_retry_id_timeout_int_sts");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 3)));
	} else if (status & 0xff0) {
		INFO(irq, "clci error: chs_cxs_mst_parity_int_sts");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(0xff << 4)));
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS1, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS1, status);
	if (status & 0x1ffff) {
		INFO(irq, "clci error: raw_pcs_bitlock_fail0_data_ent_sts\n");
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS2, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS2, status);
	if (status & 0x1ffff) {
		INFO(irq, "clci error: raw_pcs_bitlock_fail1_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, 0);
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS3, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS3, status);
	if (status & 0x1ffff) {
		INFO(irq, "clci error: raw_pcs_bitlock_retry_overflow_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, 0);
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS4, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS4, status);
	if (status & 0x1ffff) {
		INFO(irq, "clci error: raw_pcs_bitlock_ber_cnt_overflow_int_sts\n");
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS5, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS5, status);
	if (status & 0x1ffff) {
		INFO(irq, "clci error: raw_pcs_bitlock_ver_cnt2_overflow_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, 0);
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS6, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS6, status);
	if (status & 0xffff) {
		INFO(irq, "clci error: oversample_late_overflow_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(0xffff << 0)));
	} else if (status & 0xffff0000) {
		INFO(irq, "clci error: oversample_early_overflow_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(0xffff << 16)));
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS7, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS7, status);
	if (status & 0x1) {
		INFO(irq, "clci error: port0_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 0)));
	} else if ((status >> 1) & 0x1) {
		INFO(irq, "clci error: port0_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 1)));
	} else if ((status >> 2) & 0x1) {
		INFO(irq, "clci error: port0_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 2)));
	} else if ((status >> 3) & 0x1) {
		INFO(irq, "clci error: port0_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 3)));
	} else if ((status >> 4) & 0x1) {
		INFO(irq, "clci error: port0_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 4)));
	} else if ((status >> 5) & 0x1) {
		INFO(irq, "clci error: port0_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 5)));
	} else if ((status >> 6) & 0x1) {
		INFO(irq, "clci error: port0_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 6)));
	} else if ((status >> 7) & 0x1) {
		INFO(irq, "clci error: port0_cxs_mst_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 7)));
	} else if ((status >> 8) & 0x1) {
		INFO(irq, "clci error: port1_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 8)));
	} else if ((status >> 9) & 0x1) {
		INFO(irq, "clci error: port1_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 9)));
	} else if ((status >> 10) & 0x1) {
		INFO(irq, "clci error: port1_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 10)));
	} else if ((status >> 11) & 0x1) {
		INFO(irq, "clci error: port1_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 11)));
	} else if ((status >> 12) & 0x1) {
		INFO(irq, "clci error: port1_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 12)));
	} else if ((status >> 13) & 0x1) {
		INFO(irq, "clci error: port1_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 13)));
	} else if ((status >> 14) & 0x1) {
		INFO(irq, "clci error: port1_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 14)));
	} else if ((status >> 15) & 0x1) {
		INFO(irq, "clci error: port1_cxs_mst_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 15)));
	} else if ((status >> 16) & 0x1) {
		INFO(irq, "clci error: ace_slv2_cd_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 16)));
	} else if ((status >> 17) & 0x1) {
		INFO(irq, "clci error: ace_mst2_rd_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 17)));
	} else if ((status >> 18) & 0x1) {
		INFO(irq, "clci error: ace_slv2_wd_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 18)));
	} else if ((status >> 19) & 0x1) {
		INFO(irq, "clci error: ace_slv2_cr_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 19)));
	} else if ((status >> 20) & 0x1) {
		INFO(irq, "clci error: ace_mst2_ac_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 20)));
	} else if ((status >> 21) & 0x1) {
		INFO(irq, "clci error: ace_mst2_b_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 21)));
	} else if ((status >> 22) & 0x1) {
		INFO(irq, "clci error: ace_slv2_ar_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 22)));
	} else if ((status >> 23) & 0x1) {
		INFO(irq, "clci error: ace_slv2_aw_fifo_overflow_flag\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 23)));
	} else if ((status >> 24) & 0x1) {
		INFO(irq, "clci error: port3_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 24)));
	} else if ((status >> 25) & 0x1) {
		INFO(irq, "clci error: port3_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 25)));
	} else if ((status >> 26) & 0x1) {
		INFO(irq, "clci error: port3_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 26)));
	} else if ((status >> 27) & 0x1) {
		INFO(irq, "clci error: port3_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 27)));
	} else if ((status >> 28) & 0x1) {
		INFO(irq, "clci error: port3_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 28)));
	} else if ((status >> 29) & 0x1) {
		INFO(irq, "clci error: port3_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 29)));
	} else if ((status >> 30) & 0x1) {
		INFO(irq, "clci error: port3_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 30)));
	} else if ((status >> 31) & 0x1) {
		INFO(irq, "clci error: port3_cxs_mst_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 31)));
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS8, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS8, status);
	if (status & 0x1) {
		INFO(irq, "clci error: port4_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 0)));
	} else if ((status >> 1) & 0x1) {
		INFO(irq, "clci error: port4_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 1)));
	} else if ((status >> 2) & 0x1) {
		INFO(irq, "clci error: port4_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 2)));
	} else if ((status >> 3) & 0x1) {
		INFO(irq, "clci error: port4_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 3)));
	} else if ((status >> 4) & 0x1) {
		INFO(irq, "clci error: port4_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 4)));
	} else if ((status >> 5) & 0x1) {
		INFO(irq, "clci error: port4_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 5)));
	} else if ((status >> 6) & 0x1) {
		INFO(irq, "clci error: port4_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 6)));
	} else if ((status >> 7) & 0x1) {
		INFO(irq, "clci error: port4_chs_tx_fifo_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 7)));
	} else if ((status >> 8) & 0x1) {
		INFO(irq, "clci error: port5_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 8)));
	} else if ((status >> 9) & 0x1) {
		INFO(irq, "clci error: port5_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 9)));
	} else if ((status >> 10) & 0x1) {
		INFO(irq, "clci error: port5_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 10)));
	} else if ((status >> 11) & 0x1) {
		INFO(irq, "clci error: port5_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 11)));
	} else if ((status >> 12) & 0x1) {
		INFO(irq, "clci error: port5_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 12)));
	} else if ((status >> 13) & 0x1) {
		INFO(irq, "clci error: port5_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 13)));
	} else if ((status >> 14) & 0x1) {
		INFO(irq, "clci error: port5_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 14)));
	} else if ((status >> 15) & 0x1) {
		INFO(irq, "clci error: port5_cxs_mst_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 15)));
	} else if ((status >> 16) & 0x1) {
		INFO(irq, "clci error: port6_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 16)));
	} else if ((status >> 17) & 0x1) {
		INFO(irq, "clci error: port6_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 17)));
	} else if ((status >> 18) & 0x1) {
		INFO(irq, "clci error: port6_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 18)));
	} else if ((status >> 19) & 0x1) {
		INFO(irq, "clci error: port6_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 19)));
	} else if ((status >> 20) & 0x1) {
		INFO(irq, "clci error: port6_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 20)));
	} else if ((status >> 21) & 0x1) {
		INFO(irq, "clci error: port6_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 21)));
	} else if ((status >> 22) & 0x1) {
		INFO(irq, "clci error: port6_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 22)));
	} else if ((status >> 23) & 0x1) {
		INFO(irq, "clci error: port6_cxs_mst_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 23)));
	} else if ((status >> 24) & 0x1) {
		INFO(irq, "clci error: port7_pcs0_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 24)));
	} else if ((status >> 25) & 0x1) {
		INFO(irq, "clci error: port7_pcs1_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 25)));
	} else if ((status >> 26) & 0x1) {
		INFO(irq, "clci error: port7_mac_rx_ovfl_int_sts_0\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 26)));
	} else if ((status >> 27) & 0x1) {
		INFO(irq, "clci error: port7_mac_rx_ovfl_int_sts_1\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 27)));
	} else if ((status >> 28) & 0x1) {
		INFO(irq, "clci error: port7_mac_rx_ovfl_int_sts_2\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 28)));
	} else if ((status >> 29) & 0x1) {
		INFO(irq, "clci error: port7_mac_rx_ovfl_int_sts_3\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 29)));
	} else if ((status >> 30) & 0x1) {
		INFO(irq, "clci error: port7_mac_rx_ovfl_int_sts_4\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 30)));
	} else if ((status >> 31) & 0x1) {
		INFO(irq, "clci error: port7_cxs_mst_rx_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 31)));
	}

	cmd_clci_get_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS9, &status);
	INFO(irq, "clci error: addr 0x%08x data 0x%08x\n", clci_module_base_addr + CLCI_IRQ_STS9, status);
	if (status & 0x1) {
		INFO(irq, "clci error: pcs1_tx_afifo_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 0)));
	} else if (status & 0x2) {
		INFO(irq, "clci error: pcs0_tx_afifo_ovfl_int_sts\n");
		cmd_clci_set_reg(dieid, clci_module_base_addr + CLCI_IRQ_STS0, status & (~(1 << 1)));
	}
}

void clci_error_dump()
{
	uint32_t data = 0;

	data = mmio_read_32(CLCI_DEV_ERROR_REG);
	INFO(debug, "clci device status %08x\n", data);

	cmd_clci_get_reg(0, 0x30004, &data);
	INFO(debug, "clci device error code %08x\n", data);
	cmd_clci_get_reg(1, 0x30004, &data);
	INFO(debug, "clci device error code %08x\n", data);

	cmd_clci_get_reg(0, 0x17c1c, &data);
	INFO(debug, "clci link status %08x\n", data);
	cmd_clci_get_reg(1, 0x17c1c, &data);
	INFO(debug, "clci link status %08x\n", data);

	for (int lane = 0; lane < 17; lane++) {
		uint32_t addr = 0x30400 + 0x100 * lane;
		cmd_clci_get_reg(0, addr, &data);
		INFO(debug, "clci bitlock lane status %08x\n", data);
	}

	for (int lane = 0; lane < 17; lane++) {
		uint32_t addr = 0x30400 + 0x100 * lane;
		cmd_clci_get_reg(1, addr, &data);
		INFO(debug, "clci bitlock lane status %08x\n", data);
	}

	cmd_clci_get_reg(0, 0x3003c, &data);
	INFO(debug, "clci pcslock status %08x\n", data);
	cmd_clci_get_reg(1, 0x3003c, &data);
	INFO(debug, "clci pcslock status %08x\n", data);
}