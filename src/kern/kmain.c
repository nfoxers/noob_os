#include "cpu/ccpu.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "driver/fat12.h"
#include "driver/keyboard.h"
#include "driver/pci.h"
#include "driver/time.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"

extern void enditall();

void kmain(void) {
  zero_bss();
  init_video();
  printk("hello from C!\n");

  init_root_proc();

  printk("setting up the idt... ");
  set_idtr();
  printk("initializing the pic... ");
  init_pic();
  printk("initializing fat12... ");
  init_fs();

  STI;
  printk("initializing the keyboard... ");
  init_kbd();

  printk("setting up the gdt... ");
  set_gdt();

  check_capat();

  printk("initializing apic... ");
  set_apic();

  // pci_enumerate(); //uncomment when gefixiert

  printk("time of boot (gmt): ");
  struct time_s s;
  read_time(&s);

  static char buf[128];
  static char argv[8][16];

  while (1) {
    printk("input> ");
    int rd = kgets(buf, sizeof buf);
    putchr('\n');

    if (rd == 0) {
      continue;
    }

    char   *tok = kstrtok(buf, " ");
    uint8_t idx = 0;
    while (tok) {
      kstrncpy(argv[idx++], tok, 16);
      tok = kstrtok(NULL, " ");
    }

    if (!kstrcmp(buf, "help")) {
      printk("supported commands: exit ls rm touch clear\n");
    } else if (!kstrcmp(buf, "exit")) {
      printk("exiting...\n");
      break;
    } else if (!kstrcmp(buf, "ls")) {
      list_dir();
    } else if (!kstrcmp(buf, "rm")) {
      if (idx == 1) {
        printk("specify your file path!\n");
        continue;
      }
      sys_unlink(argv[1]);
    } else if (!kstrcmp(buf, "touch")) {
      if (idx == 1) {
        printk("specify your file path!\n");
        continue;
      }
      sys_open(argv[1], O_CREAT);
    } else if (!kstrcmp(buf, "clear")) {
      clr_scr();
    } else {
      int fd = sys_open(argv[0], 0);
      if (!fd) {
        printkf("unrecognized command '%s'\n", argv[0]);
        continue;
      } // todo: load and run if not INODE_DIR
    }
  }

  // todo: again, gdt (uh done), serial (for why), memory safety, etc
  // todo2: FIX fat12 driver (oh lord)
  // todo3: FIX multitasking, and SYSCALLS!!!!!!!

  enditall();

  return;
}