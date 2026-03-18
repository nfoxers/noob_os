[bits 32]

global c_switch

c_switch:
  
  push eax
  push edx

  mov eax, [esp+12] ; prev
  mov edx, [esp+16] ; next

  ;; push regss
  push gs
  push fs
  push ds
  push ss
  push es

  push edi
  push esi
  push ebp
  sub esp,4
  push ebx
  push ecx

  mov [eax], esp
  mov esp, [edx]

  ;; poppo reggto
  pop ecx
  pop ebx
  add esp, 4
  pop ebp
  pop esi
  pop edi

  pop es
  pop ss
  pop ds
  pop fs
  pop gs

  pop edx
  pop eax
  
  iret

struc regs
  .edi resd 1 
  .esi resd 1
  .ebp resd 1
  .esp resd 1
  .ebx resd 1
  .edx resd 1
  .eax resd 1

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
  ;mov [eax + regs.esp], esp
  mov esp, [edx + regs.esp] ; esp = (struct regs *)->esp

  popad
  add esp, 8 ; err code
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