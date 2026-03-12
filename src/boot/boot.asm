[org 0x7c00]
[bits 16]

jmp short start
nop

fat:
  db "HELLO WU"
  dw 512
  db 2
  dw 1
  db 2
  dw 112
  dw 625
  db 0xfa
  dw 2
  dw 8
  dw 1
  dd 0
  dd 0

  db 0
  db 0
  db 0x29
  db "HEHE"
  db "ROOTPARTITN"
  db "FAT12   "

start:
  jmp 0:boot
boot:
  cli

  xor ax, ax
  mov es, ax
  mov ds, ax
  mov ss, ax

  mov sp, 0x7c00
  mov bp, sp

  mov [0x03ff], dl
  mov [drivenum], dl

  mov si, msg
  call print

  call testa20

  mov si, msg15
  call print

  call readdisk
  
  mov si, msg2
  call print

  cli
  lgdt [gdtr]
  mov eax, cr0
  or eax, 1
  mov cr0, eax
  jmp 0x08:pmstart

  hlt

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

testa20:
  push ax
  push es 
  mov ax, 0xffff
  mov es, ax
  mov ax, es:[0x7e0e]
  cmp ax, 0xaa55
  pop es
  pop ax
  jz .dis
  ret
.dis:
  mov si, a20
  call print
  ret

readdisk:
  push si
  push ax
  push dx

  mov si, .dap
  mov ah, 0x42
  mov dl, [drivenum]
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
  dw 64 ; around 25KB of maximum kernel size ontop of FAT12
  dw 0x7c00
  dw 0x0000
  dq 0

gdt:
.null: dq 0
.code: dq 0x00CF9A000000FFFF
.data: dq 0x00CF92000000FFFF
.end:

gdtr:
  dw gdt.end - gdt - 1
  dd gdt

msg: db "hewwo world!", 0xa, 0xd, 0
a20: db "a20 line disabled", 0xa, 0xd, 0
msg15: db "trying to read disk... ", 0 
dskerr: db "read disk failed", 0xa, 0xd, 0
daskok: db "disk read persumably ok", 0xa, 0xd, 0
msg2: db "trying to go into pm", 0xa, 0xd, 0
drivenum: db 0

[bits 32]

pmstart:
  mov ax, 0x10
  mov es, ax
  mov ds, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax

  mov esp, 0x7c00
  mov ebp, esp

  jmp 0x9400

  cli
  hlt

times 510-($-$$) db 0
dw 0xaa55