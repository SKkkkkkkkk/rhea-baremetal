#include <stdio.h>
#include <stdlib.h>

#include "gicv3.h"
#include "pcie.h"
#include "systimer.h"
#include "common.h"
#include "dw_apb_gpio.h"
#include "utils_def.h"
#include "dw_apb_timers.h"
#include "lpi.h"
#include "d2d_api.h"
#include "d2d_sync.h"


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
//宏定义组合说明
//fpga平台组合比较固定
//	1、使用03互联连，一个控制器，没有npu，X8 控制器
//		SEEHI_FPGA_PCIE_TEST && SEEHI_C2C_PCIE_TEST && SEEHI_C2C_X8_TEST
//	2、使用03链接host，一个控制器，没有npu，X8 控制器
//		SEEHI_FPGA_PCIE_TEST && SEEHI_C2C_X8_TEST
//
//PLD平台会有多种组合，常用的如下
//	1、使用03互联连，一个控制器，没有npu，X8或者X16 控制器
//		PLD_Z1 && SEEHI_SINGLE_PCIE_TEST && SEEHI_AP_PCIE_TEST && SEEHI_C2C_PCIE_TEST && SEEHI_C2C_X8_TEST || SEEHI_C2C_X16_TEST
//	2、使用03链接host，使用73互联，两个控制器，有4个npu，X8或者X16控制器
//		PLD_Z2 && SEEHI_DUAL_PCIE_TEST && SEEHI_NPU_PCIE_TEST && SEEHI_C2C_PCIE_TEST && SEEHI_C2C_X8_TEST || SEEHI_C2C_X16_TEST
//	3、使用03连接host，一个控制器，没有npu，没有互联，X8或者X16控制器
//		PLD_Z1 && SEEHI_SINGLE_PCIE_TEST && SEEHI_AP_PCIE_TEST && SEEHI_C2C_X8_TEST || SEEHI_C2C_X16_TEST
//	4、使用03连接host，一个控制器，一个14NPU，没有互联，X8或者X16控制器
//		PLD_Z1 && SEEHI_SINGLE_PCIE_TEST && SEEHI_TILE14_PCIE_TEST && SEEHI_C2C_X8_TEST || SEEHI_C2C_X16_TEST
//either-or
#define SEEHI_PLD_PCIE_TEST			1
#define SEEHI_FPGA_PCIE_TEST		0

//either-or
#define PLD_Z1						1
#define PLD_Z2						0

//either-or
#define SEEHI_SINGLE_PCIE_TEST		1
#define SEEHI_DUAL_PCIE_TEST		0

//Choose more than one.
#define SEEHI_AP_PCIE_TEST			0
#define SEEHI_TILE14_PCIE_TEST		1
#define SEEHI_4TILE_PCIE_TEST		0
#define SEEHI_NPU_PCIE_TEST			0

#define SEEHI_C2C_PCIE_TEST			1

//either-or
#define SEEHI_C2C_X8_TEST			0
#define SEEHI_C2C_X16_TEST			1

#define SEEHI_C2C_ENGINE_TEST		0

#define SEEHI_MSIX_ENABLE			0

#define TCM_04_CFG_BASE         0x0015000000
#define TCM_14_CFG_BASE			0x8a22000000
#define TCM_15_CFG_BASE			0x8aa2000000
#define TCM_24_CFG_BASE			0x9222000000
#define TCM_25_CFG_BASE			0x92a2000000

#define TCM_53_CFG_BASE			0xa9a2000000
#define TCM_54_CFG_BASE			0xaa22000000
#define TCM_63_CFG_BASE			0xb1a2000000
#define TCM_64_CFG_BASE			0xb222000000

#define C2C_SYS_CFG_03       0x8180000000ULL
#define C2C_SYS_CFG_02       0x8100000000ULL
#define C2C_SYS_CFG_73       0xB980000000ULL
#define C2C_SYS_CFG_72       0xB900000000ULL

static uint64_t g_c2c_base;		// select c2c control
static uint8_t g_c2c_link;			// select link c2c 1: c2c  0: host
struct HAL_PCIE_HANDLE s_pcie;
struct HAL_PCIE_HANDLE s_pcie_02;
struct HAL_PCIE_HANDLE s_pcie_03;
struct HAL_PCIE_HANDLE s_pcie_72;
struct HAL_PCIE_HANDLE s_pcie_73;
struct HAL_PCIE_DEV g_pcieDev_02;
struct HAL_PCIE_DEV g_pcieDev_03;
struct HAL_PCIE_DEV g_pcieDev_72;
struct HAL_PCIE_DEV g_pcieDev_73;

/********************* Private MACRO Definition ******************************/
#define PCI_BASE_ADDRESS_0             0x10    /* 32 bits */
#define  PCI_BASE_ADDRESS_SPACE        0x01
#define  PCI_BASE_ADDRESS_MEM_TYPE_64  0x04    /* 64 bit address */
#define  PCI_BASE_ADDRESS_MEM_TYPE_32  0x00    /* 32 bit address */
#define  PCI_BASE_ADDRESS_MEM_PREFETCH 0x08    /* prefetchable? */

#define PCI_EXP_LNKCAP      0xc  /* Link Capabilities */
#define PCI_EXP_LNKCTL2     0x30  /* Link Control 2 */
#define PCI_EXP_LNKCTL2_TLS 0x000f
#define PCI_EXP_LNKCAP_SLS  0x0000000f

#define PCI_EXP_LNKCTL2_TLS_2_5GT 0x0001 /* Supported Speed 2.5GT/s */
#define PCI_EXP_LNKCTL2_TLS_5_0GT 0x0002 /* Supported Speed 5GT/s */
#define PCI_EXP_LNKCTL2_TLS_8_0GT 0x0003 /* Supported Speed 8GT/s */
#define PCI_EXP_LNKCTL2_TLS_16_0GT 0x0004 /* Supported Speed 16GT/s */
#define PCI_EXP_LNKCTL2_TLS_32_0GT 0x0005 /* Supported Speed 32GT/s */

#define PCI_RESBAR 0x2e8   //特殊寄存器resize

/* Synopsys-specific PCIe configuration registers */
#define PCIE_PORT_LINK_CONTROL 0x710
#define PORT_LINK_MODE_MASK    (0x3f << 16)
#define PORT_LINK_MODE_1_LANES (0x1 << 16)
#define PORT_LINK_MODE_2_LANES (0x3 << 16)
#define PORT_LINK_MODE_4_LANES (0x7 << 16)
#define PORT_LINK_MODE_8_LANES (0xf << 16)
#define PORT_LINK_MODE_16_LANES (0x1f << 16)

#define PCIE_FILTER_MASK_2 0x720
#define CX_FLT_MASK_VENMSG0_DROP (0x1 << 0)
#define CX_FLT_MASK_VENMSG1_DROP (0x1 << 1)

#define PCIE_LINK_WIDTH_SPEED_CONTROL 0x80C
#define PORT_LOGIC_SPEED_CHANGE       (0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK    (0x1f << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES (0x1 << 8)
#define PORT_LOGIC_LINK_WIDTH_2_LANES (0x2 << 8)
#define PORT_LOGIC_LINK_WIDTH_4_LANES (0x4 << 8)
#define PORT_LOGIC_LINK_WIDTH_8_LANES (0x8 << 8)
#define PORT_LOGIC_LINK_WIDTH_16_LANES (0x10 << 8)


#define PCIE_DIRECT_SPEED_CHANGE (0x1 << 17)

#define BOOT_USING_PCIE_C2C_BAR0_CPU_ADDRESS 0x10410000000  //ap cfg
#define BOOT_USING_PCIE_C2C_BAR2_CPU_ADDRESS 0x00440000000   //ap bootrom
#define BOOT_USING_PCIE_C2C_BAR3_CPU_ADDRESS 0x00400000000   //ap sram
#define BOOT_USING_PCIE_C2C_BAR4_CPU_ADDRESS 0x00440000000   //ap tcm
															 //
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x11430000000   //tile 14 npu.s2
#define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x11420000000   //tile 14 tile cfg
#define BOOT_USING_PCIE_EP_BAR3_CPU_ADDRESS 0x00457ff0000   //HDMA Link List
#define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x00440000000   //tile 04 tcm mem

#define AP_SYS_C2C0_CPU_ADDRESS_03	C2C_SYS_CFG_03
#define DWC_PCIE_CTL_X16_DBI		(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x0)
#define C2C_ENGINE_X16				(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x140000)
#define PCIE_X16_REG				(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x180000)
#define DWC_PCIE_CTL_X8_DBI			(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x200000)
#define C2C_ENGINE_X8				(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x340000)
#define PCIE_X8_REG					(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x380000)
#define PCIE_PHY_REG				(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x400000)
#define DWC_PCIE5_PHY0_CRI			(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x480000)
#define DWC_PCIE5_PHY1_CRI			(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI			(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI			(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x4e0000)
#define C2C_SS_REG					(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x580000)
#define DROUTER						(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x581000)
#define CROUTER						(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x582000)
#define DNIU						(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x583000)
#define CNIU						(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x584000)
#define MBI_TX						(AP_SYS_C2C0_CPU_ADDRESS_03 + 0x585000)

#define AP_SYS_C2C0_CPU_ADDRESS_02	C2C_SYS_CFG_02
#define DWC_PCIE_CTL_X16_DBI_02		(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x0)
#define C2C_ENGINE_X16_02			(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x140000)
#define PCIE_X16_REG_02				(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x180000)
#define DWC_PCIE_CTL_X8_DBI_02		(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x200000)
#define C2C_ENGINE_X8_02			(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x340000)
#define PCIE_X8_REG_02				(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x380000)
#define PCIE_PHY_REG_02				(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x400000)
#define DWC_PCIE5_PHY0_CRI_02		(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x480000)
#define DWC_PCIE5_PHY1_CRI_02		(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI_02		(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI_02		(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x4e0000)
#define C2C_SS_REG_02				(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x580000)
#define DROUTER_02					(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x581000)
#define CROUTER_02					(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x582000)
#define DNIU_02						(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x583000)
#define CNIU_02						(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x584000)
#define MBI_TX_02					(AP_SYS_C2C0_CPU_ADDRESS_02 + 0x585000)

#define AP_SYS_C2C0_CPU_ADDRESS_73	C2C_SYS_CFG_73
#define DWC_PCIE_CTL_X16_DBI_73		(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x0)
#define C2C_ENGINE_X16_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x140000)
#define PCIE_X16_REG_73				(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x180000)
#define DWC_PCIE_CTL_X8_DBI_73		(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x200000)
#define C2C_ENGINE_X8_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x340000)
#define PCIE_X8_REG_73				(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x380000)
#define PCIE_PHY_REG_73				(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x400000)
#define DWC_PCIE5_PHY0_CRI_73		(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x480000)
#define DWC_PCIE5_PHY1_CRI_73		(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI_73		(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI_73		(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x4e0000)
#define C2C_SS_REG_73				(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x580000)
#define DROUTER_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x581000)
#define CROUTER_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x582000)
#define DNIU_73						(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x583000)
#define CNIU_73						(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x584000)
#define MBI_TX_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x585000)

#define AP_SYS_C2C0_CPU_ADDRESS_72	C2C_SYS_CFG_72
#define DWC_PCIE_CTL_X16_DBI_72		(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x0)
#define C2C_ENGINE_X16_72			(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x140000)
#define PCIE_X16_REG_72				(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x180000)
#define DWC_PCIE_CTL_X8_DBI_72		(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x200000)
#define C2C_ENGINE_X8_72			(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x340000)
#define PCIE_X8_REG_72				(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x380000)
#define PCIE_PHY_REG_72				(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x400000)
#define DWC_PCIE5_PHY0_CRI_72		(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x480000)
#define DWC_PCIE5_PHY1_CRI_72		(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI_72		(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI_72		(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x4e0000)
#define C2C_SS_REG_72				(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x580000)
#define DROUTER_72					(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x581000)
#define CROUTER_72					(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x582000)
#define DNIU_72						(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x583000)
#define CNIU_72						(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x584000)
#define MBI_TX_72					(AP_SYS_C2C0_CPU_ADDRESS_72 + 0x585000)

