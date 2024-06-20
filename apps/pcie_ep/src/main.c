#include <stdio.h>
#include <stdlib.h>
#include "gicv3.h"
#include "pcie_ep.h"
#include "systimer.h"


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
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x0           //AP SYS BOOTROM
#define BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS 0x8010000000  //AP SYS UART
#define BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS 0x540000000   //tile 0 5


#define AP_SYS_C2C0_CPU_ADDRESS		0x8100000000
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

/********************* Private Structure Definition **************************/

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/

static inline void writel(uint32_t address, uint32_t value)
{
	uintptr_t addr = (uintptr_t)address;

	*((volatile uint32_t *)(addr)) = value;
}

static inline uint32_t readl(uint32_t address)
{
	uintptr_t addr = (uintptr_t)address;

	return *((volatile uint32_t *)(addr));
}

static int seehi_pcie_ep_set_bar_flag(uint32_t dbi_base, uint32_t barno, int flags)
{
	uint32_t bar = barno;
	uint32_t reg, val;

	reg = PCI_BASE_ADDRESS_0 + (4 * bar);
	val = readl(dbi_base + reg);
	val &= 0xfffffff0;
	writel(flags | val, dbi_base + reg);

	return 0;
}

static void dw_pcie_link_set_max_speed(uint32_t dbi_base, uint32_t link_gen)
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

static void dw_pcie_link_set_lanes(uint32_t dbi_base, uint32_t lanes)
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

static inline void dw_pcie_dbi_ro_wr_en(uint32_t dbi_base)
{
	uint32_t reg;
	uint32_t val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = readl(dbi_base + reg);
	val |= PCIE_DBI_RO_WR_EN;
	writel(val, dbi_base + reg);
}

static inline void dw_pcie_dbi_ro_wr_dis(uint32_t dbi_base)
{
	uint32_t reg;
	uint32_t val;

	reg = PCIE_MISC_CONTROL_1_OFF;
	val = readl(dbi_base + reg);
	val &= ~PCIE_DBI_RO_WR_EN;
	writel(val, dbi_base + reg);
}

static int dw_pcie_ep_set_msix(uint32_t dbi_base, uint32_t interrupts)
{
	uint8_t offset = 0xb0;
	uint32_t val, reg;

	reg = offset;
	val = readl(dbi_base + reg);
	val &= ~(PCI_MSIX_FLAGS_QSIZE << 16);
	val |= interrupts << 16;
	dw_pcie_dbi_ro_wr_en(dbi_base);
	writel(val, dbi_base + reg);
	dw_pcie_dbi_ro_wr_dis(dbi_base);

	return 0;
}

/********************* Public Function Definition ****************************/

void BSP_PCIE_EP_Init(const struct HAL_PCIE_HANDLE *pcie)
{
	uint32_t phy_base = pcie->dev->phyBase;
	uint32_t apb_base = pcie->dev->apbBase;
	uint32_t lanes = pcie->dev->lanes;
	uint32_t val;

	val = readl(apb_base + 0x100);  //RK有时钟相关配置我们是否也有
	val &= 0xfffffffe;
	writel(val, apb_base + 0x100);  //disable app_ltssm_enable

	if(lanes == 16)
		writel(0, phy_base + 0x0);  //bif_en X16
	else if(lanes == 8)
		writel(1, phy_base + 0x0);  //bif_en X8
	else
		printf("PHY bifurcation error !");

	writel(0, phy_base + 0x94);  //pipe8_lane_mode  //验证配置，不知道干嘛的

	writel(0, apb_base + 0x104);  //ep mode
}

void BSP_First_Reset(void)
{
	printf("BSP_First_Reset\n");
}

