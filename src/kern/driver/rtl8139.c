#include "driver/pci.h"
#include "cpu/idt.h"
#include "io.h"
#include "mem/mem.h"
#include "video/printf.h"
// #include "video/video.h"
#include <stddef.h>
#include <stdint.h>

uint8_t *rxbf;
uint8_t desc_idx;
uint16_t devaddr;

void rtl_handler(struct regs *r) {
  (void)r;
  uint16_t stat = inw(devaddr + 0x3e);
  outw(devaddr + 0x3e, 0x05);
  if(stat & (1 << 2)) { // todo: recv logic

    pic_eoi2();
  }
  if(stat & (1 << 0)) {
    pic_eoi2();
  }
}

void rtl_transmit(void *buf, size_t len) {
  outl(devaddr + 0x20 + desc_idx*4, (uint32_t)buf);
  outl(devaddr + 0x10 + desc_idx*4, len & 0xfff);
  while((inl(devaddr + 0x10 + desc_idx*4) & (1 << 15)) == 0);
  desc_idx = (desc_idx + 1) % 4;
}

uint16_t khtons(uint16_t h) {
  return ((h & 0xff) << 8) | ((h >> 8) & 0xff);
}

void rtl8139_init(struct pci_hdr *hdr, uint32_t bus, uint32_t dev) {
  // todo: implement network driver when not lazy
  print_info("rtl", 1, "irqline: %d", hdr->tsh.dev.iline);
  setbit_cmd(2, bus, dev);

  devaddr = hdr->tsh.dev.bar[0] & 0xfffc;
  outb(devaddr + 0x52, 0x00);
  outb(devaddr + 0x37, 0x10);
  while ((inb(devaddr + 0x37) & 0x10) != 0)
    ;
  rxbf = kmalloc(8192 + 16 + 1500);
  outl(devaddr + 0x30, (uint32_t)rxbf);
  outw(devaddr + 0x3C, 0x0005);
  outl(devaddr + 0x44, 0xf | (1 << 7));
  outb(devaddr + 0x37,0x0C);

  register_irq(rtl_handler, 11);

  pic_cm(11);
  pic_cm(2);

  uint32_t mac[2];
  mac[0] = inl(devaddr);
  mac[1] = inl(devaddr + 4);
  uint8_t *k = (uint8_t *)mac;
  print_info("rtl", 0, "mac: %02x:%02x:%02x:%02x:%02x:%02x", k[0], k[1], k[2], k[3], k[4], k[5]);
}