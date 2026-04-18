#include "cpu/idt.h"
#include "cpu/ccpu.h"
#include "io.h"
#include "video/printf.h"
#include "video/video.h"

#define EX   0x8E
#define TR   0x8F
#define EX_U 0xEE

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
  print_init("idt", "setting up the idt...", 0);

  fill_idt();
  idtr_s.addr = (uint32_t)idt_g;
  idtr_s.siz  = sizeof(idt_g) - 1;

  asm volatile("lidt (%0)" ::"r"(&idtr_s) : "memory");
  
}

const char *excname[] = {
  "div err",
  "debug",
  "nmi",
  "breakdown", // i.e. breakpoint
  "overflow",
  "bound range err",
  "invalid opcode",
  "invalid device",
  "double fault",
  "coproc seg overrun",
  "invalid tss",
  "segment not presnt",
  "stack segment",
  "general protection fault",
  "page fault",
  "res",
  "x87 float exc",
  "alignment check",
  "machine check",
  "simd float exc",
  "virtualization exc",
  "control protection exc",
};

void isr_handler(struct regs *r) {
  isr_hand e = exception_hand[r->int_no];
  if (!e) {
    printkf("interrupt at eip=%p, exc %d (%s)\n", r->eip, r->int_no, r->int_no < sizeof(excname)/sizeof(excname[0]) ? excname[r->int_no] : "out of bounds");
    printk("no exception handler! halting now...\n");
    asm volatile("cli");
    asm volatile("hlt");
  }
  e(r);
  return;
}

void register_ex(isr_hand r, uint8_t no) {
  exception_hand[no] = r;
}