#include <stdio.h>
#include <stdlib.h>

#include "gicv3.h"
#include "pcie.h"
#include "systimer.h"
#include "common.h"
#include "dw_apb_gpio.h"
#include "utils_def.h"
#include "mailbox.h"

/*                                   This is BAR Define
┌────┬─────┬────────────────┬────┬───────────────────┬────────────────┬───────────────┬───────────┐
│BARn│ Size│is Prefetchable?│bits│       name        │   assignment   │initial inbound│use inbound│
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR0│ 16MB│non-prefetchable│ 32 │    NPU_S2_BAR     │     NPU S2     │0x114_3000_0000│     0     │
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR1│  1MB│non-prefetchable│ 32 │   PCIE_DBI_BAR    │    PCIe DBI    │hardware assign│do not care│
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR2│ 16MB│non-prefetchable│ 32 │TILE_OR_TCM_CFG_BAR│tile cfg/tcm cfg│0x114_2000_0000│     1     │
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR3│ 64KB│non-prefetchable│ 32 │    HDMA_LL_BAR    │ HDMA Link List │0x004_57ff_0000│     2     │
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR4│512MB│  prefetchable  │ 64 │    TCM_MEM_BAR    │    TCM MEM     │0x004_4000_0000│     3     │
└────┴─────┴────────────────┴────┴───────────────────┴────────────────┴───────────────┴───────────┘
*/

#define SEEHI_PLD_PCIE_TEST 0
#define SEEHI_FPGA_PCIE_TEST 1

#define SEEHI_AP_PCIE_TEST 0
#define SEEHI_C2C_PCIE_TEST 1
#define SEEHI_TILE14_PCIE_TEST 0
#define SEEHI_4TILE_PCIE_TEST 0

#define SEEHI_MSIX_ENABLE 0

#define TCM_04_CFG_BASE 0x0015000000
#define TCM_14_CFG_BASE 0x8a22000000
#define TCM_15_CFG_BASE 0x8aa2000000
#define TCM_24_CFG_BASE 0x9222000000
#define TCM_25_CFG_BASE 0x92a2000000
#define C2C_SYS_CFG_03 0x8180000000ULL
#define C2C_SYS_CFG_02 0x8100000000ULL
#define C2C_SYS_CFG_73 0xB980000000ULL
#define C2C_SYS_CFG_72 0xB900000000ULL

static uint64_t g_c2c_base;
struct HAL_PCIE_HANDLE s_pcie;

struct PCIE_IDB_CFG
{
	volatile uint32_t magic;
#define PCIE_IDB_CFG_MAGIC 0x65696370
	volatile uint32_t bootconfig;
#define PCIE_IDB_CFG_PHY_MODE_SHIFT 0
#define PCIE_IDB_CFG_PHY_MODE_MASK 0x7
#define PCIE_IDB_CFG_PHY_MODE_AGGREGATION PHY_MODE_PCIE_AGGREGATION /**< PCIE3x4 */
#define PCIE_IDB_CFG_PHY_MODE_NANBNB PHY_MODE_PCIE_NANBNB			/**< P1:PCIE3x2  +  P0:PCIE3x2 */
#define PCIE_IDB_CFG_PHY_MODE_NANBBI PHY_MODE_PCIE_NANBBI			/**< P1:PCIE3x2  +  P0:PCIE3x1*2 */
#define PCIE_IDB_CFG_PHY_MODE_NABINB PHY_MODE_PCIE_NABINB			/**< P1:PCIE3x1*2 + P0:PCIE3x2 */
#define PCIE_IDB_CFG_PHY_MODE_NABIBI PHY_MODE_PCIE_NABIBI			/**< P1:PCIE3x1*2 + P0:PCIE3x1*2 */
#define PCIE_IDB_CFG_GEN_SHIFT 8
#define PCIE_IDB_CFG_GEN_MASK 0x7
#define PCIE_IDB_CFG_LANE_SHIFT 12
#define PCIE_IDB_CFG_LANE_MASK 0x7
#define PCIE_IDB_CFG_UART_ID_SHIFT 16
#define PCIE_IDB_CFG_UART_ID_MASK 0x7
#define PCIE_IDB_CFG_UART_MUX_SHIFT 20
#define PCIE_IDB_CFG_UART_MUX_MASK 0x7
#define PCIE_IDB_CFG_UART_RATE_SHIFT 24
#define PCIE_IDB_CFG_UART_RATE_MASK 0x3
#define PCIE_IDB_CFG_UART_RATE_DEFAULT 0
#define PCIE_IDB_CFG_UART_RATE_15000000 1
#define PCIE_IDB_CFG_UART_RATE_1152000 2
	volatile uint16_t vid;
	volatile uint16_t did;
};

/********************* Private MACRO Definition ******************************/
#define PCI_BASE_ADDRESS_0 0x10 /* 32 bits */
#define PCI_BASE_ADDRESS_SPACE 0x01
#define PCI_BASE_ADDRESS_MEM_TYPE_64 0x04  /* 64 bit address */
#define PCI_BASE_ADDRESS_MEM_TYPE_32 0x00  /* 32 bit address */
#define PCI_BASE_ADDRESS_MEM_PREFETCH 0x08 /* prefetchable? */

#define PCI_EXP_LNKCAP 0xc	 /* Link Capabilities */
#define PCI_EXP_LNKCTL2 0x30 /* Link Control 2 */
#define PCI_EXP_LNKCTL2_TLS 0x000f
#define PCI_EXP_LNKCAP_SLS 0x0000000f

#define PCI_EXP_LNKCTL2_TLS_2_5GT 0x0001  /* Supported Speed 2.5GT/s */
#define PCI_EXP_LNKCTL2_TLS_5_0GT 0x0002  /* Supported Speed 5GT/s */
#define PCI_EXP_LNKCTL2_TLS_8_0GT 0x0003  /* Supported Speed 8GT/s */
#define PCI_EXP_LNKCTL2_TLS_16_0GT 0x0004 /* Supported Speed 16GT/s */
#define PCI_EXP_LNKCTL2_TLS_32_0GT 0x0005 /* Supported Speed 32GT/s */

#define PCI_RESBAR 0x2e8 // 特殊寄存器resize

/* Synopsys-specific PCIe configuration registers */
#define PCIE_PORT_LINK_CONTROL 0x710
#define PORT_LINK_MODE_MASK (0x3f << 16)
#define PORT_LINK_MODE_1_LANES (0x1 << 16)
#define PORT_LINK_MODE_2_LANES (0x3 << 16)
#define PORT_LINK_MODE_4_LANES (0x7 << 16)
#define PORT_LINK_MODE_8_LANES (0xf << 16)
#define PORT_LINK_MODE_16_LANES (0x1f << 16)

#define PCIE_LINK_WIDTH_SPEED_CONTROL 0x80C
#define PORT_LOGIC_SPEED_CHANGE (0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK (0x1f << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES (0x1 << 8)
#define PORT_LOGIC_LINK_WIDTH_2_LANES (0x2 << 8)
#define PORT_LOGIC_LINK_WIDTH_4_LANES (0x4 << 8)
#define PORT_LOGIC_LINK_WIDTH_8_LANES (0x8 << 8)
#define PORT_LOGIC_LINK_WIDTH_16_LANES (0x10 << 8)

#define PCIE_DIRECT_SPEED_CHANGE (0x1 << 17)

#if SEEHI_C2C_PCIE_TEST
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x10410000000 // tile 14 cfg
#define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x00440000000 // tile 14 dram
#define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x00400000000 // tile 0 5
#else
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x11430000000 // tile 14 npu.s2
#define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x11420000000 // tile 14 tile cfg
#define BOOT_USING_PCIE_EP_BAR3_CPU_ADDRESS 0x00457ff0000 // HDMA Link List
#define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x00440000000 // tile 04 tcm mem
#endif

