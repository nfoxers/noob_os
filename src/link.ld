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
    __text_start__ = .;
    *(.text.start)
    *(.text*)
    __text_end__ = .;
  }
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data*)
  }
}
