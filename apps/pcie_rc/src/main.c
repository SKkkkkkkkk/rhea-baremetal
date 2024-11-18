#include <stdio.h>
#include <stdlib.h>
#include "gicv3.h"
#include "pcie.h"
#include "systimer.h"
#include "common.h"
#include "dw_apb_gpio.h"


#define SEEHI_PLD_PCIE_TEST			1
#define SEEHI_FPGA_PCIE_TEST		0
#define SEEHI_MSIX_ENABLE			0

#define SEEHI_C2C_PCIE_TEST			1

#define PLD_Z1						1
#define PLD_Z2						0


#define TCM_CFG_BASE          0x15000000
#define C2C_SYS_CFG_03       0x8180000000ULL
#define C2C_SYS_CFG_02       0x8100000000ULL
#define C2C_SYS_CFG_73       0xB980000000ULL
#define C2C_SYS_CFG_72       0xB900000000ULL

static uint64_t g_c2c_base;

struct PCIE_IDB_CFG {
	volatile uint32_t magic;
#define PCIE_IDB_CFG_MAGIC 0x65696370
	volatile uint32_t bootconfig;
#define PCIE_IDB_CFG_PHY_MODE_SHIFT       0
#define PCIE_IDB_CFG_PHY_MODE_MASK        0x7
#define PCIE_IDB_CFG_PHY_MODE_AGGREGATION PHY_MODE_PCIE_AGGREGATION /**< PCIE3x4 */
#define PCIE_IDB_CFG_PHY_MODE_NANBNB      PHY_MODE_PCIE_NANBNB      /**< P1:PCIE3x2  +  P0:PCIE3x2 */
#define PCIE_IDB_CFG_PHY_MODE_NANBBI      PHY_MODE_PCIE_NANBBI      /**< P1:PCIE3x2  +  P0:PCIE3x1*2 */
#define PCIE_IDB_CFG_PHY_MODE_NABINB      PHY_MODE_PCIE_NABINB      /**< P1:PCIE3x1*2 + P0:PCIE3x2 */
#define PCIE_IDB_CFG_PHY_MODE_NABIBI      PHY_MODE_PCIE_NABIBI      /**< P1:PCIE3x1*2 + P0:PCIE3x1*2 */
#define PCIE_IDB_CFG_GEN_SHIFT            8
#define PCIE_IDB_CFG_GEN_MASK             0x7
#define PCIE_IDB_CFG_LANE_SHIFT           12
#define PCIE_IDB_CFG_LANE_MASK            0x7
#define PCIE_IDB_CFG_UART_ID_SHIFT        16
#define PCIE_IDB_CFG_UART_ID_MASK         0x7
#define PCIE_IDB_CFG_UART_MUX_SHIFT       20
#define PCIE_IDB_CFG_UART_MUX_MASK        0x7
#define PCIE_IDB_CFG_UART_RATE_SHIFT      24
#define PCIE_IDB_CFG_UART_RATE_MASK       0x3
#define PCIE_IDB_CFG_UART_RATE_DEFAULT    0
#define PCIE_IDB_CFG_UART_RATE_15000000   1
#define PCIE_IDB_CFG_UART_RATE_1152000    2
	volatile uint16_t vid;
	volatile uint16_t did;
};


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

#define PCIE_LINK_WIDTH_SPEED_CONTROL 0x80C
#define PORT_LOGIC_SPEED_CHANGE       (0x1 << 17)
#define PORT_LOGIC_LINK_WIDTH_MASK    (0x1f << 8)
#define PORT_LOGIC_LINK_WIDTH_1_LANES (0x1 << 8)
#define PORT_LOGIC_LINK_WIDTH_2_LANES (0x2 << 8)
#define PORT_LOGIC_LINK_WIDTH_4_LANES (0x4 << 8)
#define PORT_LOGIC_LINK_WIDTH_8_LANES (0x8 << 8)
#define PORT_LOGIC_LINK_WIDTH_16_LANES (0x10 << 8)

#define PCIE_DIRECT_SPEED_CHANGE (0x1 << 17)

#define BOOT_USING_PCIE_EP
#define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x00400000000  //bootrom    bit40 来做C&N 区分
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x10410000000           //uart
#define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x00440000000   //ap dram

// #define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x10410000000  //AP SYS UART    bit40 来做C&N 区分
// #define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x00440000000           //AP SYS DRAM
// #define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x00540000000   //tile 0 5

