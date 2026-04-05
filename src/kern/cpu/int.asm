[bits 32]

extern isr_handler
extern irq_handler

struc regs
  .ds resd 1
  .es resd 1
  .fs resd 1
  .gs resd 1

  .edi resd 1 
  .esi resd 1
  .ebp resd 1
  .esp resd 1
  .ebx resd 1
  .edx resd 1
  .eax resd 1

  .int resd 1
  .err resd 1

  .eip resd 1
  .cs  resd 1
  .efl resd 1
  
  .esp0 resd 1
  .ss0 resd 1
endstruc

isr_stub:
  pushad
  push gs
  push fs
  push es
  push ds

  mov ax, 0x10
  mov es, ax
  mov ds, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax

  mov eax, esp ; eax = struct regs *
  mov [eax + regs.esp], esp

  push eax
  call isr_handler
  add esp, 4

intret:
  pop ds
  pop es
  pop fs 
  pop gs
  popad

  add esp, 8
  iret

irq_stub:
  pushad
  push gs
  push fs
  push es
  push ds

  mov ax, 0x10
  mov es, ax
  mov ds, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax

  mov eax, esp ; eax = struct regs *
  mov [eax + regs.esp], esp

  push esp
  call irq_handler
  add esp, 4

  jmp intret

extern sys_rval

sys_stub:
  pushad
  push gs
  push fs
  push es
  push ds

  mov ax, 0x10
  mov es, ax
  mov ds, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax

  mov eax, esp ; eax = struct regs *
  mov [eax + regs.esp], esp

  push esp
  call isr_handler
  pop eax

  pop ds
  pop es
  pop fs 
  pop gs
  popad

  mov eax, [sys_rval]

  add esp, 8
  iret

extern pic_eoi
global childret
childret:
  push dword 0
  call pic_eoi
  add esp, 4
  sti
  jmp intret

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
_irq 9
_irq 10
_irq 11
_irq 12
_irq 13
_irq 14
_irq 15

global _ex48
_ex48:
  push dword 0
  push dword 48
  jmp sys_stub