#include "driver/pci.h"
#include "io.h"
#include "lib/list.h"
#include "video/printf.h"
// #include "video/video.h"
#include <stddef.h>
#include <stdint.h>
#include "mem/mem.h"

struct pcidev {
  struct pci_hdr *hdr;
  uint32_t id;
  uint32_t loc;
  uint8_t func;

  struct msix_entry *entries;

  struct list_head node;
};

struct pcidev pcihead;

uint32_t pci_readl(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  return inl(PCI_CFG_DATA);
}

uint16_t pci_readw(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  uint32_t tmp = (uint16_t)((inl(PCI_CFG_DATA) >> ((off & 2) * 8)) & 0xffff);
  return tmp;
}

uint8_t pci_readb(uint32_t bus, uint32_t slot, uint32_t func, uint8_t off) {
  uint8_t align = off & ~3;
  uint32_t v = pci_readl(bus, slot, func, align);

  return (v >> ((off & 3) * 8)) & 0xff;
}

void pci_writel(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint32_t data) {
  uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (off & 0xFC) | ((uint32_t)0x80000000);
  outl(PCI_CFG_ADDR, addr);
  outl(PCI_CFG_DATA, data);
}

void pci_writew(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint16_t data) {
  uint32_t align = off & ~3;
  uint32_t o = pci_readl(bus, slot, func, align);
  uint8_t shift = (off & 2) * 8;
  uint32_t mask = 0xffff << shift;
  uint32_t new = (o & mask) | ((uint32_t)data << shift);

  pci_writel(bus, slot, func, align, new);
}

void pci_writeb(uint32_t bus, uint32_t slot, uint32_t func, uint32_t off, uint8_t data) {
  uint32_t align = off & ~3;
  uint32_t o = pci_readl(bus, slot, func, align);
  uint8_t shift = (off & 3) * 8;
  uint32_t mask = 0xff << shift;
  uint32_t new = (o & mask) | ((uint32_t)data << shift);

  pci_writel(bus, slot, func, align, new);
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

void pci_readall(struct pci_hdr *h, uint32_t bus, uint32_t dev, uint32_t func) {
  uint32_t *hdr = (uint32_t *)h;
  for (uint32_t i = 0; i < sizeof(struct pci_hdr) / 4; i++) {
    hdr[i] = pci_readl(bus, dev, func, i * 4);
  }
}

const char *classcode(uint8_t code) {
  switch (code) {
  case 0:
    return "unclassified";
  case 0x1:
    return "mass controller"; // mass storage
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
    return "simplecomms controller"; // simple comms
  case 0x8:
    return "base sys prp"; // base system peripheral
  case 0x9:
    return "input dev controller"; // device input
  case 0xa:
    return "dock station";
  case 0xb:
    return "proc";
  case 0xc:
    return "serialbus controll";
  case 0xd:
    return "wireless controller";
  case 0xe:
    return "intel conroller";
  case 0xf:
    return "sat comms cont"; // who even uses this
  case 0x10:
    return "enc cont";
  case 0x11:
    return "signal process cont"; // signal processing
  default:
    return "idk";
  }
}

void pci_enumerate() {
  struct list_head *head = &pcihead.node;
  if(head->next == head) return;
  head = head->next;
  while(head != &pcihead.node) {

    struct pcidev *d = container_of(head, struct pcidev, node);

    printkf("%02x:%02x.%x ", d->loc >> 16, d->loc & 0xffff, d->func);
    printkf("%s: ", classcode(d->hdr->common.clas));
    printkf("vendor=%04x device=%04x ", d->id & 0xffff, d->id >> 16);
    printkf("class=%04x subclass=%04x\n", d->hdr->common.clas, d->hdr->common.subclass);


    head = head->next;
  }
}

void rtl8139_init(struct pci_hdr *hdr, uint32_t bus, uint32_t dev);

void pci_init() {
  print_init("pci", "initializing pci devices...", 0);

  init_list(&pcihead.node);

  uint32_t bus = 0;
loop:
  for (uint32_t dev = 0; dev < 32; dev++) {
    for (uint32_t func = 0; func < 8; func++) {
      uint32_t id = pci_readl(bus, dev, func, 0);
      if (id != 0xffffffff) {
        struct pci_hdr *h = malloc(sizeof(*h));
        pci_readall(h, bus, dev, 0);

        struct pcidev *pdev = malloc(sizeof(*pdev));
        pdev->hdr = h;
        pdev->loc = bus << 16 | dev;
        pdev->id = id;
        pdev->func = func;
        pdev->entries = 0;
        list_add(&pdev->node, &pcihead.node);

        uint16_t stat = h->tsh.dev.capptr;
        if(stat & (1 << 4)) {
          // dev has capabilities, check for MSI-X

          uint8_t cap = pci_readb(bus, dev, func, 0x34);
          while(cap) {
            uint8_t cap_id = pci_readb(bus, dev, func, cap);
            uint8_t next = pci_readb(bus, dev, func, cap + 1);

            if(cap_id == 0x05) { // MSI unused
              //printkf("msi\n");
            } else if(cap_id == 0x11) { // MSI-X
              //printkf("msi-x\n");
              //uint16_t cont = pci_readw(bus, dev, func, cap + 2);
              uint32_t table = pci_readl(bus, dev, func, cap + 4);
              //uint32_t pba = pci_readl(bus, dev, func, cap + 8);

              uint8_t bir = table & 0x7;
              uint32_t off = table & ~0x7;

              uint32_t bar_addr = h->tsh.dev.bar[bir];
              //printkf("bar: %x + %x\n", bar_addr, off);
              pdev->entries = (void *)(bar_addr + off);

              // todo: configure msi-x
            }

            cap = next;
          }
        }

        // todo: remove this
        uint32_t k   = pci_readl(bus, dev, func, 0xc);
        uint8_t  hdr = (k >> 16);

        if ((hdr & 0x7f) == 0x1) {
          struct pci_hdr a;
          pci_readall(&a, bus, dev, func);
          bus = a.tsh.bridge.ss_busnum;
          goto loop;
        }

        if (func == 0) {
          if (!(hdr & 0x80)) {
            break;
          }
        }
      }
    }
  }
}