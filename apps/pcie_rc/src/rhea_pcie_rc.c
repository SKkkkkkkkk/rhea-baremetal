#include <stdio.h>
#include <stdlib.h>

#include "gicv3.h"
#include "rhea_pcie_rc.h"
#include "systimer.h"
#include "common.h"
#include "dw_apb_gpio.h"
#include "utils_def.h"
#include "dw_apb_timers.h"

#include "lpi.h"

/*                                   This is BAR Define
┌────┬─────┬────────────────┬────┬───────────────────┬────────────────┬───────────────┬───────────┐
│BARn│ Size│is Prefetchable?│bits│       name        │   assignment   │initial inbound│use inbound│
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR0│  4MB│non-prefetchable│ 32 │    NPU_S2_BAR     │     NPU S2     │0x104_1100_0000│     0     │
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR1│  1MB│non-prefetchable│ 32 │   PCIE_DBI_BAR    │    PCIe DBI    │hardware assign│do not care│
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR2│  4MB│non-prefetchable│ 32 │TILE_OR_TCM_CFG_BAR│tile cfg/tcm cfg│0x004_4000_0000│     1     │
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR3│ 64KB│non-prefetchable│ 32 │    HDMA_LL_BAR    │ HDMA Link List │0x004_0000_0000│     2     │
├────┼─────┼────────────────┼────┼───────────────────┼────────────────┼───────────────┼───────────┤
│BAR4│ 16GB│  prefetchable  │ 64 │    TCM_MEM_BAR    │    TCM MEM     │0x004_4000_0000│     3     │
└────┴─────┴────────────────┴────┴───────────────────┴────────────────┴───────────────┴───────────┘
*/

static uint64_t g_c2c_base;		// select c2c control
static uint64_t g_bar4_base;		// real bar4
static uint64_t g_bar4_size;		// real bar4
static uint8_t g_c2c_link;			// select link c2c 1: c2c  0: host
static struct c2c_ranges g_c2c_ranges;

/********************* Private Structure Definition **************************/

/********************* Private Variable Definition ***************************/

/********************* Private MACRO Definition ******************************/

#define PCIE_CLIENT_LTSSM_STATUS 0x150
#define SMLH_LINKUP              BIT(0)
#define RDLH_LINKUP              BIT(1)

#define PCIE_ATU_OFFSET 0x30000

#define PCIE_ATU_ENABLE          BIT(31)
#define PCIE_ATU_BAR_MODE_ENABLE BIT(30)

#define PCIE_ATU_UNR_REGION_CTRL1 0x00
#define PCIE_ATU_UNR_REGION_CTRL2 0x04
#define PCIE_ATU_UNR_LOWER_BASE   0x08
#define PCIE_ATU_UNR_UPPER_BASE   0x0C
#define PCIE_ATU_UNR_LOWER_LIMIT  0x10
#define PCIE_ATU_UNR_LOWER_TARGET 0x14
#define PCIE_ATU_UNR_UPPER_TARGET 0x18
#define PCIE_ATU_UNR_UPPER_LIMIT  0x20

#define LINK_WAIT_IATU 9

#define PCIE_DMA_OFFSET 0x380000

#define PCIE_DMA_WR_ENB        0xc
#define PCIE_DMA_WR_CTRL_LO    0x200
#define PCIE_DMA_WR_CTRL_HI    0x204
#define PCIE_DMA_WR_XFERSIZE   0x208
#define PCIE_DMA_WR_SAR_PTR_LO 0x20c
#define PCIE_DMA_WR_SAR_PTR_HI 0x210
#define PCIE_DMA_WR_DAR_PTR_LO 0x214
#define PCIE_DMA_WR_DAR_PTR_HI 0x218
#define PCIE_DMA_WR_WEILO      0x18
#define PCIE_DMA_WR_WEIHI      0x1c
#define PCIE_DMA_WR_DOORBELL   0x10
#define PCIE_DMA_WR_INT_STATUS 0x4c
#define PCIE_DMA_WR_INT_MASK   0x54
#define PCIE_DMA_WR_INT_CLEAR  0x58

#define PCIE_DMA_RD_ENB        0x2c
#define PCIE_DMA_RD_CTRL_LO    0x300
#define PCIE_DMA_RD_CTRL_HI    0x304
#define PCIE_DMA_RD_XFERSIZE   0x308
#define PCIE_DMA_RD_SAR_PTR_LO 0x30c
#define PCIE_DMA_RD_SAR_PTR_HI 0x310
#define PCIE_DMA_RD_DAR_PTR_LO 0x314
#define PCIE_DMA_RD_DAR_PTR_HI 0x318
#define PCIE_DMA_RD_WEILO      0x38
#define PCIE_DMA_RD_WEIHI      0x3c
#define PCIE_DMA_RD_DOORBELL   0x30
#define PCIE_DMA_RD_INT_STATUS 0xa0
#define PCIE_DMA_RD_INT_MASK   0xa8
#define PCIE_DMA_RD_INT_CLEAR  0xac

// #define BIT(nr) (1 << (nr))
/********************* Private Structure Definition **************************/

/********************* Private Variable Definition ***************************/

/********************* Private Function Definition ***************************/
static void HAL_DelayUs2(uint32_t count)
{
	uint32_t i, j, k;

	if(count > 1000)
		count = 1000;

	for(i = 0; i < count; i++)
		for(j = 0; j < 1000; j++)
			for(k = 0; k < 1000; k++);
}

static inline void HAL_PCIE_DbiWritel2(struct HAL_PCIE_DEV *dev, uint32_t reg, uint32_t val)
{
	// printf("seehi--> %s line: %d (0x%08x , 0x%lx)\n", __func__, __LINE__, val, dev->dbiBase + reg);
	writel(val, dev->dbiBase + reg);
}

static inline uint32_t HAL_PCIE_DbiReadl2(struct HAL_PCIE_DEV *dev, uint32_t reg)
{
	return readl(dev->dbiBase + reg);
}

