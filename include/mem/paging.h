#ifndef PAGING_H
#define PAGING_H

#include "stdint.h"

void debug_vaddr(uint32_t addr);
void page_init();

void alloc_page(uint32_t vaddr, uint32_t paddr);

void map_4mmio(uint32_t virt, uint32_t phys);

#endif