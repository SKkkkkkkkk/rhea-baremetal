#include <stdio.h>
void __attribute__ ((interrupt ("machine"))) trap_entry()
{
    uintptr_t epc;
    uintptr_t cause;
    uintptr_t tval;
    asm volatile("csrr %0, mepc" : "=r"(epc));
    asm volatile("csrr %0, mcause" : "=r"(cause));
    asm volatile("csrr %0, mtval" : "=r"(tval));
    printf("trap epc: %lx\n", epc);
    printf("trap cause: %lx\n", cause);
    printf("trap tval: %lx\n", tval);
    if (cause == 2) {
        printf("illegal instruction\n");
        asm volatile("csrw mepc, %0" : : "r"(epc + 4));
    }
}