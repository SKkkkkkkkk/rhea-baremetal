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

#define PCI_EXP_LNKCAP      12  /* Link Capabilities */
#define PCI_EXP_LNKCTL2     48  /* Link Control 2 */
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
#define BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS 0x3C000000
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
    uint32_t reg;

    reg = PCI_BASE_ADDRESS_0 + (4 * bar);

    /* Disabled the upper 32bits BAR to make a 64bits bar pair */
    // if (flags & PCI_BASE_ADDRESS_MEM_TYPE_64) {
        // writel(0, dbi_base + reg + 0x100000 + 4);
    // }

    writel(flags, dbi_base + reg);
    if (flags & PCI_BASE_ADDRESS_MEM_TYPE_64) {
        writel(0, dbi_base + reg + 4);
    }

    return 0;
}

static void dw_pcie_link_set_max_speed(uint32_t dbi_base, uint32_t link_gen)
{
    uint32_t cap, ctrl2, link_speed;
    uint8_t offset = 0x70;

    cap = readl(dbi_base + offset + PCI_EXP_LNKCAP);
    ctrl2 = readl(dbi_base + offset + PCI_EXP_LNKCTL2);
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
    val = readl(dbi_base + PCIE_PORT_LINK_CONTROL);
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

    /* Set link width speed control register */
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

/********************* Public Function Definition ****************************/

void BSP_PCIE_EP_Init(const struct HAL_PCIE_HANDLE *pcie)
{
    const struct HAL_PHY_SNPS_PCIE3_DEV *phy = (const struct HAL_PHY_SNPS_PCIE3_DEV *)pcie->dev->phy;
    int32_t i;
    uint32_t timeout = 500; // 500 * 10us
    void volatile *mmio = (void volatile *)0xFE8C0000;

    printf("snps pcie3phy FW update!\n");

    printf("controlelr initialed\n");
    /* S-Phy: waiting for phy locked */
    for (i = 0; i < timeout; i++) {
    }
    if (i >= timeout) {
    }
    printf("phy locked\n");
}

void BSP_First_Reset(void)
{
}

HAL_Status PCIe_EP_Init(struct HAL_PCIE_HANDLE *pcie)
{
    uint32_t val, val_cmp = 0xf55a55aa;
    uint32_t bar;
    uint32_t dbi_base = pcie->dev->dbiBase;
    uint32_t apb_base = pcie->dev->apbBase;
    uint32_t resbar_base;
    int32_t i, retries = 0, phy_linkup = 0;
    uint16_t vid, did;
    // struct PCIE_IDB_CFG *idb_cfg = (struct PCIE_IDB_CFG *)__pcie_idb_boot_cfg__;

    BSP_PCIE_EP_Init(pcie);    //时钟同步，链路稳定，状态机进入polling，ltssm可以继续
    /*
     * ltssm_enbale enhance mode and enable delaying the link training
     * after Hot Reset
     */
    writel(0x120012, apb_base + 0x180);  //host reset

    /* Unmask pm_turnoff_int */
    writel(0x04000000, apb_base + 0x18);  //unmask interrupt

    val = readl((dbi_base + 0x8BC));  //特殊配置
    printf("phy initialed\n");
    val |= 0x1;
    writel(val, dbi_base + 0x8BC);

reinit:
    writel(0, dbi_base + 0x10);  //bar地址配置
    writel(0, dbi_base + 0x14);
    writel(0, dbi_base + 0x18);
    writel(0, dbi_base + 0x1c);
    writel(0, dbi_base + 0x20);
    writel(0, dbi_base + 0x24);
    val = readl(dbi_base + 0x7c);   //LINK_CAPABILITIES_REG
    val &= ~(3 << 10);
    writel(val, dbi_base + 0x7c);

#if 0
    /* Resize BAR0 4M 32bits, BAR2 64M 64bits-pref, BAR4 1MB 32bits */
    resbar_base = dbi_base + PCI_RESBAR;   //Resizable BAR 我们好像没有
    bar = 0;
    writel(0x40, resbar_base + 0x4 + bar * 0x8);
    writel(0x2c0, resbar_base + 0x8 + bar * 0x8);
    seehi_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

    bar = 2;
    writel(0x400, resbar_base + 0x4 + bar * 0x8);
    writel(0x6c0, resbar_base + 0x8 + bar * 0x8);
    seehi_pcie_ep_set_bar_flag(dbi_base, bar,
                                  PCI_BASE_ADDRESS_MEM_PREFETCH | PCI_BASE_ADDRESS_MEM_TYPE_64);

    bar = 4;
    writel(0x10, resbar_base + 0x4 + bar * 0x8);
    writel(0xc0, resbar_base + 0x8 + bar * 0x8seehii_pcie_ep_set_bar_flag(dbi_base, bar, PCI_BASE_ADDRESS_MEM_TYPE_32);

#endif
    /* Disable BAR1 BAR5*/
    bar = 1;
    writel(0, dbi_base + 0x100000 + 0x10 + bar * 4);
    bar = 5;
    writel(0, dbi_base + 0x100000 + 0x10 + bar * 4);

    vid = 0x1d87;
    did = 0x356a;
    // if (idb_cfg->magic == PCIE_IDB_CFG_MAGIC && idb_cfg->vid) {
        // vid = idb_cfg->vid;
        // did = idb_cfg->did;
    // }
    writel(did << 16 | vid, dbi_base + 0x00);  //id

    writel(0x12000001, dbi_base + 0x08);  //class

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

    // EP mode
    // writel(0xf00000, apb_base);  //RK 自定义是能write
	systimer_delay(100, IN_US);

    if (retries) {
        writel(0x80008, apb_base + 0x180);  // Set app_dly2_done to enable app_ltssm_enable
    } else {
        writel(0xc000c, apb_base);      // Enable LTSSM
    }
    printf("Linking, ltssm:\n");

    // for (i = 0; i < 50000; i++) {
    while (1) {   //判断状态link up
        val = readl(apb_base + 0x300);
        if (((val & 0x3ffff) & ((0x3 << 16))) == 0x30000) {
            printf("val = 0x%x\n", val);
            break;
        }

        if (((val & 0x3ffff) & ((0x3 << 16))) == 0x10000) {
            phy_linkup = 1;
        }

        if (val == 0 && phy_linkup) {
            printf("Link jitter\n");
            BSP_First_Reset();
        }

		systimer_delay(10, IN_US);
        if (val != val_cmp) {
            printf("val = 0x%x\n", val);
            val_cmp = val;
        }
    }

    printf("Link up\n");
	systimer_delay(300, IN_US);

    HAL_PCIE_InboundConfig(pcie, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);

    val = readl(dbi_base + 0x4);
    writel(val | 0x6, dbi_base + 0x4);

    /* Wait for link stable */
    for (i = 0; i < 3000; i++) {
        /* hot reset */
        val = readl(apb_base + 0x10);
        if (val & 0x4) {
            writel(0x4, apb_base + 0x10);
            printf("Link hot reset\n");
            if (retries < 3) {
                retries++;
                goto reinit;
            } else {
                break;
            }
        }

        /* L2 */
        val = readl(apb_base + 0x4);
        if (val & 0x400) {
            writel(0x4, apb_base + 0x10);
            printf("in L2\n");
            BSP_First_Reset();
        }

		systimer_delay(100, IN_US);
    }
    printf("Link stable, ltssm\n");
    val = readl(apb_base + 0x300);
	printf("val = 0x%x\n", val);

    return HAL_OK;
}

#define PCIE3X2_DBI_BASE        0xF6000000 /* PCIe dbi base address */
#define GIC_DISTRIBUTOR_BASE    0xFD400000 /* GICD base address */
#define GIC_REDISTRIBUTOR_BASE  0xFD460000 /* GICR base address */
#define PCIE3X2_APB_BASE        0xFE280000 /* PCIe apb base address */
#define PCIE3PHY_GRF_BASE       0xFDCB8000 /* S-PHY_GRF base address */

struct HAL_PCIE_DEV g_pcieDev =
{
    .apbBase = PCIE3X2_APB_BASE,
    .dbiBase = PCIE3X2_DBI_BASE,
    .cfgBase = 0xF0000000,
    .lanes = 2,
    .gen = 3,
    .firstBusNo = 0x20,
    .legacyIrqNum = 1,
};

struct HAL_PCIE_HANDLE s_pcie;

int main()
{
    // struct PCIE_IDB_CFG *idb_cfg = (struct PCIE_IDB_CFG *)__pcie_idb_boot_cfg__;
    uint32_t result = HAL_ERROR;
    struct HAL_PCIE_HANDLE *pcie = &s_pcie;
    uint32_t val;

	systimer_init();

    /* Re-in pcie.bin */
    val = readl(g_pcieDev.apbBase + 0x300);
    if (((val & 0x3ffff) & ((0x3 << 16))) == 0x30000) {
        return -1;
    }

	GIC_Init();

    // if (idb_cfg->magic == PCIE_IDB_CFG_MAGIC) {
        // if (g_pcieDev.phy) {
            // ((struct HAL_PHY_SNPS_PCIE3_DEV *)(g_pcieDev.phy))->phyMode = (idb_cfg->bootconfig >> PCIE_IDB_CFG_PHY_MODE_SHIFT) & PCIE_IDB_CFG_PHY_MODE_MASK;
        // }
        // g_pcieDev.gen = (idb_cfg->bootconfig >> PCIE_IDB_CFG_GEN_SHIFT) & PCIE_IDB_CFG_GEN_MASK;
        // g_pcieDev.lanes = (idb_cfg->bootconfig >> PCIE_IDB_CFG_LANE_SHIFT) & PCIE_IDB_CFG_LANE_MASK;
    // }


    PCIe_EP_Init(pcie);

	while(1) {
		asm volatile ("nop");
	};
}
