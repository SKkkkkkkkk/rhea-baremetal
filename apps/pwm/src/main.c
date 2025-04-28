#include <stdio.h>
#include "dw_apb_gpio.h"
#include "pwm.h"
#include "pwm_test.h"

void pwm_iomux_set_default(void)
{
    pinmux_select(PORTB, 7, 0); // PWMA
    pinmux_select(PORTB, 8, 0); // PWMB
}

int main (void)
{
    printf("enter main\n");

    pwm_iomux_set_default();
    pwm_irq_init();

    /*  测试时重新load,逐条测试 */
    pwm_case_a_out_b_out();
    // pwm_case_a_out_b_in();
    // pwm_case_a_out_b_dma();
    // pwm_case_b_out_a_in();
    // pwm_case_b_out_a_dma();

    return 0;
}

