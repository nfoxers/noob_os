[bits 32]

MAGIC equ 0xe85250d6
ARCH equ 0
LEN equ head_end - head_start
CHKSUM equ -(MAGIC + ARCH + LEN)

section .multiboot

head_start:
  dd MAGIC
  dd ARCH
  dd LEN
  dd CHKSUM

null_tag:
  dw 0
  dw 0
  dd 0
head_end:

section .text

global _as_start

_as_start:
  cli

  cmp eax, 0x36d76289
  jne .err

  mov esp, stack

  extern _c_start
  push ebx
  call _c_start

  jmp $

.err:
  mov eax, 'E'
  mov [0xb8000], eax
  jmp $


section .bss
resb 8192
stack:
