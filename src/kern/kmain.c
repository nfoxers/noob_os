#include "cpu/idt.h"
#include "driver/keyboard.h"
#include "mem/mem.h"
#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"

void kmain(void) {
  zero_bss();
  init_video();
  printkf("hello frpm C!\n");
  printkf("trying to set up the idt... ");
  set_idtr();
  printkf("trying to initialize the pic... ");
  init_pic();

  STI;
  printk("initializing the keyboard... ");
  init_kbd();

  parse_bda();

  char buf[128];
  char argv[8][16];
  while (1) {
    printk("input> ");
    kgets(buf, sizeof buf);
    putchr('\n');

    char *tok = kstrtok(buf, " ");
    uint8_t idx = 0;
    while (tok) {
      kstrncpy(tok, argv[idx++]);
      tok = kstrtok(NULL, " ");
    }

    if (!kstrcmp(buf, "help")) {
      printkf("commands to be implemented\n");
    } else {
      printkf("unrecognized command '%s'\n", argv[0]);
    }
  }

  // todo: again, gdt, serial (for why), memory safety, etc
  // todo2: fat12 driver (oh lord)

  return;
}