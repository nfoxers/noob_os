#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include <cpu/idt.h>

#define MAX_DESC 40

struct irq_chip {
  const char *name;
};

struct irq_desc {
  uint32_t irq;
  uint32_t count;
  uint8_t flg;
  const char *name;

  //struct irq_chip *chip;
  const char *chipname;
};

void init_pic();

void pic_disable();

void register_irq(isr_hand r, uint8_t no, const char *name);

void show_interrupts();

#endif