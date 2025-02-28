#ifndef __PCIE_H__
#define __PCIE_H__

#include <stdint.h>
#include "memmap.h"

/***************************** MACRO Definition ******************************/
/** @defgroup PCIE_Exported_Definition_Group1 Basic Definition
 *  @{
 */

//either-or
#define SEEHI_PLD_PCIE_TEST			1
#define SEEHI_FPGA_PCIE_TEST		0

#define SEEHI_C2C_PCIE_TEST			1

#define SEEHI_NPU_PCIE_TEST			0

#define SEEHI_MSIX_ENABLE			0

#define SEEHI_RC_TEST_HOT_RESET		1

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
#define BOOT_USING_PCIE_C2C_BAR2_CPU_ADDRESS 0x00440000000   //ap tcm
#define BOOT_USING_PCIE_C2C_BAR3_CPU_ADDRESS 0x00400000000   //ap bootrom
#define BOOT_USING_PCIE_C2C_BAR4_CPU_ADDRESS 0x00440000000   //ap tcm
															 //
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x11430000000   //tile 14 npu.s2
#define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x11420000000   //tile 14 tile cfg
#define BOOT_USING_PCIE_EP_BAR3_CPU_ADDRESS 0x00457ff0000   //HDMA Link List
#define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x00440000000   //tile 04 tcm mem

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

#define PCIE_C2C_CONFIG_TARGET_02_x16		0x01000000
#define PCIE_C2C_CONFIG_BASE_02_x16			0x16000000
#define PCIE_C2C_CONFIG_BASE_SIZE_02_x16	0x00080000
#define PCIE_C2C_IO_BASE_02_x16				0x16080000
#define PCIE_C2C_IO_BASE_SIZE_02_x16		0x00080000
#define PCIE_C2C_32_MEM_BASE_02_x16			0x16100000
#define PCIE_C2C_32_MEM_BASE_SIZE_02_x16	0x00f00000
#define PCIE_C2C_64_MEM_BASE_02_x16			0xc100000000
#define PCIE_C2C_64_MEM_BASE_SIZE_02_x16	0x700000000
#define PCIE_C2C_CONFIG_TARGET_02_x8		0x11000000
#define PCIE_C2C_CONFIG_BASE_02_x8			0x17000000
#define PCIE_C2C_CONFIG_BASE_SIZE_02_x8		0x00080000
#define PCIE_C2C_IO_BASE_02_x8				0x17080000
#define PCIE_C2C_IO_BASE_SIZE_02_x8			0x00080000
#define PCIE_C2C_32_MEM_BASE_02_x8			0x17100000
#define PCIE_C2C_32_MEM_BASE_SIZE_02_x8		0x00f00000
#define PCIE_C2C_64_MEM_BASE_02_x8			0xc900000000
#define PCIE_C2C_64_MEM_BASE_SIZE_02_x8		0x700000000

#define PCIE_C2C_CONFIG_TARGET_03_x16		0x21000000
#define PCIE_C2C_CONFIG_BASE_03_x16			0x18000000
#define PCIE_C2C_CONFIG_BASE_SIZE_03_x16	0x00080000
#define PCIE_C2C_IO_BASE_03_x16				0x18080000
#define PCIE_C2C_IO_BASE_SIZE_03_x16		0x00080000
#define PCIE_C2C_32_MEM_BASE_03_x16			0x18100000
#define PCIE_C2C_32_MEM_BASE_SIZE_03_x16	0x00f00000
#define PCIE_C2C_64_MEM_BASE_03_x16			0xd100000000
#define PCIE_C2C_64_MEM_BASE_SIZE_03_x16	0x700000000
#define PCIE_C2C_CONFIG_TARGET_03_x8		0x31000000
#define PCIE_C2C_CONFIG_BASE_03_x8			0x19000000
#define PCIE_C2C_CONFIG_BASE_SIZE_03_x8		0x00080000
#define PCIE_C2C_IO_BASE_03_x8				0x19080000
#define PCIE_C2C_IO_BASE_SIZE_03_x8			0x00080000
#define PCIE_C2C_32_MEM_BASE_03_x8			0x19100000
#define PCIE_C2C_32_MEM_BASE_SIZE_03_x8		0x00f00000
#define PCIE_C2C_64_MEM_BASE_03_x8			0xd900000000
#define PCIE_C2C_64_MEM_BASE_SIZE_03_x8		0x700000000

