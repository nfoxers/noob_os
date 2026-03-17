#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "driver/fat12.h"
#include "driver/keyboard.h"
#include "driver/time.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"

extern void enditall();
extern void usermode();

void kmain(void) {
  zero_bss();
  init_video();
  printk("hello from C!\n");

  init_root_proc();

  printk("trying to set up the idt... ");
  set_idtr();
  printk("trying to initialize the pic... ");
  init_pic();
  printk("trying to intialize fat12... ");
  init_fs();

  STI;
  printk("initializing the keyboard... ");
  init_kbd();

  printk("setting up the gdt... ");
  set_gdt();


  printk("time: ");
  struct time_s s;
  read_time(&s);

  //usermode();
  alloc_task(NULL);
  yield();

  static char buf[128];
  static char argv[8][16];

  while (1) {
    printk("input> ");
    int rd = kgets(buf, sizeof buf);
    putchr('\n');

    if(rd == 0){
      continue;
    }

    char *tok = kstrtok(buf, " ");
    uint8_t idx = 0;
    while (tok) {
      kstrncpy(argv[idx++], tok, 16);
      tok = kstrtok(NULL, " ");
    }

    if (!kstrcmp(buf, "help")) {
      printk("supported commands: exit ls rm touch clear\n");
    } else if(!kstrcmp(buf, "exit")) {
      printk("exiting...\n");
      break;
    } else if(!kstrcmp(buf, "ls")) {
      list_dir();
    } else if(!kstrcmp(buf, "rm")) {
      if(idx == 1) {
        printk("specify your file path!\n");
        continue;
      }
      sys_unlink(argv[1]);
    } else if(!kstrcmp(buf, "touch")) {
        if(idx == 1) {
        printk("specify your file path!\n");
        continue;
      }
      sys_open(argv[1], O_CREAT);
    } else if(!kstrcmp(buf, "clear")) {
      clr_scr();
    }
    else {
      int fd = sys_open(argv[0], 0);
      if(!fd) {
        printkf("unrecognized command '%s'\n", argv[0]);
        continue;
      } //todo: load and run if not INODE_DIR
    }
  }

  // todo: again, gdt (uh done), serial (for why), memory safety, etc
  // todo2: FIX fat12 driver (oh lord)
  // todo3: FIX multitasking, and SYSCALLS!!!!!!!
  
  while(1);

  enditall();

  return;
}