static inline void HAL_PCIE_ApbWritel2(struct HAL_PCIE_DEV *dev, uint32_t reg, uint32_t val)
{
	writel(val, dev->apbBase + reg);
}

static inline uint32_t HAL_PCIE_ApbReadl2(struct HAL_PCIE_DEV *dev, uint32_t reg)
{
	return readl(dev->apbBase + reg);
}

static int32_t __attribute__((unused)) HAL_PCIE_GetFreeOutboundAtu2(struct HAL_PCIE_DEV *dev)
{
	char i;
	uint32_t off;

	for (i = 0; i < 8; i++) {
		off = PCIE_ATU_OFFSET + 0x200 * i;
		if (!(HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_UNR_REGION_CTRL2) & PCIE_ATU_ENABLE)) {
			return i;
		}
	}

	return -1;
}

static HAL_Check HAL_PCIE_LinkUp2(struct HAL_PCIE_DEV *dev)
{
	uint32_t val;

	val = HAL_PCIE_ApbReadl2(dev, PCIE_CLIENT_LTSSM_STATUS);
	if ((val & (RDLH_LINKUP | SMLH_LINKUP)) == 0x3) {
		return HAL_TRUE;
	}

	return HAL_FALSE;
}

static uint32_t HAL_PCIE_GetLTSSM2(struct HAL_PCIE_DEV *dev)
{
	return HAL_PCIE_ApbReadl2(dev, PCIE_CLIENT_LTSSM_STATUS);
}

static HAL_Status HAL_PCIE_InboundConfig2(struct HAL_PCIE_DEV *dev, int32_t index, int32_t bar, uint64_t cpuAddr)
{
	uint32_t val, off;
	uint32_t i;

	if(index > 7){
		printf("index max 7\n");
		return HAL_ERROR;
	}

	off = PCIE_ATU_OFFSET + 0x200 * index + 0x100;
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_TARGET, cpuAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_TARGET, (cpuAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_REGION_CTRL1, 0);   //PCIE_ATU_TYPE_MEM
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_REGION_CTRL2, PCIE_ATU_ENABLE | PCIE_ATU_BAR_MODE_ENABLE | (bar << 8));
	for (i = 0; i < 5000; i++) {
		val = HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_UNR_REGION_CTRL2);
		if (val & PCIE_ATU_ENABLE) {
			return HAL_OK;
		}
		HAL_DelayUs2(LINK_WAIT_IATU);
	}

	return HAL_ERROR;
}

static HAL_Status HAL_PCIE_InboundConfig_addr2(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size)
{
	uint32_t val, off;
	uint32_t i;
	uint64_t limit_addr;

	if(index > 7){
		printf("index max 7\n");
		return HAL_ERROR;
	}
	limit_addr = busAddr + size - 1;

	off = PCIE_ATU_OFFSET + 0x200 * index + 0x100;
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_BASE, busAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_BASE, (busAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_LIMIT, (limit_addr) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_LIMIT, (limit_addr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_TARGET, cpuAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_TARGET, (cpuAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_REGION_CTRL1, type);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_REGION_CTRL2, PCIE_ATU_ENABLE);

	for (i = 0; i < 5000; i++) {
		val = HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_UNR_REGION_CTRL2);
		if (val & PCIE_ATU_ENABLE) {
			return HAL_OK;
		}
		HAL_DelayUs2(LINK_WAIT_IATU);
	}

	return HAL_ERROR;
}

static HAL_Status HAL_PCIE_OutboundConfig2(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size)
{
	uint32_t val, off;
	int32_t i;
	uint64_t limit_addr;

	if(index > 7){
		printf("index max 7\n");
		return HAL_ERROR;
	}

	limit_addr = cpuAddr + size - 1;

	off = PCIE_ATU_OFFSET + 0x200 * index;
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_BASE, cpuAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_BASE, (cpuAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_LIMIT, (limit_addr) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_LIMIT, (limit_addr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_LOWER_TARGET, busAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_UPPER_TARGET, (busAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_REGION_CTRL1, type);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UNR_REGION_CTRL2, PCIE_ATU_ENABLE);
	for (i = 0; i < 5000; i++) {
		val = HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_UNR_REGION_CTRL2);
		if (val & PCIE_ATU_ENABLE) {
			return HAL_OK;
		}
		HAL_DelayUs2(LINK_WAIT_IATU);
	}

	return HAL_ERROR;
}

#define PCIE_ATU_VIEWPORT		0x900
#define PCIE_ATU_REGION_INBOUND		BIT(31)
#define PCIE_ATU_REGION_OUTBOUND	0
#define PCIE_ATU_CR1			0x904
// #define PCIE_ATU_INCREASE_REGION_SIZE	BIT(13)
// #define PCIE_ATU_TYPE_MEM		0x0
// #define PCIE_ATU_TYPE_IO		0x2
// #define PCIE_ATU_TYPE_CFG0		0x4
// #define PCIE_ATU_TYPE_CFG1		0x5
// #define PCIE_ATU_TD			BIT(8)
// #define PCIE_ATU_FUNC_NUM(pf)           ((pf) << 20)
#define PCIE_ATU_CR2			0x908
// #define PCIE_ATU_ENABLE			BIT(31)
// #define PCIE_ATU_BAR_MODE_ENABLE	BIT(30)
// #define PCIE_ATU_FUNC_NUM_MATCH_EN      BIT(19)
#define PCIE_ATU_LOWER_BASE		0x90C
#define PCIE_ATU_UPPER_BASE		0x910
#define PCIE_ATU_LIMIT			0x914
#define PCIE_ATU_LOWER_TARGET		0x918
#define PCIE_ATU_UPPER_TARGET		0x91C
#define PCIE_ATU_UPPER_LIMIT		0x924

