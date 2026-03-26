[bits 32]
global _start
extern kmain

section .text.start
_start:
  cli
  call kmain
.hang:
  hlt
  jmp .hang