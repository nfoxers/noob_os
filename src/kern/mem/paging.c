#include "mem/paging.h"
#include "cpu/idt.h"
#include "mem/mem.h"
#include "video/printf.h"
#include <stdint.h>

#define PAGE_ATTR 0x7
#define PDIR_ATTR 0x7
#define NOPAGES   0x200

void debug_vaddr(uint32_t addr) {
  uint32_t off = addr & 0xfff;
  uint32_t pi  = (addr >> 12) & 0x3ff; // page index in page table
  uint32_t pti = (addr >> 22) & 0x3ff; // page table index in page dir

  printkf("%08x: %06x %06x %06x\n", addr, pti, pi, off);
}

uint32_t *pagetab;
uint32_t *pagedir;

extern void switch_pd(uint32_t *pd);
extern void flush_pg();

void page_init() {
  CLI;
  print_init("mem", "initializing pages...", 0);
  pagetab = malloc_align(4 * NOPAGES, 0x1000);
  pagedir = malloc_align(4, 0x1000);

  print_info("pg", 1, "page table address: %p", pagetab);
  print_info("pg", 0, "page dir address: %p", pagedir);

  for (int i = 0; i < NOPAGES; i++) {
    pagetab[i] = (i * 0x1000) | PAGE_ATTR;
  }

  pagetab[0] &= ~1;

  *pagedir = (uint32_t)pagetab | PDIR_ATTR;
  switch_pd(pagedir);

  STI;
}

void alloc_page(uint32_t vaddr, uint32_t paddr) {
  if ((paddr | vaddr) & 0xfff)
    return;
  uint32_t pti = (vaddr >> 12) & 0x3ff;
  pagetab[pti] = paddr | PAGE_ATTR;
}