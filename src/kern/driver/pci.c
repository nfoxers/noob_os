#include "driver/pci.h"
#include "io.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC

// todo: rename these structure members!!!
struct pci_hdr_common {
  uint16_t vendid;
  uint16_t devid;
  uint16_t cmd;
  uint16_t status;
  uint8_t  revid;
  uint8_t  prog_if;
  uint8_t  subclass;
  uint8_t class;
  uint8_t clz;
  uint8_t lt;
  uint8_t htype;
  uint8_t bist;
} __attribute__((packed));

struct pci_hdr_dev {
  uint32_t bar[6];
  uint32_t cardptr;
  uint16_t subvendid;
  uint16_t subid;
  uint32_t erombs;
  uint8_t  capptr;
  uint8_t  res[7];
  uint8_t  iline;
  uint8_t  ipin;
  uint8_t  mingrant;
  uint8_t  maxlat;
} __attribute__((packed));

struct pci_hdr_brd {
  uint32_t bar[2];
  uint8_t  p_busnum;
  uint8_t  s_busnum;
  uint8_t  ss_busnum;
  uint8_t  s_lt;
  uint8_t  iobase;
  uint8_t  iolim;
  uint16_t s_stat;
  uint16_t membase;
  uint16_t memlim;
  uint16_t pmembase;
  uint16_t pmemlim;
  uint32_t pupperbase;
  uint32_t pupperlim;
  uint16_t ioupperbase;
  uint16_t ioupperlim;
  uint8_t  capptr;
  uint8_t  res[3];
  uint32_t erombs;
  uint8_t  iline;
  uint8_t  ipin;
  uint16_t bctrl;
} __attribute__((packed));

struct pci_hdr_crd {
  uint32_t cardbus_addr;
  uint8_t  cl_offset;
  uint8_t  res;
  uint16_t s_stat;
  uint8_t  pcibusnum;
  uint8_t  cbusnum;
  uint8_t  ss_busnum;
  uint8_t  cbus_lt;
  uint32_t membase0;
  uint32_t memlim0;
  uint32_t membase1;
  uint32_t memlim1;
  uint32_t iobase0;
  uint32_t iolim0;
  uint32_t iobase1;
  uint32_t iolim1;
  uint8_t  iline;
  uint8_t  ipin;
  uint16_t bctrl;
  uint16_t sdevid;
  uint16_t svendid;
  uint32_t pcmodebs;
} __attribute__((packed));

struct pci_hdr {
  struct pci_hdr_common common;
  union {
    struct pci_hdr_dev dev;
    struct pci_hdr_brd bridge;
    struct pci_hdr_crd card_bridge;
  } tsh; // "type specific header" lol
} __attribute__((packed));

uint16_t pci_readw(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  uint32_t tmp = (uint16_t)((inl(PCI_CFG_DATA) >> ((off & 2) * 8)) & 0xffff);
  return tmp;
}

uint32_t pci_readl(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  return inl(PCI_CFG_DATA);
}

uint16_t rdvendor(uint32_t bus, uint32_t slot) {
  return pci_readl(bus, slot, 0, 0) & 0xffff;
}

uint32_t get_class(uint32_t bus, uint32_t slot) {
  return pci_readl(bus, slot, 0, 0x8); // todo: figure out why this does not work
}

void pci_enumerate() {
  uint32_t bus;
  uint32_t dev;

  for (bus = 0; bus < 1; bus++) { // bus [1, 255] is rare b
    for (dev = 0; dev < 32; dev++) {
      if (rdvendor(bus, dev) != 0xffff) {
        printkf("found device at bus %d slot %d\n", bus, dev);
        printkf(" class:subclass = %04x\n", get_class(bus, dev));
      }
    }
  }
}