HAL_Status PCIe_EP_Init(struct HAL_PCIE_HANDLE *pcie)
{
	uint32_t val, val_cmp = 0xf55a55aa;
	uint32_t bar;
	uint32_t dbi_base = pcie->dev->dbiBase;
	uint32_t apb_base = pcie->dev->apbBase;
	uint32_t ss_base = pcie->dev->ssBase;
	uint32_t dniu_base = 0x11002000;
	// uint32_t resbar_base;
	int32_t i, timeout = 0, phy_linkup = 0;
	uint16_t vid, did;
	// struct PCIE_IDB_CFG *idb_cfg = (struct PCIE_IDB_CFG *)__pcie_idb_boot_cfg__;

	BSP_PCIE_EP_Init(pcie);    //时钟同步，链路稳定，状态机进入polling，ltssm可以继续

	/*
	 * ltssm_enbale enhance mode and enable delaying the link training
	 * after Hot Reset
	 */
	// writel(0x120012, apb_base + 0x180);   //RK这块会有一个Hot Reset操作，我们是否有, 
	// 这块有个问题就是我们的EP 如果采用非标准模式，可以在内核中配置，理论上需要reset后才能进入状态机，这个怎么操作

	val = readl((dbi_base + 0x8BC)); //PCIE_MISC_CONTROL_1_OFF  看起来像是锁
	val |= 0x1;
	writel(val, dbi_base + 0x8BC);

	// reinit:
	writel(0, dbi_base + 0x10);  //bar地址配置
	writel(0, dbi_base + 0x14);
	writel(0, dbi_base + 0x18);
	writel(0, dbi_base + 0x1c);
	writel(0, dbi_base + 0x20);
	writel(0, dbi_base + 0x24);

	bar = 0;
	writel(0xf0000000, dbi_base + 0x10 + bar * 0x4);   //256M     RK 使用了RESBAR这个模块动态改size，我们是否支持
	// writel(0xc0000000, dbi_base + bar * 0x4);   //1024M
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

	bar = 1;
	writel(0xfff00000, dbi_base + 0x10 + bar * 0x4);   //1M
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

	bar = 2;
	writel(0x00000000, dbi_base + 0x10 + bar * 0x4);   //
	writel(0xffffffff, dbi_base + 0x10 + bar * 0x4 + 0x4);   //4G
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_64);   //64 没有预取

	bar = 4;
	writel(0x00000000, dbi_base + 0x10 + bar * 0x4);   //
	writel(0xfffffff0, dbi_base + 0x10 + bar * 0x4 + 0x4);   //64G
	seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_64);   //64 没有预取

	vid = 0x7368;    //sh
	did = 0xa510;    //a510
	writel(did << 16 | vid, dbi_base + 0x00);  //vendor id & device id

	writel(0x12000001, dbi_base + 0x08);  //class processing accelerators

	writel(0x0, dbi_base + 0x3c);  // interrrupt pin  no legacy interrupt Message
								   //
	writel(0x00102130, dbi_base + 0x78);   //验证配置,DEVICE_CONTROL_DEVICE_STATUS 和MAX PAYLOAD SIZE 相关

	val = readl(dbi_base + 0x7c);   //LINK_CAPABILITIES_REG  No ASPM Support
	val &= ~(3 << 10);
	writel(val, dbi_base + 0x7c);

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

	systimer_delay(100, IN_US);

	val = readl(apb_base + 0x100);  // 验证配置的0x25
	val |= 0x1;
	writel(val, apb_base + 0x100);  //enable app_ltssm_enable
	printf("Linking, ltssm:\n");

	while (1) {   //判断状态link up   这块RK用smlh_link_up和rdlh_link_up做判断，确认我们用smlh_ltssm_state吗？
		val = readl(apb_base + 0x150);
		if (((val >> 8) & 0x3f) == 0x11) { //L0
			phy_linkup = 1;
			break;
		}

		if(timeout >= 10000){
			break;
		}

		systimer_delay(10, IN_US);
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
	systimer_delay(300, IN_US);

	HAL_PCIE_InboundConfig(pcie, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);    //iATU 地址用0x3_0000 还是用 0x30_0000. 我们最大映射size是多少
																				//BAR1 是否需要软件映射
	HAL_PCIE_InboundConfig(pcie, 1, 2, BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS);    //CPU 的地址我用那个物理地址和虚拟地址需要不同映射吗，对于CNOC和DNOC的访问
	HAL_PCIE_InboundConfig(pcie, 2, 4, BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS);    //关于动态映射，每次只需要该inbound就可以吗,64G映射验证说需要特殊配置，具体是什么他没说清楚

	val = readl(dbi_base + 0x4);
	writel(val | 0x6, dbi_base + 0x4);   // cmd  bus & mem enable

	/* Wait for link stable */   //RK 有特殊操作，会等一下等待Data link layer ok，我们是否需要等
	printf("Link stable, ltssm\n");
	val = readl(apb_base + 0x150);
	printf("val = 0x%x\n", val);

	writel(0, ss_base + 0xac);    //msi_reshape_bypass_en = 0, msi_reshape_mode = 0 转发4tile
	writel(0, ss_base + 0xb4);    //msi_tile_id_0  0, 0
	writel(0, ss_base + 0xb8);    //msi_tile_id_1  0, 0
	writel(0, ss_base + 0xbc);    //msi_tile_id_2  0, 0
	writel(5, ss_base + 0xc0);    //msi_tile_id_3  0, 5

	val = (0 << 0) | (4 << 6) | (8 << 12) | (12 << 18) ;   //ap interrupt 0-15
	writel(val, ss_base + 0xc4);    //msi_st_tile_id0_evt_id0  0, 0

	val = (16 << 0) | (20 << 6) | (24 << 12) | (28 << 18) ;   //ap interrupt 16-31
	writel(val, ss_base + 0xc8);    //msi_st_tile_id1_evt_id0  0, 0

	val = (32 << 0) | (36 << 6) | (40 << 12) | (44 << 18) ;   //ap interrupt 32-47
	writel(val, ss_base + 0xcc);    //msi_st_tile_id2_evt_id0  0, 0

	val = (0 << 0) | (4 << 6) | (8 << 12) | (12 << 18) ;   //tile(0,5) 0-15
	writel(val, ss_base + 0xd0);    //msi_st_tile_id3_evt_id0  0, 5

	writel(0x40000000, dbi_base + 0x940);
	writel(0x81, dbi_base + 0x944);              //MSIX_ADDRESS_MATCH_EN  addr = 0x81_4000_0000

	writel(0xd0000000, ss_base + 0x9c);    // pcie_x8 outbound X16怎么配置，手动出发中断（tile_id+event_id）怎么和X86的MSIX 的0 1 2 3 ...等对应上

	dw_pcie_ep_set_msix(dbi_base, (16 * 4));  //msix table size 64 这个是否需要软件配置，有没有默认值

	writel(0x10, dniu_base + 0x4);    //验证配置代码，目前不知道用途, 验证的原话 “没有特别说明，现在联调的、更上层的没有说明。只有模块本身的说明，但是懂了模块本身的spec，就能推断出这些”
	writel(0x10008, dniu_base + 0x8);
	writel(0x0, dniu_base + 0xc);
	writel(0x1fffffff, dniu_base + 0x10);
	writel(0x0, dniu_base + 0x14);
	writel(0x1, dniu_base + 0x0);

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
	.lanes = 16,
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
	.lanes = 8,
	.gen = 3,
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

	s_pcie.dev = &g_pcieDevX8;

	systimer_init();

	GIC_Init();

	PCIe_EP_Init(pcie);

	while(1) {
		asm volatile ("nop");
	};

	return result;
}
