#ifndef __SYSTIMER_PORT_H__
#define __SYSTIMER_PORT_H__

#include "gicv3.h"
#include "dw_apb_timers.h"

#define SYSTIMER_USE_NUMS   8   //最多可同时使用的定时器数目
#define SYSTIMER_CNT_BITS   32  //CNT占的bit数
#define SYSTIMER_UP         0   //向下计数

#define SYSTIMER_CLOCK      25000000
#define SYSTIMER_CNT        (TIMERX6->Timer1CurrentValue)
#define SYSTIMER_CLEAR_INT_FLAG() ((void)TIMERX6->Timer1EOI)
#define systimer_stop()     timer_disable(Timerx6_T1)


// #ifdef NO_SYS
    #define portSYSTIMER_ENTER_CRITICAL()           ((uint32_t)0)
    #define portSYSTIMER_EXIT_CRITICAL(irq_mask)
// #else
//     #include "FreeRTOS.h"
//     #include "task.h"
//     #define portSYSTIMER_ENTER_CRITICAL()           (taskENTER_CRITICAL(),(uint32_t)0)
//     #define portSYSTIMER_EXIT_CRITICAL(irq_mask)    taskEXIT_CRITICAL()
// #endif

// #define portSYSTIMER_ENTER_CRITICAL()
// #define portSYSTIMER_EXIT_CRITICAL()


static inline void port_systimer_init(void)
{
    timer_init_config_t timer_init_config = {
                                                .int_mask = 0,
                                                .loadcount = 0xFFFFFFFFUL,
                                                // .timer_id = Timerx6_T1,
                                                .timer_mode = Mode_User_Defined
                                            };
    timer_init_config.timer_id = Timerx6_T1;
    timer_init(&timer_init_config);
    void SysTimer_IRQHandler(void);
    IRQ_SetHandler(32, SysTimer_IRQHandler);
    GIC_SetPriority(32, 0<<3);
    GIC_EnableIRQ(32);
    timer_enable(Timerx6_T1);
}

#endif
