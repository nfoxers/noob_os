[bits 32]

global flush_gdt
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

global switch_pd
switch_pd: ; [esp] = ret, [esp+4] = pd_addr
  mov eax, [esp+4]
  mov cr3, eax

  mov eax, cr0
  or eax, 0x80000001
  mov cr0, eax

  mov eax, cr4
  or eax, (1 << 4) ; PSE
  mov cr4, eax

  ret

global flush_pg:
flush_pg:
  mov eax, cr3
  mov cr3, eax
  ret

global set_fpu
set_fpu:
  mov eax, cr0
  and eax, ~(1 << 2)
  or eax, 1 << 1
  mov cr0, eax

  mov eax, cr4
  or eax, 3 << 9
  mov cr4, eax
  ret