#define AP_SYS_C2C0_CPU_ADDRESS		C2C_SYS_CFG_03
#define DWC_PCIE_CTL_X16_DBI		(AP_SYS_C2C0_CPU_ADDRESS + 0x0)
#define C2C_ENGINE_X16				(AP_SYS_C2C0_CPU_ADDRESS + 0x140000)
#define PCIE_X16_REG				(AP_SYS_C2C0_CPU_ADDRESS + 0x180000)
#define DWC_PCIE_CTL_X8_DBI			(AP_SYS_C2C0_CPU_ADDRESS + 0x200000)
#define C2C_ENGINE_X8				(AP_SYS_C2C0_CPU_ADDRESS + 0x340000)
#define PCIE_X8_REG					(AP_SYS_C2C0_CPU_ADDRESS + 0x380000)
#define PCIE_PHY_REG				(AP_SYS_C2C0_CPU_ADDRESS + 0x400000)
#define DWC_PCIE5_PHY0_CRI			(AP_SYS_C2C0_CPU_ADDRESS + 0x480000)
#define DWC_PCIE5_PHY1_CRI			(AP_SYS_C2C0_CPU_ADDRESS + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI			(AP_SYS_C2C0_CPU_ADDRESS + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI			(AP_SYS_C2C0_CPU_ADDRESS + 0x4e0000)
#define C2C_SS_REG					(AP_SYS_C2C0_CPU_ADDRESS + 0x580000)
#define DROUTER						(AP_SYS_C2C0_CPU_ADDRESS + 0x581000)
#define CROUTER						(AP_SYS_C2C0_CPU_ADDRESS + 0x582000)
#define DNIU						(AP_SYS_C2C0_CPU_ADDRESS + 0x583000)
#define CNIU						(AP_SYS_C2C0_CPU_ADDRESS + 0x584000)
#define MBI_TX						(AP_SYS_C2C0_CPU_ADDRESS + 0x585000)

#define AP_SYS_C2C0_CPU_ADDRESS_73		C2C_SYS_CFG_73
#define DWC_PCIE_CTL_X16_DBI_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x0)
#define C2C_ENGINE_X16_73				(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x140000)
#define PCIE_X16_REG_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x180000)
#define DWC_PCIE_CTL_X8_DBI_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x200000)
#define C2C_ENGINE_X8_73				(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x340000)
#define PCIE_X8_REG_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x380000)
#define PCIE_PHY_REG_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x400000)
#define DWC_PCIE5_PHY0_CRI_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x480000)
#define DWC_PCIE5_PHY1_CRI_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x4a0000)
#define DWC_PCIE5_PHY2_CRI_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x4c0000)
#define DWC_PCIE5_PHY3_CRI_73			(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x4e0000)
#define C2C_SS_REG_73					(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x580000)
#define DROUTER_73						(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x581000)
#define CROUTER_73						(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x582000)
#define DNIU_73							(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x583000)
#define CNIU_73							(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x584000)
#define MBI_TX_73						(AP_SYS_C2C0_CPU_ADDRESS_73 + 0x585000)

#if SEEHI_C2C_PCIE_TEST && SEEHI_PLD_PCIE_TEST
#if PLD_Z1
#define PCIE_C2C_CONFIG_TARGET			0x21000000
#define PCIE_C2C_CONFIG_BASE			0x18000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x18080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x18100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xd100000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x700000000
#elif PLD_Z2
#define PCIE_C2C_CONFIG_TARGET			0x61000000
#define PCIE_C2C_CONFIG_BASE			0x1c000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x1c080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x1c100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xf100000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x700000000
#endif
#else
#define PCIE_C2C_CONFIG_BASE			0x18000000
#define PCIE_C2C_CONFIG_BASE_SIZE		0x00080000
#define PCIE_C2C_IO_BASE				0x18080000
#define PCIE_C2C_IO_BASE_SIZE			0x00080000
#define PCIE_C2C_32_MEM_BASE			0x18100000
#define PCIE_C2C_32_MEM_BASE_SIZE		0x00f00000
#define PCIE_C2C_64_MEM_BASE			0xd100000000
#define PCIE_C2C_64_MEM_BASE_SIZE		0x700000000
#endif


/********************* Private Structure Definition **************************/

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/

