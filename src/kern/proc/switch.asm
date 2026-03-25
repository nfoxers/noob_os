[bits 32]

global get_ef
get_ef:
  pushf
  pop eax
  ret

global syscall
syscall:
  int 48
  ret