[bits 32]

struc regs
  .edi resd 1 
  .esi resd 1
  .ebp resd 1
  .esp resd 1
  .ebx resd 1
  .edx resd 1
  .ecx resd 1
  .eax resd 1

  .int_no resd 1
  .err_co resd 1

  .eip resd 1
  .cs  resd 1
  .efl resd 1
  
  .esp0 resd 1
  .ss0 resd 1
endstruc

global c_switch3
c_switch3: ; esp+4 = &struct regs *prev, esp+8 = &struct regs *next
  mov eax, [esp+4] ; eax = struct regs *
  mov edx, [esp+8] 
  
  mov esp, [edx + regs.esp] ; esp = (struct regs *next)->esp

  cmp dword [edx + regs.cs], 0x1b
  jnz .nrel
  
  mov ax, 0x23 ; you can only set kernel segment regs if youre at ring 0
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
.nrel:

  popad

  add esp, 8 ; kill err code from stakc
  iret

global get_ef
get_ef:
  pushf
  pop eax
  ret

global int40
int40:
  int 40
  ret

global int41
int41:
  int 41
  ret