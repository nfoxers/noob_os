[bits 32]

extern flush_gdt
extern usermode

flush_gdt:
  mov ax, 0x010
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax
  jmp 0x08:flush_gdt.fcs
.fcs:
  mov ax, 5*8
  ltr ax 
  ret

usermode:
  mov ax, (4 * 8) | 3
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  mov eax, esp
  push dword (4 * 8) | 3

  push eax
  pushf
  push dword (3 * 8) | 3
  
  push u_tst
  iret

u_tst:
.jmp:
  jmp u_tst.jmp

extern isr_handler
extern irq_handler

isr_stub:
  push esp
  call isr_handler
  add esp, 4

  add esp, 8
  iret

irq_stub:
  push esp
  call irq_handler
  add esp, 4

  add esp, 8
  iret

%macro _ex_ne 1
global _ex%1
_ex%1:
  push dword 0 ; err
  push dword %1 ; int
  jmp isr_stub
%endmacro

%macro _ex_e 1
global _ex%1
_ex%1:
  push dword %1
  jmp isr_stub
%endmacro

%macro _irq 1
global _irq%1
_irq%1:
  push dword 0 ; err
  push dword %1+32 ; int
  jmp irq_stub
%endmacro

_ex_ne 0
_ex_ne 1
_ex_ne 2
_ex_ne 3
_ex_ne 4
_ex_ne 5
_ex_ne 6
_ex_ne 7
_ex_e 8
_ex_ne 9
_ex_e 10
_ex_e 11
_ex_e 12
_ex_e 13
_ex_e 14
_ex_ne 15
_ex_ne 16
_ex_e 17
_ex_ne 18
_ex_ne 19
_ex_ne 20
_ex_e 21
_ex_ne 22
_ex_ne 23
_ex_ne 24
_ex_ne 25
_ex_ne 26
_ex_ne 27
_ex_ne 28
_ex_ne 29
_ex_e 30
_ex_ne 31

_irq 0
_irq 1
_irq 2
_irq 3
_irq 4
_irq 5
_irq 6
_irq 7
_irq 8
