#ifndef GDT_H
#define GDT_H

#include "stdint.h"

struct gdtr {
  uint16_t lim;
  uint32_t addr;
} __attribute__((packed));

struct segdesc {
  uint16_t lim;
  uint16_t base_lo;
  uint8_t  base_ll;
  uint8_t  access;
  uint8_t  flag;
  uint8_t  base_h;
} __attribute__((packed));

#define SEGDESC_G_NULL 0x0000000000000000 // off 0  0x00
#define SEGDESC_K_CODE 0x00cf9a000000ffff // off 8  0x08
#define SEGDESC_K_DATA 0x00cf92000000ffff // off 16 0x10
#define SEGDESC_U_CODE 0x00cffa000000ffff // off 24 0x18 (0x1b)
#define SEGDESC_U_DATA 0x00cff2000000ffff // off 32 0x20 (0x23)

#define TSS_AB  0x89
#define TSS_FLG 0x00

struct tss {
  uint32_t link;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;

  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;

  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;

  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;

  uint32_t ldtr;
  uint32_t iopb; // iopb << 16
  uint32_t ssp;
} __attribute__((packed));

void set_gdt();

#define CS_K 0x0008
#define CS_U 0x001B
#define DS_K 0x0010
#define DS_U 0x0023

#endif