#define A510_APB_PCIE_EN_INT0				0xa0
#define A510_APB_PCIE_CLR_INT0				0xa4
#define A510_APB_PCIE_STAT_INT0				0xa8
#define A510_APB_PCIE_CFG_LINK_EQ_REQ_INT	BIT(0)
#define A510_APB_PCIE_CFG_BW_MGT_MSI		BIT(1)
#define A510_APB_PCIE_CFG_LINK_AUTO_BW_MSI	BIT(2)
#define A510_APB_PCIE_CFG_BW_MGT_INT		BIT(3)
#define A510_APB_PCIE_CFG_LINK_AUTO_BW_INT	BIT(4)
#define A510_APB_PCIE_RADM_MSG_UNLOCK_INT	BIT(5)
#define A510_APB_PCIE_RADM_MSG_LTR_INT		BIT(6)
#define A510_APB_PCIE_RADM_MSG_VDM_INT		BIT(7)
#define A510_APB_PCIE_RADM_MSG_SLOT_PWR_INT	BIT(8)
#define A510_APB_PCIE_DIAG_CTRL_BUS         0x110

//apb vdm
#define A510_APB_PCIE_MSG_VDM_REG0			0x300
#define A510_APB_PCIE_MSG_VDM_REG1			0x304
#define A510_APB_PCIE_MSG_VDM_REG2			0x308
#define A510_APB_PCIE_MSG_VDM_REG3			0x30c
#define A510_APB_PCIE_MSG_VDM_REG4			0x310
#define A510_APB_PCIE_MSG_VDM				0x314
#define A510_APB_PCIE_MSG_VDM_CLR			BIT(0)
#define A510_APB_PCIE_MSG_VDM_VLD_MASK		GENMASK(8, 1)
#define A510_APB_PCIE_MSG_VDM_WPTR_MASK		GENMASK(10, 9)
#define A510_APB_PCIE_MSG_VDM_RPTR_MASK		GENMASK(12, 11)
#define A510_APB_PCIE_MSG_VDM_RVLD_MASK		GENMASK(17, 16)
#define A510_APB_PCIE_MSG_VDM_RX_MODE		0x318
#define A510_APB_PCIE_MSG_VDM_LATCH			0x31c

//apb ltr
#define A510_APB_PCIE_MSG_LTR_REG0			0x320
#define A510_APB_PCIE_MSG_LTR_REG1			0x324
#define A510_APB_PCIE_MSG_LTR_REG2			0x328
#define A510_APB_PCIE_MSG_LTR_REG3			0x32c
#define A510_APB_PCIE_MSG_LTR_REG4			0x330
#define A510_APB_PCIE_MSG_LTR				0x334
#define A510_APB_PCIE_MSG_LTR_CLR			BIT(0)
#define A510_APB_PCIE_MSG_LTR_VLD_MASK		GENMASK(4, 1)
#define A510_APB_PCIE_RADM_MSG_LTR			BIT(3)
#define A510_APB_PCIE_RADM_MSG_UNLOCK		BIT(4)
#define A510_APB_PCIE_MSG_LTR_RADM			0x338
#define A510_APB_PCIE_MSG_LTR_RADM_EN		BIT(0)
#define A510_APB_PCIE_MSG_UNLOCK_RADM_EN	BIT(1)
#define A510_APB_PCIE_MSG_LTR_LATCH			0x33c
#define A510_APB_PCIE_APP_LTR_LATENCY		0x128

//apb slot power message
#define A510_APB_PCIE_MSG_SLOT_PWR_PAYLOAD	0x340
#define A510_APB_PCIE_MSG_SLOT_PWR			0x344
#define A510_APB_PCIE_MSG_SLOT_PWR_CLR		BIT(0)
#define A510_APB_PCIE_MSG_SLOT_PWR_VLD		BIT(1)

#if SEEHI_C2C_PCIE_TEST
#if SEEHI_C2C_X16_TEST
#if PLD_Z1
#define PCIE_C2C_CONFIG_TARGET			0x21000000
#define PCIE_C2C_CONFIG_BASE			0x18000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x18080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x18100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xd400000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x400000000
#elif PLD_Z2
#define PCIE_C2C_CONFIG_TARGET			0x61000000
#define PCIE_C2C_CONFIG_BASE			0x1c000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x1c080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x1c100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xf400000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x400000000
#endif
#elif SEEHI_C2C_X8_TEST
#if PLD_Z1
#define PCIE_C2C_CONFIG_TARGET			0x31000000
#define PCIE_C2C_CONFIG_BASE			0x19000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x19080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x19100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xdc00000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x400000000
#elif PLD_Z2
#define PCIE_C2C_CONFIG_TARGET			0x71000000
#define PCIE_C2C_CONFIG_BASE			0x1d000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x1d080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x1d100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xfc00000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x400000000
#endif
#endif
#endif
/********************* Private Structure Definition **************************/

typedef struct {
	union { /* 0x00 */
		struct {
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
	}cfg_ven_msg_0;

	union { /* 0x04 */
		struct {
			__IOM uint32_t cfg_ven_msg_tag : 10;
			__IOM uint32_t sts_ven_msg_busy : 1;
			__IOM uint32_t : 5;
			__IOM uint32_t cfg_ven_msg_code : 8;
			__IOM uint32_t : 8;
		};
		__IOM uint32_t all;
	}cfg_ven_msg_4;

	__IOM uint32_t cfg_ven_msg_data_l; /* 0x08 */
	__IOM uint32_t cfg_ven_msg_data_h; /* 0x0c */

} vdm_msg_typedef;

typedef struct {
	union { /* 0x00 */
		struct {
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
	}cfg_ltr_msg_0;

	__IOM uint32_t app_ltr_msg_latency; /* 0x04 */

} ltr_msg_typedef;

typedef struct {
	union { /* 0x00 */
		struct {
			__IOM uint32_t op : 4;
			__IOM uint32_t intr_en : 1;
			__IOM uint32_t last_en : 1;
			__IOM uint32_t cnt_en : 1;
			__IOM uint32_t rsrv : 1;
			__IOM uint32_t length_size : 24;
		};
		__IOM uint32_t all;
	}cfg_c2c_engine;

	union { /* 0x04 */
		struct {
			__IOM uint32_t dst_addr_h : 16;
			__IOM uint32_t src_addr_h : 16;
		};
		__IOM uint32_t all;
	}cfg_c2c_engine_addr_h;

	__IOM uint32_t cfg_c2c_engine_src_addr_l; /* 0x08 */
	__IOM uint32_t cfg_c2c_engine_dst_addr_l; /* 0x0c */

} c2c_engine_desc_typedef;

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/

#if  SEEHI_PLD_PCIE_TEST
void mc_init(uint64_t addr, uint8_t layer) {
	// global
	if (layer == 4) {
		REG32(addr+0x00013054) = 0x00000000;
		REG32(addr+0x00013004) = 0x00001000; /* 2GB: 0x00001000, 512MB: 0x00000000 */
		REG32(addr+0x00013004) = 0x80001000; /* 2GB: 0x80001000, 512MB: 0x80000000 */
	} else {
		REG32(addr+0x00013054) = 0x00000000;
		REG32(addr+0x00013004) = 0x00000010;
		REG32(addr+0x00013004) = 0x80000010;
	}

	// bank
	uint32_t i, j, k;
	for (i = 0; i < 72; i++) {
		j = i / 18;
		k = i + j; // skip hub regs
		REG32(addr+k*0x400+0x004) = 0x00000005;
		REG32(addr+k*0x400+0x004) = 0x00000001;
		REG32(addr+k*0x400+0x004) = 0x80000001;
	}
}
#endif

static inline void delay(uint32_t value)
{
	volatile uint32_t i, j;

	for(i = 0; i < value; i++)
		for(j = 0; j < 1000; j++);
}

static inline void writeb(uint8_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint8_t *)(addr)) = value;
}

static inline uint8_t readb(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint8_t *)(addr));
}

static inline void writew(uint16_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint16_t *)(addr)) = value;
}

static inline uint16_t readw(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint16_t *)(addr));
}

