#ifndef IDT_H
#define IDT_H

#include "stdint.h"

#define CLI { asm volatile("cli"); }
#define STI { asm volatile("sti"); }

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
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;

  uint32_t int_no;
  uint32_t err_code;

  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
} __attribute__((packed));

typedef void (*isr_hand)(struct regs *r);

void set_idtr();
void init_pic();

void pic_cm(uint8_t line);
void pic_sm(uint8_t line);
void pic_eoi();

void register_ex(isr_hand r, uint8_t no);
void register_irq(isr_hand r, uint8_t no);

#endif