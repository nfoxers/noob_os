#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "mem/mem.h"
#include <stddef.h>
#include <stdint.h>

static struct segdesc sd[6] = {0};
static struct gdtr    gdtr  = {0};

struct tss glob_tss;

static void fill_seg(struct segdesc *s, uint16_t lim, size_t base, uint8_t flg, uint8_t ab) {
  s->base_lo = base & 0xffff;
  s->base_ll = (base >> 16) & 0xff;
  s->base_h  = (base >> 24) & 0xff;
  s->lim     = lim;
  s->access  = ab;
  s->flag    = (flg << 4) | 0x0f;
}

extern void flush_gdt();

void set_gdt() {
  fill_seg(&sd[1], 0xffff, 0, 0x0c, 0x9a);
  fill_seg(&sd[2], 0xffff, 0, 0x0c, 0x92);
  fill_seg(&sd[3], 0xffff, 0, 0x0c, 0xfa);
  fill_seg(&sd[4], 0xffff, 0, 0x0c, 0xf2);

  glob_tss.ss0  = DS_K;
  glob_tss.esp0 = (int)&__bss_end__ + HEAP_SIZ;

  fill_seg(&sd[5], sizeof(struct tss), (size_t)&glob_tss, 0x00, 0x89);

  gdtr.lim  = sizeof(sd) - 1;
  gdtr.addr = (uint32_t)sd;

  CLI;
  asm volatile("lgdt (%0)" ::"r"(&gdtr) : "memory");
  flush_gdt();
  //STI; // its gonna be called before any interrupt uses anyway 
}