#ifndef __PWM_TEST_H__
#define __PWM_TEST_H__

#include "pwm.h"

#define PWM ((volatile pwm_regs*)PWM_BASE)
#define INT_PWM_INTR (25 + 32)

void pwm_irq_init(void);

void pwm_case_a_out_b_out(void);
void pwm_case_a_out_b_in(void);
void pwm_case_a_out_b_dma(void);
void pwm_case_b_out_a_in(void);
void pwm_case_b_out_a_dma(void);

#endif /* __PWM_TEST_H__ */