#include <stdio.h>
#include "gicv3.h"
#include "delay.h"
#include "pwm.h"
#include "pwm_test.h"
#include "dw_axi_dma.h"

#define WAIT_TIME (3 * 1000 * 1000) // 3秒
#define DST_ADDR_DMA 0x100000

volatile static uint32_t int_i = 0;
void pwm_irqhandler(void)
{
    uint32_t reg_val;
    ++int_i;

    PWM->intr_en = 0x0;
    PWM->intr_status &= ~(PWM->intr_status);
    PWM->intr_msk = 0x0;
}

void pwm_irq_init(void)
{
    GIC_Init(); //初始化GIC
    void pwm_irqhandler(void);
    IRQ_SetHandler(INT_PWM_INTR, pwm_irqhandler); // 添加中断函数
    GIC_SetPriority(INT_PWM_INTR, 0); // 配置中断优先级
    GIC_EnableIRQ(INT_PWM_INTR); // 使能中断
}

void pwm_irq_enable(void)
{
    int_i = 0;

    PWM->intr_msk = 0x1fffff;
    PWM->intr_en = 0x1;
}

void pwm_case_a_out_b_out()
{
    printf("enter case:%s\n", __FUNCTION__);

    /* 配置PWMA连续输出 */
    configurePwmAConf((pwm_regs*)PWM, 0x0, 0x0, 0x0, 0x2, 0x2,
                        0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0);
    /* 配置PWMB连续输出 */
    configurePwmBConf((pwm_regs*)PWM, 0x0, 0x0, 0x0, 0x2, 0x2,
                        0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0);

    /* 第一次配置channel A 1KHZ占空比 duty:40% */
    PWM->a_timer_unit = 0x1;
    PWM->a_timer_prd = 25000;
    PWM->a_timer_cmp1 = 10000;
    PWM->a_timer_cmp0 = 0x1;
    /* 第一次配置channel B 1KHZ占空比duty 60% */
    PWM->b_timer_unit = 0x1;
    PWM->b_timer_prd = 25000;
    PWM->b_timer_cmp1 = 15000;
    PWM->b_timer_cmp0 = 0x1;

    /* 输出通道a,b的信号 */
    PWM->pwm_out_sel = 0x1;
    configurePwmAEnableState((pwm_regs*)PWM, 1);
    configurePwmBEnableState((pwm_regs*)PWM, 1);
    udelay(WAIT_TIME);

    /* 第二次配置channel A 10KHZ占空比duty 60% */
    PWM->a_timer_prd = 2500;
    PWM->a_timer_cmp1 = 1500;
    PWM->a_timer_cmp0 = 0x1;
    /* 第二次配置channel B 10KHZ占空比duty 40% */
    PWM->b_timer_prd = 2500;
    PWM->b_timer_cmp1 = 1000;
    PWM->b_timer_cmp0 = 0x1;

    configurePwmAEnableState((pwm_regs*)PWM, 1);
    configurePwmBEnableState((pwm_regs*)PWM, 1);
    udelay(WAIT_TIME);

    /* 关闭PWM*/
    configurePwmAEnableState((pwm_regs*)PWM, 0);
    configurePwmBEnableState((pwm_regs*)PWM, 0);
    printf("exit case:%s\n", __FUNCTION__);
}

