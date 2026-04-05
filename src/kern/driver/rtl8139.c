#include "cpu/idt.h"
#include "driver/pci.h"
#include "io.h"
#include "mem/mem.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>

#define GATEWAY \
  (uint8_t[]) { 192, 168, 100, 1 } // qemu usenet gateway
#define IP \
  (uint8_t[]) { 192, 168, 100, 2 }

// #define ARP_TEST

static uint8_t *rxbf;
static uint8_t *mac;
static uint8_t  desc_idx;
static uint16_t devaddr;

void rtl_handler(struct regs *r) {
  // printkf("rtl handler\n");
  (void)r;
  uint16_t stat = inw(devaddr + 0x3e);
  outw(devaddr + 0x3e, 0x05);
  if (stat & (1 << 2)) { // todo: recv logic
    // printkf("tok\n");
    pic_eoi2();
  }
  if (stat & (1 << 0)) {
    // printkf("rok\n");
    pic_eoi2();
  }
}

void rtl_transmit(void *buf, size_t len) {
  outl(devaddr + 0x20 + desc_idx * 4, (uint32_t)buf);
  outl(devaddr + 0x10 + desc_idx * 4, len & 0xfff);
  while ((inl(devaddr + 0x10 + desc_idx * 4) & (1 << 15)) == 0)
    ;
  desc_idx = (desc_idx + 1) % 4;
}

uint16_t khtons(uint16_t h) {
  return ((h & 0xff) << 8) | ((h >> 8) & 0xff);
}

#define HTONS(x) (((x) & 0xff) << 8) | (((x) >> 8) & 0xff) // for const values

#if defined(ARP_TEST)
#include "ether/if_ether.h"
void send_arp_test() {
  uint8_t        buf[sizeof(struct arphdr) + sizeof(struct ethhdr)];
  struct arphdr *ahdr = (struct arphdr *)(buf + sizeof(struct ethhdr));
  ahdr->htype         = HTONS(ARP_HTYPE);
  ahdr->ptype         = HTONS(ARP_PTYPE);
  ahdr->plen          = ARP_PLEN;
  ahdr->hlen          = ARP_HLEN;
  ahdr->op            = HTONS(ARP_OP_REQ);

  kmemcpy(&ahdr->srcaddr, mac, ETH_ALEN);
  kmemcpy(&ahdr->saddr, IP, ARP_PLEN);

  kmemset(&ahdr->dstaddr, 0x00, ETH_ALEN);
  kmemcpy(&ahdr->daddr, GATEWAY, ARP_PLEN);

  struct ethhdr *ehdr = (struct ethhdr *)buf;
  ehdr->h_proto       = HTONS(ETH_P_ARP);
  kmemcpy(&ehdr->h_source, mac, ETH_ALEN);
  kmemset(&ehdr->h_dest, 0xff, ETH_ALEN);

  rtl_transmit(buf, sizeof(struct arphdr) + sizeof(struct ethhdr));
}
#endif

void rtl8139_init(struct pci_hdr *hdr, uint32_t bus, uint32_t dev) {
  // todo: implement network driver when not lazy
  print_info("rtl", 1, "irqline: %d", hdr->tsh.dev.iline);

  setbit_cmd(2, bus, dev);

  devaddr = hdr->tsh.dev.bar[0] & 0xfffc;
  outb(devaddr + 0x52, 0x00);
  outb(devaddr + 0x37, 0x10);
  while ((inb(devaddr + 0x37) & 0x10) != 0)
    ;
  rxbf = malloc(8192 + 16 + 1500);
  outl(devaddr + 0x30, (uint32_t)rxbf);
  outw(devaddr + 0x3C, 0x0005);
  outl(devaddr + 0x44, 0xf | (1 << 7));
  outb(devaddr + 0x37, 0x0C);

  register_irq(rtl_handler, 11);

  pic_cm(11);
  pic_cm(2);

  static uint32_t tmac[2];
  tmac[0]    = inl(devaddr);
  tmac[1]    = inl(devaddr + 4);
  uint8_t *k = (uint8_t *)tmac;
  print_info("rtl", 0, "mac: %02x:%02x:%02x:%02x:%02x:%02x", k[0], k[1], k[2], k[3], k[4], k[5]);
  mac = k;

#if defined(ARP_TEST)
  STI;
  send_arp_test();
#endif
  //kfree(rxbf);
}