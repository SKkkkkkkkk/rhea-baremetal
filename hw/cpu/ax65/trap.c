#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "irq.h"
#define INT_ID_MAX 1023
static irq_handler_t irq_table[INT_ID_MAX + 1] = {0};

void irq_set_handler(uint16_t int_id, irq_handler_t handler)
{
    assert(int_id <= INT_ID_MAX);
    irq_table[int_id] = handler;
}

irq_handler_t irq_get_handler(uint16_t int_id)
{
    assert(int_id <= INT_ID_MAX);
    return irq_table[int_id];
}

void __attribute__ ((interrupt ("machine"))) trap_entry()
{
    intptr_t mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));
    if (mcause >= 0) { // exception
        uintptr_t mepc;
        uintptr_t mtval;
        asm volatile("csrr %0, mepc" : "=r"(mepc));
        asm volatile("csrr %0, mtval" : "=r"(mtval));
        printf("trap epc: 0x%lx\n", mepc);
        printf("trap cause: 0x%lx\n", mcause);
        printf("trap tval: 0x%lx\n", mtval);
        while(1) asm volatile("wfi");
    } else if ((mcause & MCAUSE_CAUSE) == IRQ_M_EXT) { 
        // external interrupt
        uint16_t ext_id = __nds__plic_claim_interrupt();
        irq_handler_t handler = irq_get_handler(ext_id);
        if (handler) {
            handler();
        }
        __nds__plic_complete_interrupt(ext_id);
    } else if ((mcause & MCAUSE_CAUSE) == IRQ_M_TIMER) {
        // timer interrupt
    } else if ((mcause & MCAUSE_CAUSE) == IRQ_M_SOFT) {
        // software interrupt
    } else {
        // unknown interrupt
        printf("unknown interrupt\n");
        while(1) asm volatile("wfi");
    }
}