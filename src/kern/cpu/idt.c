#include "cpu/idt.h"
#include "io.h"
#include "video/video.h"
#include "video/printf.h"

#define EX 0x8E
#define TR 0x8F

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10

#define ICW4_8086 0x01
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM 0x10

#define CASCADE_IRQ 2

volatile struct idtr idtr_s = {0};
volatile struct idt_gate idt_g[40];

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

isr_hand exception_hand[32] = {0};
isr_hand irq_hand[8] = {0};

void set_g(void (*a)(void), uint8_t idx, uint8_t flg) {
  idt_g[idx].flag = flg;
  idt_g[idx].seg = 0x08;
  idt_g[idx].offsetlow = (uint32_t)a & 0xffff;
  idt_g[idx].offsethi = ((uint32_t)a >> 16) & 0xffff;
}

void fill_idt() {
  set_g(_ex0, 0, EX);
  set_g(_ex1, 1, TR);
  set_g(_ex2, 2, EX);
  set_g(_ex3, 3, TR);
  set_g(_ex4, 4, TR);
  set_g(_ex5, 5, EX);
  set_g(_ex6, 6, EX);
  set_g(_ex7, 7, EX);
  set_g(_ex8, 8, TR);
  set_g(_ex9, 9, EX);
  set_g(_ex10, 10, EX);
  set_g(_ex11, 11, EX);
  set_g(_ex12, 12, EX);
  set_g(_ex13, 13, EX);
  set_g(_ex14, 14, EX);
  set_g(_ex15, 15, EX);
  set_g(_ex16, 16, EX);
  set_g(_ex17, 17, EX);
  set_g(_ex18, 18, TR);
  set_g(_ex19, 19, EX);
  set_g(_ex20, 20, EX);
  set_g(_ex21, 21, EX);
  set_g(_ex22, 22, EX);
  set_g(_ex23, 23, EX);
  set_g(_ex24, 24, EX);
  set_g(_ex25, 25, EX);
  set_g(_ex26, 26, EX);
  set_g(_ex27, 27, EX);
  set_g(_ex28, 28, EX);
  set_g(_ex29, 29, EX);
  set_g(_ex30, 30, EX);
  set_g(_ex31, 31, EX);

  set_g(_irq0, 32, EX);
  set_g(_irq1, 33, EX);
  set_g(_irq2, 34, EX);
  set_g(_irq3, 35, EX);
  set_g(_irq4, 36, EX);
  set_g(_irq5, 37, EX);
  set_g(_irq6, 38, EX);
  set_g(_irq7, 39, EX);
}

void set_idtr() {
  fill_idt();
  idtr_s.addr = (uint32_t)idt_g;
  idtr_s.siz = sizeof(idt_g) - 1;

  asm volatile("lidt (%0)" ::"r"(&idtr_s) : "memory");

  printk("interrupt ok\n");
}

void pic_sm(uint8_t line) { outb(PIC1_DATA, inb(PIC1_DATA) | (1 << line)); }

void pic_cm(uint8_t line) { outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << line)); }

void pic_eoi() { outb(PIC1_COMMAND, PIC_EOI); }



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
  printk("pic ok\n");
}

void isr_handler(struct regs *r) {
  printkf("interrupt at eip=%p, exc %d\n", r->eip, r->int_no);
  isr_hand e = exception_hand[r->int_no];
  if(!e) {
    printk("no exception handler! halting now...\n");
    asm volatile("cli");
    asm volatile("hlt");
  } 
  e(r);
  return;
}

void irq_handler(struct regs *r) {
  //printkf("irq exception %d\n", r->int_no - 32);
  
  if(r->int_no < 32 || r->int_no >= 40) return;
  isr_hand e = irq_hand[r->int_no - 32];
  if(!e) {
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