#define AP_SYS_C2C0_CPU_ADDRESS C2C_SYS_CFG_03
#define DWC_PCIE_CTL_X16_DBI (AP_SYS_C2C0_CPU_ADDRESS + 0x0)
#define C2C_ENGINE_X16 (AP_SYS_C2C0_CPU_ADDRESS + 0x140000)
#define PCIE_X16_REG (AP_SYS_C2C0_CPU_ADDRESS + 0x180000)
#define DWC_PCIE_CTL_X8_DBI (AP_SYS_C2C0_CPU_ADDRESS + 0x200000)
#define C2C_ENGINE_X8 (AP_SYS_C2C0_CPU_ADDRESS + 0x340000)
#define PCIE_X8_REG (AP_SYS_C2C0_CPU_ADDRESS + 0x380000)
#define PCIE_PHY_REG (AP_SYS_C2C0_CPU_ADDRESS + 0x400000)
#define DWC_PCIE5_PHY0_CRI (AP_SYS_C2C0_CPU_ADDRESS + 0x480000)
#define DWC_PCIE5_PHY1_CRI (AP_SYS_C2C0_CPU_ADDRESS + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI (AP_SYS_C2C0_CPU_ADDRESS + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI (AP_SYS_C2C0_CPU_ADDRESS + 0x4e0000)
#define C2C_SS_REG (AP_SYS_C2C0_CPU_ADDRESS + 0x580000)
#define DROUTER (AP_SYS_C2C0_CPU_ADDRESS + 0x581000)
#define CROUTER (AP_SYS_C2C0_CPU_ADDRESS + 0x582000)
#define DNIU (AP_SYS_C2C0_CPU_ADDRESS + 0x583000)
#define CNIU (AP_SYS_C2C0_CPU_ADDRESS + 0x584000)
#define MBI_TX (AP_SYS_C2C0_CPU_ADDRESS + 0x585000)

#define A510_APB_PCIE_EN_INT0 0xa0
#define A510_APB_PCIE_CLR_INT0 0xa4
#define A510_APB_PCIE_STAT_INT0 0xa8
#define A510_APB_PCIE_CFG_LINK_EQ_REQ_INT BIT(0)
#define A510_APB_PCIE_CFG_BW_MGT_MSI BIT(1)
#define A510_APB_PCIE_CFG_LINK_AUTO_BW_MSI BIT(2)
#define A510_APB_PCIE_CFG_BW_MGT_INT BIT(3)
#define A510_APB_PCIE_CFG_LINK_AUTO_BW_INT BIT(4)
#define A510_APB_PCIE_RADM_MSG_UNLOCK_INT BIT(5)
#define A510_APB_PCIE_RADM_MSG_LTR_INT BIT(6)
#define A510_APB_PCIE_RADM_MSG_VDM_INT BIT(7)
#define A510_APB_PCIE_RADM_MSG_SLOT_PWR_INT BIT(8)

// apb vdm
#define A510_APB_PCIE_MSG_VDM_REG0 0x300
#define A510_APB_PCIE_MSG_VDM_REG1 0x304
#define A510_APB_PCIE_MSG_VDM_REG2 0x308
#define A510_APB_PCIE_MSG_VDM_REG3 0x30c
#define A510_APB_PCIE_MSG_VDM_REG4 0x310
#define A510_APB_PCIE_MSG_VDM 0x314
#define A510_APB_PCIE_MSG_VDM_CLR BIT(0)
#define A510_APB_PCIE_MSG_VDM_VLD_MASK GENMASK(8, 1)
#define A510_APB_PCIE_MSG_VDM_WPTR_MASK GENMASK(10, 9)
#define A510_APB_PCIE_MSG_VDM_RPTR_MASK GENMASK(12, 11)
#define A510_APB_PCIE_MSG_VDM_RVLD_MASK GENMASK(17, 16)
#define A510_APB_PCIE_MSG_VDM_RX_MODE 0x318
#define A510_APB_PCIE_MSG_VDM_LATCH 0x31c

// apb ltr
#define A510_APB_PCIE_MSG_LTR_REG0 0x320
#define A510_APB_PCIE_MSG_LTR_REG1 0x324
#define A510_APB_PCIE_MSG_LTR_REG2 0x328
#define A510_APB_PCIE_MSG_LTR_REG3 0x32c
#define A510_APB_PCIE_MSG_LTR_REG4 0x330
#define A510_APB_PCIE_MSG_LTR 0x334
#define A510_APB_PCIE_MSG_LTR_CLR BIT(0)
#define A510_APB_PCIE_MSG_LTR_VLD_MASK GENMASK(4, 1)
#define A510_APB_PCIE_RADM_MSG_LTR BIT(3)
#define A510_APB_PCIE_RADM_MSG_UNLOCK BIT(4)
#define A510_APB_PCIE_MSG_LTR_RADM 0x338
#define A510_APB_PCIE_MSG_LTR_RADM_EN BIT(0)
#define A510_APB_PCIE_MSG_UNLOCK_RADM_EN BIT(1)
#define A510_APB_PCIE_MSG_LTR_LATCH 0x33c
#define A510_APB_PCIE_APP_LTR_LATENCY 0x128

// apb slot power message
#define A510_APB_PCIE_MSG_SLOT_PWR_PAYLOAD 0x340
#define A510_APB_PCIE_MSG_SLOT_PWR 0x344
#define A510_APB_PCIE_MSG_SLOT_PWR_CLR BIT(0)
#define A510_APB_PCIE_MSG_SLOT_PWR_VLD BIT(1)

/********************* Private Structure Definition **************************/

typedef struct
{
	union
	{ /* 0x00 */
		struct
		{
			__IOM uint32_t cfg_ven_msg_fmt : 2;
			__IOM uint32_t cfg_ven_msg_type : 5;
			__IOM uint32_t cfg_ven_msg_tc : 3;
			__IOM uint32_t cfg_ven_msg_td : 1;
			__IOM uint32_t cfg_ven_msg_ep : 1;
			__IOM uint32_t cfg_ven_msg_attr : 2;
			__IOM uint32_t : 2;
			__IOM uint32_t cfg_ven_msg_len : 10;
			__IOM uint32_t : 2;
			__IOM uint32_t cfg_ven_msg_func_num : 3;
			__IOM uint32_t cfg_ven_msg_req : 1;
		};
		__IOM uint32_t all;
	} cfg_ven_msg_0;

	union
	{ /* 0x04 */
		struct
		{
			__IOM uint32_t cfg_ven_msg_tag : 10;
			__IOM uint32_t sts_ven_msg_busy : 1;
			__IOM uint32_t : 5;
			__IOM uint32_t cfg_ven_msg_code : 8;
			__IOM uint32_t : 8;
		};
		__IOM uint32_t all;
	} cfg_ven_msg_4;

	__IOM uint32_t cfg_ven_msg_data_l; /* 0x08 */
	__IOM uint32_t cfg_ven_msg_data_h; /* 0x0c */

} vdm_msg_typedef;

typedef struct
{
	union
	{ /* 0x00 */
		struct
		{
			__IOM uint32_t app_ltr_msg_req : 1;
			__IOM uint32_t app_ltr_msg_req_stat : 1;
			__IOM uint32_t : 2;
			__IOM uint32_t app_ltr_msg_func_num : 3;
			__IOM uint32_t : 1;
			__IOM uint32_t cfg_ltr_m_en : 1;
			__IOM uint32_t cfg_disable_ltr_clr_msg : 1;
			__IOM uint32_t : 22;
		};
		__IOM uint32_t all;
	} cfg_ltr_msg_0;

	__IOM uint32_t app_ltr_msg_latency; /* 0x04 */

} ltr_msg_typedef;

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/

#if SEEHI_PLD_PCIE_TEST
void mc_init(uint64_t addr, uint8_t layer)
{
	// global
	if (layer == 4)
	{
		REG32(addr + 0x00013054) = 0x00000000;
		REG32(addr + 0x00013004) = 0x00001000; /* 2GB: 0x00001000, 512MB: 0x00000000 */
		REG32(addr + 0x00013004) = 0x80001000; /* 2GB: 0x80001000, 512MB: 0x80000000 */
	}
	else
	{
		REG32(addr + 0x00013054) = 0x00000000;
		REG32(addr + 0x00013004) = 0x00000010;
		REG32(addr + 0x00013004) = 0x80000010;
	}

	// bank
	uint32_t i, j, k;
	for (i = 0; i < 72; i++)
	{
		j = i / 18;
		k = i + j; // skip hub regs
		REG32(addr + k * 0x400 + 0x004) = 0x00000005;
		REG32(addr + k * 0x400 + 0x004) = 0x00000001;
		REG32(addr + k * 0x400 + 0x004) = 0x80000001;
	}
}
#endif

uint64_t get_pcie_base(uint32_t pcie_sel)
{
	if (pcie_sel == 2)
	{
		return C2C_SYS_CFG_02;
	}
	else if (pcie_sel == 3)
	{
		return C2C_SYS_CFG_03;
	}
	else if (pcie_sel == 72)
	{
		return C2C_SYS_CFG_72;
	}
	else if (pcie_sel == 73)
	{
		return C2C_SYS_CFG_73;
	}
	else
	{
		printf("pcie_sel error !!!\n");
		return 0;
	}
}

