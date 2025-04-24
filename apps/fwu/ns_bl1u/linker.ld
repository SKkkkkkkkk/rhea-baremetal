OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(entrypoint)
MEMORY {
 ROM (rx): ORIGIN = 76K, LENGTH = 256K-76K
 SRAM (rwx): ORIGIN = 0x100000+512K-128K, LENGTH = 128K
 DRAM (rwx): ORIGIN = 0x40000000, LENGTH = 0xC0000000
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
 } >ROM
 .data :
 ALIGN(32)
 {
  _data_vma_start = .;
  *(SORT_BY_ALIGNMENT(.data*))
  . = ALIGN(32);
  _data_vma_end = .;
 } >SRAM AT>ROM
 _data_start_lma = LOADADDR(.data);
 _data_size = SIZEOF(.data);
 .bss (NOLOAD) :
 ALIGN(32)
 {
  _bss_start = .;
  *(SORT_BY_ALIGNMENT(.bss*))
  *(COMMON)
  . = ALIGN(32);
  _bss_end = .;
 } >SRAM
 .heap (NOLOAD) :
 {
  end = .;
  _heap_start = .;
  . += 0x4000;
  _heap_end = .;
 } >SRAM
 .stack (NOLOAD) :
 {
  _stack_start = .;
  . += 0x4000;
  _stack_end = .;
  _stack_top = .;
 } >SRAM
 _all_end = .;
 /DISCARD/ : { *(.eh_frame*) }
}