static inline void writel(uint32_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
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

static void dump_regs(char * name, uint64_t addr, uint32_t length)
{
	if(name == NULL)
		return;

	printf("%s\n", name);
	for(int i = 0; i < length; i = i + 4){
		if((i % 16) == 0 && i != 0){
			printf("\n");
		}
		if((i % 16) == 0){
			printf("0x%lx: ", addr + i);
		}
		printf("0x%08x ", readl(addr + i));
	}
	printf("\n");
}

static uint64_t get_pcie_base(uint32_t pcie_sel) {
	if( pcie_sel == 2){
		return C2C_SYS_CFG_02;
	}else if(pcie_sel == 3){
		return C2C_SYS_CFG_03;
	}else if(pcie_sel == 72){
		return C2C_SYS_CFG_72;
	}else if(pcie_sel == 73){
		return C2C_SYS_CFG_73;
	}else{
		printf("pcie_sel error !!!\n");
		return 0;
	}
}

static void BSP_PCIE_EP_VDM(const struct HAL_PCIE_HANDLE *pcie , int cnt, uint32_t l, uint32_t h)
{
	uint64_t apb_base = pcie->dev->apbBase;
	vdm_msg_typedef *vdm = (vdm_msg_typedef *)(apb_base + 0x130);

	cnt %= 1024;

	while(vdm->cfg_ven_msg_4.sts_ven_msg_busy == 1);

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

	// printf("0x130 0x%x 0x134 0x%x 8byte 0x%x 12byte 0x%x\n", readl(apb_base + 0x130), readl(apb_base + 0x134), l, h);
	vdm->cfg_ven_msg_0.cfg_ven_msg_req = 0x1;
}

void BSP_PCIE_EP_LTR(const struct HAL_PCIE_HANDLE *pcie, int cnt)
{
	uint64_t apb_base = pcie->dev->apbBase;
	ltr_msg_typedef *ltr = (ltr_msg_typedef *)(apb_base + 0x120);

	cnt %= 8;

	if(ltr->cfg_ltr_msg_0.cfg_ltr_m_en){

		while(ltr->cfg_ltr_msg_0.app_ltr_msg_req_stat == 1);

		ltr->cfg_ltr_msg_0.app_ltr_msg_func_num = cnt;
		ltr->app_ltr_msg_latency = 0x7f007f + cnt;

		printf("ltr->app_ltr_msg_latency 0x%x\n", ltr->app_ltr_msg_latency);
		ltr->cfg_ltr_msg_0.app_ltr_msg_req = 0x1;
	}

}

void BSP_PCIE_EP_INTX(const struct HAL_PCIE_HANDLE *pcie, int cnt)
{
	uint64_t apb_base = pcie->dev->apbBase;

	printf("seehi--> %s line: %d cnt 0x%x\n", __func__, __LINE__, cnt);
	writel(0x2, apb_base + 0x108);
	dump_regs("intx:", apb_base + 0x108, 4);
	delay(50);
	writel(0x0, apb_base + 0x108);
	dump_regs("intx:", apb_base + 0x108, 4);
}

void BSP_PCIE_EP_TEST_MSI(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t apb_base = pcie->dev->apbBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	for(i = 0; i < 32; i++){
		writel((i << 4) | 0x1, apb_base + 0x70);    //4:8  产生msi对应中断 bit0=1
		delay(2);
	}
}

void BSP_PCIE_EP_TEST_MSIX(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	for(i = 0; i < 32; i++){
		writel(i, dbi_base + 0x948);              //0:10 vector
		delay(2);
	}
}

void BSP_PCIE_EP_TEST_GEN5(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D05, dbi_base + 0x7c);
	writel(0x01000005, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN4(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D04, dbi_base + 0x7c);
	writel(0x01000004, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN3(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D03, dbi_base + 0x7c);
	writel(0x01000003, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN2(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D02, dbi_base + 0x7c);
	writel(0x01000002, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

void BSP_PCIE_EP_TEST_GEN1(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	int i;

	printf("seehi--> %s line: %d\n", __func__, __LINE__);
	writel(0x00436D01, dbi_base + 0x7c);
	writel(0x01000001, dbi_base + 0xa0);
	writel(0x300310C8, dbi_base + 0x80c);
	writel(0x300110C8, dbi_base + 0x80c);
}

static void a510_radm_msg_payload_parse(struct HAL_PCIE_HANDLE *pcie, uint32_t *req, uint32_t *byte8, uint32_t * byte12)
{
	uint32_t val, i = 0, cnt = 0;
	uint32_t temp0, temp1;
	uint32_t lanes = pcie->dev->max_lanes;

	pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_LATCH);
	val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_RX_MODE);
	if(val){
		if(lanes == 8){
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			// printf("A510_APB_PCIE_MSG_VDM 0x%x\n", val);
			while((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) != 0){

				if((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 1){

					*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
					*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

					temp0 = *(byte8 + i);
					temp1 = *(byte12 + i);
					printf("seehi--> %s line: %d x8 fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

					if(temp0 == 0x1){
						BSP_PCIE_EP_LTR(pcie, cnt++);
					}else if(temp0 == 0x2){
						BSP_PCIE_EP_INTX(pcie, cnt++);
					}else if(temp0 == 0x3){
						BSP_PCIE_EP_TEST_MSI(pcie);
					}else if(temp0 == 0x4){
						BSP_PCIE_EP_TEST_MSIX(pcie);
					}else if(temp0 == 0x5){
						BSP_PCIE_EP_TEST_GEN5(pcie);
					}else if(temp0 == 0x6){
						BSP_PCIE_EP_TEST_GEN4(pcie);
					}else if(temp0 == 0x7){
						BSP_PCIE_EP_TEST_GEN3(pcie);
					}else if(temp0 == 0x8){
						BSP_PCIE_EP_TEST_GEN2(pcie);
					}else if(temp0 == 0x9){
						BSP_PCIE_EP_TEST_GEN1(pcie);
					}else{
						BSP_PCIE_EP_VDM(pcie, cnt++, temp0, temp1);
					}
				}

				if((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 2){

					*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
					*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
				}

				if((val & A510_APB_PCIE_MSG_VDM_RVLD_MASK) >> 16 == 3){

					*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
					*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
					*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
					*(byte8 + i + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
					*(byte12 + i + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);
				}

				i++;
				if(i >= 10){
					printf("fifo full break!!!\n");
					break;
				}

				pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
				pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_LATCH);
				val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			}
		}else{
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			while((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) != 0){

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*(byte8 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*(byte12 + i) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *(byte8 + i);
				temp1 = *(byte12 + i);
				printf("seehi--> %s line: %d x16 fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

				if(temp0 == 0x1){
					BSP_PCIE_EP_LTR(pcie, cnt++);
				}else if(temp0 == 0x2){
					BSP_PCIE_EP_INTX(pcie, cnt++);
				}else if(temp0 == 0x3){
					BSP_PCIE_EP_TEST_MSI(pcie);
				}else if(temp0 == 0x4){
					BSP_PCIE_EP_TEST_MSIX(pcie);
				}else if(temp0 == 0x5){
					BSP_PCIE_EP_TEST_GEN5(pcie);
				}else if(temp0 == 0x6){
					BSP_PCIE_EP_TEST_GEN4(pcie);
				}else if(temp0 == 0x7){
					BSP_PCIE_EP_TEST_GEN3(pcie);
				}else if(temp0 == 0x8){
					BSP_PCIE_EP_TEST_GEN2(pcie);
				}else if(temp0 == 0x9){
					BSP_PCIE_EP_TEST_GEN1(pcie);
				}else{
					BSP_PCIE_EP_VDM(pcie, cnt++, temp0, temp1);
				}

				i++;
				if(i >= 10){
					printf("fifo full break!!!\n");
					break;
				}

				pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
				pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_LATCH);
				val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			}
		}
	}else{
		if(lanes == 8){
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			// printf("A510_APB_PCIE_MSG_VDM 0x%x\n", val);
			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 1){

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *byte8;
				temp1 = *byte12;
				printf("seehi--> %s line: %d x8 no_fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

				if(temp0 == 0x1){
					BSP_PCIE_EP_LTR(pcie, cnt++);
				}else if(temp0 == 0x2){
					BSP_PCIE_EP_INTX(pcie, cnt++);
				}else if(temp0 == 0x3){
					BSP_PCIE_EP_TEST_MSI(pcie);
				}else if(temp0 == 0x4){
					BSP_PCIE_EP_TEST_MSIX(pcie);
				}else if(temp0 == 0x5){
					BSP_PCIE_EP_TEST_GEN5(pcie);
				}else if(temp0 == 0x6){
					BSP_PCIE_EP_TEST_GEN4(pcie);
				}else if(temp0 == 0x7){
					BSP_PCIE_EP_TEST_GEN3(pcie);
				}else if(temp0 == 0x8){
					BSP_PCIE_EP_TEST_GEN2(pcie);
				}else if(temp0 == 0x9){
					BSP_PCIE_EP_TEST_GEN1(pcie);
				}else{
					BSP_PCIE_EP_VDM(pcie, cnt++, temp0, temp1);
				}
			}

			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 2){

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
			}

			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 3){

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG2);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG3);
				*(byte8 + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*(byte12 + 1) = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);
			}

			pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
		}else{
			val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM);
			if((val & A510_APB_PCIE_MSG_VDM_VLD_MASK) >> 1 == 1){

				*req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG4);
				*byte8 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG0);
				*byte12 = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_VDM_REG1);

				temp0 = *byte8;
				temp1 = *byte12;
				printf("seehi--> %s line: %d x16 no_fifo | bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, temp0, temp1);

				if(temp0 == 0x1){
					BSP_PCIE_EP_LTR(pcie, cnt++);
				}else if(temp0 == 0x2){
					BSP_PCIE_EP_INTX(pcie, cnt++);
				}else if(temp0 == 0x3){
					BSP_PCIE_EP_TEST_MSI(pcie);
				}else if(temp0 == 0x4){
					BSP_PCIE_EP_TEST_MSIX(pcie);
				}else if(temp0 == 0x5){
					BSP_PCIE_EP_TEST_GEN5(pcie);
				}else if(temp0 == 0x6){
					BSP_PCIE_EP_TEST_GEN4(pcie);
				}else if(temp0 == 0x7){
					BSP_PCIE_EP_TEST_GEN3(pcie);
				}else if(temp0 == 0x8){
					BSP_PCIE_EP_TEST_GEN2(pcie);
				}else if(temp0 == 0x9){
					BSP_PCIE_EP_TEST_GEN1(pcie);
				}else{
					BSP_PCIE_EP_VDM(pcie, cnt++, temp0, temp1);
				}
			}

			pcie_writel_apb(pcie, A510_APB_PCIE_MSG_VDM_CLR, A510_APB_PCIE_MSG_VDM);
		}
	}


}

void pcie_irq_handler(void)
{
	struct HAL_PCIE_HANDLE *pcie;
	uint32_t reg, val, req, byte8[10], byte12[10];

	if(g_c2c_base == C2C_SYS_CFG_02){
		pcie = &s_pcie_02;
	}else if(g_c2c_base == C2C_SYS_CFG_03){
		pcie = &s_pcie_03;
	}else if(g_c2c_base == C2C_SYS_CFG_72){
		pcie = &s_pcie_72;
	}else if(g_c2c_base == C2C_SYS_CFG_73){
		pcie = &s_pcie_73;
	}else{
		pcie = NULL;
		printf("pcie_irq_handler error !!!\n");
		return;
	}

	reg = pcie_readl_apb(pcie, A510_APB_PCIE_STAT_INT0);

	printf("seehi--> %s line: %d A510_APB_PCIE_STAT_INT0 = 0x%x \n", __func__, __LINE__, reg);
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

	if (reg & A510_APB_PCIE_RADM_MSG_LTR_INT){
		printf("A510_APB_PCIE_RADM_MSG_LTR_INT\n");
		pcie_writel_apb(pcie, 0x1, A510_APB_PCIE_MSG_LTR_LATCH);
		val = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR);
		if(val & A510_APB_PCIE_RADM_MSG_LTR){

			req = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR_REG4);
			byte8[0] = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR_REG1);
			byte12[0] = pcie_readl_apb(pcie, A510_APB_PCIE_MSG_LTR_REG0);

			pcie_writel_apb(pcie, A510_APB_PCIE_MSG_LTR_CLR, A510_APB_PCIE_MSG_LTR);
			printf("seehi--> %s line: %d req 0x%x bayte8 0x%x byte12 0x%x\n", __func__, __LINE__, req, byte8[0], byte12[0]);
		}
	}

	if (reg & A510_APB_PCIE_RADM_MSG_VDM_INT){
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
	val = readl(dbi_base + reg);
	val &= 0xfffffff0;
	writel(flags | val, dbi_base + reg);

	return 0;
}

static void dw_pcie_link_set_max_speed(uint64_t dbi_base, uint32_t link_gen)
{
	uint32_t cap, ctrl2, link_speed;
	uint8_t offset = 0x70;

	cap = readl(dbi_base + offset + PCI_EXP_LNKCAP);  //最大
	ctrl2 = readl(dbi_base + offset + PCI_EXP_LNKCTL2);  //当前的
	ctrl2 &= ~PCI_EXP_LNKCTL2_TLS;

	link_speed = link_gen;

	cap &= ~((uint32_t)PCI_EXP_LNKCAP_SLS);
	writel(ctrl2 | link_speed, dbi_base + offset + PCI_EXP_LNKCTL2);
	writel(cap | link_speed, dbi_base + offset + PCI_EXP_LNKCAP);
}

