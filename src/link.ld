ENTRY(_start)
SECTIONS
{  
  . = 0x500;
  .bss : {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    __bss_end__ = .;
  }

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
}
