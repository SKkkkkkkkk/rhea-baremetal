OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv",
       "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)
#include "memmap.h"

#ifdef VIRT
	#define __ROM_BASE VIRT_FLASH
	#define __ROM_SIZE 64M

	#define __SRAM_BASE 0x80000000
	#define __SRAM_SIZE 16M

	#define __DDR_BASE VIRT_DRAM
	#define __DDR_SIZE 0x80000000 //2G
#else
	#define __ROM_BASE BOOTROM_BASE
	#define __ROM_SIZE 256K

	#define __SRAM_BASE APRAM_BASE
	#define __SRAM_SIZE 512K

	#define __DDR_BASE AP_DRAM_BASE
	#define __DDR_SIZE 384M
#endif

#if (MEM_SCHEME == MEM_SCHEME_ROM)
	#define TEXT_REGION ROM
	#define DATA_VMA_REGION SRAM
	#define DATA_LMA_REGION ROM
	#define BSS_REGION SRAM
	#define HEAP_REGION SRAM
	#define STACK_REGION SRAM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	#define TEXT_REGION SRAM
	#define DATA_VMA_REGION SRAM
	#define DATA_LMA_REGION SRAM
	#define BSS_REGION SRAM
	#define HEAP_REGION SRAM
	#define STACK_REGION SRAM
#elif (MEM_SCHEME == MEM_SCHEME_DRAM)
	#define TEXT_REGION DRAM
	#define DATA_VMA_REGION DRAM
	#define DATA_LMA_REGION DRAM
	#define BSS_REGION DRAM
	#define HEAP_REGION DRAM
	#define STACK_REGION DRAM
#else
	#error "Invalid memory scheme"
#endif

#ifndef STACK_SIZE
	#define STACK_SIZE 0x4000 //16KB
#endif
#ifndef HEAP_SIZE
	#define HEAP_SIZE 0x4000 //16KB
#endif

MEMORY {
	ROM (rx):   ORIGIN = __ROM_BASE,    LENGTH = __ROM_SIZE
	SRAM (rwx): ORIGIN = __SRAM_BASE,   LENGTH = __SRAM_SIZE
	DRAM (rwx): ORIGIN = __DDR_BASE,    LENGTH = __DDR_SIZE
}

SECTIONS
{
    .text : {
        KEEP(*(.text.init))
        *(SORT_BY_ALIGNMENT(.text*))
    } > TEXT_REGION

    .rodata : {
		*(SORT_BY_ALIGNMENT(.srodata*))
        *(SORT_BY_ALIGNMENT(.rodata*))
    } > TEXT_REGION

    /* 这里添加DATA段的ROM地址标记，用于初始化 */
    _data_lma_start = LOADADDR(.data);

    .data : ALIGN(16) {
        _data_vma_start = .;    /* DATA段在RAM中的开始地址 */
        *(SORT_BY_ALIGNMENT(.sdata*))
        *(SORT_BY_ALIGNMENT(.data*))
        _data_vma_end = .;      /* DATA段在RAM中的结束地址 */
    } > DATA_VMA_REGION AT>DATA_LMA_REGION

    .bss : ALIGN(16) {
        _bss_start = .;     /* BSS段的开始地址 */
        *(SORT_BY_ALIGNMENT(.sbss*))
        *(SORT_BY_ALIGNMENT(.bss*))
        _bss_end = .;       /* BSS段的结束地址 */
    } > BSS_REGION

    . = ALIGN(8);
    _end = .;       /* 定义堆开始的位置 */
    
    /* 堆区 */
    .heap (NOLOAD) : {
        _heap_start = .;
        . = . + HEAP_SIZE;
        _heap_end = .; /* 定义堆结束的位置 */
    } > HEAP_REGION

	.stack (NOLOAD) :
	{
		_stack_start = .;
		. += STACK_SIZE;
		_stack_end = .;
		_stack_top = .;
	} >STACK_REGION

	_all_end = .;
	/DISCARD/ : { *(.eh_frame*) }
}
