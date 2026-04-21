ENTRY(_as_start)
SECTIONS
{  
  . = 1M;
  .text : {
    __text_start__ = .;
    KEEP(*(.multiboot))
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
