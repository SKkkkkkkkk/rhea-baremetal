#include <stdio.h>
#include <stdlib.h>
#include "gicv3.h"
#include "pcie.h"
#include "systimer.h"
#include "common.h"
#include "dw_apb_gpio.h"


#define SEEHI_PLD_PCIE_TEST			0
#define SEEHI_FPGA_PCIE_TEST		1
#define SEEHI_MSIX_ENABLE			0

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

#define PCIE_X8_2_IO_BASE				0x7fb80000
#define PCIE_X8_2_IO_BASE_SIZE			0x00080000
#define PCIE_X8_2_32_MEM_BASE			0xb0000000
#define PCIE_X8_2_32_MEM_BASE_SIZE		0x10000000
#define PCIE_X8_2_64_MEM_BASE			0xd820000000
#define PCIE_X8_2_64_MEM_BASE_SIZE		0x7e0000000


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

static inline void writel(uint32_t value, uint32_t address)
{
	uintptr_t addr = (uintptr_t)address;

	printf("writel addr 0x%lx value 0x%x\n", addr, value);
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

	printf("writeq addr 0x%lx value 0x%x\n", addr, value);
	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readq(uint64_t address)
{
	uintptr_t addr = (uintptr_t)address;
	uint32_t value = *((volatile uint32_t *)(addr));

	// printf("readq addr 0x%lx value 0x%x\n", addr, value);
	return value;
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
	uint32_t lanes = pcie->dev->max_lanes;
	uint32_t val;

	val = readq(apb_base + 0x100);  //RK有时钟相关配置我们是否也有
	val &= 0xfffffffe;
	writeq(val, apb_base + 0x100);  //disable app_ltssm_enable

#if  SEEHI_PLD_PCIE_TEST 

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


	// writeq(0, apb_base + 0x110);  //close fast link
	writeq(4, apb_base + 0x104);  //rc mode
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
		.gpio_mode = GPIO_Input_Mode,
		.pin = 3
	};
	gpio_init(&gpio_init_config);
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
	// struct PCIE_IDB_CFG *idb_cfg = (struct PCIE_IDB_CFG *)__pcie_idb_boot_cfg__;

	a510_pcie_ap_dniu(pcie);

	dw_pcie_dbi_ro_wr_en(dbi_base);

#if	SEEHI_PLD_PCIE_TEST
	// temp = readq(dbi_base + 0x8bc);
	// REG32(0x12000fec)=temp;
#endif

	BSP_PCIE_RC_Init(pcie);    //时钟同步，链路稳定，状态机进入polling，ltssm可以继续

	writeq(0x1, dbi_base + 0xa0);
	writeq(0x737c41, dbi_base + 0x7c); //LINK_CAPABILITIES_REG
	writeq(0x10120, dbi_base + 0x710);
	writeq(0x10100, dbi_base + 0x80c);
	writeq(0x4, dbi_base + 0x10);  //bar0
	writeq(0x0, dbi_base + 0x14);  //bar1
	writeq(0x1ff, dbi_base + 0x3c);  // interrrupt pin
	writeq(0xff0100, dbi_base + 0x18);
	writeq(0x100107, dbi_base + 0x4);  // cmd  bus & mem enable

	vid = 0x7368;    //sh
	did = 0xa530;    //a510
	writeq(did << 16 | vid, dbi_base + 0x00);  //vendor id & device id

	// writeq(0x00102130, dbi_base + 0x78);   //验证配置,DEVICE_CONTROL_DEVICE_STATUS 和MAX PAYLOAD SIZE 相关

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

	delay(1);

	// writeq(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	// writeq(0x4d004071, dbi_base + 0x8a8);  //GEN3_EQ_CONTROL_OFF

#if  SEEHI_PLD_PCIE_TEST
	HAL_PCIE_OutboundConfig(pcie, 1, PCIE_ATU_TYPE_MEM, PCIE_X8_2_32_MEM_BASE, PCIE_X8_2_32_MEM_BASE, PCIE_X8_2_32_MEM_BASE_SIZE);
	HAL_PCIE_OutboundConfig(pcie, 2, PCIE_ATU_TYPE_MEM, PCIE_X8_2_64_MEM_BASE, PCIE_X8_2_64_MEM_BASE, PCIE_X8_2_64_MEM_BASE_SIZE);
	HAL_PCIE_OutboundConfig(pcie, 3, PCIE_ATU_TYPE_IO, PCIE_X8_2_IO_BASE, 0, PCIE_X8_2_IO_BASE_SIZE);

#elif SEEHI_FPGA_PCIE_TEST
	dw_pcie_prog_outbound_atu(pcie, 1, PCIE_ATU_TYPE_MEM, PCIE_X8_2_32_MEM_BASE, PCIE_X8_2_32_MEM_BASE, PCIE_X8_2_32_MEM_BASE_SIZE);
	dw_pcie_prog_outbound_atu(pcie, 2, PCIE_ATU_TYPE_MEM, PCIE_X8_2_64_MEM_BASE, PCIE_X8_2_64_MEM_BASE, PCIE_X8_2_64_MEM_BASE_SIZE);
	dw_pcie_prog_outbound_atu(pcie, 3, PCIE_ATU_TYPE_IO, PCIE_X8_2_IO_BASE, 0, PCIE_X8_2_IO_BASE_SIZE);

#else

#error

#endif

	gpio_write_pin(PORTB, 3, GPIO_PIN_RESET);

	val = readq(apb_base + 0x100);  // 验证配置的0x25
	val |= 0x1;
	writeq(val, apb_base + 0x100);  //enable app_ltssm_enable

#if SEEHI_FPGA_PCIE_TEST
	// delay(1000);
	systimer_delay(100, IN_MS);
#endif

	gpio_write_pin(PORTB, 3, GPIO_PIN_SET);

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

#if SEEHI_FPGA_PCIE_TEST
		systimer_delay(1000, IN_US);
#endif
		// delay(1);
		timeout++;

		if (val != val_cmp) {
			val_cmp = val;
#if SEEHI_FPGA_PCIE_TEST
			printf("ctrl_link_status = 0x%x\n", val);
#endif
		}
	}

	if(phy_linkup == 0){
		BSP_First_Reset();
	}

