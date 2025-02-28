#include <stdio.h>
#include <stdint.h>
#include "memmap.h"
#include "gicv3.h"
#include "common.h"

#include "hdma.h"

#if PLD_Z1
  #define HDMA_BASE   (0x18800000 + 0xb0000)
  #define HDMA_LL_PADDR 0x18300000
#elif PLD_Z2
  #define HDMA_BASE   (0x1c800000 + 0xb0000)
  #define HDMA_LL_PADDR 0x1c300000
#endif

#define HDMA_LL_SIZE 0x2000

#define HDMA_EN_OFF             0x0
#define HDMA_DOORBELL_OFF       0x4
#define HDMA_LLP_LOW_OFF        0x10
#define HDMA_LLP_HIGH_OFF       0x14
#define HDMA_CYCLE_OFF          0x18
#define HDMA_XFERSIZE_OFF       0x1c
#define HDMA_SAR_LOW_OFF        0x20
#define HDMA_SAR_HIGH_OFF       0x24
#define HDMA_DAR_LOW_OFF        0x28
#define HDMA_DAR_HIGH_OFF       0x2c
#define HDMA_CONTROL1_OFF       0x34
#define HDMA_STATUS_OFF         0x80
#define HDMA_INT_STATUS_OFF     0x84
#define HDMA_INT_SETUP_OFF      0x88
#define HDMA_INT_CLEAR_OFF      0x8C
#define HDMA_MSI_STOP_LOW_OFF   0x90
#define HDMA_MSI_STOP_HIGH_OFF  0x94
#define HDMA_MSI_ABORT_LOW_OFF  0xa0
#define HDMA_MSI_ABORT_HIGH_OFF 0xa4
#define HDMA_MSI_MSGD_OFF       0xa8

#define HDMA_MAX_CH   4
#define HDMA_ABORT_INT_MASK   BIT(2)
#define HDMA_STOP_INT_MASK    BIT(0)

uint32_t val;
uint32_t id;

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

void hdma_select_ch(void)
{
#if WR_CH0
    id = 0;
#elif  RD_CH0
    id = 1;
#elif  WR_CH1
    id = 2;
#elif  RD_CH1
    id = 3;
#endif
}

void hdma_write_data(uint64_t start_addr,uint64_t end_addr)
{
  uint32_t val = 0;
	
    for (uint64_t addr = start_addr; addr < end_addr; addr += 4) {
        writel(val, addr);
        val++;
    }
}

void hdma_core_off(void)
{
  hdma_select_ch();
  val = readl(HDMA_BASE + HDMA_INT_SETUP_OFF + id * 0x100);
  writel(val | HDMA_STOP_INT_MASK | HDMA_ABORT_INT_MASK,HDMA_BASE + HDMA_INT_SETUP_OFF + id * 0x100);

  val = readl(HDMA_BASE + HDMA_INT_CLEAR_OFF + id * 0x100);
  writel(val | HDMA_STOP_INT_MASK | HDMA_ABORT_INT_MASK,HDMA_BASE + HDMA_INT_CLEAR_OFF + id * 0x100);

  writel(0,HDMA_BASE + HDMA_EN_OFF + id * 0x100);

}

void hdma_core_ch_status(void)
{
  val = readl(HDMA_BASE + HDMA_STATUS_OFF + id * 0x100);
  printf("agic hdma chanel status 0x%x\n",val);

}

void dw_hdma_v0_core_status_int(void)
{
  val = readl(HDMA_BASE + HDMA_INT_STATUS_OFF + id * 0x100);
  printf("agic hdma int status 0x%x\n",val);
}

void dw_hdma_v0_core_handle_int(void)
{
  dw_hdma_v0_core_status_int();

  /*clear int*/
  writel(0x5 ,HDMA_BASE + HDMA_INT_CLEAR_OFF + id * 0x100);
}

void dw_hdma_v0_write_ll_data(void)
{
  writel(0x1,HDMA_LL_PADDR + (id * HDMA_LL_SIZE));//control
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE));

  writel(SIZE,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x4);//transfer size
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x4);

  writel(SAR_LOW_MASK,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x8);//sar low
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x8);

  writel(SAR_HIGH_MASK,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0xc);//sar high
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0xc);

  writel(DAR_LOW_MASK,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x10);//dar low 
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x10);

  writel(DAR_HIGH_MASK,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x14);//dar high
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x14);
}

void dw_hdma_v0_write_ll_link(void)
{

  writel(0x6,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x18);//control
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x18);

  writel(0x0,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x1c);

  writel(0x00600000 + (id * HDMA_LL_SIZE),HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x20);//llp low
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x20);

  writel(0x103,HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x24);//llp high
  readl(HDMA_LL_PADDR + (id * HDMA_LL_SIZE) + 0x24);
}

void dw_hdma_v0_core_start(void)
{
  dw_hdma_v0_write_ll_data();
  
  dw_hdma_v0_write_ll_link();
  
  /* Enable engine */
  writel(0x1 ,HDMA_BASE + HDMA_EN_OFF + id * 0x100);
  
  /* Interrupt enable&unmask - done, abort */
  writel(0x50 ,HDMA_BASE + HDMA_INT_SETUP_OFF + id * 0x100);
  
  /* Channel control */
  writel(0x1 ,HDMA_BASE + HDMA_CONTROL1_OFF + id * 0x100);
  
  /* Linked list */
  writel(0x00600000+ (id * HDMA_LL_SIZE) ,HDMA_BASE + HDMA_LLP_LOW_OFF + id * 0x100);
  writel(0x103 ,HDMA_BASE + HDMA_LLP_HIGH_OFF + id * 0x100);
  
  /* Set consumer cycle */
  writel(0x3 ,HDMA_BASE + HDMA_CYCLE_OFF + id * 0x100);
  
  readl(HDMA_LL_PADDR);//sync
  
  /* Doorbell */
  writel(0x1 ,HDMA_BASE + HDMA_DOORBELL_OFF + id * 0x100);
  
}

void dw_hdma_v0_core_ch_config(void)
{
  /* MSI done addr - low, high */
  writel(0x400 ,HDMA_BASE + HDMA_MSI_STOP_LOW_OFF + id * 0x100);
  writel(0x0 ,HDMA_BASE + HDMA_MSI_STOP_HIGH_OFF + id * 0x100);

  /* MSI abort addr - low, high */
  writel(0x400 ,HDMA_BASE + HDMA_MSI_ABORT_LOW_OFF + id * 0x100);
  writel(0x0 ,HDMA_BASE + HDMA_MSI_ABORT_HIGH_OFF + id * 0x100);

  /* config MSI data */
  writel(0,HDMA_BASE + HDMA_MSI_MSGD_OFF + id * 0x100);
}