#define PCIE_C2C_CONFIG_TARGET_72_x16		0x41000000
#define PCIE_C2C_CONFIG_BASE_72_x16			0x1a000000
#define PCIE_C2C_CONFIG_BASE_SIZE_72_x16	0x00080000
#define PCIE_C2C_IO_BASE_72_x16				0x1a080000
#define PCIE_C2C_IO_BASE_SIZE_72_x16		0x00080000
#define PCIE_C2C_32_MEM_BASE_72_x16			0x1a100000
#define PCIE_C2C_32_MEM_BASE_SIZE_72_x16	0x00f00000
#define PCIE_C2C_64_MEM_BASE_72_x16			0xe100000000
#define PCIE_C2C_64_MEM_BASE_SIZE_72_x16	0x700000000
#define PCIE_C2C_CONFIG_TARGET_72_x8		0x51000000
#define PCIE_C2C_CONFIG_BASE_72_x8			0x1b000000
#define PCIE_C2C_CONFIG_BASE_SIZE_72_x8		0x00080000
#define PCIE_C2C_IO_BASE_72_x8				0x1b080000
#define PCIE_C2C_IO_BASE_SIZE_72_x8			0x00080000
#define PCIE_C2C_32_MEM_BASE_72_x8			0x1b100000
#define PCIE_C2C_32_MEM_BASE_SIZE_72_x8		0x00f00000
#define PCIE_C2C_64_MEM_BASE_72_x8			0xe900000000
#define PCIE_C2C_64_MEM_BASE_SIZE_72_x8		0x700000000

#define PCIE_C2C_CONFIG_TARGET_73_x16		0x61000000
#define PCIE_C2C_CONFIG_BASE_73_x16			0x1c000000
#define PCIE_C2C_CONFIG_BASE_SIZE_73_x16	0x00080000
#define PCIE_C2C_IO_BASE_73_x16				0x1c080000
#define PCIE_C2C_IO_BASE_SIZE_73_x16		0x00080000
#define PCIE_C2C_32_MEM_BASE_73_x16			0x1c100000
#define PCIE_C2C_32_MEM_BASE_SIZE_73_x16	0x00f00000
#define PCIE_C2C_64_MEM_BASE_73_x16			0xf100000000
#define PCIE_C2C_64_MEM_BASE_SIZE_73_x16	0x700000000
#define PCIE_C2C_CONFIG_TARGET_73_x8		0x71000000
#define PCIE_C2C_CONFIG_BASE_73_x8			0x1d000000
#define PCIE_C2C_CONFIG_BASE_SIZE_73_x8		0x00080000
#define PCIE_C2C_IO_BASE_73_x8				0x1d080000
#define PCIE_C2C_IO_BASE_SIZE_73_x8			0x00080000
#define PCIE_C2C_32_MEM_BASE_73_x8			0x1d100000
#define PCIE_C2C_32_MEM_BASE_SIZE_73_x8		0x00f00000
#define PCIE_C2C_64_MEM_BASE_73_x8			0xf900000000
#define PCIE_C2C_64_MEM_BASE_SIZE_73_x8		0x700000000
#define PCIE_C2C_64_MEM_BASE_SIZE			0x700000000

struct c2c_ranges{
	uint32_t tile;
	uint32_t control;
	uint32_t config_target;
	uint32_t config_base;
	uint32_t config_base_size;
	uint32_t io_base;
	uint32_t io_base_size;
	uint32_t mem32_base;
	uint32_t mem32_base_size;
	uint64_t mem64_base;
	uint64_t mem64_base_size;
};

/***************************** Structure Definition **************************/

#define PHY_MODE_PCIE_AGGREGATION 4       /**< PCIE3x4 */
#define PHY_MODE_PCIE_NANBNB      0       /**< P1:PCIE3x2  +  P0:PCIE3x2 */
#define PHY_MODE_PCIE_NANBBI      1       /**< P1:PCIE3x2  +  P0:PCIE3x1*2 */
#define PHY_MODE_PCIE_NABINB      2       /**< P1:PCIE3x1*2 + P0:PCIE3x2 */
#define PHY_MODE_PCIE_NABIBI      3       /**< P1:PCIE3x1*2 + P0:PCIE3x1*2 */

#define PCIE_ATU_REGION_INDEX1 (0x1 << 0)
#define PCIE_ATU_REGION_INDEX0 (0x0 << 0)
#define PCIE_ATU_TYPE_MEM      (0x0 << 0)
#define PCIE_ATU_TYPE_IO       (0x2 << 0)
#define PCIE_ATU_TYPE_CFG0     (0x4 << 0)
#define PCIE_ATU_TYPE_CFG1     (0x5 << 0)

#define PCI_VENDOR_ID 0x00  /* 16 bits */
#define PCI_DEVICE_ID 0x02  /* 16 bits */

#define PCI_CLASS_REVISION 0x08 /* High 24 bits are class, low 8 revision */
#define PCI_CLASS_DEVICE   0x0a /* Device class */
#define PCI_CLASS_CODE     0x0b /* Device class code */

typedef int32_t HAL_PCI_DevT;

#define PCI_BUS(d)              (((d) >> 16) & 0xff)
#define PCI_DEVFN(d, f)         ((d) << 11 | (f) << 8)
#define PCI_MASK_BUS(bdf)       ((bdf) & 0xffff)
#define PCI_ADD_BUS(bus, devfn) (((bus) << 16) | (devfn))
#define PCI_BDF(b, d, f)        ((b) << 16 | PCI_DEVFN(d, f))

/**
 * @brief PCI size type
 */
