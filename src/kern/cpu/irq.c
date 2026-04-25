#include <cpu/ccpu.h>
#include <cpu/idt.h>
#include <io.h>
#include <video/printf.h>
#include <video/video.h>
#include <cpu/irq.h>
#include <mem/acpi.h>

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

static uint8_t  pic_enabled  = 0;
static isr_hand irq_hand[16] = {0};

#define DESC_EXISTS 0x80
static struct irq_desc irq_descs[MAX_DESC];
extern struct int_override irq_overrides[];

void general_eoi() {
  lapic_eoi();
}

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

  pic_enabled = 0;
}

void pic_disable() {
  outb(PIC1_DATA, 0xff);
  outb(PIC2_DATA, 0xff);
  pic_enabled = 0;
}

void init_pic() {
  print_init("pic", "initializing pic remaps...", 0);
  pic_remap(0x20, 0x28);
}

void irq_handler(struct regs *r) {
  // printkf("irq exception %d\n", r->int_no - 32);

  struct irq_desc *d = &irq_descs[r->int_no - 32];

  if(d->flg & DESC_EXISTS) {
    d->count++;
  }

  if (r->int_no < 32 || r->int_no >= 48)
    return;
  isr_hand e = irq_hand[r->int_no - 32];
  if (!e) {
    printkf("no irq handler! unhandling irq %d...\n", r->int_no - 32);
    return;
  }

  e(r);
  return;
}

const char *getflow(uint8_t flg) {
  uint8_t a = (flg & MENTRY_TRG_MASK);
  switch(a) {
    case MENTRY_TRG_LEVEL:
      return "level";
    case MENTRY_TRG_EDGE:
      return "edge";
    case MENTRY_TRG_NONE:
      return "edge";
  };
  return "idk";
}

char getpol(uint8_t flg) {
  uint8_t a = (flg & MENTRY_POL_MASK);
  switch (a) {
    case MENTRY_POL_NONE:    
    case MENTRY_POL_HI:
      return '+';
    case MENTRY_POL_LO:
      return '-';
  }
  return ' ';
}

void show_interrupts() {
  for(int i = 0; i < MAX_DESC; i++) {
    struct irq_desc *dsc = &irq_descs[i];
    if(dsc->flg & DESC_EXISTS) {
      printkf("% 2d: % 8d", i, dsc->count);
      printkf("% 8s% 5d-", dsc->chipname, dsc->irq);
      printkf("%-7s %c %s\n", getflow(dsc->flg), getpol(dsc->flg), dsc->name);
    }
  }
}

void register_irq(isr_hand r, uint8_t no, const char *name) {
  ioapic_set_irq(no, no + 32);

  irq_descs[no].chipname = "IO-APIC";
  irq_descs[no].count = 0;
  irq_descs[no].flg = irq2flg(no) | DESC_EXISTS;
  irq_descs[no].name = name;
  irq_descs[no].irq = irq2gsi(no);

  irq_hand[no] = r;
}