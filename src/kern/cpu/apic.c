#include "cpu/irq.h"
#include "mem/mem.h"
#include "mem/acpi.h"
#include "mem/paging.h"
#include "video/printf.h"
#include "cpu/ccpu.h"

#define MAX_OVERRIDE 20
#define OVERRIDE_EXISTS 0x80

#define LAPIC_ID_NONE 0xde

static volatile void *ioapic_addr;
static void *gsi_base;
static uint8_t lapic_id = LAPIC_ID_NONE;

struct int_override irq_overrides[MAX_OVERRIDE];

#define IOAPIC ((volatile uint32_t *)ioapic_addr)

void parse_madt() {
  struct madt_tab *tab = get_acpitab("APIC");
  if(!tab) return;
  struct madt_entry *ptr = tab->mentry;
  void *end = (void *)tab + tab->h.len;

  while((void *)ptr < end) {
    //printkf("type: %d\n", ptr->type);

    switch (ptr->type) {
      case MENTRY_LAPIC: {
        struct mentry_lapic *lapic = (void *)ptr->data;
        if(lapic_id == LAPIC_ID_NONE)
          lapic_id = lapic->apic_id;
        break;
      }
      case MENTRY_IOPIC: {
        struct mentry_iopic *ioapic = (void *)ptr->data;
        ioapic_addr = (void *)ioapic->ioapic_addr;
        gsi_base = (void *)ioapic->gsi_base;
        break;
      }
      case MENTRY_IOVERRIDE: {
        struct mentry_iover *iso = (void *)ptr->data;
        irq_overrides[iso->irq_src].bus = iso->bus_src;
        irq_overrides[iso->irq_src].source = iso->irq_src;
        irq_overrides[iso->irq_src].gsi = iso->gsi;
        irq_overrides[iso->irq_src].flg = iso->flags | OVERRIDE_EXISTS;
        break;
      }
    }

    ptr = (void *)ptr + ptr->len;
  }
}

#define IOREGSEL 0x00
#define IOWIN 0x10

uint32_t ioapic_read(uint8_t reg) {
  IOAPIC[IOREGSEL/4] = reg;
  return IOAPIC[IOWIN/4];
}

void ioapic_write(uint8_t reg, uint32_t value) {
  IOAPIC[IOREGSEL/4] = reg;
  IOAPIC[IOWIN/4] = value;
}

int irq2gsi(int irq) {
  if(irq_overrides[irq].flg & OVERRIDE_EXISTS) {
    return irq_overrides[irq].gsi;
  }
  return irq;
}

int irq2flg(int irq) {
  if(irq_overrides[irq].flg & OVERRIDE_EXISTS) {
    return irq_overrides[irq].flg & 0xF;
  }
  return 0x00;
}

void ioapic_set_irq(int irq, int vect) {
  int gsi = irq2gsi(irq);

  uint64_t entry = 0;
  entry |= vect;
  entry |= ((uint64_t)lapic_id) << 56;

  ioapic_write(0x10 + gsi * 2, (uint32_t)entry);
  ioapic_write(0x10 + gsi * 2 + 1, entry >> 32);
}

/* lapic subsystem */

#define APIC_MSR_BASE 0x1B
#define APIC_MSR_BSP  0x100
#define APIC_MSR_EN   0x800

#define APIC_REG(off) (*(volatile uint32_t *)(apic_base + (off)))

uint32_t apic_base = 0;

#define LAPIC ((volatile uint32_t *)apic_base)
#define LAPIC_EOI 0xb0

uint32_t get_apic_base() {
  uint32_t lo, hi;
  rdmsr(APIC_MSR_BASE, &lo, &hi);
  apic_base = lo & 0xfffff000;
  return apic_base;
}

void set_apic_base(uint32_t addr) {
  uint32_t hi = 0;
  uint32_t lo = (addr & 0xfffff000) | APIC_MSR_EN;
  wrmsr(APIC_MSR_BASE, lo, hi);
}

void lapic_write(uint8_t reg, uint32_t val) {
  LAPIC[reg / 4] = val;
  (void)LAPIC[0x20/4]; // "flush" as required by osdev wiki
}

uint32_t lapic_read(uint8_t reg) {
  return LAPIC[reg/4];
}

void lapic_eoi() {
  lapic_write(LAPIC_EOI, 0);
}

void set_apic() {
  print_init("apic", "initializing the apic...", 0);

  set_apic_base(get_apic_base());
  lapic_write(0xf0, lapic_read(0xf0) | 0x100);
}

void ioapic_init() {
  // map_4mmio(0xfec00000, 0xfec00000);
  // see mem/paging.c - kinda hardcoded but ok
  print_init("ioapic", "initializing the io apic...", 0);

  parse_madt();

  pic_disable();

  ioapic_set_irq(0, 32); // timer
  ioapic_set_irq(1, 33); // kbd

  ioapic_set_irq(4, 36); // serial
}