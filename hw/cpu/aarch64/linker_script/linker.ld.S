/*
 * The .data section gets copied from ROM to RAM at runtime.
 * Its LMA should be 16-byte aligned to allow efficient copying of 16-bytes
 * aligned regions in it.
 * Its VMA must be page-aligned as it marks the first read/write page.
 */
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

	#define __ROM_BASE A55_BOOTROM_BASE
	#define __ROM_SIZE 256K

	#define __SRAM_BASE APRAM_BASE
	#define __SRAM_SIZE 256K

	#define __DDR_BASE AP_DRAM_BASE
	#define __DDR_SIZE 0xC0000000 //3G

#endif

#define __STACK_SIZE 0x4000 //16KB
#define __HEAP_SIZE  0x4000 //16KB
#define PAGE_SIZE	4096

MEMORY {
	ROM (rx):   ORIGIN = __ROM_BASE,    LENGTH = __ROM_SIZE
	SRAM (rwx): ORIGIN = __SRAM_BASE,   LENGTH = __SRAM_SIZE
	DRAM (rwx): ORIGIN = __DDR_BASE,    LENGTH = __DDR_SIZE
}

SECTIONS
{
#if (MEM_SCHEME == MEM_SCHEME_ROM)
    . = ORIGIN(ROM);
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	. = ORIGIN(SRAM);
#else // DRAM
    . = ORIGIN(DRAM);
#endif
	ASSERT(. == ALIGN(PAGE_SIZE), "RO_BASE address is not aligned on a page boundary.")
	.text . :
	{
		__TEXT_START__ = .;
		*entrypoint*(.text*)
		*(SORT_BY_ALIGNMENT(.text*))
		*(.vectors)
		. = ALIGN(((1) << (12)));
		__TEXT_END__ = .;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.ARM.extab . : 
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.ARM.exidx . : 
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.gcc_except_table . : ONLY_IF_RO 
	{ 
		*(.gcc_except_table .gcc_except_table.*) 
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.preinit_array . : {
		. = ALIGN(8);
		__preinit_array_start = .;
		KEEP(*(.preinit_array))
		__preinit_array_end = .;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.init_array . : {
		. = ALIGN(8);
		__init_array_start = .;
		KEEP(*(.init_array))
		__init_array_end = .;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.fini_array . : {
		. = ALIGN(8);   
		__fini_array_start = .;
		KEEP(*(.fini_array))
		__fini_array_end = .;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


	.rodata . : 
	{
		__RODATA_START__ = .;
		*(SORT_BY_ALIGNMENT(.rodata*))
		__tinyCLI_CMD_START__ = .;
		KEEP(*(.tinyCLI_CMD*))
		__tinyCLI_CMD_END__ = .;
		__RODATA_END__ = .;
		. = ALIGN(16); //.data LMA should be 16-byte aligned.
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif


#if (MEM_SCHEME == MEM_SCHEME_ROM)
	. = ORIGIN(SRAM);
	. = ALIGN(PAGE_SIZE);
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	. = ALIGN(PAGE_SIZE);
#else // DRAM
	. = ALIGN(PAGE_SIZE);
#endif
	ASSERT(. == ALIGN(PAGE_SIZE), "RW_BASE address is not aligned on a page boundary.")
	.data . :
	ALIGN(16)
	{
		__DATA_START__ = .; 
		*(SORT_BY_ALIGNMENT(.data*))
		__DATA_END__ = .; 
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >SRAM AT>ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif

	.resource_table . : 
	{
		. = ALIGN(4);
		*(.resource_table)
		. = ALIGN(16);
		__RSC_TABLE_END__ = .;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >SRAM AT>ROM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else
	} >DRAM
#endif

	__RW_RAM_START__ = __DATA_START__;
	__RW_RAM_END__ = __RSC_TABLE_END__;
	ASSERT(__RW_RAM_END__ == ALIGN(16), "__RW_RAM_END__ address is not aligned on 16B.")

	.bss (NOLOAD) : ALIGN(16) 
	{
		__BSS_START__ = .;
		*(SORT_BY_ALIGNMENT(.bss*)) 
		*(COMMON)
		. = ALIGN(16);
		*(base_xlat_table)
		. = ALIGN(16);
		__BSS_END__ = .;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >SRAM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else // DRAM
	} >DRAM
#endif
	ASSERT(__BSS_END__ == ALIGN(16), "__BSS_END__ address is not aligned on 16B.")

	.xlat_table (NOLOAD) : 
	{
		__XLAT_TABLE_START__ = .; 
		*(xlat_table)
		__XLAT_TABLE_END__ = .; 
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >SRAM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else // DRAM
	} >DRAM
#endif

	.heap (NOLOAD): ALIGN(16)
	{
		__HEAP_START__ = .;
		end = __HEAP_START__;
		. = . + __HEAP_SIZE;
		__HEAP_END__ = .;
		__HEAPLIMIT = __HEAP_END__;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >SRAM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else // DRAM
	} >DRAM
#endif

	.stack (NOLOAD): ALIGN(16)
	{
		__STACKS_START__ = .;
		. = . + __STACK_SIZE;
		. = ALIGN(16);
		__STACKS_END__ = .;
		__STACK_TOP__ = __STACKS_END__;
#if (MEM_SCHEME == MEM_SCHEME_ROM)
	} >SRAM
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
	} >SRAM
#else // DRAM
	} >DRAM
#endif

	ASSERT(__STACK_TOP__ == ALIGN(16), "__STACK_TOP__ address is not aligned on 16B.")

	__RAM_START__ = ADDR(.data);
	__RAM_END__ = .;
	__RW_ROM_START__ = LOADADDR(.data);
	__RW_SIZE__ = __RW_RAM_END__ - __RW_RAM_START__;
	__BSS_SIZE__ = SIZEOF(.bss);

#if (MEM_SCHEME == MEM_SCHEME_ROM)
    __ROM_END__ = __RW_ROM_START__ + __RW_SIZE__;
    ASSERT(__ROM_END__ <= (__ROM_BASE + __ROM_SIZE), "ROM content has exceeded its limit.")
#elif (MEM_SCHEME == MEM_SCHEME_SRAM)
    ASSERT(__RAM_END__ <= (__SRAM_BASE + __SRAM_SIZE), "RW section has exceeded its limit.")
#else
    ASSERT(__RAM_END__ <= (__DDR_BASE + __DDR_SIZE), "RW section has exceeded its limit.")
#endif

	/DISCARD/ : { *(.eh_frame) }
}
