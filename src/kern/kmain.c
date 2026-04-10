#include "cpu/ccpu.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
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
  init_fs();

  init_rootfs(); 

  set_gdt();
  set_apic();
  
  syscall_init();

  init_pit(1);
  pci_init();
  init_serial(9600);
  fpu_init();

  smbios_scan();

  page_init();
  init_kbd();
 


  check_capat();
  printk("time of boot: ");  
  print_time();

  printkf("used dynamic memory: %d K\n", getused()/1024);
}

void kmain(void) {
  setup();

  // TODO: fork and execve
  /*
  int fd[2];
  if(pipe(fd) == -1) perror("pipe");
  printkf("fd: %d %d\n", fd[0], fd[1]);
  if(close(fd[0]) == -1) perror("close");
  if(close(fd[1]) == -1) perror("close");
*/
  STI;  
  shell();

  enditall();

  return;
}