[bits 32]
[global _start]
[extern tmain]

_start:
  call tmain
  ; todo: call _exit
  jmp $