static void dw_pcie_link_set_lanes(uint64_t dbi_base, uint32_t lanes)
{
	uint32_t val;

	/* Set the number of lanes */
	val = readl(dbi_base + PCIE_PORT_LINK_CONTROL);  //支持最大lanes
	val &= ~PORT_LINK_MODE_MASK;
	switch (lanes) {
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
	writel(val, dbi_base + PCIE_PORT_LINK_CONTROL);

	/* Set link width speed control register */  //正确链接的lanes
	val = readl(dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (lanes) {
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

	writel(val, dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
}

/* MSI-X registers (in MSI-X capability) */
#define PCI_MSIX_FLAGS		2	/* Message Control */
#define  PCI_MSIX_FLAGS_QSIZE	0x07FF	/* Table size */
#define  PCI_MSIX_FLAGS_MASKALL	0x4000	/* Mask all vectors for this function */
#define  PCI_MSIX_FLAGS_ENABLE	0x8000	/* MSI-X enable */
#define PCI_MSIX_TABLE		4	/* Table offset */
#define  PCI_MSIX_TABLE_BIR	0x00000007 /* BAR index */
#define  PCI_MSIX_TABLE_OFFSET	0xfffffff8 /* Offset into specified BAR */
#define PCI_MSIX_PBA		8	/* Pending Bit Array offset */
#define  PCI_MSIX_PBA_BIR	0x00000007 /* BAR index */
#define  PCI_MSIX_PBA_OFFSET	0xfffffff8 /* Offset into specified BAR */
#define PCI_MSIX_FLAGS_BIRMASK	PCI_MSIX_PBA_BIR /* deprecated */
#define PCI_CAP_MSIX_SIZEOF	12	/* size of MSIX registers */

/* MSI-X Table entry format (in memory mapped by a BAR) */
#define PCI_MSIX_ENTRY_SIZE		16
#define PCI_MSIX_ENTRY_LOWER_ADDR	0  /* Message Address */
#define PCI_MSIX_ENTRY_UPPER_ADDR	4  /* Message Upper Address */
#define PCI_MSIX_ENTRY_DATA		8  /* Message Data */
#define PCI_MSIX_ENTRY_VECTOR_CTRL	12 /* Vector Control */
#define  PCI_MSIX_ENTRY_CTRL_MASKBIT	0x00000001

#define PCIE_MISC_CONTROL_1_OFF		0x8BC    //MISC_CONTROL_1_OFF
#define PCIE_DBI_RO_WR_EN		1

static inline void dw_pcie_dbi_ro_wr_en(uint64_t dbi_base)
{
	uint32_t reg;
	uint32_t val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = readl(dbi_base + reg);
	val |= PCIE_DBI_RO_WR_EN;
	writel(val, dbi_base + reg);
}

static inline void dw_pcie_dbi_ro_wr_dis(uint64_t dbi_base)
{
	uint32_t reg;
	uint32_t val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = readl(dbi_base + reg);
	val &= ~PCIE_DBI_RO_WR_EN;
	writel(val, dbi_base + reg);
}

static int dw_pcie_ep_set_msix(uint64_t dbi_base, uint32_t interrupts, uint32_t bar_offset, uint32_t bir)
{
	uint8_t offset = 0xb0;
	uint32_t val, reg;

	reg = offset;
	val = readl(dbi_base + reg);
	val &= ~(PCI_MSIX_FLAGS_QSIZE << 16);
	val |= interrupts << 16;
	writel(val, dbi_base + reg);

	reg = offset + PCI_MSIX_TABLE;
	val = bar_offset | bir;
	writel(val, dbi_base + reg);

	reg = offset + PCI_MSIX_PBA;
	val = (bar_offset + (2048 * PCI_MSIX_ENTRY_SIZE)) | bir;
	writel(val, dbi_base + reg);

	return 0;
}

#define PCI_MSI_FLAGS		2	/* Message Control */
#define  PCI_MSI_FLAGS_ENABLE	0x0001	/* MSI feature enabled */
#define  PCI_MSI_FLAGS_QMASK	0x000e	/* Maximum queue size available */
#define  PCI_MSI_FLAGS_QSIZE	0x0070	/* Message queue size configured */
#define  PCI_MSI_FLAGS_64BIT	0x0080	/* 64-bit addresses allowed */
#define  PCI_MSI_FLAGS_MASKBIT	0x0100	/* Per-vector masking capable */
#define  PCI_MSI_EXT_DATA_EN	0x4000000	/* 32 data */
#define PCI_MSI_RFU		3	/* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO	4	/* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI	8	/* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32		8	/* 16 bits of data for 32-bit devices */
#define PCI_MSI_MASK_32		12	/* Mask bits register for 32-bit devices */
#define PCI_MSI_PENDING_32	16	/* Pending intrs for 32-bit devices */
#define PCI_MSI_DATA_64		12	/* 16 bits of data for 64-bit devices */
#define PCI_MSI_MASK_64		16	/* Mask bits register for 64-bit devices */
#define PCI_MSI_PENDING_64	20	/* Pending intrs for 64-bit devices */

static int dw_pcie_ep_set_msi(uint64_t dbi_base, uint32_t interrupts)
{
	uint32_t val, reg;
	uint8_t offset = 0x50;

	reg = offset;
	val = readl(dbi_base + reg);
	val &= ~(PCI_MSI_FLAGS_QMASK << 16);
	val |= interrupts << 17;
	writel(val, dbi_base + reg);

	return 0;
}

static int dw_pcie_ep_msi_32_data(uint64_t dbi_base)
{
	uint32_t val, reg;
	uint8_t offset = 0x50;

	reg = offset;
	val = readl(dbi_base + reg);
	val |= PCI_MSI_EXT_DATA_EN;
	writel(val, dbi_base + reg);

	return 0;
}

/********************* Public Function Definition ****************************/

void BSP_PCIE_EP_Init(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t phy_base = pcie->dev->phyBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint32_t lanes = pcie->dev->max_lanes;
	uint64_t ss_base = pcie->dev->ssBase;
	uint8_t bif_en = pcie->dev->bif_en;
	uint8_t pipe8 = pcie->dev->pipe8;
	uint32_t val;

	val = readl(apb_base + 0x100);
	val &= 0xfffffffe;
	writel(val, apb_base + 0x100);  //disable app_ltssm_enable

#if  SEEHI_PLD_PCIE_TEST
	// if(lanes == 16){
		// writel(0, phy_base + 0x0);  //bif_en X16
		// writel(0, phy_base + 0x94);  //pipe8_lane_mode  //
	// }else if(lanes == 8){
		// writel(1, phy_base + 0x0);  //bif_en X8
		// writel(8, phy_base + 0x94);  //pipe8_lane_mode  //选phy clk
	// }else{
		// printf("PHY bifurcation error !\n");
	// }

	writel(bif_en, phy_base + 0x0);  //bif_en X16
	writel(pipe8, phy_base + 0x94);  //pipe8_lane_mode  //

	if(g_c2c_link){
		writel(0, ss_base + 0x18);  //reg_resetn_button_ctl_x16
		writel(0, ss_base + 0x1c);
		delay(10);
		writel(1, ss_base + 0x18);
		writel(1, ss_base + 0x1c);

		writel(3, phy_base + 0x18);	//pipe16_pclkchange_hs_en

		// val = readl(apb_base + 0x100);
		// val |= 1 << 3;
		// writel(val, apb_base + 0x100);  //app_hold_phy_rst
	}

#elif SEEHI_FPGA_PCIE_TEST

#else
#error
#endif

	writel(0, apb_base + 0x110);  ////close fast link
	writel(0, apb_base + 0x104);  //ep mode
}

static void BSP_First_Reset(void)
{
	// printf("BSP_First_Reset\n");
}

static void gpio_sync_init(void)
{
#if  SEEHI_PLD_PCIE_TEST
	pinmux_select(PORTA, 0, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTA,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode,
		.pin = 0
	};
	gpio_init(&gpio_init_config);
#else
	pinmux_select(PORTA, 24, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTA,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Input_Mode,
		.pin = 24
	};
	gpio_init(&gpio_init_config);
#endif
}

void LPI_8192_Handler(void)
{
	printf("LPI 8192 received !!!!\n");
}

static void dw_timer_mbi_tx(uint32_t id)
{
	uint32_t val;

	writel(((uint64_t)(MBI_RX_BASE+0x40) & 0xFFFFFFFF), MBI_TX_BASE + 0x18);
	writel(((uint64_t)(MBI_RX_BASE+0x40) >> 32), MBI_TX_BASE + 0x1c);

	val = readl(MBI_TX_BASE + 0x30);
	val |= id;
	writel(val, MBI_TX_BASE + 0x30);

	val = readl(MBI_TX_BASE + 0x40);
	val |= id;
	writel(val, MBI_TX_BASE + 0x40);
}

static void dw_timer_init(void)
{
	timer_init_config_t timer_init_config = {
		.int_mask = 0, .loadcount = 25000000, .timer_id = Timerx6_T2, .timer_mode = Mode_User_Defined
	};
	timer_init(&timer_init_config);
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

	BSP_PCIE_EP_Init(pcie);    //时钟同步，链路稳定，状态机进入polling，ltssm可以继续

	bar = 0;
	resbar_base = dbi_base + 0x10000;
	if(g_c2c_link)
		writel(0x003fffff, resbar_base + 0x10 + bar * 0x4);   //4M
	else
		writel(0x00ffffff, resbar_base + 0x10 + bar * 0x4);   //16M
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

	bar = 1;
	writel(0x000fffff, resbar_base + 0x10 + bar * 0x4);   //1M
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

	if(g_c2c_link){
		bar = 2;
		writel(0x003fffff, resbar_base + 0x10 + bar * 0x4);   //4M
		seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

		bar = 3;
		writel(0x0000ffff, resbar_base + 0x10 + bar * 0x4);   //64K
		seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);
	}else{
		bar = 2;
		writel(0x00ffffff, resbar_base + 0x10 + bar * 0x4);   //16M
		seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

		bar = 3;
		writel(0x0000ffff, resbar_base + 0x10 + bar * 0x4);   //64K
		seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);
	}

	bar = 4;
	if(g_c2c_link){
		writel(0xffffffff, resbar_base + 0x10 + bar * 0x4);   //
		writel(0x00000003, resbar_base + 0x10 + bar * 0x4 + 0x4);   //16G
	}else{
		writel(0x1fffffff, resbar_base + 0x10 + bar * 0x4);   //
		writel(0x00000000, resbar_base + 0x10 + bar * 0x4 + 0x4);   //512M
	}
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_64 | PCI_BASE_ADDRESS_MEM_PREFETCH);
	/* BAR Config End */

	if(g_c2c_link){
		vid = 0x5348;    //AG
	}else{
		vid = 0x4147;    //SH
	}
	did = 0xa510;    //a510
	writel(did << 16 | vid, dbi_base + 0x00);  //vendor id & device id

	writel(0x20002, dbi_base + 0x2c);  //sub vendor id & device id
	writel(0x12000001, dbi_base + 0x08);  //class processing accelerators

	writel(0x1ff, dbi_base + 0x3c);  // interrrupt pin  legacy interrupt Message
									 //
	writel(0x00102130, dbi_base + 0x78);   //验证配置,DEVICE_CONTROL_DEVICE_STATUS 和MAX PAYLOAD SIZE 相关

	val = readl(dbi_base + 0x7c);   //LINK_CAPABILITIES_REG  No ASPM Support
	val &= ~(3 << 10);
	writel(val, dbi_base + 0x7c);

	val = readl(dbi_base + 0x98);   //DEVICE_CONTROL2_DEVICE_STATUS2_REG  PCIE_CAP_LTR_EN
	val |= 1 << 10;
	writel(val, dbi_base + 0x98);

	val = readl(dbi_base + 0x80c);   //GEN2_CTRL_OFF  DIRECT_SPEED_CHANGE
	val |= 1 << 17;
	writel(val, dbi_base + 0x80c);

	val = readl(dbi_base + 0x708);
	val |= 0x400000;
	writel(val, dbi_base + 0x708);  // Poling Active to Poling x1 x4 x8

	val = readl(dbi_base + PCIE_FILTER_MASK_2);   //FILTER_MASK_2_OFF
	val |= CX_FLT_MASK_VENMSG0_DROP | CX_FLT_MASK_VENMSG1_DROP;
	writel(val, dbi_base + PCIE_FILTER_MASK_2);

	switch (pcie->dev->gen) {  //gen speed
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
	dw_pcie_link_set_lanes(dbi_base, pcie->dev->lanes);  //lanes
	printf("pcie ep GEN %d, x%d\n", pcie->dev->gen, pcie->dev->lanes);

	systimer_delay(1, IN_MS);

	//rc only
	writel(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	writel(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0

	writel(0x01402200, dbi_base + 0x890);  //GEN4_RELATED_OFF.EQ_PHASE_2_3=0
	writel(0x01402200, dbi_base + 0x890);  //GEN4_RELATED_OFF.EQ_PHASE_2_3=0

	writel(0x02402200, dbi_base + 0x890);  //GEN5_RELATED_OFF.EQ_PHASE_2_3=0
	writel(0x02402200, dbi_base + 0x890);  //GEN5_RELATED_OFF.EQ_PHASE_2_3=0

	writel(0x4d004071, dbi_base + 0x8a8);  //GEN3_EQ_CONTROL_OFF

	dw_pcie_ep_set_msix(dbi_base, 31, 0x70000, 1);
	dw_pcie_ep_set_msi(dbi_base, 5);
	dw_pcie_ep_msi_32_data(dbi_base); //32 data

	while(g_c2c_link){
#if SEEHI_FPGA_PCIE_TEST
		val = gpio_read_pin(PORTA, 24);
		break;  //bug  todo
#elif SEEHI_PLD_PCIE_TEST
		val = gpio_read_pin(PORTA, 0);
#endif
		if(val == 0){
			temp = 0x55;
		}

		if(val == 1 && temp == 0x55){
			printf("get rc sysc single\n");
			break;
		}

		systimer_delay(1, IN_MS);
	}
	// val = readl(apb_base + 0x100);
	// val &= ~(1 << 3);
	// writel(val, apb_base + 0x100);  //disable app_hold_phy_rst

	val = readl(apb_base + 0x100);  // 验证配置的0x25
	val |= 0x1;
	writel(val, apb_base + 0x100);  //enable app_ltssm_enable

#if SEEHI_PLD_PCIE_TEST
	if(g_c2c_link){
		//only pld use
		val = readw(dbi_base + 0x70a);  // force phy link to state 1
		val |= 0x1;
		writew(val, dbi_base + 0x70a);  // force phy link to state 1

		val = readw(dbi_base + 0x708);  // force phy link to state 1
		val |= 0x8000;
		writew(val, dbi_base + 0x708);  // force phy link to state 1
		printf("force phy link to state 1\n");
	}
#endif

	/////////////////////////////////////END//////////////////////////////////////////////////////
	dw_pcie_dbi_ro_wr_dis(dbi_base);

	return HAL_OK;
}

HAL_Status PCIe_EP_Link(struct HAL_PCIE_HANDLE *pcie)
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

	if(g_c2c_link)
#if PLD_Z1
		printf("seehi--> %s line: %d for c2c link 03 \n", __func__, __LINE__);
#else
		printf("seehi--> %s line: %d for c2c link 73 \n", __func__, __LINE__);
#endif
	else
		printf("seehi--> %s line: %d for x86 link 03\n", __func__, __LINE__);

	while (1) {   //判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readl(apb_base + 0x150);
		if ((val & 0xffff) == 0x1103) { //L0
			printf("ep--> ctrl_link_status = 0x%x\n", val);
			phy_linkup = 1;
			break;
		}

		if(timeout >= 1000000){
			// printf("timeout !!!\n");
			timeout=0;
			// break;
		}

		systimer_delay(1, IN_MS);
		timeout++;

		if (val != val_cmp) {
			val_cmp = val;
			printf("ep--> ctrl_link_status = 0x%x\n", val);
		}
	}

	if(phy_linkup == 0){
		BSP_First_Reset();
	}

	printf("Link up\n");

#if  SEEHI_PLD_PCIE_TEST

	if(g_c2c_link){
#if SEEHI_NPU_PCIE_TEST
		HAL_PCIE_InboundConfig_addr(pcie, 0, 0, 0x10410000000, 0x1c800000, 0x400000);
		HAL_PCIE_InboundConfig_addr(pcie, 1, 0, 0x16330000000, 0xf430000000, 0x800000);
		HAL_PCIE_InboundConfig_addr(pcie, 2, 0, 0x6330800000, 0xf430800000, 0x20800000);

		HAL_PCIE_OutboundConfig(pcie, 0, 0, 0x430000000, 0x16330000000, 0x800000);
		HAL_PCIE_OutboundConfig(pcie, 1, 0, 0x430800000, 0x6330800000, 0x20800000);
#elif SEEHI_TILE14_PCIE_TEST
		HAL_PCIE_InboundConfig_addr(pcie, 0, 0, 0x10410000000, 0x18800000, 0x400000);
		HAL_PCIE_InboundConfig_addr(pcie, 1, 0, 0x11430000000, 0xd430000000, 0x800000);
		HAL_PCIE_InboundConfig_addr(pcie, 2, 0, 0x1430800000, 0xd430800000, 0x20800000);

		HAL_PCIE_OutboundConfig(pcie, 0, 0, 0x430000000, 0x11430000000, 0x800000);
		HAL_PCIE_OutboundConfig(pcie, 1, 0, 0x430800000, 0x1430800000, 0x20800000);
#else
		HAL_PCIE_InboundConfig(pcie, 0, 0, BOOT_USING_PCIE_C2C_BAR0_CPU_ADDRESS);
		HAL_PCIE_InboundConfig(pcie, 1, 2, BOOT_USING_PCIE_C2C_BAR2_CPU_ADDRESS);
		HAL_PCIE_InboundConfig(pcie, 2, 3, BOOT_USING_PCIE_C2C_BAR3_CPU_ADDRESS);

		HAL_PCIE_InboundConfig_addr(pcie, 3, 0, BOOT_USING_PCIE_C2C_BAR4_CPU_ADDRESS, PCIE_C2C_64_MEM_BASE, PCIE_C2C_64_MEM_BASE_SIZE);
#endif
	}else{
		HAL_PCIE_InboundConfig(pcie, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);
		HAL_PCIE_InboundConfig(pcie, 1, 2, BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS);
		HAL_PCIE_InboundConfig(pcie, 2, 3, BOOT_USING_PCIE_EP_BAR3_CPU_ADDRESS);
		HAL_PCIE_InboundConfig(pcie, 3, 4, BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS);
	}

#elif SEEHI_FPGA_PCIE_TEST
	dw_pcie_prog_inbound_atu(pcie, 0, 0, BOOT_USING_PCIE_C2C_BAR0_CPU_ADDRESS);
	dw_pcie_prog_inbound_atu(pcie, 1, 2, BOOT_USING_PCIE_C2C_BAR2_CPU_ADDRESS);
	dw_pcie_prog_inbound_atu(pcie, 2, 3, BOOT_USING_PCIE_C2C_BAR3_CPU_ADDRESS);

	dw_pcie_prog_inbound_atu_addr(pcie, 3, 0, BOOT_USING_PCIE_C2C_BAR4_CPU_ADDRESS, PCIE_C2C_64_MEM_BASE, PCIE_C2C_64_MEM_BASE_SIZE);
#else

#error

#endif

	val = readl(dbi_base + 0x4);
	writel(val | 0x6, dbi_base + 0x4);   // cmd  bus & mem enable

	val = readl(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);

#if SEEHI_MSIX_ENABLE
	/////////////////////////////////////MSIX//////////////////////////////////////////////////////
	if(pcie->dev->max_lanes == 16){
		// writel(0x40000, ss_base + 0x94);    // int_mbi_message_for_vector_00
		writel(0x40001, ss_base + 0x98);    // int_mbi_message_for_vector_01
		writel(0x40002, ss_base + 0x9c);    // int_mbi_message_for_vector_02
		writel(0x40003, ss_base + 0xa0);    // int_mbi_message_for_vector_03
		writel(0x40004, ss_base + 0xa4);    // int_mbi_message_for_vector_04
		writel(0x40005, ss_base + 0xa8);    // int_mbi_message_for_vector_05
		writel(0x40006, ss_base + 0xac);    // int_mbi_message_for_vector_06
		writel(0x40007, ss_base + 0xb0);    // int_mbi_message_for_vector_07
		writel(0x40008, ss_base + 0xb4);    // int_mbi_message_for_vector_08
		writel(0x40009, ss_base + 0xb8);    // int_mbi_message_for_vector_09
		writel(0x4000a, ss_base + 0xbc);    // int_mbi_message_for_vector_10
		writel(0x4000b, ss_base + 0xc0);    // int_mbi_message_for_vector_11
		writel(0x4000c, ss_base + 0xc4);    // int_mbi_message_for_vector_12
		writel(0x4000d, ss_base + 0xc8);    // int_mbi_message_for_vector_13
		writel(0x4000e, ss_base + 0xcc);    // int_mbi_message_for_vector_14
		writel(0x4000f, ss_base + 0xd0);    // int_mbi_message_for_vector_15
		writel(0x40010, ss_base + 0xd4);    // int_mbi_message_for_vector_16
		writel(0x40011, ss_base + 0xd8);    // int_mbi_message_for_vector_17
		writel(0x40012, ss_base + 0xdc);    // int_mbi_message_for_vector_18
		writel(0x40013, ss_base + 0xe0);    // int_mbi_message_for_vector_19
		writel(0x40014, ss_base + 0xe4);    // int_mbi_message_for_vector_20
		writel(0x40015, ss_base + 0xe8);    // int_mbi_message_for_vector_21
		writel(0x40016, ss_base + 0xec);    // int_mbi_message_for_vector_22
		writel(0x40017, ss_base + 0xf0);    // int_mbi_message_for_vector_23
		writel(0x40018, ss_base + 0xf4);    // int_mbi_message_for_vector_24
		writel(0x40019, ss_base + 0xf8);    // int_mbi_message_for_vector_25
		writel(0x4001a, ss_base + 0xfc);    // int_mbi_message_for_vector_26
		writel(0x4001b, ss_base + 0x100);    // int_mbi_message_for_vector_27
		writel(0x4001c, ss_base + 0x104);    // int_mbi_message_for_vector_28
		writel(0x4001d, ss_base + 0x108);    // int_mbi_message_for_vector_29
		writel(0x4001e, ss_base + 0x10c);    // int_mbi_message_for_vector_30
		writel(0, ss_base + 0x110);    // int_mbi_message_for_vector_31

		writel(0x1 << 0, ss_base + 0x194);    // pcie_x16 outbound bit0
		writel(0x6, ss_base + 0x1a0);    // pcie_x16 int_tx_msi_doorbell_x16
										 //
		writel(0x60000001, dbi_base + 0x940);   //MSIX_ADDRESS_MATCH_LOW_OFF  doorbell 只有32位，高位截断
		writel(0x0, dbi_base + 0x944);              //MSIX_ADDRESS_MATCH_EN  addr = 0x81_c000_000
	}else if(pcie->dev->max_lanes == 8){
		// writel(0x40000, ss_base + 0x114);    // int_mbi_message_for_vector_00
		writel(0x40001, ss_base + 0x118);    // int_mbi_message_for_vector_01
		writel(0x40002, ss_base + 0x11c);    // int_mbi_message_for_vector_02
		writel(0x40003, ss_base + 0x120);    // int_mbi_message_for_vector_03
		writel(0x40004, ss_base + 0x124);    // int_mbi_message_for_vector_04
		writel(0x40005, ss_base + 0x128);    // int_mbi_message_for_vector_05
		writel(0x40006, ss_base + 0x12c);    // int_mbi_message_for_vector_06
		writel(0x40007, ss_base + 0x130);    // int_mbi_message_for_vector_07
		writel(0x40008, ss_base + 0x134);    // int_mbi_message_for_vector_08
		writel(0x40009, ss_base + 0x138);    // int_mbi_message_for_vector_09
		writel(0x4000a, ss_base + 0x13c);    // int_mbi_message_for_vector_10
		writel(0x4000b, ss_base + 0x140);    // int_mbi_message_for_vector_11
		writel(0x4000c, ss_base + 0x144);    // int_mbi_message_for_vector_12
		writel(0x4000d, ss_base + 0x148);    // int_mbi_message_for_vector_13
		writel(0x4000e, ss_base + 0x14c);    // int_mbi_message_for_vector_14
		writel(0x4000f, ss_base + 0x150);    // int_mbi_message_for_vector_15
		writel(0x40010, ss_base + 0x154);    // int_mbi_message_for_vector_16
		writel(0x40011, ss_base + 0x158);    // int_mbi_message_for_vector_17
		writel(0x40012, ss_base + 0x15c);    // int_mbi_message_for_vector_18
		writel(0x40013, ss_base + 0x160);    // int_mbi_message_for_vector_19
		writel(0x40014, ss_base + 0x164);    // int_mbi_message_for_vector_20
		writel(0x40015, ss_base + 0x168);    // int_mbi_message_for_vector_21
		writel(0x40016, ss_base + 0x16c);    // int_mbi_message_for_vector_22
		writel(0x40017, ss_base + 0x170);    // int_mbi_message_for_vector_23
		writel(0x40018, ss_base + 0x174);    // int_mbi_message_for_vector_24
		writel(0x40019, ss_base + 0x178);    // int_mbi_message_for_vector_25
		writel(0x4001a, ss_base + 0x17c);    // int_mbi_message_for_vector_26
		writel(0x4001b, ss_base + 0x180);    // int_mbi_message_for_vector_27
		writel(0x4001c, ss_base + 0x184);    // int_mbi_message_for_vector_28
		writel(0x4001d, ss_base + 0x188);    // int_mbi_message_for_vector_29
		writel(0x4001e, ss_base + 0x18c);    // int_mbi_message_for_vector_30
		writel(0, ss_base + 0x190);    // int_mbi_message_for_vector_31

		writel(0x1 << 1, ss_base + 0x194);    // pcie_x8 outbound bit1
		writel(0x7, ss_base + 0x1b0);    // pcie_x16 int_tx_msi_doorbell_x8
										 //
		writel(0x70000001, dbi_base + 0x940);   //MSIX_ADDRESS_MATCH_LOW_OFF  doorbell 只有32位，高位截断
		writel(0x0, dbi_base + 0x944);              //MSIX_ADDRESS_MATCH_EN  addr = 0x81_c000_000
	}else{
		printf("msi config error !!!\n");
	}

	if(g_c2c_base == C2C_SYS_CFG_02){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0x60000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0x70000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}else if(g_c2c_base == C2C_SYS_CFG_03){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0xe0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0xf0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}else if(g_c2c_base == C2C_SYS_CFG_72){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0x60000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0x70000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}else if(g_c2c_base == C2C_SYS_CFG_73){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0xe0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0xf0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}else{
		printf("msi config error !!!\n");
	}
	writel(0xfffffffe, mbitx_ap_base + 0x30);    //时能对应bit中断，总共32个bit
	writel(0x0, mbitx_ap_base + 0x40);    //时能对应bit目标remote|local，总共32个bit

	// writel(0x0, dbi_base + 0x948);              //0:10 vector

#else

	if(pcie->dev->max_lanes == 16){
		// writel(0x40000, ss_base + 0x94);    // int_mbi_message_for_vector_00
#if SEEHI_TILE14_PCIE_TEST
		writel(0x00140020, ss_base + 0x98);    // int_mbi_message_for_vector_01
#elif SEEHI_4TILE_PCIE_TEST
		writel(0x00240020, ss_base + 0x98);    // int_mbi_message_for_vector_01
#else
		writel(0x40001, ss_base + 0x98);    // int_mbi_message_for_vector_01
#endif
		writel(0x40002, ss_base + 0x9c);    // int_mbi_message_for_vector_02
		writel(0x40003, ss_base + 0xa0);    // int_mbi_message_for_vector_03
		writel(0x40004, ss_base + 0xa4);    // int_mbi_message_for_vector_04
		writel(0x40005, ss_base + 0xa8);    // int_mbi_message_for_vector_05
		writel(0x40006, ss_base + 0xac);    // int_mbi_message_for_vector_06
		writel(0x40007, ss_base + 0xb0);    // int_mbi_message_for_vector_07
		writel(0x40008, ss_base + 0xb4);    // int_mbi_message_for_vector_08
		writel(0x40009, ss_base + 0xb8);    // int_mbi_message_for_vector_09
		writel(0x4000a, ss_base + 0xbc);    // int_mbi_message_for_vector_10
		writel(0x4000b, ss_base + 0xc0);    // int_mbi_message_for_vector_11
		writel(0x4000c, ss_base + 0xc4);    // int_mbi_message_for_vector_12
		writel(0x4000d, ss_base + 0xc8);    // int_mbi_message_for_vector_13
		writel(0x4000e, ss_base + 0xcc);    // int_mbi_message_for_vector_14
		writel(0x4000f, ss_base + 0xd0);    // int_mbi_message_for_vector_15
		writel(0x40010, ss_base + 0xd4);    // int_mbi_message_for_vector_16
		writel(0x40011, ss_base + 0xd8);    // int_mbi_message_for_vector_17
		writel(0x40012, ss_base + 0xdc);    // int_mbi_message_for_vector_18
		writel(0x40013, ss_base + 0xe0);    // int_mbi_message_for_vector_19
		writel(0x40014, ss_base + 0xe4);    // int_mbi_message_for_vector_20
		writel(0x40015, ss_base + 0xe8);    // int_mbi_message_for_vector_21
		writel(0x40016, ss_base + 0xec);    // int_mbi_message_for_vector_22
		writel(0x40017, ss_base + 0xf0);    // int_mbi_message_for_vector_23
		writel(0x40018, ss_base + 0xf4);    // int_mbi_message_for_vector_24
		writel(0x40019, ss_base + 0xf8);    // int_mbi_message_for_vector_25
		writel(0x4001a, ss_base + 0xfc);    // int_mbi_message_for_vector_26
		writel(0x4001b, ss_base + 0x100);    // int_mbi_message_for_vector_27
		writel(0x4001c, ss_base + 0x104);    // int_mbi_message_for_vector_28
		writel(0x4001d, ss_base + 0x108);    // int_mbi_message_for_vector_29
		writel(0x4001e, ss_base + 0x10c);    // int_mbi_message_for_vector_30
		writel(0, ss_base + 0x110);    // int_mbi_message_for_vector_31

		writel(0x0 << 0, ss_base + 0x194);    // pcie_x16 outbound bit0
		writel(0x40000000, ss_base + 0x198);    // pcie_x16 int_tx_msi_doorbell_x16
	}else if(pcie->dev->max_lanes == 8){
		// writel(0x40000, ss_base + 0x114);    // int_mbi_message_for_vector_00
		writel(0x40001, ss_base + 0x118);    // int_mbi_message_for_vector_01
		writel(0x40002, ss_base + 0x11c);    // int_mbi_message_for_vector_02
		writel(0x40003, ss_base + 0x120);    // int_mbi_message_for_vector_03
		writel(0x40004, ss_base + 0x124);    // int_mbi_message_for_vector_04
		writel(0x40005, ss_base + 0x128);    // int_mbi_message_for_vector_05
		writel(0x40006, ss_base + 0x12c);    // int_mbi_message_for_vector_06
		writel(0x40007, ss_base + 0x130);    // int_mbi_message_for_vector_07
		writel(0x40008, ss_base + 0x134);    // int_mbi_message_for_vector_08
		writel(0x40009, ss_base + 0x138);    // int_mbi_message_for_vector_09
		writel(0x4000a, ss_base + 0x13c);    // int_mbi_message_for_vector_10
		writel(0x4000b, ss_base + 0x140);    // int_mbi_message_for_vector_11
		writel(0x4000c, ss_base + 0x144);    // int_mbi_message_for_vector_12
		writel(0x4000d, ss_base + 0x148);    // int_mbi_message_for_vector_13
		writel(0x4000e, ss_base + 0x14c);    // int_mbi_message_for_vector_14
		writel(0x4000f, ss_base + 0x150);    // int_mbi_message_for_vector_15
		writel(0x40010, ss_base + 0x154);    // int_mbi_message_for_vector_16
		writel(0x40011, ss_base + 0x158);    // int_mbi_message_for_vector_17
		writel(0x40012, ss_base + 0x15c);    // int_mbi_message_for_vector_18
		writel(0x40013, ss_base + 0x160);    // int_mbi_message_for_vector_19
		writel(0x40014, ss_base + 0x164);    // int_mbi_message_for_vector_20
		writel(0x40015, ss_base + 0x168);    // int_mbi_message_for_vector_21
		writel(0x40016, ss_base + 0x16c);    // int_mbi_message_for_vector_22
		writel(0x40017, ss_base + 0x170);    // int_mbi_message_for_vector_23
		writel(0x40018, ss_base + 0x174);    // int_mbi_message_for_vector_24
		writel(0x40019, ss_base + 0x178);    // int_mbi_message_for_vector_25
		writel(0x4001a, ss_base + 0x17c);    // int_mbi_message_for_vector_26
		writel(0x4001b, ss_base + 0x180);    // int_mbi_message_for_vector_27
		writel(0x4001c, ss_base + 0x184);    // int_mbi_message_for_vector_28
		writel(0x4001d, ss_base + 0x188);    // int_mbi_message_for_vector_29
		writel(0x4001e, ss_base + 0x18c);    // int_mbi_message_for_vector_30
		writel(0, ss_base + 0x190);    // int_mbi_message_for_vector_31

		writel(0x0 << 1, ss_base + 0x194);    // pcie_x8 outbound bit1
		writel(0x50000000, ss_base + 0x1a8);    // pcie_x16 int_tx_msi_doorbell_x8
	}else{
		printf("msi config error !!!\n");
	}

#if SEEHI_TILE14_PCIE_TEST
	if(pcie->dev->max_lanes == 16){
		writel(0x40000000, mbitx_14tile_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
	}else if(pcie->dev->max_lanes == 8){
		writel(0x50000000, mbitx_14tile_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
	}else{
		printf("msi config error !!!\n");
	}

	writel(0x03, mbitx_14tile_base + 0x14);
	// writel(0x7fffffff, mbitx_14tile_base + 0x30);    //时能对应bit中断，总共32个bit
	writel(0x1, mbitx_14tile_base + 0x34);    //时能对应bit中断，总共32个bit

	// writel(0x0, mbitx_14tile_base + 0x40);    //时能对应bit目标remote|local，总共32个bit
#elif SEEHI_4TILE_PCIE_TEST
	if(pcie->dev->max_lanes == 16){
		writel(0x40000000, mbitx_24tile_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
	}else if(pcie->dev->max_lanes == 8){
		writel(0x50000000, mbitx_24tile_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
	}else{
		printf("msi config error !!!\n");
	}

	writel(0x03, mbitx_24tile_base + 0x14);
	// writel(0x7fffffff, mbitx_14tile_base + 0x30);    //时能对应bit中断，总共32个bit
	writel(0x1, mbitx_24tile_base + 0x34);    //时能对应bit中断，总共32个bit

	// writel(0x0, mbitx_24tile_base + 0x40);    //时能对应bit目标remote|local，总共32个bit
#else
	// writel((0 << 4), apb_base + 0x70);    //4:8  产生msi对应中断 bit0=1

	if(g_c2c_base == C2C_SYS_CFG_02){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0x40000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0x50000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}else if(g_c2c_base == C2C_SYS_CFG_03){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0xc0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0xd0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0x81, mbitx_ap_base + 0x14);
	}else if(g_c2c_base == C2C_SYS_CFG_72){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0x40000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0x50000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}else if(g_c2c_base == C2C_SYS_CFG_73){
		//配置AP SYS MBI_TX 0x1005_0000
		if(pcie->dev->max_lanes == 16){
			writel(0xc0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x16
		}else if(pcie->dev->max_lanes == 8){
			writel(0xd0000000, mbitx_ap_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
		}else{
			printf("msi config error !!!\n");
		}

		writel(0xb9, mbitx_ap_base + 0x14);
	}else{
		printf("msi config error !!!\n");
	}

	writel(0xfffffffe, mbitx_ap_base + 0x30);    //时能对应bit中断，总共32个bit
	writel(0x0, mbitx_ap_base + 0x40);    //时能对应bit目标remote|local，总共32个bit
#endif //SEEHI_TILE14_PCIE_TEST

#endif //SEEHI_MSIX_ENABLE
	   /////////////////////////////////////END//////////////////////////////////////////////////////
	dw_pcie_dbi_ro_wr_dis(dbi_base);

	pcie_writel_apb(pcie, 0x1ff, A510_APB_PCIE_EN_INT0);
	pcie_writel_apb(pcie, 0x1, A510_APB_PCIE_MSG_VDM_RX_MODE);

	return HAL_OK;
}

void LPI_8193_Handler(void)
{
	struct HAL_PCIE_HANDLE *pcie;

	if(g_c2c_base == C2C_SYS_CFG_02){
		pcie = &s_pcie_02;
	}else if(g_c2c_base == C2C_SYS_CFG_03){
		pcie = &s_pcie_03;
	}else if(g_c2c_base == C2C_SYS_CFG_72){
		pcie = &s_pcie_72;
	}else if(g_c2c_base == C2C_SYS_CFG_73){
		pcie = &s_pcie_73;
	}else{
		pcie = NULL;
		printf("pcie_irq_handler error !!!\n");
		return;
	}

	uint64_t engine_base = pcie->dev->engineBase;

	writel(0xff, engine_base + 0x48);		// clear status
											//
	printf("LPI 8193 received !!!!\n");
}

#if SEEHI_PLD_PCIE_TEST && SEEHI_C2C_ENGINE_TEST
static void c2c_engine_mbi_tx(struct HAL_PCIE_HANDLE *pcie, int id, int local)
{
	uint64_t mbi_tx_base = pcie->dev->mbitxBase;
	uint32_t val;

	writel(((MBI_RX_BASE+0x40) & 0xFFFFFFFF), mbi_tx_base + 0x18);
	writel(0x04, mbi_tx_base + 0x1c);

	val = readl(mbi_tx_base + 0x30);
	val |= id;
	writel(val, mbi_tx_base + 0x30);

	val = readl(mbi_tx_base + 0x40);
	val |= local;
	writel(val, mbi_tx_base + 0x40);
}

static void c2c_engine_mbi_tx_dump(struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t mbi_tx_base = pcie->dev->mbitxBase;

	printf("seehi--> %s line: %d mbi_tx_base 0x%lx\n", __func__, __LINE__, mbi_tx_base);

	printf("mbi_tx_base + 0x00 0x%x\n", readl(mbi_tx_base + 0x00));
	printf("mbi_tx_base + 0x04 0x%x\n", readl(mbi_tx_base + 0x04));

	printf("mbi_tx_base + 0x10 0x%x\n", readl(mbi_tx_base + 0x10));
	printf("mbi_tx_base + 0x14 0x%x\n", readl(mbi_tx_base + 0x14));
	printf("mbi_tx_base + 0x18 0x%x\n", readl(mbi_tx_base + 0x18));
	printf("mbi_tx_base + 0x1c 0x%x\n", readl(mbi_tx_base + 0x1c));

	printf("mbi_tx_base + 0x30 0x%x\n", readl(mbi_tx_base + 0x30));
	printf("mbi_tx_base + 0x34 0x%x\n", readl(mbi_tx_base + 0x34));
	printf("mbi_tx_base + 0x40 0x%x\n", readl(mbi_tx_base + 0x40));
	printf("mbi_tx_base + 0x44 0x%x\n", readl(mbi_tx_base + 0x44));
}

static void dsc_table_init(struct HAL_PCIE_HANDLE *pcie, uint32_t tcm_addr_h, uint32_t tcm_addr_l, uint32_t size)
{
	uint64_t tcm_addr = (uint64_t)tcm_addr_h << 32 | tcm_addr_l;
	c2c_engine_desc_typedef *desc = (c2c_engine_desc_typedef *)tcm_addr;
	printf("seehi--> %s line: %d desc_addr 0x%lx\n", __func__, __LINE__, tcm_addr);

	desc->cfg_c2c_engine.op = 0x2;  // 0 max  1 min 2 sum
	desc->cfg_c2c_engine.intr_en = 0x1;
	desc->cfg_c2c_engine.cnt_en = 0x1;
	desc->cfg_c2c_engine.length_size = 0xf;

	desc->cfg_c2c_engine_addr_h.src_addr_h = 0x4;
	desc->cfg_c2c_engine_addr_h.dst_addr_h = 0x4;

	desc->cfg_c2c_engine_src_addr_l = 0x40060000;
	desc->cfg_c2c_engine_dst_addr_l = 0x40070000;  // host addr
	printf("seehi--> %s line: %d tcm_addr 0x%x\n", __func__, __LINE__, 0x40060000);
	printf("seehi--> %s line: %d host_addr 0x%x\n", __func__, __LINE__, 0x40070000);

	desc++;

	desc->cfg_c2c_engine.op = 0x2;  // 0 max  1 min 2 sum
	desc->cfg_c2c_engine.intr_en = 0x1;
	desc->cfg_c2c_engine.cnt_en = 0x1;
	desc->cfg_c2c_engine.last_en = 0x1;
	desc->cfg_c2c_engine.length_size = 0xf;

	desc->cfg_c2c_engine_addr_h.src_addr_h = 0x4;
	desc->cfg_c2c_engine_addr_h.dst_addr_h = 0x4;

	desc->cfg_c2c_engine_src_addr_l = 0x40061000;
	desc->cfg_c2c_engine_dst_addr_l = 0x40071000;  // host addr
	printf("seehi--> %s line: %d tcm_addr 0x%x\n", __func__, __LINE__, 0x40061000);
	printf("seehi--> %s line: %d host_addr 0x%x\n", __func__, __LINE__, 0x40071000);
}

static void tcm_data_init(struct HAL_PCIE_HANDLE *pcie, uint32_t tcm_addr_h, uint32_t tcm_addr_l, uint32_t size)
{
	uint32_t i;
	uint64_t tcm_addr = (uint64_t)tcm_addr_h << 32 | tcm_addr_l;

	for(i = 0; i <= size; i++){
		writel(0x40200000, tcm_addr + i * 4);  // src data  0x40200000  dst data 0x00002040  global 0x40a00000
	}
}

static void prefetch_dscpt_eng(struct HAL_PCIE_HANDLE *pcie, uint32_t addr_h, uint32_t addr_l)
{
	uint64_t engine_base = pcie->dev->engineBase;

	writel(addr_l, engine_base + 0x0c);		// 软件配置‘ARD LL’在TCM中的起始位置
	writel(addr_h, engine_base + 0x10);		// 软件配置‘ARD LL’在TCM中的起始位置
	writel(0x1, engine_base + 0x08);		// 软件配置‘ARD LL’ doorbell，当该doorbell拉高后，可以读取‘ARD LL’中descriptor
}

static void c2c_engine_init(struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t engine_base = pcie->dev->engineBase;

	writel(0x0, engine_base + 0x04);		// no bypass
	writel(0x1, engine_base + 0x6c);		// local  engine_cnt
	writel(0x1, engine_base + 0x60);		// engine_sync cnt
	writel(0x4f000000, engine_base + 0x64); // engine_sync addr,c2c sync_engine寄存器地址
	writel(0x4, engine_base + 0x68);		// engine_sync addr,c2c sync_engine寄存器地址
	writel(0x2, engine_base + 0x14);		// FP32
	writel(0x1, engine_base + 0x40);		// irq enable
	writel(0x0, engine_base + 0x44);		// no mask
}

static void c2c_engine_dump(struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t engine_base = pcie->dev->engineBase;
	printf("seehi--> %s line: %d engine_base 0x%lx\n", __func__, __LINE__, engine_base);

	printf("engine_base + 0x04 0x%x\n", readl(engine_base + 0x04));
	printf("engine_base + 0x6c 0x%x\n", readl(engine_base + 0x6c));
	printf("engine_base + 0x60 0x%x\n", readl(engine_base + 0x60));
	printf("engine_base + 0x64 0x%x\n", readl(engine_base + 0x64));
	printf("engine_base + 0x68 0x%x\n", readl(engine_base + 0x68));
	printf("engine_base + 0x14 0x%x\n", readl(engine_base + 0x14));
	printf("engine_base + 0x40 0x%x\n", readl(engine_base + 0x40));
	printf("engine_base + 0x44 0x%x\n", readl(engine_base + 0x44));
	printf("engine_base + 0x0c 0x%x\n", readl(engine_base + 0x0c));
	printf("engine_base + 0x10 0x%x\n", readl(engine_base + 0x10));
	printf("engine_base + 0x08 0x%x\n", readl(engine_base + 0x08));
}

static void c2c_engine_dump_addr(struct HAL_PCIE_HANDLE *pcie, uint32_t ddr)
{
	uint32_t i;

	printf("ddr addr : 0x%08x\n", ddr);
	for(i = 0; i < 8; i++){
		if(i != 0 && i % 4 == 0)
			printf("\n");
		printf("0x%08x ", readl(ddr + i * 4));
	}
	printf("\n");
}
#endif

struct HAL_PCIE_DEV g_pcieDevX16_03 =
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
	.firstBusNo = 0x20,
	.ltrIrqNum = 172,
	.vdmIrqNum = 173,
	.bif_en = 0,
	.pipe8 = 0,
};

struct HAL_PCIE_DEV g_pcieDevX16_02 =
{
	.apbBase = PCIE_X16_REG_02,
	.engineBase = C2C_ENGINE_X16_02,
	.dbiBase = DWC_PCIE_CTL_X16_DBI_02,
	.phyBase = PCIE_PHY_REG_02,
	.ssBase = C2C_SS_REG_02,
	.drouterBase = DROUTER_02,
	.crouterBase = CROUTER_02,
	.dniuBase = DNIU_02,
	.cniuBase = CNIU_02,
	.mbitxBase = MBI_TX_02,
	.max_lanes = 16,
	.lanes = 16,
	.gen = 5,
	.firstBusNo = 0x0,
	.ltrIrqNum = 108,
	.vdmIrqNum = 109,
	.bif_en = 0,
	.pipe8 = 0,
};

struct HAL_PCIE_DEV g_pcieDevX16_73 =
{
	.apbBase = PCIE_X16_REG_73,
	.engineBase = C2C_ENGINE_X16_73,
	.dbiBase = DWC_PCIE_CTL_X16_DBI_73,
	.phyBase = PCIE_PHY_REG_73,
	.ssBase = C2C_SS_REG_73,
	.drouterBase = DROUTER_73,
	.crouterBase = CROUTER_73,
	.dniuBase = DNIU_73,
	.cniuBase = CNIU_73,
	.mbitxBase = MBI_TX_73,
	.max_lanes = 16,
	.lanes = 16,
	.gen = 5,
	.firstBusNo = 0x60,
	.ltrIrqNum = 300,
	.vdmIrqNum = 301,
	.bif_en = 0,
	.pipe8 = 0,
};

struct HAL_PCIE_DEV g_pcieDevX8_73 =
{
	.apbBase = PCIE_X8_REG_73,
	.engineBase = C2C_ENGINE_X8_73,
	.dbiBase = DWC_PCIE_CTL_X8_DBI_73,
	.phyBase = PCIE_PHY_REG_73,
	.ssBase = C2C_SS_REG_73,
	.drouterBase = DROUTER_73,
	.crouterBase = CROUTER_73,
	.dniuBase = DNIU_73,
	.cniuBase = CNIU_73,
	.mbitxBase = MBI_TX_73,
	.max_lanes = 8,
	.lanes = 8,
	.gen = 5,
	.firstBusNo = 0x70,
	.ltrIrqNum = 332,
	.vdmIrqNum = 333,
	.bif_en = 1,
	.pipe8 = 8,
};

struct HAL_PCIE_DEV g_pcieDevX16toX8_03 =
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
	.lanes = 8,
	.gen = 5,
	.firstBusNo = 0x20,
	.ltrIrqNum = 172,
	.vdmIrqNum = 173,
	.bif_en = 1,
	.pipe8 = 8,
};

struct HAL_PCIE_DEV g_pcieDevX8_03 =
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
#if SEEHI_FPGA_PCIE_TEST
	.lanes = 1,
	.gen = 1,
#elif SEEHI_PLD_PCIE_TEST
	.lanes = 8,
	.gen = 5,
#endif
	.firstBusNo = 0x30,
	.ltrIrqNum = 204,
	.vdmIrqNum = 205,
	.bif_en = 1,
	.pipe8 = 8,
};

static void init_g_pcie(struct HAL_PCIE_HANDLE *pcie, char tile, char mode, char lanes, char gen)
{
	struct HAL_PCIE_DEV *dev = pcie->dev;
	g_c2c_base = get_pcie_base(tile);

	uint64_t dwc_pcie_ctl_x16_dbi = (g_c2c_base + 0x0);
	uint64_t c2c_engine_x16		  = (g_c2c_base + 0x140000);
	uint64_t pcie_x16_reg		  = (g_c2c_base + 0x180000);
	uint64_t dwc_pcie_ctl_x8_dbi  = (g_c2c_base + 0x200000);
	uint64_t c2c_engine_x8		  = (g_c2c_base + 0x340000);
	uint64_t pcie_x8_reg		  = (g_c2c_base + 0x380000);
	uint64_t pcie_phy_reg		  = (g_c2c_base + 0x400000);
	uint64_t dwc_pcie5_phy0_cri	  = (g_c2c_base + 0x480000);
	uint64_t dwc_pcie5_phy1_cri	  = (g_c2c_base + 0x4a0000);
	uint64_t dwc_pcie5_phy2_cri	  = (g_c2c_base + 0x4c0000);
	uint64_t dwc_pcie5_phy3_cri	  = (g_c2c_base + 0x4e0000);
	uint64_t c2c_ss_reg			  = (g_c2c_base + 0x580000);
	uint64_t drouter			  = (g_c2c_base + 0x581000);
	uint64_t crouter			  = (g_c2c_base + 0x582000);
	uint64_t dniu				  = (g_c2c_base + 0x583000);
	uint64_t cniu				  = (g_c2c_base + 0x584000);
	uint64_t mbi_tx				  = (g_c2c_base + 0x585000);

	if(mode == 16){
		dev->apbBase = pcie_x16_reg;
		dev->engineBase = c2c_engine_x16;
		dev->dbiBase = dwc_pcie_ctl_x8_dbi;
		dev->max_lanes = 16;

		if(tile == 2){
			dev->firstBusNo = 0x0;
			dev->ltrIrqNum = 140;
			dev->vdmIrqNum = 141;
		}else if(tile == 3){
			dev->firstBusNo = 0x20;
			dev->ltrIrqNum = 172;
			dev->vdmIrqNum = 173;
		}else if(tile == 72){
		}else if(tile == 73){
		}
	}else if(mode == 8){
		dev->apbBase = pcie_x8_reg;
		dev->engineBase = c2c_engine_x8;
		dev->dbiBase = dwc_pcie_ctl_x8_dbi;
		dev->max_lanes = 8;

		if(tile == 2){
			dev->firstBusNo = 0x0;
			dev->ltrIrqNum = 172;
			dev->vdmIrqNum = 172;
		}else if(tile == 3){
			dev->firstBusNo = 0x20;
			dev->ltrIrqNum = 204;
			dev->vdmIrqNum = 205;
		}else if(tile == 72){
		}else if(tile == 73){
		}
	}

	dev->phyBase = pcie_phy_reg;
	dev->ssBase = c2c_ss_reg;
	dev->drouterBase = drouter;
	dev->crouterBase = crouter;
	dev->dniuBase = dniu;
	dev->cniuBase = cniu;
	dev->mbitxBase = mbi_tx;

	dev->lanes = lanes;
	dev->gen = gen;
}

int main()
{
	int ret;
	uint32_t result = HAL_ERROR;
	uint32_t cnt = 0;
	struct HAL_PCIE_HANDLE *pcie = &s_pcie;
	printf("chip%d die%d %s mode\n",
		D2D_C2C_CHIP, CONFIG_RHEA_D2D_SELF_ID,
		D2D_C2C_CHIP ? "ep" : "rc");
	rhea_clci_clk_init();

	g_c2c_base = 0;
	g_c2c_link = 0;

#if SEEHI_FPGA_PCIE_TEST
	s_pcie.dev = &g_pcieDevX8_03;

	g_c2c_base = get_pcie_base(3);
	pcie = &s_pcie_03;
#if SEEHI_C2C_X16_TEST
	s_pcie_03.dev = &g_pcieDevX16_03;
#elif SEEHI_C2C_X8_TEST
	s_pcie_03.dev = &g_pcieDevX8_03;
#endif

	gpio_sync_init();
#elif SEEHI_PLD_PCIE_TEST
	mc_init(TCM_04_CFG_BASE, 4);
#if SEEHI_AP_PCIE_TEST
#elif SEEHI_TILE14_PCIE_TEST
	mc_init(TCM_14_CFG_BASE, 4);
#elif SEEHI_4TILE_PCIE_TEST
	mc_init(TCM_14_CFG_BASE, 4);
	mc_init(TCM_15_CFG_BASE, 4);
	mc_init(TCM_24_CFG_BASE, 4);
	mc_init(TCM_25_CFG_BASE, 4);
#elif SEEHI_NPU_PCIE_TEST
	mc_init(TCM_53_CFG_BASE, 4);
	mc_init(TCM_54_CFG_BASE, 4);
	mc_init(TCM_63_CFG_BASE, 4);
	mc_init(TCM_64_CFG_BASE, 4);
#else
#error
#endif

#if SEEHI_C2C_PCIE_TEST
#if SEEHI_DUAL_PCIE_TEST
	g_c2c_base = get_pcie_base(03);
	pcie = &s_pcie_03;
#if SEEHI_C2C_X16_TEST
	s_pcie_03.dev = &g_pcieDevX16_03;
#elif SEEHI_C2C_X8_TEST
	s_pcie_03.dev = &g_pcieDevX8_03;
#endif

#elif SEEHI_SINGLE_PCIE_TEST
#if PLD_Z1
	g_c2c_base = get_pcie_base(03);
	pcie = &s_pcie_03;
#if SEEHI_C2C_X16_TEST
	s_pcie_03.dev = &g_pcieDevX16_03;
	// s_pcie_03.dev = &g_pcieDevX16toX8_03;
#elif SEEHI_C2C_X8_TEST
	s_pcie_03.dev = &g_pcieDevX8_03;
#endif
#elif PLD_Z2
	g_c2c_base = get_pcie_base(73);
	pcie = &s_pcie_73;
#if SEEHI_C2C_X16_TEST
	s_pcie_73.dev = &g_pcieDevX16_73;
#elif SEEHI_C2C_X8_TEST
	s_pcie_73.dev = &g_pcieDevX8_73;
#endif
#else
#error
#endif
#else
#error
#endif  //SEEHI_DUAL_PCIE_TEST
#else
	g_c2c_base = get_pcie_base(03);
	pcie = &s_pcie_03;
#if SEEHI_C2C_X16_TEST
	s_pcie_03.dev = &g_pcieDevX16_03;
#elif SEEHI_C2C_X8_TEST
	s_pcie_03.dev = &g_pcieDevX8_03;
#endif
#endif  //SEEHI_C2C_PCIE_TEST
#else
#error
#endif //SEEHI_FPGA_PCIE_TEST

	systimer_init();

	// GIC_Init();
	init_gic();
#if SEEHI_PLD_PCIE_TEST && SEEHI_C2C_ENGINE_TEST
	lpi_init();
#endif

#if SEEHI_C2C_PCIE_TEST
	IRQ_SetHandler(pcie->dev->vdmIrqNum, pcie_irq_handler);
	GIC_SetPriority(pcie->dev->vdmIrqNum, 0 << 3);
	GIC_EnableIRQ(pcie->dev->vdmIrqNum);
#endif

	printf("PCIe_EP_Init start !!!\n");

#if SEEHI_4TILE_PCIE_TEST
	/* when print start, please power on x86 pc */
	printf("t14:0x%08x\n", REG32(0x1440000000 + 536870912 + 0xc0));
	printf("t15:0x%08x\n", REG32(0x1540000000 + 536870912 + 0xc0));
	printf("t24:0x%08x\n", REG32(0x2440000000 + 536870912 + 0xc0));
	printf("t25:0x%08x\n", REG32(0x2540000000 + 536870912 + 0xc0));
#endif

#if SEEHI_NPU_PCIE_TEST
	/* when print start, please power on x86 pc */
	printf("t53:0x%08x\n", REG32(0x5340000000 + 536870912 + 0xc0));
	printf("t54:0x%08x\n", REG32(0x5440000000 + 536870912 + 0xc0));
	printf("t63:0x%08x\n", REG32(0x6340000000 + 536870912 + 0xc0));
	printf("t64:0x%08x\n", REG32(0x6440000000 + 536870912 + 0xc0));
#endif

#if  SEEHI_PLD_PCIE_TEST && SEEHI_C2C_PCIE_TEST && SEEHI_DUAL_PCIE_TEST
	g_c2c_link = 0;
	PCIe_EP_Init(pcie);
	PCIe_EP_Link(pcie);

	g_c2c_base = get_pcie_base(73);
	pcie = &s_pcie_73;
#if SEEHI_C2C_X16_TEST
	s_pcie_73.dev = &g_pcieDevX16_73;
#elif SEEHI_C2C_X8_TEST
	s_pcie_73.dev = &g_pcieDevX8_73;
#endif

	IRQ_SetHandler(pcie->dev->vdmIrqNum, pcie_irq_handler);
	GIC_SetPriority(pcie->dev->vdmIrqNum, 0 << 3);
	GIC_EnableIRQ(pcie->dev->vdmIrqNum);

	printf("seehi_dual_pcie_test c2c\n");
#endif

#if SEEHI_C2C_PCIE_TEST
	g_c2c_link = 1;
	gpio_sync_init();
#endif

	PCIe_EP_Init(pcie);
	PCIe_EP_Link(pcie);

	printf("PCIe_EP_Init end !!!\n");

	// register_lpi(4, 0, 8192);
	// IRQ_SetHandler(8192, LPI_8192_Handler);
	// dw_timer_mbi_tx(0x1);

#if SEEHI_PLD_PCIE_TEST && SEEHI_C2C_ENGINE_TEST
#if SEEHI_C2C_X16_TEST
	register_lpi(0x3, 0x11, 8193);
#else
	register_lpi(0x3, 0xc, 8193);
#endif
	IRQ_SetHandler(8193, LPI_8193_Handler);

	c2c_engine_mbi_tx(pcie, 0x21 << 12, 0x21 << 12);
	c2c_engine_init(pcie);

	dsc_table_init(pcie, 0x00, 0x40051000, 2);  // desc addr
	tcm_data_init(pcie, 0x00, 0x40060000, 0xf);  // src addr
	tcm_data_init(pcie, 0x00, 0x40061000, 0xf);  // src addr
	prefetch_dscpt_eng(pcie, 0x04, 0x40051000);  // start
#endif

	ret = rhea_d2d_init();
    if (ret) return ret;

    ret = rhea_d2d_sync_init();
    if (ret) return ret;

#if	SEEHI_PLD_PCIE_TEST
	REG32(0x12000fe0)=0x4;
#endif

	while(1) {
        ret = d2d_sync_obtain_cmd();
        if (ret < 0) 
			printf("Obtain cmd error (%d).\n", ret);
	};

	return result;
}
