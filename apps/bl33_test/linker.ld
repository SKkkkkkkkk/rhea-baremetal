OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(entrypoint)
MEMORY {
 ROM (rx): ORIGIN = 0x00000000, LENGTH = 256K
 SRAM (rwx): ORIGIN = 0x00100000, LENGTH = 256K
 DRAM (rwx): ORIGIN = 0x42000000, LENGTH = 0xC0000000
}
SECTIONS
{
    . = ORIGIN(DRAM);
 ASSERT(. == ALIGN(4096), "RO_BASE address is not aligned on a page boundary.")
 .text . :
 {
  __TEXT_START__ = .;
  *entrypoint*(.text*)
  *(SORT_BY_ALIGNMENT(.text*))
  *(.vectors)
  . = ALIGN(((1) << (12)));
  __TEXT_END__ = .;
 } >DRAM
 .ARM.extab . :
 {
  *(.ARM.extab* .gnu.linkonce.armextab.*)
 } >DRAM
 .ARM.exidx . :
 {
  *(.ARM.exidx* .gnu.linkonce.armexidx.*)
 } >DRAM
 .gcc_except_table . : ONLY_IF_RO
 {
  *(.gcc_except_table .gcc_except_table.*)
 } >DRAM
 .preinit_array . : {
  . = ALIGN(8);
  __preinit_array_start = .;
  KEEP(*(.preinit_array))
  __preinit_array_end = .;
 } >DRAM
 .init_array . : {
  . = ALIGN(8);
  __init_array_start = .;
  KEEP(*(.init_array))
  __init_array_end = .;
 } >DRAM
 .fini_array . : {
  . = ALIGN(8);
  __fini_array_start = .;
  KEEP(*(.fini_array))
  __fini_array_end = .;
 } >DRAM
 .rodata . :
 {
  __RODATA_START__ = .;
  *(SORT_BY_ALIGNMENT(.rodata*))
  __tinyCLI_CMD_START__ = .;
  KEEP(*(.tinyCLI_CMD*))
  __tinyCLI_CMD_END__ = .;
  __RODATA_END__ = .;
  . = ALIGN(16);
 } >DRAM
 . = ALIGN(4096);
 ASSERT(. == ALIGN(4096), "RW_BASE address is not aligned on a page boundary.")
 .data . :
 ALIGN(16)
 {
  __DATA_START__ = .;
  *(SORT_BY_ALIGNMENT(.data*))
  __DATA_END__ = .;
 } >DRAM
 .resource_table . :
 {
  . = ALIGN(4);
  *(.resource_table)
  . = ALIGN(16);
  __RSC_TABLE_END__ = .;
 } >DRAM
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
 } >DRAM
 ASSERT(__BSS_END__ == ALIGN(16), "__BSS_END__ address is not aligned on 16B.")
 .xlat_table (NOLOAD) :
 {
  __XLAT_TABLE_START__ = .;
  *(xlat_table)
  __XLAT_TABLE_END__ = .;
 } >DRAM
 .heap (NOLOAD): ALIGN(16)
 {
  __HEAP_START__ = .;
  end = __HEAP_START__;
  . = . + 0x4000;
  __HEAP_END__ = .;
  __HEAPLIMIT = __HEAP_END__;
 } >DRAM
 .stack (NOLOAD): ALIGN(16)
 {
  __STACKS_START__ = .;
  . = . + 0x4000;
  . = ALIGN(16);
  __STACKS_END__ = .;
  __STACK_TOP__ = __STACKS_END__;
 } >DRAM
 ASSERT(__STACK_TOP__ == ALIGN(16), "__STACK_TOP__ address is not aligned on 16B.")
 __RAM_START__ = ADDR(.data);
 __RAM_END__ = .;
 __RW_ROM_START__ = LOADADDR(.data);
 __RW_SIZE__ = __RW_RAM_END__ - __RW_RAM_START__;
 __BSS_SIZE__ = SIZEOF(.bss);
    ASSERT(__RAM_END__ <= (0x42000000 + 0xC0000000), "RW section has exceeded its limit.")
}