static HAL_Status dw_pcie_prog_inbound_atu2(struct HAL_PCIE_DEV *dev, int32_t index, int32_t bar, uint64_t cpuAddr)
{
	uint32_t val, i;
	uint32_t off;

	off = 0x0;
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_VIEWPORT, PCIE_ATU_REGION_INBOUND |index);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LOWER_TARGET, cpuAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_TARGET, (cpuAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_CR1, PCIE_ATU_TYPE_MEM);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_CR2, PCIE_ATU_ENABLE | PCIE_ATU_BAR_MODE_ENABLE | (bar << 8));

	for (i = 0; i < 5000; i++) {
		val = HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_CR2);
		if (val & PCIE_ATU_ENABLE) {
			return HAL_OK;
		}
		HAL_DelayUs2(LINK_WAIT_IATU);
	}

	return HAL_ERROR;
}

static HAL_Status dw_pcie_prog_inbound_atu_addr2(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size)
{
	uint32_t val, off;
	uint32_t i;
	uint64_t limit_addr;

	if(index > 7){
		printf("index max 7\n");
		return HAL_ERROR;
	}
	limit_addr = busAddr + size - 1;

	off = 0x0;
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_VIEWPORT, PCIE_ATU_REGION_INBOUND |index);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LOWER_BASE, busAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_BASE, (busAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LIMIT, limit_addr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_LIMIT, (limit_addr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LOWER_TARGET, cpuAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_TARGET, (cpuAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_CR1, type);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_CR2, PCIE_ATU_ENABLE);

	for (i = 0; i < 5000; i++) {
		val = HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_CR2);
		if (val & PCIE_ATU_ENABLE) {
			return HAL_OK;
		}
		HAL_DelayUs2(LINK_WAIT_IATU);
	}

	return HAL_ERROR;
}

static HAL_Status dw_pcie_prog_outbound_atu2(struct HAL_PCIE_DEV *dev, int32_t index, int type, uint64_t cpuAddr, uint64_t busAddr, uint64_t size)
{
	uint32_t off;
	uint32_t val;
	uint64_t limit_addr;
	int32_t i;

	limit_addr = cpuAddr + size - 1;

	off = 0x0;
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_VIEWPORT, PCIE_ATU_REGION_OUTBOUND |index);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LOWER_BASE, cpuAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_BASE, (cpuAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LIMIT, limit_addr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_LIMIT, (limit_addr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_LOWER_TARGET, busAddr & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_UPPER_TARGET, (busAddr >> 32) & 0xFFFFFFFF);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_CR1, type);
	HAL_PCIE_DbiWritel2(dev, off + PCIE_ATU_CR2, PCIE_ATU_ENABLE);

	for (i = 0; i < 5000; i++) {
		val = HAL_PCIE_DbiReadl2(dev, off + PCIE_ATU_CR2);
		if (val & PCIE_ATU_ENABLE) {
			return HAL_OK;
		}
		HAL_DelayUs2(LINK_WAIT_IATU);
	}

	return HAL_ERROR;
}

/********************* Private Function Definition ***************************/

