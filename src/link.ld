ENTRY(_start)
SECTIONS
{
  . = 0x9400;
  .text : {
    *(.text.start)
    *(.text*)
  }
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data*)
  }
  .bss : {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    __bss_end__ = .;
  }
}
