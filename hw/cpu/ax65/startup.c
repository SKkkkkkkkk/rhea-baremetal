#include <stdint.h>

#define HART_NUM 8

// External symbols from linker script
extern char _bss_start[];
extern char _bss_end[];
extern char _data_vma_start[];
extern char _data_vma_end[];
extern char _data_lma_start[];
extern char _stack_top[];
extern char _stack_start[];

// Forward declarations
void main(void);
static uint32_t get_hartid(void) __attribute__((naked));
static inline void cenv_init(void);
static void secondary_core_boot(void) __attribute__((noreturn));
static void trap_entry(void) __attribute__ ((interrupt ("machine")));

void __attribute__((naked, section(".text.init"))) 
_start(void) {
    // Set up Machine Trap-Vector Base-Address Register
    __asm__ volatile (
        "csrw mtvec, %0\n"
        :
        : "r"(&trap_entry)
    );

    // Enable FPU by setting mstatus.FS to initial state (01)
    __asm__ volatile (
        "li t0, 0x00002000\n"      // MSTATUS_FS_INITIAL
        "csrrs t0, mstatus, t0\n"  // Set FS bits in mstatus
    );

    // Get hardware thread ID
    uint32_t hartid = get_hartid();

    // Set up stack pointer for each core
    __asm__ volatile (
        "la t0, _stack_top\n"
        "la t1, _stack_start\n"
        "sub t0, t0, t1\n"         // t0 = total stack size
        "li t2, %0\n"              // t2 = HART_NUM
        "div t0, t0, t2\n"         // t0 = per-core stack size
        "mul t0, t0, %1\n"         // t0 = hartid * per-core stack size
        "la t1, _stack_top\n"
        "sub sp, t1, t0\n"         // sp = _stack_top - offset
        :
        : "i"(HART_NUM), "r"(hartid)
        : "t0", "t1", "t2"
    );

    // Route secondary cores to their boot handler
    if (hartid != 0) {
        secondary_core_boot();
    }

    // Initialize C environment
    cenv_init();

    // Jump to main function
    main();

#ifdef QEMU
    // Exit QEMU
    *((volatile uint32_t*)0x100000) = 0x5555;
#endif

    // Infinite loop if we return from main
    while (1)
        asm volatile("wfi");
}

// Get hart ID from mhartid CSR
static uint32_t get_hartid(void) {
    asm volatile("csrr a0, mhartid");
    asm volatile("ret");
}

// Secondary core boot function
static void secondary_core_boot(void) {
    while (1) {}
} 

static void cenv_init(void) {
    // clear BSS
    uintptr_t *bss_start = (uintptr_t *)_bss_start;
    uintptr_t *bss_end = (uintptr_t *)_bss_end;
    while (bss_start < bss_end) {
        *bss_start++ = 0;
    }

    // copy DATA
    uintptr_t *data_vma_start = (uintptr_t *)_data_vma_start;
    uintptr_t *data_vma_end = (uintptr_t *)_data_vma_end;
    uintptr_t *data_lma_start = (uintptr_t *)_data_lma_start;
    if (data_vma_start != data_lma_start) {
        while (data_vma_start < data_vma_end) {
            *data_vma_start++ = *data_lma_start++;
        }
    }
}

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