#if  SEEHI_PLD_PCIE_TEST
static void __attribute__((unused)) mc_init(uint64_t addr, uint8_t layer) {
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

static void delay(uint32_t value)
{
	volatile uint32_t i, j;

	for(i = 0; i < value; i++)
		for(j = 0; j < 1000; j++);
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

static uint64_t get_pcie_base(HAL_TileSelect pcie_sel)
{
	if( pcie_sel == TILE_02){
		return C2C_SYS_CFG_02;
	}else if(pcie_sel == TILE_03){
		return C2C_SYS_CFG_03;
	}else if(pcie_sel == TILE_72){
		return C2C_SYS_CFG_72;
	}else if(pcie_sel == TILE_73){
		return C2C_SYS_CFG_73;
	}else{
		printf("pcie_sel error !!!\n");
		return 0;
	}
}

static int get_pcie_local_info(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control)
{
	switch(tile){
	case TILE_02:
		if(control == HAL_X8){
			dev->firstBusNo = 0x20;
			dev->ltrIrqNum = 140;
			dev->vdmIrqNum = 141;
		}else{
			dev->firstBusNo = 0x10;
			dev->ltrIrqNum = 108;
			dev->vdmIrqNum = 109;
		}
		break;
	case TILE_03:
		if(control == HAL_X8){
			dev->firstBusNo = 0x40;
			dev->ltrIrqNum = 204;
			dev->vdmIrqNum = 205;
		}else{
			dev->firstBusNo = 0x30;
			dev->ltrIrqNum = 172;
			dev->vdmIrqNum = 173;
		}
		break;
	case TILE_72:
		if(control == HAL_X8){
			dev->firstBusNo = 0x60;
			dev->ltrIrqNum = 268;
			dev->vdmIrqNum = 269;
		}else{
			dev->firstBusNo = 0x50;
			dev->ltrIrqNum = 236;
			dev->vdmIrqNum = 237;
		}
		break;
	case TILE_73:
		if(control == HAL_X8){
			dev->firstBusNo = 0x80;
			dev->ltrIrqNum = 332;
			dev->vdmIrqNum = 333;
		}else{
			dev->firstBusNo = 0x70;
			dev->ltrIrqNum = 300;
			dev->vdmIrqNum = 301;
		}
		break;
	default:
		return -1;
	}
	return 0;
}

static uint64_t __attribute__((unused)) get_real_bar4(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control)
{
	switch(tile){
	case TILE_02:
		if(control == HAL_X8){
			g_bar4_base = PCIE_C2C_64_MEM_BASE_02_x8;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}else{
			g_bar4_base = PCIE_C2C_64_MEM_BASE_02_x16;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}
		break;
	case TILE_03:
		if(control == HAL_X8){
			g_bar4_base = PCIE_C2C_64_MEM_BASE_03_x8;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}else{
			g_bar4_base = PCIE_C2C_64_MEM_BASE_03_x16;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}
		break;
	case TILE_72:
		if(control == HAL_X8){
			g_bar4_base = PCIE_C2C_64_MEM_BASE_72_x8;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}else{
			g_bar4_base = PCIE_C2C_64_MEM_BASE_72_x16;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}
		break;
	case TILE_73:
		if(control == HAL_X8){
			g_bar4_base = PCIE_C2C_64_MEM_BASE_73_x8;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}else{
			g_bar4_base = PCIE_C2C_64_MEM_BASE_73_x16;
			g_bar4_size = PCIE_C2C_64_MEM_BASE_SIZE;
		}
		break;
	default:
		return -1;
	}

	g_bar4_base = (g_bar4_base & 0xfe00000000) | 0x400000000;
	// printf("seehi--> %s line: %d g_bar4_base 0x%lx\n", __func__, __LINE__, g_bar4_base);
	// printf("seehi--> %s line: %d g_bar4_size 0x%lx\n", __func__, __LINE__, g_bar4_size);
	return 0;
}

static uint64_t get_bar_rangs_info(struct c2c_ranges *c2c_ranges, HAL_TileSelect tile, HAL_ControlType control)
{
	c2c_ranges->tile = tile;
	c2c_ranges->control = control;

	switch(tile){
	case TILE_02:
		if(control == HAL_X8){
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_02_x8;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_02_x8;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_02_x8;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_02_x8;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_02_x8;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_02_x8;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_02_x8;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_02_x8;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_02_x8;
		}else{
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_02_x16;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_02_x16;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_02_x16;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_02_x16;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_02_x16;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_02_x16;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_02_x16;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_02_x16;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_02_x16;
		}
		break;
	case TILE_03:
		if(control == HAL_X8){
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_03_x8;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_03_x8;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_03_x8;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_03_x8;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_03_x8;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_03_x8;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_03_x8;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_03_x8;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_03_x8;
		}else{
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_03_x16;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_03_x16;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_03_x16;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_03_x16;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_03_x16;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_03_x16;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_03_x16;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_03_x16;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_03_x16;
		}
		break;
	case TILE_72:
		if(control == HAL_X8){
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_72_x8;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_72_x8;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_72_x8;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_72_x8;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_72_x8;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_72_x8;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_72_x8;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_72_x8;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_72_x8;
		}else{
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_72_x16;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_72_x16;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_72_x16;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_72_x16;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_72_x16;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_72_x16;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_72_x16;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_72_x16;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_72_x16;
		}
		break;
	case TILE_73:
		if(control == HAL_X8){
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_73_x8;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_73_x8;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_73_x8;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_73_x8;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_73_x8;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_73_x8;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_73_x8;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_73_x8;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_73_x8;
		}else{
			c2c_ranges->config_target = PCIE_C2C_CONFIG_TARGET_73_x16;
			c2c_ranges->config_base = PCIE_C2C_CONFIG_BASE_73_x16;
			c2c_ranges->config_base_size = PCIE_C2C_CONFIG_BASE_SIZE_73_x16;
			c2c_ranges->io_base = PCIE_C2C_IO_BASE_73_x16;
			c2c_ranges->io_base_size = PCIE_C2C_IO_BASE_SIZE_73_x16;
			c2c_ranges->mem32_base = PCIE_C2C_32_MEM_BASE_73_x16;
			c2c_ranges->mem32_base_size = PCIE_C2C_32_MEM_BASE_SIZE_73_x16;
			c2c_ranges->mem64_base = PCIE_C2C_64_MEM_BASE_73_x16;
			c2c_ranges->mem64_base_size = PCIE_C2C_64_MEM_BASE_SIZE_73_x16;
		}
		break;
	default:
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) seehi_pcie_ep_set_bar_flag(uint64_t dbi_base, uint32_t barno, int flags)
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

static int __attribute__((unused)) dw_pcie_ep_set_msix(uint64_t dbi_base, uint32_t interrupts, uint32_t bar_offset, uint32_t bir)
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
	val = (offset + (interrupts * PCI_MSIX_ENTRY_SIZE)) | bir;
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

static int __attribute__((unused)) dw_pcie_ep_set_msi(uint64_t dbi_base, uint32_t interrupts)
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

static int __attribute__((unused)) dw_pcie_ep_msi_32_data(uint64_t dbi_base)
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

static int a510_pcie_ap_dniu(const struct HAL_PCIE_DEV *dev)
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

static void BSP_First_Reset(void)
{
	printf("BSP_First_Reset\n");
}

static void BSP_PCIE_RC_Init(const struct HAL_PCIE_DEV *dev)
{
	uint64_t phy_base = dev->phyBase;
	uint64_t apb_base = dev->apbBase;
	uint64_t ss_base = dev->ssBase;
	uint32_t lanes = dev->max_lanes;
	uint32_t val;

	// printf("seehi--> %s line: %d phy_base 0x%lx\n", __func__, __LINE__, phy_base);
	// printf("seehi--> %s line: %d apb_base 0x%lx\n", __func__, __LINE__, apb_base);
	// printf("seehi--> %s line: %d ss_base 0x%lx\n", __func__, __LINE__, ss_base);
	val = readl(apb_base + 0x100);  //
	val &= 0xfffffffe;
	writel(val, apb_base + 0x100);  //disable app_ltssm_enable

#if  SEEHI_PLD_PCIE_TEST
	if(lanes == 16){
		writel(0, phy_base + 0x0);  //bif_en X16
		writel(0, phy_base + 0x94);  //pipe8_lane_mode  //
	}else if(lanes == 8){
		writel(1, phy_base + 0x0);  //bif_en X8
		writel(8, phy_base + 0x94);  //pipe8_lane_mode  //选phy clk
	}else{
		printf("PHY bifurcation error !\n");
	}

#if SEEHI_C2C_PCIE_TEST
	writel(0, ss_base + 0x18);  //reg_resetn_button_ctl_x16
	writel(0, ss_base + 0x1c);
	delay(10);
	writel(1, ss_base + 0x18);
	writel(1, ss_base + 0x1c);

	writel(3, phy_base + 0x18);	//pipe16_pclkchange_hs_en

	// val = readl(apb_base + 0x100);
	// val |= 1 << 3;
	// writel(val, apb_base + 0x100);  //app_hold_phy_rst
#endif
#elif SEEHI_FPGA_PCIE_TEST

#else
#endif


	writel(0, apb_base + 0x110);  //close fast link PCIE_DIAG_CTRL_BUS
	writel(4, apb_base + 0x104);  //rc mode

	if(g_c2c_ranges.control == HAL_X8){
		writel(g_c2c_ranges.config_base, ss_base + 0x208);
		writel(0x8, ss_base + 0x20c);
	}else{
		writel(g_c2c_ranges.config_base, ss_base + 0x200);
		writel(0x0, ss_base + 0x204);
	}

	// printf("seehi--> %s line: %d g_c2c_ranges.config_base 0x%x\n", __func__, __LINE__, g_c2c_ranges.config_base);
	writel(0x1ff, apb_base + 0xa0);
	writel(0x1, apb_base + 0x318);

}

static void rc_init_pre(struct HAL_PCIE_DEV *dev)
{
	uint32_t val;//, val_cmp = 0xf55a55aa;
	// uint32_t bar;
	uint64_t dbi_base = dev->dbiBase;
	// uint64_t apb_base = dev->apbBase;
	uint64_t ss_base = dev->ssBase;
	// uint32_t dniu_base = 0x11002000;
	// uint32_t mbitx_ap_base = 0x10050000;
	// uint64_t resbar_base;
	// int32_t i, timeout = 0, phy_linkup = 0;
	uint16_t vid, did;
	// uint32_t temp;
	uint64_t real_addr;

	BSP_PCIE_RC_Init(dev);    //

	writel(0x1, ss_base + 0x1b8);    //MSI MBI
	writel(0xffff0000, ss_base + 0x1cc);  //MBI MASK
	writel(g_c2c_ranges.config_target, ss_base + 0x1d0);
	writel(0x0, ss_base + 0x1d0);  //bugs todo

	val = readl(dbi_base + 0x708);
	val |= 0x400000;
	writel(val, dbi_base + 0x708);  // Poling Active to Poling x1 x4 x8

	writel(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0
	writel(0x00402200, dbi_base + 0x890);  //GEN3_RELATED_OFF.EQ_PHASE_2_3=0

	writel(0x01402200, dbi_base + 0x890);  //GEN4_RELATED_OFF.EQ_PHASE_2_3=0
	writel(0x01402200, dbi_base + 0x890);  //GEN4_RELATED_OFF.EQ_PHASE_2_3=0

	writel(0x02402200, dbi_base + 0x890);  //GEN5_RELATED_OFF.EQ_PHASE_2_3=0
	writel(0x02402200, dbi_base + 0x890);  //GEN5_RELATED_OFF.EQ_PHASE_2_3=0

	writel(0x4d004071, dbi_base + 0x8a8);  //GEN3_EQ_CONTROL_OFF

	writel(0x10001, dbi_base + 0x24);  //pref
	writel(0x536105, dbi_base + 0x7c); //LINK_CAPABILITIES_REG

	writel(0x3, dbi_base + 0x720);   //vdm msg drop

	writel(0x11110000, dbi_base + 0x30014);
	writel(0x11110000, dbi_base + 0x30214);
	writel(0x11110000, dbi_base + 0x30414);
	writel(0x11110000, dbi_base + 0x30614);
	writel(0x11110000, dbi_base + 0x30814);
	writel(0x11110000, dbi_base + 0x30a14);
	writel(0x11110000, dbi_base + 0x30c14);
	writel(0x11110000, dbi_base + 0x30e14);
	writel(0x11110000, dbi_base + 0x30114);
	writel(0x11110000, dbi_base + 0x30314);
	writel(0x11110000, dbi_base + 0x30514);
	writel(0x11110000, dbi_base + 0x30714);
	writel(0x11110000, dbi_base + 0x30914);
	writel(0x11110000, dbi_base + 0x30b14);
	writel(0x11110000, dbi_base + 0x30d14);
	writel(0x11110000, dbi_base + 0x30f14);

	val = (dev->firstBusNo + 0xf) << 16 | (dev->firstBusNo + 0x1) << 8 | dev->firstBusNo;
	writel(val, dbi_base + 0x18);

	writel(0x300310c8, dbi_base + 0x80c);
	writel(0x4, dbi_base + 0x10);  //bar0
	writel(0x0, dbi_base + 0x14);  //bar1
	writel(0x0, dbi_base + 0x10014);
	writel(0x1ff, dbi_base + 0x3c);  // interrrupt pin
									 //
	writel(0x100107, dbi_base + 0x4);  // cmd  bus & mem enable

	writel(0x7fffffff, dbi_base + 0x30014);
	writel(0x7fffffff, dbi_base + 0x30214);
	writel(0x7fffffff, dbi_base + 0x30414);
	writel(0x7fffffff, dbi_base + 0x30614);
	writel(0x7fffffff, dbi_base + 0x30814);
	writel(0x7fffffff, dbi_base + 0x30a14);
	writel(0x7fffffff, dbi_base + 0x30c14);
	writel(0x7fffffff, dbi_base + 0x30e14);

#if  SEEHI_PLD_PCIE_TEST
	HAL_PCIE_OutboundConfig2(dev, 0, PCIE_ATU_TYPE_CFG0, g_c2c_ranges.config_base, g_c2c_ranges.config_target, g_c2c_ranges.config_base_size);
	HAL_PCIE_OutboundConfig2(dev, 1, PCIE_ATU_TYPE_MEM, g_c2c_ranges.mem32_base, g_c2c_ranges.mem32_base, g_c2c_ranges.mem32_base_size);
	real_addr = g_c2c_ranges.mem64_base & 0x7ffffffff;
	printf("seehi--> %s line: %d real_addr 0x%lx\n", __func__, __LINE__, real_addr);
	HAL_PCIE_OutboundConfig2(dev, 2, PCIE_ATU_TYPE_MEM | 0x2000, real_addr, g_c2c_ranges.mem64_base, g_c2c_ranges.mem64_base_size);
	HAL_PCIE_OutboundConfig2(dev, 3, PCIE_ATU_TYPE_IO, g_c2c_ranges.io_base, g_c2c_ranges.io_base, g_c2c_ranges.io_base_size);

#elif SEEHI_FPGA_PCIE_TEST
	dw_pcie_prog_outbound_atu2(dev, 0, PCIE_ATU_TYPE_CFG0, g_c2c_ranges.config_base, g_c2c_ranges.config_target, g_c2c_ranges.config_base_size);
	dw_pcie_prog_outbound_atu2(dev, 1, PCIE_ATU_TYPE_MEM, g_c2c_ranges.mem32_base, g_c2c_ranges.mem32_base, g_c2c_ranges.mem32_base_size);
	real_addr = g_c2c_ranges.mem64_base & 0x7ffffffff;
	printf("seehi--> %s line: %d real_addr 0x%lx\n", __func__, __LINE__, real_addr);
	dw_pcie_prog_outbound_atu2(dev, 2, PCIE_ATU_TYPE_MEM | 0x2000, real_addr, g_c2c_ranges.mem64_base, g_c2c_ranges.mem64_base_size);
	dw_pcie_prog_outbound_atu2(dev, 3, PCIE_ATU_TYPE_IO, g_c2c_ranges.io_base, g_c2c_ranges.io_base, g_c2c_ranges.io_base_size);
#else

#error

#endif

	writel(0x0, dbi_base + 0x10);  //bar0
	writel(0x0, dbi_base + 0x10010);
	writew(0x604, dbi_base + 0xa);
	writel(0x300310c8, dbi_base + 0x80c);

	vid = 0x7368;    //sh
	did = 0xa510;    //a510
	writel(did << 16 | vid, dbi_base + 0x00);  //vendor id & device id

	// writel(0x00102130, dbi_base + 0x78);   //DEVICE_CONTROL_DEVICE_STATUS 和MAX PAYLOAD SIZE 相关

	switch (dev->gen) {  //gen speed
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
	dw_pcie_link_set_lanes(dbi_base, dev->lanes);  //lanes
	printf("pcie rc GEN %d, x%d\n", dev->gen, dev->lanes);
}


static void rc_rescan(struct HAL_PCIE_DEV *dev)
{
	uint32_t val;
	uint64_t dbi_base = dev->dbiBase;
	// uint64_t apb_base = dev->apbBase;
	// uint64_t ss_base = dev->ssBase;

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

	writew(0x0, g_c2c_ranges.config_base + 0x4);

	writel(0x0, g_c2c_ranges.config_base + 0x10);
	writel(0x0, g_c2c_ranges.config_base + 0x14);
	writel(0x0, g_c2c_ranges.config_base + 0x18);
	writel(0x0, g_c2c_ranges.config_base + 0x1c);
	writel(0xc, g_c2c_ranges.config_base + 0x20);
	writel(0x0, g_c2c_ranges.config_base + 0x24);

	writel(0x0, g_c2c_ranges.config_base + 0x30);
	writew(0x2110, g_c2c_ranges.config_base + 0x78);

	writew(0x8008, g_c2c_ranges.config_base + 0x44);

	writew(0x2, dbi_base + 0x3e);

	val = (dev->firstBusNo + 0xf) << 16 | (dev->firstBusNo + 0x1) << 8 | dev->firstBusNo;
	writel(val, dbi_base + 0x18);
	writel(g_c2c_ranges.mem32_base, dbi_base + 0x14);

	writew(0x0, g_c2c_ranges.config_base + 0x4);
	writel(g_c2c_ranges.mem32_base + 0x300000, g_c2c_ranges.config_base + 0x10);
	writel(g_c2c_ranges.mem32_base + 0x100000, g_c2c_ranges.config_base + 0x14);
	writel(g_c2c_ranges.mem32_base + 0x700000, g_c2c_ranges.config_base + 0x18);
	writel(g_c2c_ranges.mem32_base + 0x200000, g_c2c_ranges.config_base + 0x1c);
	writel(0xc, g_c2c_ranges.config_base + 0x20);
	val = (g_c2c_ranges.mem64_base >> 32 & 0xfe) | 0x4;
	writel(val, g_c2c_ranges.config_base + 0x24);
	printf("bar0 0x%x\n", g_c2c_ranges.mem32_base + 0x300000);
	printf("bar1 0x%x\n", g_c2c_ranges.mem32_base + 0x100000);
	printf("bar2 0x%x\n", g_c2c_ranges.mem32_base + 0x700000);
	printf("bar3 0x%x\n", g_c2c_ranges.mem32_base + 0x200000);
	printf("bar4 0x%lx\n", (g_c2c_ranges.mem64_base & 0xfe00000000) | 0x400000000);

	writel(0xffff, dbi_base + 0x30);
	writew(0xf0, dbi_base + 0x1c);
	writel(0x0, dbi_base + 0x30);

	val = ((g_c2c_ranges.config_base >> 16) + 0xf0) << 16 | ((g_c2c_ranges.config_base >> 16) + 0x20);
	writel(val, dbi_base + 0x20);
	writel(0xfff10001, dbi_base + 0x24);
	val = (g_c2c_ranges.mem64_base >> 32);
	writel(val, dbi_base + 0x28);
	val = (g_c2c_ranges.mem64_base >> 32) + 0x6;
	writel(val, dbi_base + 0x2c);

	writew(0x2, dbi_base + 0x3e);

	delay(1);

	writew(0x6, g_c2c_ranges.config_base + 0x4);

}

static void __attribute__((unused)) rc_init_msi_msg(struct HAL_PCIE_DEV *dev, uint32_t data)
{
	uint32_t pos = 0x50;
	uint32_t msgctl, control;

	msgctl = readw(g_c2c_ranges.config_base + pos + PCI_MSI_FLAGS);
	msgctl &= ~PCI_MSI_FLAGS_QSIZE;
	msgctl |= 5 << 4;
	writew(msgctl, g_c2c_ranges.config_base + pos + PCI_MSI_FLAGS);

	writel(0x105f0040, g_c2c_ranges.config_base + pos + PCI_MSI_ADDRESS_LO);
	writel(0x104, g_c2c_ranges.config_base + pos + PCI_MSI_ADDRESS_HI);
	writel(data, g_c2c_ranges.config_base + pos + PCI_MSI_DATA_64);

	control = readw(g_c2c_ranges.config_base + pos + PCI_MSI_FLAGS);
	control &= ~PCI_MSI_FLAGS_ENABLE;
	control |= PCI_MSI_FLAGS_ENABLE;
	writew(control, g_c2c_ranges.config_base + pos + PCI_MSI_FLAGS);

	return;
}

static void pci_msix_clear_and_set_ctrl(uint32_t pos, uint16_t clear, uint16_t set)
{
	uint16_t ctrl;

	ctrl = readw(g_c2c_ranges.config_base + pos + PCI_MSIX_FLAGS);
	ctrl &= ~clear;
	ctrl |= set;
	writew(ctrl, g_c2c_ranges.config_base + pos + PCI_MSIX_FLAGS);
}

static uint32_t pci_msix_desc_addr(struct HAL_PCIE_DEV *dev)
{
#if SEEHI_FPGA_PCIE_TEST
	printf("fpga no msix Function\n");
	return 0;
#elif SEEHI_PLD_PCIE_TEST
	return (g_c2c_ranges.mem32_base + 0x100000);
#endif
}

static void __attribute__((unused)) rc_init_msix_msg(struct HAL_PCIE_DEV *dev, uint32_t offset, uint32_t data)
{
	uint32_t pos = 0xb0;
	uint32_t bar1_base;

	bar1_base = pci_msix_desc_addr(dev);
	offset += 0x70000;

	writel(0x105f0040, bar1_base + offset + PCI_MSIX_ENTRY_LOWER_ADDR);
	writel(0x104, bar1_base + offset + PCI_MSIX_ENTRY_UPPER_ADDR);
	writel(data, bar1_base + offset + PCI_MSIX_ENTRY_DATA);
	writel(0, bar1_base + offset + PCI_MSIX_ENTRY_VECTOR_CTRL);

	pci_msix_clear_and_set_ctrl(pos, 0, PCI_MSIX_FLAGS_MASKALL |
				    PCI_MSIX_FLAGS_ENABLE);
	return;
}

static HAL_Status PCIe_RC_Init(struct HAL_PCIE_DEV *dev)
{
	uint32_t val, val_cmp = 0xf55a55aa;
	uint64_t __attribute__((unused)) val64;
	// uint32_t bar;
	uint64_t dbi_base = dev->dbiBase;
	uint64_t apb_base = dev->apbBase;
	// uint64_t ss_base = dev->ssBase;
	// uint32_t dniu_base = 0x11002000;
	// uint32_t mbitx_ap_base = 0x10050000;
	// uint64_t resbar_base;
	int32_t timeout = 0, phy_linkup = 0;
	// uint16_t vid, did;
	// uint32_t temp;
	// uint64_t real_addr;

	a510_pcie_ap_dniu(dev);

	dw_pcie_dbi_ro_wr_en(dbi_base);

	rc_init_pre(dev);

#if SEEHI_FPGA_PCIE_TEST
	gpio_write_pin(PORTB, 3, GPIO_PIN_RESET);
	delay(100);
	gpio_write_pin(PORTB, 3, GPIO_PIN_SET);

	gpio_write_pin(PORTA, 24, GPIO_PIN_RESET);
	delay(100);
	gpio_write_pin(PORTA, 24, GPIO_PIN_SET);
#else
	delay(20);
	gpio_write_pin(PORTA, 0, GPIO_PIN_RESET);
	delay(5);
	gpio_write_pin(PORTA, 0, GPIO_PIN_SET);
#endif

	val = readl(apb_base + 0x100);  //
	val |= 0x1;
	writel(val, apb_base + 0x100);  //enable app_ltssm_enable

#if SEEHI_PLD_PCIE_TEST && SEEHI_C2C_PCIE_TEST
	//only pld use
	val = readw(dbi_base + 0x70a);  // force phy link to state 1
	val |= 0x1;
	writew(val, dbi_base + 0x70a);  // force phy link to state 1

	val = readw(dbi_base + 0x708);  // force phy link to state 1
	val |= 0x8000;
	writew(val, dbi_base + 0x708);  // force phy link to state 1
	printf("force phy link to state 1\n");
#endif

	while (1) {   //判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readl(apb_base + 0x150);
		if ((val & 0xffff) == 0x110f) { //L0
			phy_linkup = 1;
			break;
		}

		if(timeout >= 1000000){
			timeout=0;
			// break;
		}

		delay(1);
		timeout++;

		if (val != val_cmp) {
			val_cmp = val;
			printf("rc--> ctrl_link_status = 0x%x\n", val);
		}
	}

	if(phy_linkup == 0){
		BSP_First_Reset();
	}

	printf("Link up\n");

	val = readl(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);

	val = readl(g_c2c_ranges.config_base + 0x80);
	printf("read LinkSta 0x%x\n", val);

	rc_rescan(dev);
	rc_init_msi_msg(dev, 0);
#if SEEHI_FPGA_PCIE_TEST
	delay(100);
#else
	delay(1);
#endif

#if 1
	dump_regs("read rc config space 00-ff:", dbi_base, 128);
	dump_regs("read ep config space 00-ff:", g_c2c_ranges.config_base, 128);

	dump_regs("bar0:", g_c2c_ranges.mem32_base + 0x300000, 32);
	dump_regs("bar2:", g_c2c_ranges.mem32_base + 0x700000, 32);
	dump_regs("bar3:", g_c2c_ranges.mem32_base + 0x200000, 32);
	val64 = (g_c2c_ranges.mem64_base & 0xfe00000000) | 0x400000000;
	dump_regs("bar4:", val64, 32);

#if SEEHI_PLD_PCIE_TEST && SEEHI_RC_TEST_HOT_RESET
	printf("rc set hot reset start test:\n");
	// writel(0x004201ff, dbi_base + 0x3c);  // hot reset
	// delay(1);
	// writel(0x000201ff, dbi_base + 0x3c);  // hot reset

	writel(0x1, apb_base + 0x108);  // hot reset
									//
	while (1) {   //判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readl(apb_base + 0x150);
		if ((val & 0xffff) == 0x1143) { //L0
			phy_linkup = 1;
			break;
		}

		if(timeout >= 1000000){
			timeout=0;
			// break;
		}

		delay(1);
		timeout++;

		if (val != val_cmp) {
			val_cmp = val;
			printf("ctrl_link_status = 0x%x\n", val);
		}
	}

	printf("retry link up\n");
	val = readl(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);
	val = readl(g_c2c_ranges.config_base + 0x80);
	printf("read LinkSta 0x%x\n", val);

	rc_rescan(dev);

	dump_regs("read rc config space 00-ff:", dbi_base, 128);
	dump_regs("read ep config space 00-ff:", g_c2c_ranges.config_base, 128);

	dump_regs("bar0:", g_c2c_ranges.mem32_base + 0x300000, 32);
	dump_regs("bar2:", g_c2c_ranges.mem32_base + 0x700000, 32);
	dump_regs("bar3:", g_c2c_ranges.mem32_base + 0x200000, 32);
	val64 = (g_c2c_ranges.mem64_base & 0xfe00000000) | 0x400000000;
	dump_regs("bar4:", val64, 32);

#endif  //hot reset

#if 1
	// dump_regs("vdm before:", apb_base + 0x130, 16);
	writel(0x4d, apb_base + 0x130);
	writel(0x7e0000, apb_base + 0x134);
	writel(0x12345678, apb_base + 0x138);
	writel(0x87654321, apb_base + 0x13c);
	writel(0x8000004d, apb_base + 0x130);
	// dump_regs("vdm after:", apb_base + 0x130, 16);
#endif  //vdm test

#endif
		/////////////////////////////////////END//////////////////////////////////////////////////////

	dw_pcie_dbi_ro_wr_dis(dbi_base);

	return HAL_OK;
}

static void gpio_sync_init(void)
{
#if SEEHI_PLD_PCIE_TEST
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
#endif  //SEEHI_PLD_PCIE_TEST
}

static int pcie_addr_resolution(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control)
{
	int ret = 0;

	g_c2c_base = get_pcie_base(tile);
	if(g_c2c_base == 0)
		return -1;

	ret = get_pcie_local_info(dev, tile, control);
	if(ret < 0)
		return -1;

	switch(control){
	case HAL_X16:
		dev->dbiBase = g_c2c_base + 0x000000;
		dev->engineBase = g_c2c_base + 0x140000;
		dev->apbBase = g_c2c_base + 0x180000;
		dev->max_lanes = 16;
		dev->lanes = 16;
		dev->gen = 5;
		dev->bif_en = 0;
		dev->pipe8 = 0;
		break;
	case HAL_X16toX8:
		dev->dbiBase = g_c2c_base + 0x000000;
		dev->engineBase = g_c2c_base + 0x140000;
		dev->apbBase = g_c2c_base + 0x180000;
		dev->max_lanes = 16;
		dev->lanes = 8;
		dev->gen = 5;
		dev->bif_en = 1;
		dev->pipe8 = 8;
		break;
	case HAL_X8:
		dev->dbiBase = g_c2c_base + 0x200000;
		dev->engineBase = g_c2c_base + 0x340000;
		dev->apbBase = g_c2c_base + 0x380000;
		dev->max_lanes = 8;
		dev->lanes = 8;
		dev->gen = 5;
		dev->bif_en = 1;
		dev->pipe8 = 8;
		break;
	default:
		return -1;
	}

	dev->phyBase = g_c2c_base + 0x400000;
	dev->ssBase = g_c2c_base + 0x580000;
	dev->drouterBase = g_c2c_base + 0x581000;
	dev->crouterBase = g_c2c_base + 0x582000;
	dev->dniuBase = g_c2c_base + 0x583000;
	dev->cniuBase = g_c2c_base + 0x584000;
	dev->mbitxBase = g_c2c_base + 0x585000;

	return 0;
}

int rhea_pcie_rc_init(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control, HAL_ModeSelect select)
{
	int ret = 0;

	if(select == C2C_EP){
		printf("C2C_EP control X%x\n", control);
		return -1;
	}

	if(select == C2C_RC){
		g_c2c_link = 1;
		printf("C2C_RC control X%x\n", control);
	}else{
		g_c2c_link = 0;
		printf("X86_EP control X%x\n", control);
	}

	ret = pcie_addr_resolution(dev, tile, control);
	if(ret < 0)
		return -1;

	ret = get_real_bar4(dev, tile, control);
	if(ret < 0)
		return -1;

	ret = get_bar_rangs_info(&g_c2c_ranges, tile, control);
	if(ret < 0)
		return -1;

	printf("PCIe_RC_Init start !!!\n");

	gpio_sync_init();

	PCIe_RC_Init(dev);

	printf("PCIe_RC_Init end !!!\n");

	return ret;
}
