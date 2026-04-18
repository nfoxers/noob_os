#ifndef ACPI_H
#define ACPI_H

#include "mem/mem.h"

struct acpi_rsdp {
  char     sig[8];
  uint8_t  checksum;
  char     oem[6];
  uint8_t  rev;
  uint32_t rsdt_addr;
} __attribute__((packed));

struct acpi_xsdp {
  struct acpi_rsdp rsdp;

  uint32_t len;
  uint64_t xsdt_addr;
  uint8_t  ext_checksum;
  uint8_t  res[3];
} __attribute__((packed));

struct acpi_header_g {
  char     sig[4];
  uint32_t len;
  uint8_t  rev;
  uint8_t  chk;
  char     oem[6];
  char     oemtab[8];
  uint32_t oemrev;
  uint32_t creatid;
  uint32_t creatrev;
} __attribute__((packed));

struct acpi_rsdt {
  struct acpi_header_g h;
  uint32_t             ptr[];
} __attribute__((packed));

/* rsdt sdt types */

struct fadt_tab {
  struct acpi_header_g h;
  uint32_t             fctrl;
  uint32_t             dsdt;

  // todo: add more fields when necessary
} __attribute__((packed));

struct madt_entry {
  uint8_t type;
  uint8_t len;
  uint8_t data[];
} __attribute__((packed));

struct madt_tab { // mad table table :-)
  struct acpi_header_g h;
  uint32_t             lapic_addr;
  uint32_t             flags;
  struct madt_entry    mentry[];
} __attribute__((packed));

struct hpet_tab {
  struct acpi_header_g h;
  uint8_t              hrevid;

  uint8_t ccount : 5;
  uint8_t csiz : 1;
  uint8_t res : 1;
  uint8_t legacy : 1;

  uint16_t pcivendid;

  struct {
    uint8_t  addr_id;
    uint8_t  reg_bitwidth;
    uint8_t  reg_bitoff;
    uint8_t  res2;
    uint64_t addr;
  } addr __attribute__((packed));

  uint8_t  hpetno;
  uint16_t min_tick;
  uint8_t  pgprot;
} __attribute__((packed));

/* other tables */

struct dsdt_tab {
  struct acpi_header_g h;
  uint8_t              aml[];
} __attribute__((packed));

/* constants */

#define MENTRY_LAPIC         0
#define MENTRY_IOPIC         1
#define MENTRY_IOVERRIDE     2
#define MENTRY_NMISRC        3
#define MENTRY_LAPICNMI      4
#define MENTRY_LAPICOVERRIDE 5
#define MENTRY_X2APIC        9

// structures for only important things

#define MENTRY_POL_NONE 0x00
#define MENTRY_POL_HI   0x01
#define MENTRY_POL_RES  0x02
#define MENTRY_POL_LO   0x03
#define MENTRY_POL_MASK MENTRY_POL_LO

#define MENTRY_TRG_NONE  0x00
#define MENTRY_TRG_EDGE  (0x01 << 2)
#define MENTRY_TRG_RES   (0x02 << 2)
#define MENTRY_TRG_LEVEL (0x03 << 2)
#define MENTRY_TRG_MASK MENTRY_TRG_LEVEL

struct mentry_lapic {
  uint8_t  acpi_id;
  uint8_t  apic_id;
  uint32_t flags;
} __attribute__((packed));

struct mentry_iopic {
  uint8_t  ioapic_id;
  uint8_t  res;
  uint32_t ioapic_addr;
  uint32_t gsi_base;
} __attribute__((packed));

struct mentry_iover {
  uint8_t  bus_src;
  uint8_t  irq_src;
  uint32_t gsi;
  uint16_t flags;
} __attribute__((packed));

/* data structures for kernel usage */

struct int_override {
  uint8_t bus, source, flg;
  uint32_t gsi;
};

/* functions */

void  scan_acpi();
void *get_acpitab(char sig[4]);

#endif