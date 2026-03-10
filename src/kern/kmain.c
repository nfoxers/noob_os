#include "stdint.h"
#include "io.h"
#include "video/printf.h"

extern void set_idtr();
extern void init_pic();

void kmain(void) {
  clr_scr();
  printkf("hello from C!\n");
  printkf("trying to set up the idt...\n");
  set_idtr();
  printkf("trying to initialize pics...\n");
  init_pic();
  asm volatile("sti");
  return;
}