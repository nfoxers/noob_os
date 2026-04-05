[bits 32]

global enditall

enditall:
  cli

  mov eax, cr0
  and eax, 0x7fffffff
  mov cr0, eax

  lgdt [gdtr]
  jmp 0x08:pm16

gdt_16bit:
.null: dq 0
.code: dq 0x008F9A000000FFFF
.data: dq 0x008F92000000FFFF
.end:

gdtr:
  dw gdt_16bit.end - gdt_16bit - 1
  dd gdt_16bit

ldtr:
  dw 0x3ff
  dd 0x00

[bits 16]
pm16:
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  lidt [ldtr]

  mov eax, cr0
  and eax, ~1
  mov cr0, eax

  jmp 0:final16

final16:
  xor ax, ax

  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  mov sp, 0x7c00
  mov bp, sp

  sti ; WE'VE DONE IT!!!! 

  mov ax, 0x3
  int 10h

  mov si, msg
  call print

  mov si, wrdsk
  call print

  call writedisk

  mov si, helt
  call print

  jmp $

print:
  push si
  push ax
  mov ah, 0x0e
.loop:
  lodsb
  or al, al
  jz .done
  int 0x10
  jmp .loop
.done:
  pop ax
  pop si
  ret

writedisk:
  push si
  push ax
  push dx

  mov si, .dap
  mov ah, 0x43
  mov dl, [0x3ff]
  int 0x13
  jc .err

  mov si, daskok
  call print

  pop dx
  pop ax
  pop si
  ret
.err:
  mov si, dskerr
  call print
  cli
  hlt
.dap:
  db 0x10
  db 0x00
  dw 64
  dw 0x7c00
  dw 0x0000
  dq 0

msg: db "return to 16-bit real mode ok", 0xa, 0xd, 0
wrdsk: db "writing back to disk...", 0xa, 0xd, 0
daskok: db "writing to disk persumably ok", 0xa, 0xd, 0
dskerr: db "writing to disk error", 0xa, 0xd, 0
helt: db "you can leave the os now", 0xa, 0xd, 0
