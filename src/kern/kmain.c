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
#include "proc/shell.h"
#include <stdarg.h>
#include "driver/serial.h"
#include "cpu/syscall.h"

extern void enditall();

void kmain(void) {
  zero_bss();
  kmalloc_init();
  init_video();
  
  printkf("hello from C!\n");
  init_root_proc();

  set_idtr();  
  init_pic();
  init_fs();

  init_kbd();
  set_gdt();
  set_apic();
  
  syscall_init();

  init_pit(1);
  pci_init();
  init_serial(9600);

  smbios_scan();

  check_capat();
  printk("time of boot: ");

  print_time();
  
  //spawn_proc(testfunc, CS_U);

  STI;  

  shell();

  // todo: memory safety, etc
  // todo2: FIX fat12 driver (oh lord)
  // todo3: FIX multitasking, and SYSCALLS!!!!!!!

  enditall();

  return;
}