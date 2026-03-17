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

extern cur
extern next

struc task
  .t_proc resd 1
  .t_next resd 1
  .t_esp resd 1
endstruc

global c_switch2
c_switch2:
  pushad

  ; symbol cur = struct task **
  mov eax, [cur] ; eax = struct task *
  mov [eax + task.t_esp], esp ; (struct task *)->t_esp AHA YES!

  mov eax, [next]
  mov esp, [eax + task.t_esp]

  mov edx, [cur]
  mov [cur], eax
  mov [next], edx

  popad
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