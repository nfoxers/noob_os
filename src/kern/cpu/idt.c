#include "cpu/idt.h"
#include "io.h"
#include "video/printf.h"
#include "video/video.h"

#define EX   0x8E
#define TR   0x8F
#define EX_U 0xEE

#define PIC1         0x20
#define PIC2         0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)

#define PIC_EOI 0x20

#define ICW1_ICW4      0x01
#define ICW1_SINGLE    0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL     0x08
#define ICW1_INIT      0x10

#define ICW4_8086       0x01
#define ICW4_AUTO       0x02
#define ICW4_BUF_SLAVE  0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM       0x10

#define CASCADE_IRQ 2

volatile struct idtr     idtr_s = {0};
volatile struct idt_gate idt_g[50];

extern void _ex0(void);
extern void _ex1(void);
extern void _ex2(void);
extern void _ex3(void);
extern void _ex4(void);
extern void _ex5(void);
extern void _ex6(void);
extern void _ex7(void);
extern void _ex8(void);
extern void _ex9(void);
extern void _ex10(void);
extern void _ex11(void);
extern void _ex12(void);
extern void _ex13(void);
extern void _ex14(void);
extern void _ex15(void);
extern void _ex16(void);
extern void _ex17(void);
extern void _ex18(void);
extern void _ex19(void);
extern void _ex20(void);
extern void _ex21(void);
extern void _ex22(void);
extern void _ex23(void);
extern void _ex24(void);
extern void _ex25(void);
extern void _ex26(void);
extern void _ex27(void);
extern void _ex28(void);
extern void _ex29(void);
extern void _ex30(void);
extern void _ex31(void);

extern void _irq0(void);
extern void _irq1(void);
extern void _irq2(void);
extern void _irq3(void);
extern void _irq4(void);
extern void _irq5(void);
extern void _irq6(void);
extern void _irq7(void);

extern void _irq8(void);
extern void _irq9(void);
extern void _irq10(void);
extern void _irq11(void);
extern void _irq12(void);
extern void _irq13(void);
extern void _irq14(void);
extern void _irq15(void);

extern void _ex48(void);

static isr_hand exception_hand[50] = {0};
static isr_hand irq_hand[16]       = {0};

static void set_g(void (*a)(void), uint8_t idx, uint8_t flg) {
  idt_g[idx].flag      = flg;
  idt_g[idx].seg       = 0x08;
  idt_g[idx].offsetlow = (uint32_t)a & 0xffff;
  idt_g[idx].offsethi  = ((uint32_t)a >> 16) & 0xffff;
}

static void (*const itab[])(void) = {
  _ex0,  _ex1, _ex2, _ex3, _ex4, _ex5, _ex6, _ex7, _ex8, _ex9,
  _ex10,  _ex11, _ex12, _ex13, _ex14, _ex15, _ex16, _ex17, _ex18, _ex19,
  _ex20,  _ex21, _ex22, _ex23, _ex24, _ex25, _ex26, _ex27, _ex28, _ex29,
  _ex30, _ex31,

  _irq0, _irq1, _irq2, _irq3, _irq4, _irq5, _irq6, _irq7, _irq8, _irq9,
  _irq10, _irq11, _irq12, _irq13, _irq14, _irq15,

  _ex48
};

void fill_idt() {
  for(uint32_t i = 0; i < sizeof(itab)/sizeof(itab[0]); i++) {
    set_g(itab[i], i, EX_U);
  }
}

void set_idtr() {
  fill_idt();
  idtr_s.addr = (uint32_t)idt_g;
  idtr_s.siz  = sizeof(idt_g) - 1;

  asm volatile("lidt (%0)" ::"r"(&idtr_s) : "memory");
}

void pic_sm(uint8_t line) {
  if (line < 8) {
    outb(PIC1_DATA, inb(PIC1_DATA) | (1 << line));
    return;
  }
  outb(PIC2_DATA, inb(PIC2_DATA) | (1 << (line - 8)));
}

void pic_cm(uint8_t line) {
  if (line < 8) {
    outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << line));
    return;
  }
  outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << (line - 8)));
}

void pic_eoi() { outb(PIC1_COMMAND, PIC_EOI); }
void pic_eoi2() { outb(PIC2_COMMAND, PIC_EOI); }

void pic_remap(int offset1, int offset2) {
  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC1_DATA, offset1);
  io_wait();
  outb(PIC2_DATA, offset2);
  io_wait();
  outb(PIC1_DATA, 1 << CASCADE_IRQ);
  io_wait();
  outb(PIC2_DATA, 2);
  io_wait();

  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  outb(PIC1_DATA, 0xff); // disable irqs alltogether
  outb(PIC2_DATA, 0xff);
}

void init_pic() {
  pic_remap(0x20, 0x28);
}

void isr_handler(struct regs *r) {
  isr_hand e = exception_hand[r->int_no];
  if (!e) {
    printkf("interrupt at eip=%p, exc %d\n", r->eip, r->int_no);
    printk("no exception handler! halting now...\n");
    asm volatile("cli");
    asm volatile("hlt");
  }
  e(r);
  return;
}

void irq_handler(struct regs *r) {
  //printkf("irq exception %d\n", r->int_no - 32);

  if (r->int_no < 32 || r->int_no >= 48)
    return;
  isr_hand e = irq_hand[r->int_no - 32];
  if (!e) {
    printk("no irq handler! unhandling irq...\n");
    return;
  }
  e(r);
  return;
}

void register_ex(isr_hand r, uint8_t no) {
  exception_hand[no] = r;
}

void register_irq(isr_hand r, uint8_t no) {
  irq_hand[no] = r;
}