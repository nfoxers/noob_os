#include "mem/mem.h"
#include "video/printf.h"
#include <stdint.h>

#define SMB_START 0x000f0000
#define SMB_END   0x00ffffff

struct eps3 {
  char string[5];
  uint8_t checksum;
  uint8_t len;
  uint8_t majver;
  uint8_t minver;
  uint8_t docrev;
  uint8_t epsrev;
  uint8_t res;
  uint32_t maxsiz;
  uint32_t tabaddr_lo;
  uint32_t tabaddr_hi;
} __attribute__((packed));

struct smbhead {
  uint8_t typ;
  uint8_t len;
  uint16_t handle;
} __attribute__((packed));

struct smbhead_typ0 { // firmware info
  struct smbhead head;
  uint8_t vendor;
  uint8_t firmver;
  uint16_t bios_startaddr;
  uint8_t reldat;
  uint8_t romsiz;
  uint64_t charbmp;
} __attribute__((packed));

size_t smbios_headlen(struct smbhead *hd) {
  size_t i;
  const char *strtab = (const char *)hd + hd->len;

  i = 1;
  while((strtab[i-1] != 0) || (strtab[i] != 0))
    i++;

  size_t k = hd->len + i + 1;
  return k;
}

const char *getstr(struct smbhead *c, uint32_t idx) {
  const char *strtab = (const char *)c + c->len;
  idx--;
  while(idx) {
    strtab = strtab + kstrlen(strtab);
    idx--;
  }
  return strtab;
}

void print_smbinfo(struct smbhead *start) {
  struct smbhead *cur = start;

  // here we go the iterator
  while(cur->typ != 127) {

    if(cur->typ == 0) {
      //struct smbhead_typ0 *t = (struct smbhead_typ0 *)cur;
      // todo: proper smbios handling and output
    }
    cur = (struct smbhead *)((uint8_t*)cur + smbios_headlen(cur));
  }
}

void smbios_scan(void) {
  print_info("info", 2, "scanning for smbios...");
  uint8_t *eps = (uint8_t *)SMB_START;
  uint32_t len, i;
  uint8_t chk = 0;

  while(eps <= (uint8_t*)SMB_END) {
    if(!kmemcmp(eps, "_SM3_", 5)) {
      len = eps[6];
      chk = 0;
      for(i = 0; i < len; i++) {
        chk += eps[i];
      }
      if(chk == 0) {
        struct eps3 *ep = (struct eps3 *)eps;
        print_init("smbios", "smbios found", 0);
        print_info("addr", 0, "table addr: %p", ep);
        print_smbinfo((struct smbhead *)ep->tabaddr_lo);
        break;
      }
    }
    eps += 16;
  }

  if((uint32_t)eps == SMB_END + 1) {
    print_init("smbios", "smbios not found", 1);
    return;
  }
}