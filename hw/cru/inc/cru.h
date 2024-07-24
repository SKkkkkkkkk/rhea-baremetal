#ifndef __CRU_H__
#define __CRU_H__

typedef enum {
    CLK_CFG,
    CLK_TILE,
    CLK_TCM,
    CLK_BOOTSPI,
    CLK_CPU_ACE,
    CLK_CPU_ATB,
    CLK_CPU_CORE0,
    CLK_CPU_CORE1,
    CLK_CPU_CORE2,
    CLK_CPU_CORE3,
    CLK_CPU_GIC,
    CLK_CPU_PERI,
    CLK_EFUSE,
    CLK_EMMC_2X,
    CLK_GMAC_PTP,
    CLK_GPIO_DB,
    CLK_GTIMER,
    CLK_HIPERI_AXI,
    CLK_I2C,
    CLK_PWM,
    CLK_SPACC,
    CLK_TIMER,
    CLK_UART,
    CLK_UART_DBG,
    CLK_WDT,
    CLK_VPU,
    CLK_CLCI,
    CLK_CLCI_CFG,
    CLK_CLCI_AXI
} ClkDev;

unsigned int get_clk(ClkDev clk_dev);

#endif // __CRU_H__
