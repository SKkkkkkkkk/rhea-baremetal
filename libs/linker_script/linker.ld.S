OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(entrypoint)

#include "memmap.h"

#ifdef QEMU
	#define __ROM_BASE VIRT_FLASH
	#define __ROM_SIZE 64M

	#define __SRAM_BASE VIRT_SECURE_MEM
	#define __SRAM_SIZE 16M

	#define __DDR_BASE VIRT_MEM
	#define __DDR_SIZE 0xC0000000 //3G
#else
	#define __ROM_BASE BOOTROM_BASE
	#define __ROM_SIZE 256K

	#define __SRAM_BASE APRAM_BASE
	#define __SRAM_SIZE 512K

	#define __DDR_BASE AP_DRAM_BASE
	#define __DDR_SIZE 384M
#endif

#if (MEM_SCHEME == MEM_SCHEME_ROM)
	#define RO_REGION ROM
	#define DATA_VMA_REGION SRAM
	#define DATA_LMA_REGION ROM
	#define BSS_REGION SRAM
	#define HEAP_REGION SRAM
	#define STACK_REGION SRAM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	#define RO_REGION SRAM
	#define DATA_VMA_REGION SRAM
	#define DATA_LMA_REGION SRAM
	#define BSS_REGION SRAM
	#define HEAP_REGION SRAM
	#define STACK_REGION SRAM
#elif (MEM_SCHEME == MEM_SCHEME_DRAM)
	#define RO_REGION DRAM
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
	.ro :
	{
		_ro_start = .;
		*entrypoint*(.text*)
		*(SORT_BY_ALIGNMENT(.text*))
		*(SORT_BY_ALIGNMENT(.rodata*))
		*(.vectors)
		_ro_end = .;
	} >RO_REGION

	.data :
	/* Make sure VMA & LMA alignment >= 32B, 
	   which allows us to use 16-byte load/store instructions.
	*/
	ALIGN(32) 
	{
		_data_start = .;
		*(SORT_BY_ALIGNMENT(.data*))
		. = ALIGN(32); /* Not only the address should be 32B aligned, but also the size */
		_data_end = .;
	} >DATA_VMA_REGION AT>DATA_LMA_REGION
	_data_start_lma = LOADADDR(.data);
	_data_size = SIZEOF(.data);

	.bss (NOLOAD) :
	/* Make sure VMA & LMA alignment >= 32B, 
	   which allows us to use 16-byte load/store instructions.
	*/
	ALIGN(32) 
	{
		_bss_start = .;
		*(SORT_BY_ALIGNMENT(.bss*))
		*(COMMON)
		. = ALIGN(32); /* Not only the address should be 32B aligned, but also the size */
		_bss_end = .;
	} >BSS_REGION

	.heap (NOLOAD) :
	{
		end = .;
		_heap_start = .;
		. += HEAP_SIZE;
		_heap_end = .;
	} >HEAP_REGION

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
