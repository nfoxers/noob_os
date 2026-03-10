#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"
#include "cpu/idt.h"

void kmain(void) {
  init_video();
  printkf("hello from C!\n");
  printkf("trying to set up the idt... ");
  set_idtr();
  printkf("trying to initialize the pic... ");
  init_pic();
  //asm volatile("sti");
  //todo: uhh serial, complete gdt n tss, maybe graphics mode
  return;
}