void pwm_case_a_out_b_in()
{
    printf("enter case:%s\n", __FUNCTION__);

    /* 配置PWMA连续输出 */
    configurePwmAConf((pwm_regs*)PWM, 0x0, 0x0, 0x0, 0x2, 0x2,
                        0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0);
    /* 配置PWMB为常规捕获模式 */
    configurePwmBConf((pwm_regs*)PWM, 0x3, 0x1, 0x0, 0x0, 0x0,
                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
    /* 第一次配置channel A 1KHZ 占空比 duty:40% */
    PWM->a_timer_unit = 0x1;
    PWM->a_timer_prd = 25000;
    PWM->a_timer_cmp1 = 10000;
    PWM->a_timer_cmp0 = 0x1;

    /* 输出PWM信号*/
    PWM->pwm_out_sel = 0x1;
    configurePwmAEnableState((pwm_regs*)PWM, 1);
    configurePwmBEnableState((pwm_regs*)PWM, 1);

    /* 获取chan B的捕获数据 */
    pwm_irq_enable();
    udelay(WAIT_TIME);
    printf("b_cap_lpc:0x%x,b_cap_hpc:0x%x\n", PWM->b_cap_lpc, PWM->b_cap_hpc);

    /* 关闭PWM*/
    configurePwmAEnableState((pwm_regs*)PWM, 0);
    configurePwmBEnableState((pwm_regs*)PWM, 0);
    printf("exit case:%s\n", __FUNCTION__);
}

void pwm_case_a_out_b_dma()
{
    printf("enter case:%s\n", __FUNCTION__);

    /* 配置PWMA连续输出 */
    configurePwmAConf((pwm_regs*)PWM, 0x0, 0x0, 0x0, 0x2, 0x2,
                        0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0);
    /* 配置PWMB连续输出为常规捕获模式 */
    configurePwmBConf((pwm_regs*)PWM, 0x3, 0x1, 0x0, 0x0, 0x0,
                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
    /* 第一次配置channel A 10KHZ 占空比 duty:40% */
    PWM->a_timer_unit = 0x1;
    PWM->a_timer_prd = 2500;
    PWM->a_timer_cmp1 = 1500;
    PWM->a_timer_cmp0 = 0x1;

    /* 开启dma传输，为b通道连接dma通道 */
    configureDMARegisters((pwm_regs*)PWM , 0x1, 0x0, 0x1);
    PWM->cap_dma_fifo_thr = 0x1;

    /* config dma chanel */
    dma_config_t config;
    DMA_Channel_t ch = 1;
    config.ch = ch;
    config.dir = PER_TO_MEM;
    config.sar = (uint64_t)&(PWM->cap_dma_fifo);
    config.dar = DST_ADDR_DMA;
    config.size = 0x10;
    config.is_src_addr_increse = SRC_ADDR_NOCHANGE;
    config.is_dst_addr_increse = DST_ADDR_INCREMENT;
    config.handle_shake = 12; // PWM和DMA的握手号为12
    dma_config(&config);

    /* 输出PWM信号*/
    PWM->pwm_out_sel = 0x1;
    configurePwmAEnableState((pwm_regs*)PWM, 1);
    configurePwmBEnableState((pwm_regs*)PWM, 1);

    dma_channel_start(ch);
    while (1) {
        if(is_dma_channel_transfer_done(ch)) {
            clear_channel_transfer_done_irq(ch);
            break;
        }
    }
    configureDMARegisters((pwm_regs*)PWM , 0x0, 0x0, 0x1);
    printf("cap_lpc:0x%x,cap_hpc:0x%x\n", REG32(DST_ADDR_DMA + 12), REG32(DST_ADDR_DMA + 8));

    /* 关闭PWM*/
    configurePwmAEnableState((pwm_regs*)PWM, 0);
    configurePwmBEnableState((pwm_regs*)PWM, 0);
    printf("exit case:%s\n", __FUNCTION__);
}

void pwm_case_b_out_a_in()
{
    printf("enter case:%s\n", __FUNCTION__);

    /* 配置PWMA为常规捕获模式 */
    configurePwmAConf((pwm_regs*)PWM, 0x3, 0x1, 0x0, 0x0, 0x0,
                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
    /* 配置PWMB连续输出 */
    configurePwmBConf((pwm_regs*)PWM, 0x0, 0x0, 0x0, 0x2, 0x2,
                        0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0);
    /* 第一次配置channel B 10KHZ 占空比 duty:60% */
    PWM->b_timer_unit = 0x1;
    PWM->b_timer_prd = 2500;
    PWM->b_timer_cmp1 = 1500;
    PWM->b_timer_cmp0 = 0x1;

    /* 输出PWM信号*/
    PWM->pwm_out_sel = 0x1;
    configurePwmAEnableState((pwm_regs*)PWM, 1);
    configurePwmBEnableState((pwm_regs*)PWM, 1);
    /* 获取chan A的捕获数据 */
    pwm_irq_enable();
    udelay(WAIT_TIME);
    printf("a_cap_lpc:0x%x,a_cap_hpc:0x%x\n", PWM->a_cap_lpc, PWM->a_cap_hpc);

    /* 关闭PWM*/
    configurePwmAEnableState((pwm_regs*)PWM, 0);
    configurePwmBEnableState((pwm_regs*)PWM, 0);
    printf("exit case:%s\n", __FUNCTION__);
}

void pwm_case_b_out_a_dma()
{
    printf("enter case:%s\n", __FUNCTION__);

    /* 配置PWMA为常规捕获模式 */
    configurePwmAConf((pwm_regs*)PWM, 0x3, 0x1, 0x0, 0x0, 0x0,
                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
    /* 配置PWMB连续输出 */
    configurePwmBConf((pwm_regs*)PWM, 0x0, 0x0, 0x0, 0x2, 0x2,
                        0x0, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0);
    /* 第一次配置channel B 10KHZ 占空比 duty:60% */
    PWM->b_timer_unit = 0x1;
    PWM->b_timer_prd = 2500;
    PWM->b_timer_cmp1 = 1500;
    PWM->b_timer_cmp0 = 0x1;

    /* 开启dma传输，为a通道连接dma通道 */
    configureDMARegisters((pwm_regs*)PWM , 0x1, 0x1, 0x1);
    PWM->cap_dma_fifo_thr = 0x1;
    /* config dma chanel */
    dma_config_t config;
    DMA_Channel_t ch = 2;
    config.ch = ch;
    config.dir = PER_TO_MEM;
    config.sar = (uint64_t)&(PWM->cap_dma_fifo);
    config.dar = DST_ADDR_DMA;
    config.size = 0x10;
    config.is_src_addr_increse = SRC_ADDR_NOCHANGE;
    config.is_dst_addr_increse = DST_ADDR_INCREMENT;
    config.handle_shake = 12; // PWM和DMA的握手号为12
    dma_config(&config);

    /* 输出PWM信号*/
    PWM->pwm_out_sel = 0x1;
    configurePwmAEnableState((pwm_regs*)PWM, 1);
    configurePwmBEnableState((pwm_regs*)PWM, 1);

    dma_channel_start(ch);
    while (1) {
        if(is_dma_channel_transfer_done(ch)) {
            clear_channel_transfer_done_irq(ch);
            break;
        }
    }
    configureDMARegisters((pwm_regs*)PWM , 0x0, 0x1, 0x1);
    printf("cap_lpc:0x%x,cap_hpc:0x%x\n", REG32(DST_ADDR_DMA + 12), REG32(DST_ADDR_DMA + 8));

    /* 关闭PWM*/
    configurePwmAEnableState((pwm_regs*)PWM, 0);
    configurePwmBEnableState((pwm_regs*)PWM, 0);
    printf("exit case:%s\n", __FUNCTION__);
}
