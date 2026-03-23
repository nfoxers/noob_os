#include "cpu/ccpu.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "driver/fat12.h"
#include "driver/keyboard.h"
//#include "driver/pci.h"
#include "driver/pci.h"
#include "driver/time.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"
#include "proc/shell.h"
#include <stdarg.h>

extern void enditall();

volatile int counter1 = 0;

void testfunc() {
  printkf("why doesnt preemptive work\n");
  exit_cur();
}

void kmain(void) {
  zero_bss();
  init_video();
  printk("hello from C!\n");

  init_root_proc();

  print_init("idt", "setting up the idt...", 0);
  set_idtr();
  print_init("pic", "initializing pic...", 0);
  init_pic();
  print_init("fat", "initializing filesystem driver...", 0);
  init_fs();

  print_init("kbd", "initializing the keyboard...", 0);
  init_kbd();
  print_init("gdt", "setting up the gdt...", 0);
  set_gdt();
  print_init("apic", "initializing the apic...", 0);
  set_apic();
  
  print_init("pit", "intializing the pit...", 0);
  init_pit(1);
  print_init("pci", "initializing pci devices...", 0);
  pci_init();
  print_init("mem", "initializing the dynamic allocator...", 0);
  kmalloc_init();

  check_capat();
  printk("time of boot: ");
  struct time_s s;
  read_time(&s);
  
  CLI;
  spawn_proc(testfunc, CS_K);

  STI;
  shell();

  // todo: again, gdt (uh done), serial (for why), memory safety, etc
  // todo2: FIX fat12 driver (oh lord)
  // todo3: FIX multitasking, and SYSCALLS!!!!!!!

  enditall();

  return;
}