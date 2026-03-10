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
    *(.bss*)
    *(COMMON)
  }
}