typedef enum {
	PCI_SIZE_8,
	PCI_SIZE_16,
	PCI_SIZE_32,
} ePCI_Size;

/** HAL boolean type definition */
typedef enum {
	HAL_FALSE = 0x00U,
	HAL_TRUE  = 0x01U
} HAL_Check;

/** HAL error code definition */
typedef enum {
	HAL_OK      = 0x00U,
	HAL_ERROR   = (-1),
	HAL_BUSY    = (-16),
	HAL_NODEV   = (-19),
	HAL_INVAL   = (-22),
	HAL_NOSYS   = (-38),
	HAL_TIMEOUT = (-110)
} HAL_Status;

/** HAL functional status definition */
typedef enum {
	HAL_DISABLE = 0x00U,
	HAL_ENABLE  = 0x01U
} HAL_FuncStatus;

/** HAL lock structures definition */
typedef enum {
	HAL_UNLOCKED = 0x00U,
	HAL_LOCKED   = 0x01U
} HAL_LockStatus;

typedef enum {
	HAL_X16 = 0x16U,
	HAL_X16toX8 = 0x17U,
	HAL_X8 = 0x08U,
} HAL_ControlType;

typedef enum {
	TILE_02 = 0x02U,
	TILE_03 = 0x03U,
	TILE_72 = 0x72U,
	TILE_73 = 0x73U,
} HAL_TileSelect;

typedef enum {
	X86_EP = 0x00U,
	C2C_EP = 0x01U,
	C2C_RC = 0x02U,
} HAL_ModeSelect;

struct HAL_PHY_SNPS_PCIE3_DEV {
	uint32_t phyMode;
};

/** PCIe handler */
struct HAL_PCIE_DEV {
	uint64_t dbiBase;
	uint64_t engineBase;
	uint64_t apbBase;
	uint64_t cfgBase;
	uint64_t phyBase;
	uint64_t ssBase;
	uint64_t drouterBase;
	uint64_t crouterBase;
	uint64_t dniuBase;
	uint64_t cniuBase;
	uint64_t mbitxBase;
	uint8_t max_lanes;
	uint8_t lanes;
	uint8_t gen;
	uint8_t firstBusNo;
	uint32_t ltrIrqNum;
	uint32_t vdmIrqNum;
	uint32_t dstateIrqNum;
	uint32_t linkstIrqNum;
	uint32_t ltssmIrqNum;
	uint32_t hp_msiIrqNum;
	uint8_t bif_en;
	uint8_t pipe8;
	void *phy;
};

struct HAL_PCIE_HANDLE {
	struct HAL_PCIE_DEV *dev;
};
/** @} */
/***************************** Function Declare ******************************/
/** @defgroup PCIE_Public_Function_Declare Public Function Declare
 *  @{
 */

inline void writeb(uint8_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint8_t *)(addr)) = value;
}

inline uint8_t readb(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint8_t *)(addr));
}

inline void writew(uint16_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint16_t *)(addr)) = value;
}

inline uint16_t readw(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint16_t *)(addr));
}

inline void writel(uint32_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

inline uint32_t readl(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
}

inline void writeq(uint32_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	// printf("writeq addr 0x%lx value 0x%x\n", addr, value);
	*((volatile uint32_t *)(addr)) = value;
}

inline uint32_t readq(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;
	uint32_t value = *((volatile uint32_t *)(addr));

	// printf("readq addr 0x%lx value 0x%x\n", addr, value);
	return value;
}

inline void pcie_writel_apb(struct HAL_PCIE_DEV *dev, uint32_t value, uint64_t address)
{
	uint64_t apb_base = dev->apbBase;
	uintptr_t addr = (uintptr_t)(apb_base + address);

	// printf("writeq addr 0x%lx value 0x%x\n", addr, value);
	*((volatile uint32_t *)(addr)) = value;
}

inline uint32_t pcie_readl_apb(struct HAL_PCIE_DEV *dev, uint64_t address)
{
	uint64_t apb_base = dev->apbBase;
	uintptr_t addr = (uintptr_t)(apb_base + address);
	uint32_t value = *((volatile uint32_t *)(addr));

	// printf("readq addr 0x%lx value 0x%x\n", addr, value);
	return value;
}

// HAL_Status HAL_PCIE_InboundConfig(struct HAL_PCIE_DEV *dev, int32_t index, int32_t bar, uint64_t cpuAddr);
// HAL_Status HAL_PCIE_InboundConfig_addr(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);
// HAL_Status HAL_PCIE_OutboundConfig(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);
// HAL_Status dw_pcie_prog_inbound_atu(struct HAL_PCIE_DEV *dev, int32_t index, int32_t bar, uint64_t cpuAddr);
// HAL_Status dw_pcie_prog_inbound_atu_addr(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);
// HAL_Status dw_pcie_prog_outbound_atu(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size);

int rhea_pcie_ep_init(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control, HAL_ModeSelect select);
int rhea_pcie_rc_init(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control, HAL_ModeSelect select);
#endif // __PCIE_H__
