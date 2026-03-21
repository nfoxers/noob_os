#include "driver/pci.h"
#include "io.h"
#include "video/printf.h"
// #include "video/video.h"
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

void pci_writel(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint32_t data) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  outl(PCI_CFG_DATA, data);
}

uint16_t rdvendor(uint32_t bus, uint32_t slot) {
  return pci_readl(bus, slot, 0, 0) & 0xffff;
}

uint32_t get_class(uint32_t bus, uint32_t slot) {
  return pci_readl(bus, slot, 0, 0x8) >> 16;
}

void setbit_cmd(uint8_t bit, uint32_t bus, uint32_t slot) {
  bit = (bit >= 16) ? 15 : bit;

  uint32_t tmp = pci_readl(bus, slot, 0, 0x04);
  pci_writel(bus, slot, 0, 0x04, tmp | (1 << bit));
}

void pci_readall(struct pci_hdr *h, uint32_t bus, uint32_t dev) {
  uint32_t *hdr = (uint32_t *)h;
  for (uint32_t i = 0; i < sizeof(struct pci_hdr) / 4; i++) {
    hdr[i] = pci_readl(bus, dev, 0, i * 4);
  }
}

void rtl8139_init(struct pci_hdr *hdr, uint32_t bus, uint32_t dev);

const char *classcode(uint8_t code) {
  switch (code) {
  case 0:
    return "unclassified";
  case 0x1:
    return "ms controller"; // mass storage
  case 0x2:
    return "network controler";
  case 0x3:
    return "disp controller";
  case 0x4:
    return "mult controller"; // i.e. multimedia
  case 0x5:
    return "mem controller";
  case 0x6:
    return "bridge";
  case 0x7:
    return "sc controller"; // simple comms
  case 0x8:
    return "base sys prp"; // base system peripheral
  case 0x9:
    return "di controller"; // device input
  case 0xa:
    return "dock station";
  case 0xb:
    return "proc";
  case 0xc:
    return "sb controll";
  case 0xd:
    return "wireless controller";
  case 0xe:
    return "intel conroller";
  case 0xf:
    return "sat comms cont"; // who even uses this
  case 0x10:
    return "enc cont";
  case 0x11:
    return "sp cont"; // signal processing
  default:
    return "idk";
  }
}

void pci_enumerate() {
  for (uint32_t bus = 0; bus < 1; bus++) { // bus [1, 255] is rare b
    for (uint32_t dev = 0; dev < 32; dev++) {
      for (uint32_t func = 0; func < 8; func++) {
        uint32_t id = pci_readl(bus, dev, func, 0);
        if (id != 0xffffffff) {
          uint32_t reg = pci_readl(bus, dev, func, 0x08);

          printkf("%02x:%02x.%x ", bus, dev, func);
          printkf("%s: ", classcode((reg >> 24) & 0xff));
          printkf("vendor=%04x device=%04x ", id & 0xffff, id >> 16);
          printkf("class=%04x subclass=%04x\n", (reg >> 24) & 0xff, (reg >> 16) & 0xff);

          if (func == 0) {
            uint8_t hdr = (pci_readl(bus, dev, func, 0x0c) >> 16) & 0xff;
            if (!(hdr & 0x80)) {
              break;
            }
          }
        }
      }
    }
  }
}

void pci_init() {
  for (uint32_t bus = 0; bus < 1; bus++) { // bus [1, 255] is rare b
    for (uint32_t dev = 0; dev < 32; dev++) {
      for (uint32_t func = 0; func < 8; func++) {
        uint32_t id = pci_readl(bus, dev, func, 0);
        if (id != 0xffffffff) {
          struct pci_hdr h;
          pci_readall(&h, bus, dev);
          if(id == 0x813910ec) rtl8139_init(&h, bus, dev);
          
          if (func == 0) {
            uint8_t hdr = (pci_readl(bus, dev, func, 0x0c) >> 16) & 0xff;
            if (!(hdr & 0x80)) {
              break;
            }
          }
        }
      }
    }
  }
}

uint8_t rxbf[1024];

void rtl8139_init(struct pci_hdr *hdr, uint32_t bus, uint32_t dev) {
  // todo: implement network driver when not lazy
  setbit_cmd(2, bus, dev);
  uint16_t devaddr = hdr->tsh.dev.bar[0] & 0xfffc;
  printkf("da: %x\n", devaddr);
  outb(devaddr + 0x52, 0x00);
  outb(devaddr + 0x37, 0x10);
  while ((inb(devaddr + 0x37) & 0x10) != 0)
    ;
  outl(devaddr + 0x30, (uint32_t)rxbf);
  outw(devaddr + 0x3C, 0x0005);
  outl(devaddr + 0x44, 0xf | (1 << 7));
  outb(devaddr + 0x37, 0x0c);
  printkf("rtl minit\n");
}