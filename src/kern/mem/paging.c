#include "mem/paging.h"
#include "cpu/idt.h"
#include "mem/mem.h"
#include "video/printf.h"
#include <stdint.h>

#define ALIGN_DOWN(x, n) ((x) & ~((1 << (n)) - 1))

#define PAGE_ATTR 0x7
#define PAGE_PS (1 << 7)

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

void page_fault_hand(struct regs *r) {
  printkf("\n\nPAGE FAULT! (eip %p)\n", r->eip);
  printkf("err: %x\n", r->err_code);
  uint32_t cr2;
  asm volatile("mov %%cr2, %[o]" : [o]"=r" (cr2) :: "eax");
  printkf("cr2: %x\n", cr2);
  while(1);
}

void page_init() {
  CLI;
  print_init("mem", "initializing pages...", 0);
  pagetab = malloc_align(4 * NOPAGES, 0x1000);
  pagedir = malloc_align(4 * 1024, 0x1000);

  print_info("pg", 1, "page table address: %p", pagetab);
  print_info("pg", 0, "page dir address: %p", pagedir);

  for (int i = 0; i < NOPAGES; i++) {
    pagetab[i] = (i * 0x1000) | PAGE_ATTR;
  }

  pagetab[0] &= ~1;

  *pagedir = (uint32_t)pagetab | PDIR_ATTR;

  map_4mmio(0xfec00000, 0xfec00000);

  switch_pd(pagedir);

  register_ex(page_fault_hand, 14);
  STI;
}

void alloc_page(uint32_t vaddr, uint32_t paddr) {
  if ((paddr | vaddr) & 0xfff)
    return;
  uint32_t pti = (vaddr >> 12) & 0x3ff;
  pagetab[pti] = paddr | PAGE_ATTR;
}

#define PDIR_PCD (1 << 4)
#define PDIR_PWT (1 << 3)

void map_4mmio(uint32_t virt, uint32_t phys) {
  int idx = virt >> 22;
  //printkf("idx: %d\n", idx);
  pagedir[idx] = (phys & 0xffc00000) | PDIR_ATTR | PAGE_PS | PDIR_PCD | PDIR_PWT;
  //while(1);
}