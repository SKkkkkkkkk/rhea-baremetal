#include <stdio.h>
#include <stdint.h>
#include "cru.h"

const uint8_t fpga_clock[] = {
    24,         // clk_cfg
    24,         // clk_tile
    60,         // clk_tcm
    100,        // clk_bootspi
    60,         // clk_cpu_ace
    20,         // clk_cpu_atb
    60,         // clk_cpu_core0
    60,         // clk_cpu_core1
    60,         // clk_cpu_core2
    60,         // clk_cpu_core3
    20,         // clk_cpu_gic
    20,         // clk_cpu_peri
    10,         // clk_efuse
    100,        // clk_emmc_2x
    50,         // clk_gmac_ptp
    1,          // clk_gpio_db
    24,         // clk_gtimer
    50,         // clk_hiperi_axi
    25,         // clk_i2c
    25,         // clk_pwm
    60,         // clk_spacc
    25,         // clk_timer
    25,         // clk_uart
    24,         // clk_uart_dbg
    24,         // clk_wdt
    60,         // clk_vpu
    24,         // clk_clci
    24,         // clk_clci_cfg
    24,         // clk_clci_axi
};

unsigned int get_clk(ClkDev clk_dev)
{
    unsigned int d_freq;
    d_freq = fpga_clock[clk_dev] * 1000000;
    return d_freq;
}
