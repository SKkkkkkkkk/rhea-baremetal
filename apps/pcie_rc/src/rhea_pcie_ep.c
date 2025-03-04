#include <stdio.h>
#include <stdlib.h>

#include "gicv3.h"
#include "rhea_pcie_ep.h"
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
	writel(val, dev->dbiBase + reg);
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

static uint64_t get_real_bar4(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control)
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

static uint32_t get_doorbell_l_info(HAL_TileSelect tile, HAL_ControlType control, HAL_MbiSelect mbi)
{

	if(control == HAL_X8 && mbi != MBI_AP)
		return 0x50000000;
	else if(control == HAL_X16 && mbi != MBI_AP)
		return 0x40000000;
	else{
		switch(tile){
		case TILE_02:
		case TILE_72:
			if(control == HAL_X8){
				return 0x50000000;
			}else{
				return 0x40000000;
			}
			break;
		case TILE_03:
		case TILE_73:
			if(control == HAL_X8){
				return 0xd0000000;
			}else{
				return 0xc0000000;
			}
			break;
		default:
			return -1;
		}
	}
}

static uint32_t get_doorbell_h_info(HAL_TileSelect tile, HAL_ControlType control, HAL_MbiSelect mbi)
{

	if(mbi != MBI_AP){
		switch(tile){
		case TILE_02:
			return 0x02;
			break;
		case TILE_03:
			return 0x03;
			break;
		case TILE_72:
			return 0x72;
			break;
		case TILE_73:
			return 0x73;
			break;
		default:
			return -1;
		}
	}else{
		switch(tile){
		case TILE_02:
		case TILE_03:
			return 0x81;
			break;
		case TILE_72:
		case TILE_73:
			return 0xb9;
			break;
		default:
			return -1;
		}
	}
}

