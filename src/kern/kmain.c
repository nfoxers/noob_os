#include "cpu/idt.h"
#include "driver/fat12.h"
#include "driver/keyboard.h"
#include "mem/mem.h"
#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"

extern void enditall();

void kmain(void) {
  zero_bss();
  init_video();
  printk("hello from C!\n");

  printk("trying to set up the idt... ");
  set_idtr();
  printk("trying to initialize the pic... ");
  init_pic();
  printk("trying to intialize fat12... ");
  init_fs();

  STI;
  printk("initializing the keyboard... ");
  init_kbd();

  parse_bda();

  static char buf[128];
  static char argv[8][16];
  while (1) {
    printk("input> ");
    kgets(buf, sizeof buf);
    putchr('\n');

    char *tok = kstrtok(buf, " ");
    uint8_t idx = 0;
    while (tok) {
      kstrncpy(tok, argv[idx++], 16);
      tok = kstrtok(NULL, " ");
    }

    if (!kstrcmp(buf, "help")) {
      printk("commands to be implemented\n");
    } else if(!kstrcmp(buf, "end")) {
      printk("ending...\n");
      break;
    }
    else {
      printkf("unrecognized command '%s'\n", argv[0]);
    }
  }

  // todo: again, gdt, serial (for why), memory safety, etc
  // todo2: fat12 driver (oh lord) (partially done)
  
  enditall();

  return;
}