void mc_init(uint64_t addr, uint8_t layer) {
	// global
	if (layer == 4) {
		REG32(addr+0x00013054) = 0x00000000;
		REG32(addr+0x00013004) = 0x00000000;
		REG32(addr+0x00013004) = 0x80000000;
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

uint64_t get_pcie_base(uint32_t pcie_sel) {
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

static inline void delay(uint32_t value)
{
	volatile uint32_t i, j;// k;

	for(i = 0; i < value; i++)
		for(j = 0; j < 1000; j++);
			// for(k = 0; k < 1000; k++);
}

static inline void writeb(uint8_t value, uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;

	// printf("seehi--> %s line: %d (0x%x , 0x%lx)\n", __func__, __LINE__, value, address);
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

	// printf("seehi--> %s line: %d (0x%x , 0x%lx)\n", __func__, __LINE__, value, address);
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

	// printf("seehi--> %s line: %d (0x%x , 0x%lx)\n", __func__, __LINE__, value, address);
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

	// printf("seehi--> %s line: %d (0x%x , 0x%lx)\n", __func__, __LINE__, value, address);
	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readq(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;
	uint32_t value = *((volatile uint32_t *)(addr));

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

	cap = readq(dbi_base + offset + PCI_EXP_LNKCAP);  //最大
	ctrl2 = readq(dbi_base + offset + PCI_EXP_LNKCTL2);  //当前的
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
	val = readq(dbi_base + PCIE_PORT_LINK_CONTROL);  //支持最大lanes
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
	writeq(val, dbi_base + PCIE_PORT_LINK_CONTROL);

	/* Set link width speed control register */  //正确链接的lanes
	val = readq(dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
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

	writeq(val, dbi_base + PCIE_LINK_WIDTH_SPEED_CONTROL);
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

#define PCI_MSI_FLAGS		2	/* Message Control */
#define  PCI_MSI_FLAGS_ENABLE	0x0001	/* MSI feature enabled */
#define  PCI_MSI_FLAGS_QMASK	0x000e	/* Maximum queue size available */
#define  PCI_MSI_FLAGS_QSIZE	0x0070	/* Message queue size configured */
#define  PCI_MSI_FLAGS_64BIT	0x0080	/* 64-bit addresses allowed */
#define  PCI_MSI_FLAGS_MASKBIT	0x0100	/* Per-vector masking capable */
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
	val = readq(dbi_base + reg);
	val &= ~(PCI_MSI_FLAGS_QMASK << 16);
	val |= interrupts << 17;
	writeq(val, dbi_base + reg);

	return 0;
}

/********************* Public Function Definition ****************************/

static int a510_pcie_ap_dniu(const struct HAL_PCIE_HANDLE *pcie)
{
	uint32_t dniu_base = 0x11002000;

	writel(0x11, dniu_base + 0x4);
	writel((((1<<0) << 16) + (1<<2)), dniu_base + 0x8);
	writel(0xc000000, dniu_base + 0xc);
	writel(0xcffffff, dniu_base + 0x10);
	writel(0x0, dniu_base + 0x14);
	writel(0x1, dniu_base + 0x0);

	writel(0x11, dniu_base + 0x24);
	writel((((1<<0) << 16) + (1<<3)), dniu_base + 0x28);
	writel(0xd000000, dniu_base + 0x2c);
	writel(0xdffffff, dniu_base + 0x30);
	writel(0x0, dniu_base + 0x34);
	writel(0x1, dniu_base + 0x20);

	writel(0x11, dniu_base + 0x44);
	writel((((1<<7) << 16) + (1<<2)), dniu_base + 0x48);
	writel(0xe000000, dniu_base + 0x4c);
	writel(0xeffffff, dniu_base + 0x50);
	writel(0x0, dniu_base + 0x54);
	writel(0x1, dniu_base + 0x40);

	writel(0x11, dniu_base + 0x64);
	writel((((1<<7) << 16) + (1<<3)), dniu_base + 0x68);
	writel(0xf000000, dniu_base + 0x6c);
	writel(0xfffffff, dniu_base + 0x70);
	writel(0x0, dniu_base + 0x74);
	writel(0x1, dniu_base + 0x60);

	return 0;
}

void BSP_PCIE_RC_Init(const struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t phy_base = pcie->dev->phyBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint64_t ss_base = pcie->dev->ssBase;
	uint32_t lanes = pcie->dev->max_lanes;
	uint32_t val;

	val = readq(apb_base + 0x100);  //
	val &= 0xfffffffe;
	writeq(val, apb_base + 0x100);  //disable app_ltssm_enable

#if  SEEHI_PLD_PCIE_TEST

#if SEEHI_C2C_PCIE_TEST
	writeq(1, phy_base + 0x18);	//pipe16_pclkchange_hs_en

	// val = readq(apb_base + 0x100);
	// val |= 1 << 3;
	// writeq(val, apb_base + 0x100);  //app_hold_phy_rst
#endif
#elif SEEHI_FPGA_PCIE_TEST

#else
	if(lanes == 16){
		writeq(0, phy_base + 0x0);  //bif_en X16
		writeq(0, phy_base + 0x94);  //pipe8_lane_mode  //
	}else if(lanes == 8){
		writeq(1, phy_base + 0x0);  //bif_en X8
		writeq(8, phy_base + 0x94);  //pipe8_lane_mode  //选phy clk
	}else{
		printf("PHY bifurcation error !\n");
	}
#endif


	writeq(0, apb_base + 0x110);  //close fast link PCIE_DIAG_CTRL_BUS
	writeq(4, apb_base + 0x104);  //rc mode
#if PLD_Z1
	writeq(0x18000000, ss_base + 0x200);  //config space todo
	writeq(0x0, ss_base + 0x204);
#elif PLD_Z2
	writeq(0x1c000000, ss_base + 0x200);  //config space todo
	writeq(0x0, ss_base + 0x204);
#endif

	writeq(0x1ff, apb_base + 0xa0);
	writeq(0x1, apb_base + 0x318);

}

static void BSP_First_Reset(void)
{
	// printf("BSP_First_Reset\n");
}

static void gpio_perst_init(void)
{
	pinmux_select(PORTB, 3, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTB,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Output_Mode,
		.pin = 3
	};
	gpio_init(&gpio_init_config);
}

static void gpio_sync_init(void)
{
#if SEEHI_C2C_PCIE_TEST
	pinmux_select(PORTA, 0, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTA,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Output_Mode,
		.pin = 0
	};
	gpio_init(&gpio_init_config);
	gpio_write_pin(PORTA, 0, GPIO_PIN_SET);

#else

	pinmux_select(PORTA, 24, 7);
	gpio_init_config_t gpio_init_config = {
		.port = PORTA,
		.gpio_control_mode = Software_Mode,
		.gpio_mode = GPIO_Output_Mode,
		.pin = 24
	};
	gpio_init(&gpio_init_config);
	gpio_write_pin(PORTA, 24, GPIO_PIN_SET);
#endif  //SEEHI_C2C_PCIE_TEST
}

static void rc_init_pre(struct HAL_PCIE_HANDLE *pcie)
{
	uint32_t val, val_cmp = 0xf55a55aa;
	uint32_t bar;
	uint64_t dbi_base = pcie->dev->dbiBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint64_t ss_base = pcie->dev->ssBase;
	uint32_t dniu_base = 0x11002000;
	uint32_t mbitx_ap_base = 0x10050000;
	uint64_t resbar_base;
	int32_t i, timeout = 0, phy_linkup = 0;
	uint16_t vid, did;
	uint32_t temp;
	uint64_t real_addr;

	BSP_PCIE_RC_Init(pcie);    //

	writeq(0x1, ss_base + 0x1b8);    //MSI MBI
	writeq(0xffff0000, ss_base + 0x1cc);  //MBI MASK
#if PLD_Z1
	writeq(0x21000000, ss_base + 0x1d0); //todo
#elif PLD_Z2
	writeq(0x61000000, ss_base + 0x1d0); //todo
#endif

	val = readl(dbi_base + 0x708);
	val |= 0x400000;
	writel(val, dbi_base + 0x708);  // Poling Active to Poling x1 x4 x8

	writeq(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	writeq(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0

	writeq(0x01402200, dbi_base + 0x890);  //GEN4_RELATED_OFF.EQ_PHASE_2_3=0
	writeq(0x01402200, dbi_base + 0x890);  //GEN4_RELATED_OFF.EQ_PHASE_2_3=0

	writeq(0x02402200, dbi_base + 0x890);  //GEN5_RELATED_OFF.EQ_PHASE_2_3=0
	writeq(0x02402200, dbi_base + 0x890);  //GEN5_RELATED_OFF.EQ_PHASE_2_3=0

	writeq(0x4d004071, dbi_base + 0x8a8);  //GEN3_EQ_CONTROL_OFF

	writeq(0x10001, dbi_base + 0x24);  //pref
	writeq(0x536105, dbi_base + 0x7c); //LINK_CAPABILITIES_REG

	writeq(0x3, dbi_base + 0x720);   //vdm msg drop

	writeq(0x11110000, dbi_base + 0x30014);
	writeq(0x11110000, dbi_base + 0x30214);
	writeq(0x11110000, dbi_base + 0x30414);
	writeq(0x11110000, dbi_base + 0x30614);
	writeq(0x11110000, dbi_base + 0x30814);
	writeq(0x11110000, dbi_base + 0x30a14);
	writeq(0x11110000, dbi_base + 0x30c14);
	writeq(0x11110000, dbi_base + 0x30e14);
	writeq(0x11110000, dbi_base + 0x30114);
	writeq(0x11110000, dbi_base + 0x30314);
	writeq(0x11110000, dbi_base + 0x30514);
	writeq(0x11110000, dbi_base + 0x30714);
	writeq(0x11110000, dbi_base + 0x30914);
	writeq(0x11110000, dbi_base + 0x30b14);
	writeq(0x11110000, dbi_base + 0x30d14);
	writeq(0x11110000, dbi_base + 0x30f14);

#if PLD_Z1
	writeq(0x2f2120, dbi_base + 0x18);
#elif PLD_Z2
	writeq(0x6f6160, dbi_base + 0x18);
#endif

	writeq(0x300310c8, dbi_base + 0x80c);
	writeq(0x4, dbi_base + 0x10);  //bar0
	writeq(0x0, dbi_base + 0x14);  //bar1
	writeq(0x0, dbi_base + 0x10014);
	writeq(0x1ff, dbi_base + 0x3c);  // interrrupt pin
									 //
	writeq(0x100107, dbi_base + 0x4);  // cmd  bus & mem enable

	writeq(0x7fffffff, dbi_base + 0x30014);
	writeq(0x7fffffff, dbi_base + 0x30214);
	writeq(0x7fffffff, dbi_base + 0x30414);
	writeq(0x7fffffff, dbi_base + 0x30614);
	writeq(0x7fffffff, dbi_base + 0x30814);
	writeq(0x7fffffff, dbi_base + 0x30a14);
	writeq(0x7fffffff, dbi_base + 0x30c14);
	writeq(0x7fffffff, dbi_base + 0x30e14);

#if  SEEHI_PLD_PCIE_TEST
	HAL_PCIE_OutboundConfig(pcie, 0, PCIE_ATU_TYPE_CFG0, PCIE_C2C_CONFIG_BASE, PCIE_C2C_CONFIG_TARGET, PCIE_C2C_CONFIG_BASE_SIZE);  //todo
	HAL_PCIE_OutboundConfig(pcie, 1, PCIE_ATU_TYPE_MEM, PCIE_C2C_32_MEM_BASE, PCIE_C2C_32_MEM_BASE, PCIE_C2C_32_MEM_BASE_SIZE);
	real_addr = PCIE_C2C_64_MEM_BASE & 0x7ffffffff;
	printf("seehi--> %s line: %d real_addr 0x%lx\n", __func__, __LINE__, real_addr);
	HAL_PCIE_OutboundConfig(pcie, 2, PCIE_ATU_TYPE_MEM | 0x2000, real_addr, PCIE_C2C_64_MEM_BASE, PCIE_C2C_64_MEM_BASE_SIZE);
	HAL_PCIE_OutboundConfig(pcie, 3, PCIE_ATU_TYPE_IO, PCIE_C2C_IO_BASE, PCIE_C2C_IO_BASE, PCIE_C2C_IO_BASE_SIZE);

#elif SEEHI_FPGA_PCIE_TEST
	dw_pcie_prog_outbound_atu(pcie, 0, PCIE_ATU_TYPE_CFG0, PCIE_C2C_CONFIG_BASE, PCIE_C2C_CONFIG_TARGET, PCIE_C2C_CONFIG_BASE_SIZE); //todo
	dw_pcie_prog_outbound_atu(pcie, 1, PCIE_ATU_TYPE_MEM, PCIE_C2C_32_MEM_BASE, PCIE_C2C_32_MEM_BASE, PCIE_C2C_32_MEM_BASE_SIZE);
	real_addr = PCIE_C2C_64_MEM_BASE & 0x7ffffffff;
	printf("seehi--> %s line: %d real_addr 0x%lx\n", __func__, __LINE__, real_addr);
	dw_pcie_prog_outbound_atu(pcie, 2, PCIE_ATU_TYPE_MEM | 0x2000, real_addr, PCIE_C2C_64_MEM_BASE, PCIE_C2C_64_MEM_BASE_SIZE);
	dw_pcie_prog_outbound_atu(pcie, 3, PCIE_ATU_TYPE_IO, PCIE_C2C_IO_BASE, 0, PCIE_C2C_IO_BASE_SIZE);

#else

#error

#endif

	writeq(0x0, dbi_base + 0x10);  //bar0
	writeq(0x0, dbi_base + 0x10010);
	writew(0x604, dbi_base + 0xa);
	writeq(0x300310c8, dbi_base + 0x80c);

	vid = 0x7368;    //sh
	did = 0xa510;    //a510
	writeq(did << 16 | vid, dbi_base + 0x00);  //vendor id & device id

	// writeq(0x00102130, dbi_base + 0x78);   //DEVICE_CONTROL_DEVICE_STATUS 和MAX PAYLOAD SIZE 相关

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
}


static void rc_rescan(struct HAL_PCIE_HANDLE *pcie)
{
	uint64_t dbi_base = pcie->dev->dbiBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint64_t ss_base = pcie->dev->ssBase;

	writew(0x104, dbi_base + 0x4);
	writel(0x0, dbi_base + 0x10);
	writew(0x104, dbi_base + 0x4);
	writel(0x0, dbi_base + 0x14);
	writew(0x107, dbi_base + 0x4);

	writew(0x104, dbi_base + 0x4);
	writel(0xfffff800, dbi_base + 0x38);
	writel(0x0, dbi_base + 0x38);
	writew(0x107, dbi_base + 0x4);
	writew(0xe0f0, dbi_base + 0x1c);
	writew(0x0, dbi_base + 0x1c);
	writel(0x0, dbi_base + 0x28);
	writew(0x400, dbi_base + 0x98);
	writew(0x2, dbi_base + 0x3e);

	writew(0x8008, dbi_base + 0x44);

	writew(0x2, dbi_base + 0x3e);
	writel(0x0, dbi_base + 0x18);
	writew(0x2, dbi_base + 0x3e);

	writew(0xffff, dbi_base + 0x6);

	writew(0x0, PCIE_C2C_CONFIG_BASE + 0x4);

	writel(0x0, PCIE_C2C_CONFIG_BASE + 0x10);
	writel(0x0, PCIE_C2C_CONFIG_BASE + 0x14);
	writel(0xc, PCIE_C2C_CONFIG_BASE + 0x18);
	writel(0x0, PCIE_C2C_CONFIG_BASE + 0x1c);
	writel(0xc, PCIE_C2C_CONFIG_BASE + 0x20);
	writel(0x0, PCIE_C2C_CONFIG_BASE + 0x24);

	writel(0x0, PCIE_C2C_CONFIG_BASE + 0x30);
	writew(0x2110, PCIE_C2C_CONFIG_BASE + 0x78);

	writew(0x8008, PCIE_C2C_CONFIG_BASE + 0x44);
#if PLD_Z1
	writeq(0x212120, dbi_base + 0x18);
	writel(0x18100000, dbi_base + 0x14);
#elif PLD_Z2
	writeq(0x616160, dbi_base + 0x18);
	writel(0x1c100000, dbi_base + 0x14);
#endif
	writew(0x2, dbi_base + 0x3e);

#if PLD_Z1
	writew(0x0, PCIE_C2C_CONFIG_BASE + 0x4);
	writel(0x18800000, PCIE_C2C_CONFIG_BASE + 0x10);  //todo
	writel(0x18400000, PCIE_C2C_CONFIG_BASE + 0x14);  //todo
	writel(0xc, PCIE_C2C_CONFIG_BASE + 0x18);
	writel(0xd2, PCIE_C2C_CONFIG_BASE + 0x1c);  //todo
	writel(0xc, PCIE_C2C_CONFIG_BASE + 0x20);
	writel(0xd4, PCIE_C2C_CONFIG_BASE + 0x24);  //todo
#elif PLD_Z2
	writel(0x1c800000, PCIE_C2C_CONFIG_BASE + 0x10);  //todo
	writel(0x1c400000, PCIE_C2C_CONFIG_BASE + 0x14);  //todo
	writel(0xc, PCIE_C2C_CONFIG_BASE + 0x18);
	writel(0xf2, PCIE_C2C_CONFIG_BASE + 0x1c);  //todo
	writel(0xc, PCIE_C2C_CONFIG_BASE + 0x20);
	writel(0xf4, PCIE_C2C_CONFIG_BASE + 0x24);  //todo
#endif

	writel(0xffff, dbi_base + 0x30);
	writew(0xf0, dbi_base + 0x1c);
	writel(0x0, dbi_base + 0x30);

#if PLD_Z1
	writel(0x18f01840, dbi_base + 0x20);
	writel(0xfff10001, dbi_base + 0x24);
	writel(0xd2, dbi_base + 0x28);
	writel(0xd7, dbi_base + 0x2c);
#elif PLD_Z2
	writel(0x1cf01c40, dbi_base + 0x20);
	writel(0xfff10001, dbi_base + 0x24);
	writel(0xf2, dbi_base + 0x28);
	writel(0xf7, dbi_base + 0x2c);
#endif
	writew(0x2, dbi_base + 0x3e);

	systimer_delay(1, IN_MS);

	writew(0x6, PCIE_C2C_CONFIG_BASE + 0x4);

}

HAL_Status PCIe_RC_Init(struct HAL_PCIE_HANDLE *pcie)
{
	uint32_t val, val_cmp = 0xf55a55aa;
	uint32_t bar;
	uint64_t dbi_base = pcie->dev->dbiBase;
	uint64_t apb_base = pcie->dev->apbBase;
	uint64_t ss_base = pcie->dev->ssBase;
	uint32_t dniu_base = 0x11002000;
	uint32_t mbitx_ap_base = 0x10050000;
	uint64_t resbar_base;
	int32_t i, timeout = 0, phy_linkup = 0;
	uint16_t vid, did;
	uint32_t temp;
	uint64_t real_addr;

	a510_pcie_ap_dniu(pcie);

	dw_pcie_dbi_ro_wr_en(dbi_base);

	rc_init_pre(pcie);

#if SEEHI_FPGA_PCIE_TEST
	gpio_write_pin(PORTB, 3, GPIO_PIN_RESET);
	systimer_delay(100, IN_MS);
	gpio_write_pin(PORTB, 3, GPIO_PIN_SET);

	gpio_write_pin(PORTA, 24, GPIO_PIN_RESET);
	systimer_delay(100, IN_MS);
	gpio_write_pin(PORTA, 24, GPIO_PIN_SET);
#else
#if SEEHI_C2C_PCIE_TEST
	gpio_write_pin(PORTA, 0, GPIO_PIN_RESET);
	systimer_delay(1, IN_MS);
	gpio_write_pin(PORTA, 0, GPIO_PIN_SET);
#endif
#endif

	val = readq(apb_base + 0x100);  // 
	val |= 0x1;
	writeq(val, apb_base + 0x100);  //enable app_ltssm_enable

	systimer_delay(1, IN_MS);

	while (1) {   //判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readq(apb_base + 0x150);
		if ((val & 0xffff) == 0x110f) { //L0
			phy_linkup = 1;
			break;
		}

		if(timeout >= 1000000){
			timeout=0;
			// break;
		}

		systimer_delay(1, IN_MS);
		timeout++;

		if (val != val_cmp) {
			val_cmp = val;
			printf("ctrl_link_status = 0x%x\n", val);
		}
	}

	if(phy_linkup == 0){
		BSP_First_Reset();
	}

	printf("Link up\n");

	val = readq(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);

#if SEEHI_C2C_PCIE_TEST

	val = readl(PCIE_C2C_CONFIG_BASE + 0x80);
	printf("read LinkSta 0x%x\n", val);

	rc_rescan(pcie);

	dump_regs("read rc config space 00-ff:", dbi_base, 64);
	dump_regs("read ep config space 00-ff:", PCIE_C2C_CONFIG_BASE, 64);

#if PLD_Z1
	dump_regs("bar0:", 0x18800000, 32);
	dump_regs("bar2:", 0xd200000000, 32);
	dump_regs("bar4:", 0xd400000000, 32);
#elif PLD_Z2
	dump_regs("bar0:", 0x1c800000, 64);
	dump_regs("bar2:", 0xf200000000, 64);
	dump_regs("bar4:", 0xf400000000, 64);
#endif

#if 1
	printf("rc set hot reset start test:\n");
	// writeq(0x004201ff, dbi_base + 0x3c);  // hot reset
	// systimer_delay(1, IN_MS);
	// writeq(0x000201ff, dbi_base + 0x3c);  // hot reset

	writeq(0x1, apb_base + 0x108);  // hot reset
										  //
	while (1) {   //判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readq(apb_base + 0x150);
		if ((val & 0xffff) == 0x1143) { //L0
			phy_linkup = 1;
			break;
		}

		if(timeout >= 1000000){
			timeout=0;
			// break;
		}

		systimer_delay(1, IN_MS);
		timeout++;

		if (val != val_cmp) {
			val_cmp = val;
			printf("ctrl_link_status = 0x%x\n", val);
		}
	}

	printf("retry link up\n");
	val = readq(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);
	val = readl(PCIE_C2C_CONFIG_BASE + 0x80);
	printf("read LinkSta 0x%x\n", val);

	rc_rescan(pcie);

	dump_regs("read rc config space 00-ff:", dbi_base, 64);
	dump_regs("read ep config space 00-ff:", PCIE_C2C_CONFIG_BASE, 64);

#if PLD_Z1
	dump_regs("bar0:", 0x18800000, 32);
	dump_regs("bar2:", 0xd200000000, 32);
	dump_regs("bar4:", 0xd400000000, 32);
#elif PLD_Z2
	dump_regs("bar0:", 0x1c800000, 64);
	dump_regs("bar2:", 0xf200000000, 64);
	dump_regs("bar4:", 0xf400000000, 64);
#endif
#endif  //hot reset

#if 1
	dump_regs("vdm before:", apb_base + 0x130, 16);
	writel(0x4d, apb_base + 0x130);
	writel(0x7e0000, apb_base + 0x134);
	writel(0x12345678, apb_base + 0x138);
	writel(0x87654321, apb_base + 0x13c);
	writel(0x8000004d, apb_base + 0x130);
	dump_regs("vdm after:", apb_base + 0x130, 16);
#endif  //vdm test

#endif  //SEEHI_C2C_PCIE_TEST
	/////////////////////////////////////END//////////////////////////////////////////////////////

	dw_pcie_dbi_ro_wr_dis(dbi_base);


	return HAL_OK;
}

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
	.firstBusNo = 0x0,
	.ltrIrqNum = 172,
	.vdmIrqNum = 173,
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
	.lanes = 8,
	.gen = 5,
	.firstBusNo = 0x30,
	.ltrIrqNum = 204,
	.vdmIrqNum = 205,
};

struct HAL_PCIE_HANDLE s_pcie;

int main()
{
	uint32_t result = HAL_ERROR;
	struct HAL_PCIE_HANDLE *pcie = &s_pcie;

	g_c2c_base = get_pcie_base(3);

#if SEEHI_FPGA_PCIE_TEST
	s_pcie.dev = &g_pcieDevX8;

	gpio_perst_init();
	gpio_sync_init();
#elif SEEHI_PLD_PCIE_TEST
	mc_init(TCM_CFG_BASE, 4);

#if SEEHI_C2C_PCIE_TEST
	gpio_sync_init();
#endif
	// s_pcie.dev = &g_pcieDevX8_03;
	// s_pcie.dev = &g_pcieDevX16_03;
#if PLD_Z1
	s_pcie.dev = &g_pcieDevX16_03;
	// s_pcie.dev = &g_pcieDevX16toX8_03;
	// s_pcie.dev = &g_pcieDevX8_03;
#elif PLD_Z2
	s_pcie.dev = &g_pcieDevX16_73;
#else
	#error
#endif  //PLD_Z1

#else
	#error
#endif  //SEEHI_FPGA_PCIE_TEST

	systimer_init();

	GIC_Init();

	printf("PCIe_RC_Init start !!!\n");

	PCIe_RC_Init(pcie);

	printf("PCIe_RC_Init end !!!\n");

#if	SEEHI_PLD_PCIE_TEST
	REG32(0x12000fe0)=0x4;
#endif

	while(1) {
		asm volatile ("nop");
	};

	return result;
}