static uint64_t get_msi_rangs_info(struct c2c_ranges *c2c_ranges, HAL_TileSelect tile, HAL_ControlType control, HAL_MbiSelect mbi)
{
	c2c_ranges->tile = tile;
	c2c_ranges->control = control;

	c2c_ranges->doorbell_base_l = get_doorbell_l_info(tile, control, mbi);
	c2c_ranges->doorbell_base_h = get_doorbell_h_info(tile, control, mbi);

	if(mbi == MBI_AP){
		c2c_ranges->mbitx_base = 0x10050000;
	}else if(mbi == MBI_14){
		c2c_ranges->mbitx_base = 0x8a20000000;
	}else if(mbi == MBI_15){
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}else if(mbi == MBI_24){
		c2c_ranges->mbitx_base = 0x9220000000;
	}else if(mbi == MBI_25){
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}else if(mbi == MBI_53){
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}else if(mbi == MBI_54){
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}else if(mbi == MBI_63){
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}else if(mbi == MBI_64){
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}else{
		printf("seehi--> %s line: %d  mbi select error\n", __func__, __LINE__);
	}

	return 0;
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

static void BSP_PCIE_EP_Init(struct HAL_PCIE_DEV *dev)
{
	uint64_t phy_base = dev->phyBase;
	uint64_t apb_base = dev->apbBase;
	// uint32_t lanes = dev->max_lanes;
	uint64_t ss_base = dev->ssBase;
	uint8_t bif_en = dev->bif_en;
	uint8_t pipe8 = dev->pipe8;
	uint32_t val;

	// printf("seehi--> %s line: %d phy_base 0x%lx\n", __func__, __LINE__, phy_base);
	// printf("seehi--> %s line: %d apb_base 0x%lx\n", __func__, __LINE__, apb_base);
	// printf("seehi--> %s line: %d ss_base 0x%lx\n", __func__, __LINE__, ss_base);
	val = readl(apb_base + 0x100);
	val &= 0xfffffffe;
	writel(val, apb_base + 0x100);  //disable app_ltssm_enable

#if  SEEHI_PLD_PCIE_TEST
	writel(bif_en, phy_base + 0x0);  //bif_en X16
	writel(pipe8, phy_base + 0x94);  //pipe8_lane_mode  //

	if(g_c2c_link){
		writel(0, ss_base + 0x18);  //reg_resetn_button_ctl_x16
		writel(0, ss_base + 0x1c);
		delay(1);
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
	printf("BSP_First_Reset\n");
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

static HAL_Status PCIe_EP_Init(struct HAL_PCIE_DEV *dev)
{
	uint32_t val, temp = 0;//, val_cmp = 0xf55a55aa;
	uint32_t bar;
	uint64_t dbi_base = dev->dbiBase;
	uint64_t apb_base = dev->apbBase;
	// uint64_t ss_base = dev->ssBase;
	// uint32_t dniu_base = 0x11002000;
	// uint32_t mbitx_ap_base = 0x10050000;
	uint64_t resbar_base;
	// int32_t i, timeout = 0, phy_linkup = 0;
	uint16_t vid, did;

	dw_pcie_dbi_ro_wr_en(dbi_base);

	BSP_PCIE_EP_Init(dev);    //时钟同步，链路稳定，状态机进入polling，ltssm可以继续

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

	if(g_c2c_link)
		writel(0x20002, dbi_base + 0x2c);  //sub vendor id & device id
	else
		writel(0x10001, dbi_base + 0x2c);  //sub vendor id & device id
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
	printf("pcie ep GEN %d, x%d\n", dev->gen, dev->lanes);

	delay(1);

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

		delay(1);
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

static HAL_Status PCIe_EP_Link(struct HAL_PCIE_DEV *dev)
{
	uint32_t val, val_cmp = 0xf55a55aa;
	// uint32_t bar;
	uint64_t dbi_base = dev->dbiBase;
	uint64_t apb_base = dev->apbBase;
	uint64_t ss_base = dev->ssBase;
	// uint32_t dniu_base = 0x11002000;
	uint32_t mbitx_ap_base = 0x10050000;
	// uint64_t resbar_base;
	int32_t timeout = 0, phy_linkup = 0;
	// uint16_t vid, did;

	dw_pcie_dbi_ro_wr_en(dbi_base);

	while (1) {   //判断状态link up 用smlh_link_up和rdlh_link_up,smlh_ltssm_state
		val = readl(apb_base + 0x150);
		if ((val & 0xffff) == 0x1103) { //L0
			// printf("ep--> ctrl_link_status = 0x%x\n", val);
			phy_linkup = 1;
			break;
		}

		if(timeout >= 1000000){
			// printf("timeout !!!\n");
			timeout=0;
			// break;
		}

		delay(1);
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
		HAL_PCIE_InboundConfig_addr2(pcie, 0, 0, 0x10410000000, 0x1c800000, 0x400000);
		HAL_PCIE_InboundConfig_addr2(pcie, 1, 0, 0x16330000000, 0xf430000000, 0x800000);
		HAL_PCIE_InboundConfig_addr2(pcie, 2, 0, 0x6330800000, 0xf430800000, 0x20800000);

		HAL_PCIE_OutboundConfig2(pcie, 0, 0, 0x430000000, 0x16330000000, 0x800000);
		HAL_PCIE_OutboundConfig2(pcie, 1, 0, 0x430800000, 0x6330800000, 0x20800000);
		// writel(0x30000000, ss_base + 0x208);
		// writel(0x0, ss_base + 0x20c);
#else
		HAL_PCIE_InboundConfig2(dev, 0, 0, BOOT_USING_PCIE_C2C_BAR0_CPU_ADDRESS);
		HAL_PCIE_InboundConfig2(dev, 1, 2, BOOT_USING_PCIE_C2C_BAR2_CPU_ADDRESS);
		HAL_PCIE_InboundConfig2(dev, 2, 3, BOOT_USING_PCIE_C2C_BAR3_CPU_ADDRESS);

		HAL_PCIE_InboundConfig_addr2(dev, 3, 0, BOOT_USING_PCIE_C2C_BAR4_CPU_ADDRESS, g_bar4_base, g_bar4_size);
#endif
	}else{
		HAL_PCIE_InboundConfig2(dev, 0, 0, BOOT_USING_PCIE_EP_BAR0_CPU_ADDRESS);
		HAL_PCIE_InboundConfig2(dev, 1, 2, BOOT_USING_PCIE_EP_BAR2_CPU_ADDRESS);
		HAL_PCIE_InboundConfig2(dev, 2, 3, BOOT_USING_PCIE_EP_BAR3_CPU_ADDRESS);
		HAL_PCIE_InboundConfig2(dev, 3, 4, BOOT_USING_PCIE_EP_BAR4_CPU_ADDRESS);  // addr > bar size
	}

#elif SEEHI_FPGA_PCIE_TEST
	dw_pcie_prog_inbound_atu2(dev, 0, 0, BOOT_USING_PCIE_C2C_BAR0_CPU_ADDRESS);
	dw_pcie_prog_inbound_atu2(dev, 1, 2, BOOT_USING_PCIE_C2C_BAR2_CPU_ADDRESS);
	dw_pcie_prog_inbound_atu2(dev, 2, 3, BOOT_USING_PCIE_C2C_BAR3_CPU_ADDRESS);

	dw_pcie_prog_inbound_atu_addr2(dev, 3, 0, BOOT_USING_PCIE_C2C_BAR4_CPU_ADDRESS, g_bar4_base, g_bar4_size);
#else

#error

#endif

	val = readl(dbi_base + 0x4);
	writel(val | 0x6, dbi_base + 0x4);   // cmd  bus & mem enable

	val = readl(apb_base + 0x150);
	printf("Link stable, ltssm: 0x%x\n", val);

#if SEEHI_MSIX_ENABLE
	/////////////////////////////////////MSIX//////////////////////////////////////////////////////
	if(dev->max_lanes == 16){
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
	}else if(dev->max_lanes == 8){
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
		return HAL_ERROR;
	}
													//
	// writel(0x0, dbi_base + 0x948);              //0:10 vector

#else

	if(dev->max_lanes == 16){
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

		writel(0x0 << 0, ss_base + 0x194);    // pcie_x16 outbound bit0
		writel(0x40000000, ss_base + 0x198);    // pcie_x16 int_tx_msi_doorbell_x16
	}else if(dev->max_lanes == 8){
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
		return HAL_ERROR;
	}

	// writel((0 << 4), apb_base + 0x70);    //4:8  产生msi对应中断 bit0=1

#endif //SEEHI_MSIX_ENABLE

	writel(g_c2c_ranges.doorbell_base_l, g_c2c_ranges.mbitx_base + 0x10);    //AP 这边需要和doorbell地址能匹配上 x8
	writel(g_c2c_ranges.doorbell_base_h, g_c2c_ranges.mbitx_base + 0x14);    //AP 这边需要和doorbell地址能匹配上 x8

	writel(0xfffffffe, g_c2c_ranges.mbitx_base + 0x30);    //时能对应bit中断，总共32个bit
	writel(0x0, g_c2c_ranges.mbitx_base + 0x40);    //时能对应bit目标remote|local，总共32个bit

	   /////////////////////////////////////END//////////////////////////////////////////////////////
	dw_pcie_dbi_ro_wr_dis(dbi_base);

	pcie_writel_apb(dev, 0x1ff, A510_APB_PCIE_EN_INT0);
	pcie_writel_apb(dev, 0x1, A510_APB_PCIE_MSG_VDM_RX_MODE);

	return HAL_OK;
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

int rhea_pcie_ep_init(struct HAL_PCIE_DEV *dev, HAL_TileSelect tile, HAL_ControlType control, HAL_ModeSelect select, HAL_MbiSelect mbi)
{
	int ret = 0;

	if(select == C2C_RC){
		printf("C2C_RC control X%x tile 0x%x\n", control, tile);
		return -1;
	}

	if(select == C2C_EP){
		g_c2c_link = 1;
		printf("C2C_EP control X%x tile 0x%x\n", control, tile);
	}else{
		g_c2c_link = 0;
		printf("X86_EP control X%x tile 0x%x\n", control, tile);
	}

	ret = pcie_addr_resolution(dev, tile, control);
	if(ret < 0)
		return -1;

	ret = get_real_bar4(dev, tile, control);
	if(ret < 0)
		return -1;

	ret = get_msi_rangs_info(&g_c2c_ranges, tile, control, mbi);
	if(ret < 0)
		return -1;

	printf("PCIe_EP_Init start !!!\n");

	gpio_sync_init();

	PCIe_EP_Init(dev);
	PCIe_EP_Link(dev);

	printf("PCIe_EP_Init end !!!\n");

	return ret;
}
