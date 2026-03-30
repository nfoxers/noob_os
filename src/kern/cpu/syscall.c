#include "cpu/syscall.h"
#include "cpu/idt.h"
#include "driver/fat12.h"
#include "video/printf.h"
#include <stdarg.h>
#include <stdint.h>

/* one little note so i dont forget
  eax: sys nr
  ebx: arg1
  ecx: ...2
  edx: ...3
  esi: ...4
  edi: ...5
  ebp: arg6

  eax: retval
*/

#define NSYS         50
#define ARGS_USELESS (void)(a1 + a2 + a3 + a4 + a5 + a6)

uint32_t sys_rval;
typedef uint32_t (*syshand)(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6);

uint32_t errno;

/* core functions */

#define ARGS uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6

uint32_t sys_read(ARGS) {
  ARGS_USELESS;
  return fsys_read(a1, (void*)a2, a3);
}

uint32_t sys_open(ARGS) {
  ARGS_USELESS;
  return fsys_open((char *)a1, a2);
}

uint32_t sys_close(ARGS) {
  ARGS_USELESS;
  return fsys_close(a1);
}

uint32_t sys_chdir(ARGS) {
  ARGS_USELESS;
  return fsys_cd((char *)a1);
}

uint32_t sys_mkdir(ARGS) {
  ARGS_USELESS;
  return fsys_mkdir((char *)a1);
}

uint32_t sys_unlink(ARGS) {
  ARGS_USELESS;
  return fsys_unlink((char*)a1);
}

/* jump table & handlers */

syshand systab[NSYS] = {
    0, 0, 0, sys_read, 0, sys_open, sys_close, 0, 0, sys_chdir, sys_mkdir,
  sys_unlink};

void sys_hand(struct regs *r) {
  syshand e = systab[r->eax];
  if (e && r->eax << 2 <= sizeof systab) {
    sys_rval = e(r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp);
  } else {
    printkf("error: invalid syscall nr %d\n", r->eax);
    while (1)
      asm volatile("hlt");
  }
}

void register_sys(syshand f, uint32_t no) {
  if (no >= NSYS)
    return;
  systab[no] = f;
}

void syscall_init() {
  print_init("sys", "initializing syscalls", 0);


  register_ex(sys_hand, SYS_INTNO);
}