static inline void delay(uint32_t value)
{
	volatile uint32_t i, j;

	for (i = 0; i < value; i++)
		for (j = 0; j < 1000; j++)
			;
}

static inline void writel(uint32_t value, uint32_t address)
{
	uintptr_t addr = (uintptr_t)address;

	// printf("writel addr 0x%lx value 0x%x\n", addr, value);
	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(uint32_t address)
{
	uintptr_t addr = (uintptr_t)address;
	uint32_t value = *((volatile uint32_t *)(addr));

	// printf("readl addr 0x%lx value 0x%x\n", addr, value);
	return value;
}

static inline void writeq(uint32_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	// printf("writeq addr 0x%lx value 0x%x\n", addr, value);
	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readq(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;
	uint32_t value = *((volatile uint32_t *)(addr));

	// printf("readq addr 0x%lx value 0x%x\n", addr, value);
	return value;
}

static inline void pcie_writel_apb(struct HAL_PCIE_HANDLE *pcie, uint32_t value, uint64_t address)
{
	uint64_t apb_base = pcie->dev->apbBase;
	uintptr_t addr = (uintptr_t)(apb_base + address);

	// printf("writeq addr 0x%lx value 0x%x\n", addr, value);
	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t pcie_readl_apb(struct HAL_PCIE_HANDLE *pcie, uint64_t address)
{
	uint64_t apb_base = pcie->dev->apbBase;
	uintptr_t addr = (uintptr_t)(apb_base + address);
	uint32_t value = *((volatile uint32_t *)(addr));

	// printf("readq addr 0x%lx value 0x%x\n", addr, value);
	return value;
}

void BSP_PCIE_EP_VDM(const struct HAL_PCIE_HANDLE *pcie, int cnt, uint32_t l, uint32_t h)
{
	uint64_t apb_base = pcie->dev->apbBase;
	vdm_msg_typedef *vdm = (vdm_msg_typedef *)(apb_base + 0x130);

	cnt %= 1024;

	while (vdm->cfg_ven_msg_4.sts_ven_msg_busy == 1)
		;

	vdm->cfg_ven_msg_0.cfg_ven_msg_fmt = 0x1;
	vdm->cfg_ven_msg_0.cfg_ven_msg_type = 0x10;
	vdm->cfg_ven_msg_0.cfg_ven_msg_tc = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_td = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_ep = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_attr = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_len = 0x0;
	vdm->cfg_ven_msg_0.cfg_ven_msg_func_num = 0x0;

	vdm->cfg_ven_msg_4.cfg_ven_msg_tag = cnt;
	vdm->cfg_ven_msg_4.cfg_ven_msg_code = 0x7e;

	vdm->cfg_ven_msg_data_l = l;
	vdm->cfg_ven_msg_data_h = h;

	// printf("0x130 0x%x 0x134 0x%x 8byte 0x%x 12byte 0x%x\n", readq(apb_base + 0x130), readq(apb_base + 0x134), l, h);
	vdm->cfg_ven_msg_0.cfg_ven_msg_req = 0x1;
}

void BSP_PCIE_EP_LTR(const struct HAL_PCIE_HANDLE *pcie, int cnt)
{
	uint64_t apb_base = pcie->dev->apbBase;
	ltr_msg_typedef *ltr = (ltr_msg_typedef *)(apb_base + 0x120);

	cnt %= 8;

	if (ltr->cfg_ltr_msg_0.cfg_ltr_m_en)
	{

		while (ltr->cfg_ltr_msg_0.app_ltr_msg_req_stat == 1)
			;

		ltr->cfg_ltr_msg_0.app_ltr_msg_func_num = cnt;
		ltr->app_ltr_msg_latency = 0xff00ff + cnt;

		printf("ltr->app_ltr_msg_latency 0x%x\n", ltr->app_ltr_msg_latency);
		ltr->cfg_ltr_msg_0.app_ltr_msg_req = 0x1;
	}
}

static void a510_radm_msg_payload_parse(struct HAL_PCIE_HANDLE *pcie, uint32_t *req, uint32_t *byte8, uint32_t *byte12)
{
	uint32_t val, i = 0, cnt = 0;
	uint32_t temp0, temp1;
	uint32_t lanes = pcie->dev->max_lanes;

	pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_LATCH);
	val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_RX_MODE);
	if (val)
	{
		if (lanes == 8)
		{
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			// printf("A510_APB_PCIE_MSG_VDM 0x%x\n", val);
			while ((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) != 0)
			{

				if ((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 1)
				{

					*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
					*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

					temp0 = *(byte8 + i);
					temp1 = *(byte12 + i);
					printf("seehi--> %s line: %d bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);
					BSP_PCIE_EP_VDM(pcie, cnt++, temp0, temp1);
				}

				if ((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 2)
				{

					*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
					*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
				}

				if ((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 3)
				{

					*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
					*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
					*(byte8 + i + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
					*(byte12 + i + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);
				}

				i++;
				if (i >= 10)
				{
					printf("fifo full break!!!\n");
					break;
				}

				pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
				pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_LATCH);
				val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			}
		}
		else
		{
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			while ((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) != 0)
			{

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

				i++;
				if (i >= 10)
				{
					printf("fifo full break!!!\n");
					break;
				}

				pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
				pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_LATCH);
				val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			}
		}
	}
	else
	{
		if (lanes == 8)
		{
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			// printf("A510_APB_PCIE_MSG_VDM 0x%x\n", val);
			if ((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 1)
			{

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *byte8;
				temp1 = *byte12;
				printf("seehi--> %s line: %d bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);
			}

			if ((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 2)
			{

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
			}

			if ((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 3)
			{

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
				*(byte8 + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*(byte12 + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);
			}

			pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
		}
		else
		{
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			if ((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 1)
			{

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);
			}

			pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
		}
	}
}

void pcie_irq_handler(void)
{
	struct HAL_PCIE_HANDLE *pcie = &s_pcie;
	uint32_t reg, val, req, byte8[10], byte12[10];

	reg = pcie_readl_apb(pcie, A510_APB_PCIE_STAT_INT0);

	// printf("seehi--> %s line: %d ********** reg = 0x%x **********\n", __func__, __LINE__, reg);
	if (reg & A510_APB_PCIE_CFG_LINK_EQ_REQ_INT)
		printf("A510_APB_PCIE_CFG_LINK_EQ_REQ_INT\n");

	if (reg & A510_APB_PCIE_CFG_BW_MGT_MSI)
		printf("A510_APB_PCIE_CFG_BW_MGT_MSI\n");

	if (reg & A510_APB_PCIE_CFG_LINK_AUTO_BW_MSI)
		printf("A510_APB_PCIE_CFG_LINK_AUTO_BW_MSI\n");

	if (reg & A510_APB_PCIE_CFG_BW_MGT_INT)
		printf("A510_APB_PCIE_CFG_BW_MGT_INT\n");

	if (reg & A510_APB_PCIE_CFG_LINK_AUTO_BW_INT)
		printf("A510_APB_PCIE_CFG_LINK_AUTO_BW_INT\n");

	if (reg & A510_APB_PCIE_RADM_MSG_UNLOCK_INT)
		printf("A510_APB_PCIE_RADM_MSG_UNLOCK_INT\n");

	if (reg & A510_APB_PCIE_RADM_MSG_LTR_INT)
	{
		printf("A510_APB_PCIE_RADM_MSG_LTR_INT\n");
		pcie_writel_apb(pcie, 0x1, A510_APB_PCIE_MSG_LTR_LATCH);
		val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR);
		if (val & A510_APB_PCIE_RADM_MSG_LTR)
		{

			req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR_REG4);
			byte8[0] = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR_REG1);
			byte12[0] = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR_REG0);

			pcie_writel_apb(pcie, A510_APB_PCIE_MSG_LTR_CLR, A510_APB_PCIE_MSG_LTR);
			printf("seehi--> %s line: %d req 0x%x bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, req, byte8[0], byte12[0]);
		}
	}

	if (reg & A510_APB_PCIE_RADM_MSG_VDM_INT)
	{
		printf("A510_APB_PCIE_RADM_MSG_VDM_INT\n");
		a510_radm_msg_payload_parse(pcie, &req, byte8, byte12);
	}

	if (reg & A510_APB_PCIE_RADM_MSG_SLOT_PWR_INT)
		printf("A510_APB_PCIE_RADM_MSG_SLOT_PWR_INT\n");

	pcie_writel_apb(pcie, reg, A510_APB_PCIE_CLR_INT0);

	return;
}

static int seehi_pcie_ep_set_bar_flag(uint64_t dbi_base, uint32_t barno, int flags)
{
	uint32_t bar = barno;
	uint32_t reg, val;

	reg = PCI_BASE_ADDRESS_0 + (4 * bar);
	val = readq(dbi_base + reg);
	val &= 0xfffffff0;
	writeq(flags | val, dbi_base + reg);

	return 0;
}

static void dw_pcie_link_set_max_speed(uint64_t dbi_base, uint32_t link_gen)
{
	uint32_t cap, ctrl2, link_speed;
	uint8_t offset = 0x70;

	cap = readq(dbi_base + offset + PCI_EXP_LNKCAP);	// 最大
	ctrl2 = readq(dbi_base + offset + PCI_EXP_LNKCTL2); // 当前的
	ctrl2 &= ~PCI_EXP_LNKCTL2_TLS;

	link_speed = link_gen;

	cap &= ~((uint32_t)PCI_EXP_LNKCAP_SLS);
	writeq(ctrl2 | link_speed, dbi_base + offset + PCI_EXP_LNKCTL2);
	writeq(cap | link_speed, dbi_base + offset + PCI_EXP_LNKCAP);
}

static void dw_pcie_link_set_lanes(uint64_t dbi_base, uint32_t lanes)
{
	uint32_t val;

	/* Set the number of lanes */
	val = readq(dbi_base + PCIE_PORT_LINK_CONTROL); // 支持最大lanes
	val &= ~PORT_LINK_MODE_MASK;
	switch (lanes)
	{
	case 1:
		val |= PORT_LINK_MODE_1_LANES;
		break;
	case 2:
		val |= PORT_LINK_MODE_2_LANES;
		break;
	case 4:
		val |= PORT_LINK_MODE_4_LANES;
		break;
	case 8:
		val |= PORT_LINK_MODE_8_LANES;
		break;
	case 16:
		val |= PORT_LINK_MODE_16_LANES;
		break;
	default:
		printf("num-lanes invalid value\n");

		return;
	}
	writeq(val, dbi_base + PCIE_PORT_LINK_CONTROL);

	/* Set link width speed control register */ // 正确链接的lanes
	val = readq(dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (lanes)
	{
	case 1:
		val |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		val |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		val |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	case 8:
		val |= PORT_LOGIC_LINK_WIDTH_8_LANES;
		break;
	case 16:
		val |= PORT_LOGIC_LINK_WIDTH_16_LANES;
		break;
	}

	val |= PCIE_DIRECT_SPEED_CHANGE;

	writeq(val, dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
}

/* MSI-X registers (in MSI-X capability) */
#define PCI_MSIX_FLAGS 2						/* Message Control */
#define PCI_MSIX_FLAGS_QSIZE 0x07FF				/* Table size */
#define PCI_MSIX_FLAGS_MASKALL 0x4000			/* Mask all vectors for this function */
#define PCI_MSIX_FLAGS_ENABLE 0x8000			/* MSI-X enable */
#define PCI_MSIX_TABLE 4						/* Table offset */
#define PCI_MSIX_TABLE_BIR 0x00000007			/* BAR index */
#define PCI_MSIX_TABLE_OFFSET 0xfffffff8		/* Offset into specified BAR */
#define PCI_MSIX_PBA 8							/* Pending Bit Array offset */
#define PCI_MSIX_PBA_BIR 0x00000007				/* BAR index */
#define PCI_MSIX_PBA_OFFSET 0xfffffff8			/* Offset into specified BAR */
#define PCI_MSIX_FLAGS_BIRMASK PCI_MSIX_PBA_BIR /* deprecated */
#define PCI_CAP_MSIX_SIZEOF 12					/* size of MSIX registers */

/* MSI-X Table entry format (in memory mapped by a BAR) */
#define PCI_MSIX_ENTRY_SIZE 16
#define PCI_MSIX_ENTRY_LOWER_ADDR 0	  /* Message Address */
#define PCI_MSIX_ENTRY_UPPER_ADDR 4	  /* Message Upper Address */
#define PCI_MSIX_ENTRY_DATA 8		  /* Message Data */
#define PCI_MSIX_ENTRY_VECTOR_CTRL 12 /* Vector Control */
#define PCI_MSIX_ENTRY_CTRL_MASKBIT 0x00000001

#define PCIE_MISC_CONTROL_1_OFF 0x8BC // MISC_CONTROL_1_OFF
#define PCIE_DBI_RO_WR_EN 1

static inline void dw_pcie_dbi_ro_wr_en(uint64_t dbi_base)
{
	uint32_t reg;
	uint32_t val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = readq(dbi_base + reg);
	val |= PCIE_DBI_RO_WR_EN;
	writeq(val, dbi_base + reg);
}

static inline void dw_pcie_dbi_ro_wr_dis(uint64_t dbi_base)
{
	uint32_t reg;
	uint32_t val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = readq(dbi_base + reg);
	val &= ~PCIE_DBI_RO_WR_EN;
	writeq(val, dbi_base + reg);
}

static int dw_pcie_ep_set_msix(uint64_t dbi_base, uint32_t interrupts, uint32_t bar_offset, uint32_t bir)
{
	uint8_t offset = 0xb0;
	uint32_t val, reg;

	reg = offset;
	val = readq(dbi_base + reg);
	val &= ~(PCI_MSIX_FLAGS_QSIZE << 16);
	val |= interrupts << 16;
	writeq(val, dbi_base + reg);

	reg = offset + PCI_MSIX_TABLE;
	val = bar_offset | bir;
	writeq(val, dbi_base + reg);

	reg = offset + PCI_MSIX_PBA;
	val = (offset + (interrupts * PCI_MSIX_ENTRY_SIZE)) | bir;
	writeq(val, dbi_base + reg);

	return 0;
}

#define PCI_MSI_FLAGS 2				  /* Message Control */
#define PCI_MSI_FLAGS_ENABLE 0x0001	  /* MSI feature enabled */
#define PCI_MSI_FLAGS_QMASK 0x000e	  /* Maximum queue size available */
#define PCI_MSI_FLAGS_QSIZE 0x0070	  /* Message queue size configured */
#define PCI_MSI_FLAGS_64BIT 0x0080	  /* 64-bit addresses allowed */
#define PCI_MSI_FLAGS_MASKBIT 0x0100  /* Per-vector masking capable */
#define PCI_MSI_EXT_DATA_EN 0x4000000 /* 32 data */
#define PCI_MSI_RFU 3				  /* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO 4		  /* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI 8		  /* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32 8			  /* 16 bits of data for 32-bit devices */
#define PCI_MSI_MASK_32 12			  /* Mask bits register for 32-bit devices */
#define PCI_MSI_PENDING_32 16		  /* Pending intrs for 32-bit devices */
#define PCI_MSI_DATA_64 12			  /* 16 bits of data for 64-bit devices */
#define PCI_MSI_MASK_64 16			  /* Mask bits register for 64-bit devices */
#define PCI_MSI_PENDING_64 20		  /* Pending intrs for 64-bit devices */

static int dw_pcie_ep_set_msi(uint64_t dbi_base, uint32_t interrupts)
{
	uint32_t val, reg;
	uint8_t offset = 0x50;

	reg = offset;
	val = readq(dbi_base + reg);
	val &= ~(PCI_MSI_FLAGS_QMASK << 16);
	val |= interrupts << 17;
	writeq(val, dbi_base + reg);

	return 0;
}

static int dw_pcie_ep_msi_32_data(uint64_t dbi_base)
{
	uint32_t val, reg;
	uint8_t offset = 0x50;

	reg = offset;
	val = readq(dbi_base + reg);
	val |= PCI_MSI_EXT_DATA_EN;
	writeq(val, dbi_base + reg);

	return 0;
}

/********************* Public Function Definition ****************************/

void BSP_PCIE_EP_Init(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t phy_base = pcie->dev->phyBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint32_t lanes = pcie->dev->max_lanes;
	uint32_t val;

	val = readq(apb_base + 0x100); // RK有时钟相关配置我们是否也有
	val &= 0xfffffffe;
	writeq(val, apb_base + 0x100); // disable app_ltssm_enable

#if SEEHI_PLD_PCIE_TEST
	if (lanes == 16)
	{
		writeq(0, phy_base + 0x0);	// bif_en X16
		writeq(0, phy_base + 0x94); // pipe8_lane_mode  //
	}
	else if (lanes == 8)
	{
		writeq(1, phy_base + 0x0);	// bif_en X8
		writeq(8, phy_base + 0x94); // pipe8_lane_mode  //选phy clk
	}
	else
	{
		printf("PHY bifurcation error !\n");
	}
#elif SEEHI_FPGA_PCIE_TEST

#else
#endif

	writeq(0, apb_base + 0x110); ////close fast link
	writeq(0, apb_base + 0x104); // ep mode
}

void BSP_First_Reset(void)
{
	// printf("BSP_First_Reset\n");
}

static void gpio_sync_init(void)
{
	pinmux_select(PORTA, 24, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTA,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode,
		.pin = 24};
	gpio_init(&gpio_init_config);
}

HAL_Status PCIe_EP_Init(struct HAL_PCIE_HANDLE *pcie)
{
	uint32_t val, temp = 0, val_cmp = 0xf55a55aa;
	uint32_t bar;
	uint64_t dbi_base = pcie->dev->dbiBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint64_t ss_base = pcie->dev->ssBase;
	uint32_t dniu_base = 0x11002000;
	uint32_t mbitx_ap_base = 0x10050000;
#if SEEHI_TILE14_PCIE_TEST
	uint64_t mbitx_14tile_base = 0x8a20000000;
#elif SEEHI_4TILE_PCIE_TEST
	uint64_t mbitx_24tile_base = 0x9220000000;
#endif
	uint64_t resbar_base;
	int32_t i, timeout = 0, phy_linkup = 0;
	uint16_t vid, did;

	dw_pcie_dbi_ro_wr_en(dbi_base);

	BSP_PCIE_EP_Init(pcie); // 时钟同步，链路稳定，状态机进入polling，ltssm可以继续

	bar = 0;
	resbar_base = dbi_base + 0x10000;
#if SEEHI_C2C_PCIE_TEST
	writeq(0x007fffff, resbar_base + 0x10 + bar * 0x4); // 8M
#else
	writeq(0x00ffffff, resbar_base + 0x10 + bar * 0x4); // 16M
#endif
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

	bar = 1;
	writeq(0x000fffff, resbar_base + 0x10 + bar * 0x4); // 1M
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

#if SEEHI_C2C_PCIE_TEST
	bar = 2;
	writeq(0x3fffffff, resbar_base + 0x10 + bar * 0x4);		  //
	writeq(0x00000000, resbar_base + 0x10 + bar * 0x4 + 0x4); // 1G
#else
	bar = 2;
	writeq(0x00ffffff, resbar_base + 0x10 + bar * 0x4); // 16M
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

	bar = 3;
	writeq(0x0000ffff, resbar_base + 0x10 + bar * 0x4); // 64K
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);
#endif

	bar = 4;
#if SEEHI_C2C_PCIE_TEST
	writeq(0xffffffff, resbar_base + 0x10 + bar * 0x4);		  //
	writeq(0x00000003, resbar_base + 0x10 + bar * 0x4 + 0x4); // 16G
#else
	writeq(0x1fffffff, resbar_base + 0x10 + bar * 0x4);		  //
	writeq(0x00000000, resbar_base + 0x10 + bar * 0x4 + 0x4); // 512M
#endif
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_64 | PCI_BASE_ADDRESS_MEM_PREFETCH);
	/* BAR Config End */

	vid = 0x5348;							  // SH
	did = 0xa510;							  // a510
	writeq(did << 16 | vid, dbi_base + 0x00); // vendor id & device id

	writeq(0x12000001, dbi_base + 0x08); // class processing accelerators

	writeq(0x0, dbi_base + 0x3c);		 // interrrupt pin  no legacy interrupt Message
										 //
	writeq(0x00102130, dbi_base + 0x78); // 验证配置,DEVICE_CONTROL_DEVICE_STATUS 和MAX PAYLOAD SIZE 相关

	val = readq(dbi_base + 0x7c); // LINK_CAPABILITIES_REG  No ASPM Support
	val &= ~(3 << 10);
	writeq(val, dbi_base + 0x7c);

	val = readq(dbi_base + 0x98); // DEVICE_CONTROL2_DEVICE_STATUS2_REG  PCIE_CAP_LTR_EN
	val |= 1 << 10;
	writeq(val, dbi_base + 0x98);
	printf("PCIE_CAP_LTR_EN  !!!\n");

	val = readq(dbi_base + 0x80c); // GEN2_CTRL_OFF
	val |= 1 << 17;
	writeq(val, dbi_base + 0x80c);

	val = readq(dbi_base + 0x720); // FILTER_MASK_2_OFF
	val |= 0x3;
	writeq(val, dbi_base + 0x720);

	switch (pcie->dev->gen)
	{ // gen speed
	case 1:
		dw_pcie_link_set_max_speed(dbi_base, PCI_EXP_LNKCTL2_TLS_2_5GT);
		break;
	case 2:
		dw_pcie_link_set_max_speed(dbi_base, PCI_EXP_LNKCTL2_TLS_5_0GT);
		break;
	case 3:
		dw_pcie_link_set_max_speed(dbi_base, PCI_EXP_LNKCTL2_TLS_8_0GT);
		break;
	case 4:
		dw_pcie_link_set_max_speed(dbi_base, PCI_EXP_LNKCTL2_TLS_16_0GT);
		break;
	case 5:
		dw_pcie_link_set_max_speed(dbi_base, PCI_EXP_LNKCTL2_TLS_32_0GT);
		break;
	default:
		printf("Gen unkown\n");
	}
	dw_pcie_link_set_lanes(dbi_base, pcie->dev->lanes); // lanes

	systimer_delay(1, IN_MS);

	writeq(0x00402200, dbi_base + 0x890); // GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	writeq(0x01402200, dbi_base + 0x890); // GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	writeq(0x02402200, dbi_base + 0x890); // GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	writeq(0x4d004071, dbi_base + 0x8a8); // GEN3_EQ_CONTROL_OFF

	dw_pcie_ep_set_msix(dbi_base, 31, 0x70000, 1); // 有默认值不需要软件配置
	dw_pcie_ep_set_msi(dbi_base, 5);
	dw_pcie_ep_msi_32_data(dbi_base); // 32 data

#if SEEHI_FPGA_PCIE_TEST
	while (1)
	{
		val = gpio_read_pin(PORTA, 24);
		if (val == 0)
		{
			temp = 0x55;
		}

		if (val == 1 && temp == 0x55)
		{
			printf("get rc sysc single\n");
			break;
		}

		systimer_delay(1, IN_MS);
	}
#endif

	val = readq(apb_base + 0x100); // 验证配置的0x25
	val |= 0x1;
	writeq(val, apb_base + 0x100); // enable app_ltssm_enable

	while (1)
	{ // 判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readq(apb_base + 0x150);
		if ((val & 0xffff) == 0x1103)
		{ // L0
			printf("ctrl_link_status = 0x%x\n", val);
			phy_linkup = 1;
			break;
		}

		if (timeout >= 1000000)
		{
			// printf("timeout !!!\n");
			timeout = 0;
			// break;
		}

		systimer_delay(1, IN_MS);
		timeout++;

		if (val != val_cmp)
		{
			val_cmp = val;
			printf("ctrl_link_status = 0x%x\n", val);
		}
	}

	if (phy_linkup == 0)
	{
		BSP_First_Reset();
	}

#if SEEHI_FPGA_PCIE_TEST
	printf("Link up\n");
#endif

#if SEEHI_PLD_PCIE_TEST
#if SEEHI_C2C_PCIE_TEST
	HAL_PCIE_InboundConfig(pcie, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);
	HAL_PCIE_InboundConfig(pcie, 1, 2, BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS);
	HAL_PCIE_InboundConfig(pcie, 3, 4, BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS);
#else
	HAL_PCIE_InboundConfig(pcie, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);
	HAL_PCIE_InboundConfig(pcie, 1, 2, BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS);
	HAL_PCIE_InboundConfig(pcie, 2, 3, BOOT_USING_PCIE_EP_BAR3_CPU_ADDRESS);
	HAL_PCIE_InboundConfig(pcie, 3, 4, BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS);
#endif

#elif SEEHI_FPGA_PCIE_TEST
	dw_pcie_prog_inbound_atu(pcie, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);
	dw_pcie_prog_inbound_atu(pcie, 1, 2, BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS);
	dw_pcie_prog_inbound_atu(pcie, 2, 4, BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS);

#else

#error

#endif

	val = readq(dbi_base + 0x4);
	writeq(val | 0x6, dbi_base + 0x4); // cmd  bus & mem enable

#if SEEHI_FPGA_PCIE_TEST
	val = readq(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);
#endif

#if SEEHI_MSIX_ENABLE
	/////////////////////////////////////MSIX//////////////////////////////////////////////////////
	if (pcie->dev->max_lanes == 16)
	{
		writeq(0x40000, ss_base + 0x94);  // int_mbi_message_for_vector_00
		writeq(0x40001, ss_base + 0x98);  // int_mbi_message_for_vector_01
		writeq(0x40002, ss_base + 0x9c);  // int_mbi_message_for_vector_02
		writeq(0x40003, ss_base + 0xa0);  // int_mbi_message_for_vector_03
		writeq(0x40004, ss_base + 0xa4);  // int_mbi_message_for_vector_04
		writeq(0x40005, ss_base + 0xa8);  // int_mbi_message_for_vector_05
		writeq(0x40006, ss_base + 0xac);  // int_mbi_message_for_vector_06
		writeq(0x40007, ss_base + 0xb0);  // int_mbi_message_for_vector_07
		writeq(0x40008, ss_base + 0xb4);  // int_mbi_message_for_vector_08
		writeq(0x40009, ss_base + 0xb8);  // int_mbi_message_for_vector_09
		writeq(0x4000a, ss_base + 0xbc);  // int_mbi_message_for_vector_10
		writeq(0x4000b, ss_base + 0xc0);  // int_mbi_message_for_vector_11
		writeq(0x4000c, ss_base + 0xc4);  // int_mbi_message_for_vector_12
		writeq(0x4000d, ss_base + 0xc8);  // int_mbi_message_for_vector_13
		writeq(0x4000e, ss_base + 0xcc);  // int_mbi_message_for_vector_14
		writeq(0x4000f, ss_base + 0xd0);  // int_mbi_message_for_vector_15
		writeq(0x40010, ss_base + 0xd4);  // int_mbi_message_for_vector_16
		writeq(0x40011, ss_base + 0xd8);  // int_mbi_message_for_vector_17
		writeq(0x40012, ss_base + 0xdc);  // int_mbi_message_for_vector_18
		writeq(0x40013, ss_base + 0xe0);  // int_mbi_message_for_vector_19
		writeq(0x40014, ss_base + 0xe4);  // int_mbi_message_for_vector_20
		writeq(0x40015, ss_base + 0xe8);  // int_mbi_message_for_vector_21
		writeq(0x40016, ss_base + 0xec);  // int_mbi_message_for_vector_22
		writeq(0x40017, ss_base + 0xf0);  // int_mbi_message_for_vector_23
		writeq(0x40018, ss_base + 0xf4);  // int_mbi_message_for_vector_24
		writeq(0x40019, ss_base + 0xf8);  // int_mbi_message_for_vector_25
		writeq(0x4001a, ss_base + 0xfc);  // int_mbi_message_for_vector_26
		writeq(0x4001b, ss_base + 0x100); // int_mbi_message_for_vector_27
		writeq(0x4001c, ss_base + 0x104); // int_mbi_message_for_vector_28
		writeq(0x4001d, ss_base + 0x108); // int_mbi_message_for_vector_29
		writeq(0x4001e, ss_base + 0x10c); // int_mbi_message_for_vector_30
		writeq(0, ss_base + 0x110);		  // int_mbi_message_for_vector_31

		writeq(0x1 << 0, ss_base + 0x194);	  // pcie_x16 outbound bit0
		writeq(0x6, ss_base + 0x1a0);		  // pcie_x16 int_tx_msi_doorbell_x16
											  //
		writeq(0x60000001, dbi_base + 0x940); // MSIX_ADDRESS_MATCH_LOW_OFF  doorbell 只有32位，高位截断
		writeq(0x0, dbi_base + 0x944);		  // MSIX_ADDRESS_MATCH_EN  addr = 0x81_c000_000
	}
	else if (pcie->dev->max_lanes == 8)
	{
		writeq(0x40000, ss_base + 0x114); // int_mbi_message_for_vector_00
		writeq(0x40001, ss_base + 0x118); // int_mbi_message_for_vector_01
		writeq(0x40002, ss_base + 0x11c); // int_mbi_message_for_vector_02
		writeq(0x40003, ss_base + 0x120); // int_mbi_message_for_vector_03
		writeq(0x40004, ss_base + 0x124); // int_mbi_message_for_vector_04
		writeq(0x40005, ss_base + 0x128); // int_mbi_message_for_vector_05
		writeq(0x40006, ss_base + 0x12c); // int_mbi_message_for_vector_06
		writeq(0x40007, ss_base + 0x130); // int_mbi_message_for_vector_07
		writeq(0x40008, ss_base + 0x134); // int_mbi_message_for_vector_08
		writeq(0x40009, ss_base + 0x138); // int_mbi_message_for_vector_09
		writeq(0x4000a, ss_base + 0x13c); // int_mbi_message_for_vector_10
		writeq(0x4000b, ss_base + 0x140); // int_mbi_message_for_vector_11
		writeq(0x4000c, ss_base + 0x144); // int_mbi_message_for_vector_12
		writeq(0x4000d, ss_base + 0x148); // int_mbi_message_for_vector_13
		writeq(0x4000e, ss_base + 0x14c); // int_mbi_message_for_vector_14
		writeq(0x4000f, ss_base + 0x150); // int_mbi_message_for_vector_15
		writeq(0x40010, ss_base + 0x154); // int_mbi_message_for_vector_16
		writeq(0x40011, ss_base + 0x158); // int_mbi_message_for_vector_17
		writeq(0x40012, ss_base + 0x15c); // int_mbi_message_for_vector_18
		writeq(0x40013, ss_base + 0x160); // int_mbi_message_for_vector_19
		writeq(0x40014, ss_base + 0x164); // int_mbi_message_for_vector_20
		writeq(0x40015, ss_base + 0x168); // int_mbi_message_for_vector_21
		writeq(0x40016, ss_base + 0x16c); // int_mbi_message_for_vector_22
		writeq(0x40017, ss_base + 0x170); // int_mbi_message_for_vector_23
		writeq(0x40018, ss_base + 0x174); // int_mbi_message_for_vector_24
		writeq(0x40019, ss_base + 0x178); // int_mbi_message_for_vector_25
		writeq(0x4001a, ss_base + 0x17c); // int_mbi_message_for_vector_26
		writeq(0x4001b, ss_base + 0x180); // int_mbi_message_for_vector_27
		writeq(0x4001c, ss_base + 0x184); // int_mbi_message_for_vector_28
		writeq(0x4001d, ss_base + 0x188); // int_mbi_message_for_vector_29
		writeq(0x4001e, ss_base + 0x18c); // int_mbi_message_for_vector_30
		writeq(0, ss_base + 0x190);		  // int_mbi_message_for_vector_31

		writeq(0x1 << 1, ss_base + 0x194);	  // pcie_x8 outbound bit1
		writeq(0x7, ss_base + 0x1b0);		  // pcie_x16 int_tx_msi_doorbell_x8
											  //
		writeq(0x70000001, dbi_base + 0x940); // MSIX_ADDRESS_MATCH_LOW_OFF  doorbell 只有32位，高位截断
		writeq(0x0, dbi_base + 0x944);		  // MSIX_ADDRESS_MATCH_EN  addr = 0x81_c000_000
	}
	else
	{
		printf("msi config error !!!\n");
	}

	if (g_c2c_base == C2C_SYS_CFG_02)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0x60000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0x70000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}
	else if (g_c2c_base == C2C_SYS_CFG_03)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0xe0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0xf0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}
	else if (g_c2c_base == C2C_SYS_CFG_72)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0x60000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0x70000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}
	else if (g_c2c_base == C2C_SYS_CFG_73)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0xe0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0xf0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}
	else
	{
		printf("msi config error !!!\n");
	}
	writel(0xffffffff, mbitx_ap_base + 0x30); // 时能对应bit中断，总共32个bit
	writel(0x0, mbitx_ap_base + 0x40);		  // 时能对应bit目标remote|local，总共32个bit

	// writeq(0x0, dbi_base + 0x948);              //0:10 vector

#else

	if (pcie->dev->max_lanes == 16)
	{
		writeq(0x40000, ss_base + 0x94);	// int_mbi_message_for_vector_00
#if SEEHI_TILE14_PCIE_TEST
		writeq(0x00140020, ss_base + 0x98); // int_mbi_message_for_vector_01
#elif SEEHI_4TILE_PCIE_TEST
		writeq(0x00240020, ss_base + 0x98); // int_mbi_message_for_vector_01
#else
		writeq(0x40001, ss_base + 0x98); // int_mbi_message_for_vector_01
#endif
		writeq(0x40002, ss_base + 0x9c);	// int_mbi_message_for_vector_02
		writeq(0x40003, ss_base + 0xa0);	// int_mbi_message_for_vector_03
		writeq(0x40004, ss_base + 0xa4);	// int_mbi_message_for_vector_04
		writeq(0x40005, ss_base + 0xa8);	// int_mbi_message_for_vector_05
		writeq(0x40006, ss_base + 0xac);	// int_mbi_message_for_vector_06
		writeq(0x40007, ss_base + 0xb0);	// int_mbi_message_for_vector_07
		writeq(0x40008, ss_base + 0xb4);	// int_mbi_message_for_vector_08
		writeq(0x40009, ss_base + 0xb8);	// int_mbi_message_for_vector_09
		writeq(0x4000a, ss_base + 0xbc);	// int_mbi_message_for_vector_10
		writeq(0x4000b, ss_base + 0xc0);	// int_mbi_message_for_vector_11
		writeq(0x4000c, ss_base + 0xc4);	// int_mbi_message_for_vector_12
		writeq(0x4000d, ss_base + 0xc8);	// int_mbi_message_for_vector_13
		writeq(0x4000e, ss_base + 0xcc);	// int_mbi_message_for_vector_14
		writeq(0x4000f, ss_base + 0xd0);	// int_mbi_message_for_vector_15
		writeq(0x40010, ss_base + 0xd4);	// int_mbi_message_for_vector_16
		writeq(0x40011, ss_base + 0xd8);	// int_mbi_message_for_vector_17
		writeq(0x40012, ss_base + 0xdc);	// int_mbi_message_for_vector_18
		writeq(0x40013, ss_base + 0xe0);	// int_mbi_message_for_vector_19
		writeq(0x40014, ss_base + 0xe4);	// int_mbi_message_for_vector_20
		writeq(0x40015, ss_base + 0xe8);	// int_mbi_message_for_vector_21
		writeq(0x40016, ss_base + 0xec);	// int_mbi_message_for_vector_22
		writeq(0x40017, ss_base + 0xf0);	// int_mbi_message_for_vector_23
		writeq(0x40018, ss_base + 0xf4);	// int_mbi_message_for_vector_24
		writeq(0x40019, ss_base + 0xf8);	// int_mbi_message_for_vector_25
		writeq(0x4001a, ss_base + 0xfc);	// int_mbi_message_for_vector_26
		writeq(0x4001b, ss_base + 0x100);	// int_mbi_message_for_vector_27
		writeq(0x4001c, ss_base + 0x104);	// int_mbi_message_for_vector_28
		writeq(0x4001d, ss_base + 0x108);	// int_mbi_message_for_vector_29
		writeq(0x4001e, ss_base + 0x10c);	// int_mbi_message_for_vector_30
		writeq(0, ss_base + 0x110);			// int_mbi_message_for_vector_31

		writeq(0x0 << 0, ss_base + 0x194);	 // pcie_x16 outbound bit0
		writeq(0x40000000, ss_base + 0x198); // pcie_x16 int_tx_msi_doorbell_x16
	}
	else if (pcie->dev->max_lanes == 8)
	{
		writeq(0x40000, ss_base + 0x114); // int_mbi_message_for_vector_00
		writeq(0x40001, ss_base + 0x118); // int_mbi_message_for_vector_01
		writeq(0x40002, ss_base + 0x11c); // int_mbi_message_for_vector_02
		writeq(0x40003, ss_base + 0x120); // int_mbi_message_for_vector_03
		writeq(0x40004, ss_base + 0x124); // int_mbi_message_for_vector_04
		writeq(0x40005, ss_base + 0x128); // int_mbi_message_for_vector_05
		writeq(0x40006, ss_base + 0x12c); // int_mbi_message_for_vector_06
		writeq(0x40007, ss_base + 0x130); // int_mbi_message_for_vector_07
		writeq(0x40008, ss_base + 0x134); // int_mbi_message_for_vector_08
		writeq(0x40009, ss_base + 0x138); // int_mbi_message_for_vector_09
		writeq(0x4000a, ss_base + 0x13c); // int_mbi_message_for_vector_10
		writeq(0x4000b, ss_base + 0x140); // int_mbi_message_for_vector_11
		writeq(0x4000c, ss_base + 0x144); // int_mbi_message_for_vector_12
		writeq(0x4000d, ss_base + 0x148); // int_mbi_message_for_vector_13
		writeq(0x4000e, ss_base + 0x14c); // int_mbi_message_for_vector_14
		writeq(0x4000f, ss_base + 0x150); // int_mbi_message_for_vector_15
		writeq(0x40010, ss_base + 0x154); // int_mbi_message_for_vector_16
		writeq(0x40011, ss_base + 0x158); // int_mbi_message_for_vector_17
		writeq(0x40012, ss_base + 0x15c); // int_mbi_message_for_vector_18
		writeq(0x40013, ss_base + 0x160); // int_mbi_message_for_vector_19
		writeq(0x40014, ss_base + 0x164); // int_mbi_message_for_vector_20
		writeq(0x40015, ss_base + 0x168); // int_mbi_message_for_vector_21
		writeq(0x40016, ss_base + 0x16c); // int_mbi_message_for_vector_22
		writeq(0x40017, ss_base + 0x170); // int_mbi_message_for_vector_23
		writeq(0x40018, ss_base + 0x174); // int_mbi_message_for_vector_24
		writeq(0x40019, ss_base + 0x178); // int_mbi_message_for_vector_25
		writeq(0x4001a, ss_base + 0x17c); // int_mbi_message_for_vector_26
		writeq(0x4001b, ss_base + 0x180); // int_mbi_message_for_vector_27
		writeq(0x4001c, ss_base + 0x184); // int_mbi_message_for_vector_28
		writeq(0x4001d, ss_base + 0x188); // int_mbi_message_for_vector_29
		writeq(0x4001e, ss_base + 0x18c); // int_mbi_message_for_vector_30
		writeq(0, ss_base + 0x190);		  // int_mbi_message_for_vector_31

		writeq(0x0 << 1, ss_base + 0x194);	 // pcie_x8 outbound bit1
		writeq(0x50000000, ss_base + 0x1a8); // pcie_x16 int_tx_msi_doorbell_x8
	}
	else
	{
		printf("msi config error !!!\n");
	}

#if SEEHI_TILE14_PCIE_TEST
	if (pcie->dev->max_lanes == 16)
	{
		writeq(0x40000000, mbitx_14tile_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
	}
	else if (pcie->dev->max_lanes == 8)
	{
		writeq(0x50000000, mbitx_14tile_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
	}
	else
	{
		printf("msi config error !!!\n");
	}

	writeq(0x03, mbitx_14tile_base + 0x14);
	// writel(0x7fffffff, mbitx_14tile_base + 0x30);    //时能对应bit中断，总共32个bit
	writeq(0x1, mbitx_14tile_base + 0x34); // 时能对应bit中断，总共32个bit
	// writel(0x0, mbitx_14tile_base + 0x40);    //时能对应bit目标remote|local，总共32个bit
#elif SEEHI_4TILE_PCIE_TEST
	if (pcie->dev->max_lanes == 16)
	{
		writeq(0x40000000, mbitx_24tile_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
	}
	else if (pcie->dev->max_lanes == 8)
	{
		writeq(0x50000000, mbitx_24tile_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
	}
	else
	{
		printf("msi config error !!!\n");
	}

	writeq(0x03, mbitx_24tile_base + 0x14);
	// writel(0x7fffffff, mbitx_14tile_base + 0x30);    //时能对应bit中断，总共32个bit
	writeq(0x1, mbitx_24tile_base + 0x34); // 时能对应bit中断，总共32个bit
	// writel(0x0, mbitx_14tile_base + 0x40);    //时能对应bit目标remote|local，总共32个bit
#else
	// writeq((0 << 4), apb_base + 0x70);    //4:8  产生msi对应中断 bit0=1

	if (g_c2c_base == C2C_SYS_CFG_02)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0x40000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0x50000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}
	else if (g_c2c_base == C2C_SYS_CFG_03)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0xc0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0xd0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}
	else if (g_c2c_base == C2C_SYS_CFG_72)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0x40000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0x50000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}
	else if (g_c2c_base == C2C_SYS_CFG_73)
	{
		// 配置AP SYS MBI_TX 0x1005_0000
		if (pcie->dev->max_lanes == 16)
		{
			writel(0xc0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x16
		}
		else if (pcie->dev->max_lanes == 8)
		{
			writel(0xd0000000, mbitx_ap_base + 0x10); // AP 这边需要和doorbell地址能匹配上 x8
		}
		else
		{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}
	else
	{
		printf("msi config error !!!\n");
	}

	writel(0xffffffff, mbitx_ap_base + 0x30); // 时能对应bit中断，总共32个bit
	writel(0x0, mbitx_ap_base + 0x40);		  // 时能对应bit目标remote|local，总共32个bit
#endif // SEEHI_TILE14_PCIE_TEST

#endif // SEEHI_MSIX_ENABLE
	/////////////////////////////////////END//////////////////////////////////////////////////////
	dw_pcie_dbi_ro_wr_dis(dbi_base);

	pcie_writel_apb(pcie, 0x1ff, A510_APB_PCIE_EN_INT0);
	pcie_writel_apb(pcie, 0x1, A510_APB_PCIE_MSG_VDM_RX_MODE);

	return HAL_OK;
}

struct HAL_PCIE_DEV g_pcieDevX16 =
	{
		.apbBase = PCIE_X16_REG,
		.engineBase = C2C_ENGINE_X16,
		.dbiBase = DWC_PCIE_CTL_X16_DBI,
		.phyBase = PCIE_PHY_REG,
		.ssBase = C2C_SS_REG,
		.drouterBase = DROUTER,
		.crouterBase = CROUTER,
		.dniuBase = DNIU,
		.cniuBase = CNIU,
		.mbitxBase = MBI_TX,
		.max_lanes = 16,
		.lanes = 16,
		.gen = 5,
		.firstBusNo = 0x0,
		.ltrIrqNum = 172,
		.vdmIrqNum = 173,
};

struct HAL_PCIE_DEV g_pcieDevX16toX8 =
	{
		.apbBase = PCIE_X16_REG,
		.engineBase = C2C_ENGINE_X16,
		.dbiBase = DWC_PCIE_CTL_X16_DBI,
		.phyBase = PCIE_PHY_REG,
		.ssBase = C2C_SS_REG,
		.drouterBase = DROUTER,
		.crouterBase = CROUTER,
		.dniuBase = DNIU,
		.cniuBase = CNIU,
		.mbitxBase = MBI_TX,
		.max_lanes = 8,
		.lanes = 8,
		.gen = 3,
		.firstBusNo = 0x0,
		.ltrIrqNum = 172,
		.vdmIrqNum = 173,
};

struct HAL_PCIE_DEV g_pcieDevX8 =
	{
		.apbBase = PCIE_X8_REG,
		.engineBase = C2C_ENGINE_X8,
		.dbiBase = DWC_PCIE_CTL_X8_DBI,
		.phyBase = PCIE_PHY_REG,
		.ssBase = C2C_SS_REG,
		.drouterBase = DROUTER,
		.crouterBase = CROUTER,
		.dniuBase = DNIU,
		.cniuBase = CNIU,
		.mbitxBase = MBI_TX,
		.max_lanes = 8,
		.lanes = 4,
		.gen = 1,
		.firstBusNo = 0x20,
		.ltrIrqNum = 204,
		.vdmIrqNum = 205,
};

#define Mailbox_IRQn 59

static volatile uint32_t mailbox_i = 0U;
void mailbox_irqhandler(void)
{
	uint32_t sta;
	unsigned int lock0, lock1;
	printf("mailbox irq !!! \n");
	lock0 = get_lock(0);
	lock1 = get_lock(1);
	printf("rc get lock0 is %u\n", lock0);
	printf("rc get lock1 is %u\n", lock1);
	sta = MAILBOX_B2A->b2a_status;
	for (int i = 0; i < 4; i++)
	{
		if (((sta >> i) & 1) == 1)
		{
			printf("ep receive: cmd: %u  data: %u by ch%u\n\r", a_get_cmd(i), a_get_data(i), i);
			switch (i)
			{
			case 0:
				MAILBOX_B2A->b2a_status = 0x01;
				if (lock0 == 0 && lock1 == 1)
				{
					a2b_send(0, 0, 0);
					printf("ep send: cmd: %u  data: %u by ch%u\n\r", 0, 0, 0);
				}

				break;
			case 1:
				MAILBOX_B2A->b2a_status = 0x02;
				if (lock0 == 0 && lock1 == 1)
				{
					a2b_send(1, 1, 1);
					printf("ep send: cmd: %u  data: %u by ch%u\n\r", 1, 1, 1);
				}
				break;
			case 2:
				MAILBOX_B2A->b2a_status = 0x04;
				if (lock0 == 0 && lock1 == 1)
				{
					a2b_send(2, 2, 2);
					printf("ep send: cmd: %u  data: %u by ch%u\n\r", 2, 2, 2);
				}
				break;
			case 3:
				MAILBOX_B2A->b2a_status = 0x08;
				if (lock0 == 0 && lock1 == 1)
				{
					a2b_send(3, 3, 3);
					printf("ep send: cmd: %u  data: %u by ch%u\n\r", 3, 3, 3);
				}
				break;
			default:
				break;
			}
			lock1 = 1;
			MAILBOX_LOCK->atomic_lock[1] = lock1;
		}
	}
}

static void a2b_init()
{
	MAILBOX_A2B->a2b_status = 0xf; // Clear the interrupt by writing 1 to this bit.
	IRQ_SetHandler(Mailbox_IRQn, mailbox_irqhandler);
	GIC_SetPriority(Mailbox_IRQn, 0 << 3);
	GIC_EnableIRQ(Mailbox_IRQn);
}

int main()
{
	// struct PCIE_IDB_CFG *idb_cfg = (struct PCIE_IDB_CFG *)__pcie_idb_boot_cfg__;
	uint32_t result = HAL_ERROR;
	uint32_t cnt = 0;
	struct HAL_PCIE_HANDLE *pcie = &s_pcie;

#if SEEHI_FPGA_PCIE_TEST
	s_pcie.dev = &g_pcieDevX8;

	g_c2c_base = get_pcie_base(3);
	gpio_sync_init();

#elif SEEHI_PLD_PCIE_TEST
	mc_init(TCM_04_CFG_BASE, 4);

	g_c2c_base = get_pcie_base(3);

#if SEEHI_TILE14_PCIE_TEST
	mc_init(TCM_14_CFG_BASE, 4);
#elif SEEHI_4TILE_PCIE_TEST
	mc_init(TCM_14_CFG_BASE, 4);
	mc_init(TCM_15_CFG_BASE, 4);
	mc_init(TCM_24_CFG_BASE, 4);
	mc_init(TCM_25_CFG_BASE, 4);
#endif

	s_pcie.dev = &g_pcieDevX16;
	// s_pcie.dev = &g_pcieDevX16toX8;
	// s_pcie.dev = &g_pcieDevX8;
#endif

	systimer_init();

	GIC_Init();

	IRQ_SetHandler(pcie->dev->vdmIrqNum, pcie_irq_handler);
	GIC_SetPriority(pcie->dev->vdmIrqNum, 0 << 3);
	GIC_EnableIRQ(pcie->dev->vdmIrqNum);

#if SEEHI_FPGA_PCIE_TEST
	printf("PCIe_EP_Init start !!!\n");
#endif

#if SEEHI_4TILE_PCIE_TEST
	/* when print start, please power on x86 pc */
	printf("t14:0x%08x\n", REG32(0x1440000000 + 536870912 + 0xc0));
	printf("t15:0x%08x\n", REG32(0x1540000000 + 536870912 + 0xc0));
	printf("t24:0x%08x\n", REG32(0x2440000000 + 536870912 + 0xc0));
	printf("t25:0x%08x\n", REG32(0x2540000000 + 536870912 + 0xc0));
#endif

	PCIe_EP_Init(pcie);

#if SEEHI_FPGA_PCIE_TEST
	printf("PCIe_EP_Init end !!!\n");
#endif

#if SEEHI_C2C_PCIE_TEST
	// printf("BSP_PCIE_EP_VDM !!!\n");
	// systimer_delay(10, IN_S);
	// while (cnt < 10)
	// {
	// 	// BSP_PCIE_EP_VDM(pcie , cnt);
	// 	// BSP_PCIE_EP_LTR(pcie, cnt);
	// 	// printf("BSP_PCIE_EP_LTR !!!\n");
	// 	cnt++;
	// 	systimer_delay(10, IN_S);
	// }
#endif

#if SEEHI_C2C_PCIE_TEST
	printf("mailbox test !!!\n");
	a2b_init();
	unsigned int lock0;
	lock0 = get_lock(0);
	printf("rc get lock0 is %u\n", lock0);

#endif

#if SEEHI_PLD_PCIE_TEST
	REG32(0x12000fe0) = 0x4;
#endif

	while (1)
	{
		asm volatile("nop");
	};

	return result;
}
