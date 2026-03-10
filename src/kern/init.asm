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

section .text.aux
global scroll
global clear
scroll:
  push edi
  push esi

  mov ecx, 4000/4
  mov esi, 0xb8000 + 160
  mov edi, 0xb8000
  cld
  rep movsd

  pop esi
  pop edi
  ret

clear:
  push esi
  mov eax, 0
  mov edi, 0xb8000
  mov ecx, 4000/4
  rep stosd
  pop esi
  ret