#if SEEHI_FPGA_PCIE_TEST
	printf("Link up\n");

	/* Wait for link stable */   //RK 有特殊操作，会等一下等待Data link layer ok，我们是否需要等
	val = readq(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);

	val = readq(dbi_base + 0x00);  //vendor id & device id
	printf("read dbi vid 0x%x\n", val);
#endif
	/////////////////////////////////////END//////////////////////////////////////////////////////

	dw_pcie_dbi_ro_wr_dis(dbi_base);

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
	.gen = 3,
	.firstBusNo = 0x0,
	.legacyIrqNum = 0,
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
	.legacyIrqNum = 0,
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
	.legacyIrqNum = 0,
};

struct HAL_PCIE_HANDLE s_pcie;

int main()
{
	//C2C_ENGINE_X8 这个寄存器干什么用的
	// struct PCIE_IDB_CFG *idb_cfg = (struct PCIE_IDB_CFG *)__pcie_idb_boot_cfg__;
	uint32_t result = HAL_ERROR;
	struct HAL_PCIE_HANDLE *pcie = &s_pcie;

	g_c2c_base = get_pcie_base(3);

#if SEEHI_FPGA_PCIE_TEST
	s_pcie.dev = &g_pcieDevX8;

	systimer_init();

	GIC_Init();

#elif SEEHI_PLD_PCIE_TEST
	mc_init(TCM_CFG_BASE, 4);

	// s_pcie.dev = &g_pcieDevX8;
	s_pcie.dev = &g_pcieDevX16;
#endif

	gpio_perst_init();

#if SEEHI_FPGA_PCIE_TEST
	printf("PCIe_RC_Init start !!!\n");
#endif

	PCIe_RC_Init(pcie);

#if SEEHI_FPGA_PCIE_TEST
	printf("PCIe_RC_Init end !!!\n");
#endif

#if	SEEHI_PLD_PCIE_TEST
	REG32(0x12000fe0)=0x4;
#endif

	while(1) {
		asm volatile ("nop");
	};

	return result;
}
