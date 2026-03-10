#ifndef IDT_H
#define IDT_H

#include "stdint.h"

struct idtr {
  uint16_t siz;
  uint32_t addr;
} __attribute__((packed));

struct idt_gate {
  uint16_t offsetlow;
  uint16_t seg;
  uint8_t res;
  uint8_t flag;
  uint16_t offsethi;
} __attribute__((packed));

struct regs {
  uint32_t int_no;
  uint32_t err_code;
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
} __attribute__((packed));

void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(uint8_t irq);

#endif