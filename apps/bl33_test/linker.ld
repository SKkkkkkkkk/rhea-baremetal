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
 .ro :
 {
  _ro_start = .;
  *entrypoint*(.text*)
  *(SORT_BY_ALIGNMENT(.text*))
  *(SORT_BY_ALIGNMENT(.rodata*))
  *(.vectors)
  _ro_end = .;
 } >DRAM
 .data :
 ALIGN(16)
 {
  _data_start = .;
  *(SORT_BY_ALIGNMENT(.data*))
  . = ALIGN(16);
  _data_end = .;
 } >DRAM AT>DRAM
 _data_start_lma = LOADADDR(.data);
 _data_size = SIZEOF(.data);
 .bss (NOLOAD) :
 ALIGN(16)
 {
  _bss_start = .;
  *(SORT_BY_ALIGNMENT(.bss*))
  *(COMMON)
  . = ALIGN(16);
  _bss_end = .;
 } >DRAM
 .heap (NOLOAD) :
 {
  end = .;
  _heap_start = .;
  . += 0x4000;
  _heap_end = .;
 } >DRAM
 .stack (NOLOAD) :
 {
  _stack_start = .;
  . += 0x4000;
  _stack_end = .;
  _stack_top = .;
 } >DRAM
 /DISCARD/ : { *(.eh_frame*) }
}
