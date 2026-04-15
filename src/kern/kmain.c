#include "cpu/ccpu.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "driver/tty.h"
#include "fs/devfs.h"
#include "fs/fat12.h"
#include "driver/keyboard.h"
#include "driver/pci.h"
#include "driver/time.h"
#include "fs/vfs.h"
#include "mem/mem.h"
#include "mem/paging.h"
#include "proc/proc.h"
#include "stdint.h"
#include "video/printf.h"
#include "video/video.h"
#include "proc/shell.h"
#include <stdarg.h>
#include <lib/errno.h>
#include "driver/serial.h"
#include "syscall/syscall.h"

extern void enditall();

void setup() {
  zero_bss();
  kmalloc_init();
  init_video();
  
  printkf("hello from C!\n");
  init_root_proc();

  set_idtr();  
  init_pic();


  //init_rootfs(); 

  set_gdt();
  set_apic();
  
  syscall_init();

  init_pit(1);
  pci_init();
  init_serial(9600);
  fpu_init();

  smbios_scan();

  page_init();

  init_fs();
  
  //init_devs();
  init_kbd();
  init_tty();


  check_capat();
  printk("time of boot: ");  
  print_time();

  printkf("used dynamic memory: %d KiB (%d B)\n", getused()/1024, getused());
}

void kmain(void) {
  setup();

  //printkf("stat for root: %d\n", chkcred("user", "toor"));

  // TODO: fork and execve

  STI;  
  shell();

  enditall();

  return;
}