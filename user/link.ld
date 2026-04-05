ENTRY(_start)
SECTIONS
{  
  . = 0x0;
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
  .bss : {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    __bss_end__ = .;
  }
}
