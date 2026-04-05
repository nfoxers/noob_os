[bits 32]

extern errno

global syscall
syscall:
  push ebx
  push esi
  push edi
  push ebp

  mov eax, [esp+20]

  mov ebx, [esp+24]
  mov ecx, [esp+28]
  mov edx, [esp+32]
  mov esi, [esp+36]
  mov edi, [esp+40]
  mov ebp, [esp+44]

  int 48 ; sys intno

  pop ebp
  pop edi
  pop esi
  pop ebx

  cmp eax, -4095
  jae .err
  ret
.err:
  not eax
  add eax, 1
  mov [errno], eax ; errno = -ret

  mov eax, -1
  ret