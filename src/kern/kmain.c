#include "cpu/ccpu.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "crypt/blake2s.h"
#include "crypt/crypt.h"
#include "driver/disk/ide.h"
#include "driver/keyboard.h"
#include "driver/pci.h"
#include "driver/serial.h"
#include "driver/time.h"
#include "driver/tty.h"
#include "fs/devfs.h"
#include "fs/fat12.h"
#include "fs/vfs.h"
#include "mem/acpi.h"
#include "mem/mem.h"
#include "mem/paging.h"
#include "proc/proc.h"
#include "proc/shell.h"
#include "stdint.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <cpu/irq.h>
#include <lib/errno.h>
#include <stdarg.h>

extern void parse_multiboot2(void *ptr);

void setup(void *ptr) {
  //zero_bss();
  kmalloc_init();
  init_video();

  printkf("hello from C!\n");
  init_root_proc();

  set_gdt();

  set_idtr();
  smbios_scan();
  scan_acpi();  

  set_apic();
  ioapic_init();

  syscall_init();

  // uncomment when useful
  //parse_multiboot2(ptr); 

  ata_init();


  init_pit(1);
  pci_init();
  init_serial(9600);
  fpu_init();

  page_init();

  // init_devs();
  init_kbd();
  init_tty();

  //pic_disable();

  mkadv();

  check_capat();
  printk("time of boot: ");
  print_time();

  printkf("used dynamic memory: %d KiB (%d B)\n", getused() / 1024, getused());
}

void kmain(void *ptr);

void _c_start(void *ptr) {
  //*(char *)0xb8000 = 'A';
  kmain(ptr);
}

int checks() {
  int fd = open("/etc", O_RDONLY);
  if(fd == -1) {
    perror("open /etc");
    return -1;
  }

  close(fd);
  return 0;
};

void kmain(void *ptr) {
  setup(ptr);

  if(checks() == -1) {
    printkf("some checks failed...\n");
    while(1);
  }

  // printkf("stat for root: %d\n", chkcred("user", "toor"));

  // TODO: fork and execve

  STI;
  shell();

  return;
}