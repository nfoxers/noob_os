#include "mem/acpi.h"
#include "video/printf.h"
#include <mem/mem.h>

static struct acpi_rsdt *rsdt;

void *_get_tab(char sig[4], struct acpi_rsdt *r) {
  size_t len    = r->h.len;
  size_t tables = (len - sizeof(r->h)) / sizeof(r->ptr[0]);

  for(size_t i = 0; i < tables; i++) {
    struct acpi_header_g *g = (struct acpi_header_g *)r->ptr[i];
    //printkf("tab %d: %.4s\n", i, g->sig);
    if(!memcmp(g->sig, sig, 4)) {
      return g;
    }
  }

  //printkf("tables: %d\n", tables);
  return NULL;
}

void scan_acpi() {

  char *addr = (void *)(uintptr_t)(*(uint16_t *)0x40e << 4);

  for (size_t i = 0; i < 1024 * 1024; i += 16) {
    if (!memcmp(addr + i, "RSD PTR ", 8)) {
      addr = addr + i;
      goto fond;
    }
  }

  for (addr = (char *)0xe0000; addr < (char *)0xfffff; addr += 16) {
    if (!memcmp(addr, "RSD PTR ", 8)) {
      goto fond;
    }
  }

fond:;
  struct acpi_rsdp *r = (struct acpi_rsdp *)addr;
  rsdt = (struct acpi_rsdt *)r->rsdt_addr;

  return;
}

void *get_acpitab(char sig[4]) {
  if(!rsdt) {
    scan_acpi();
  }
  return _get_tab(sig, rsdt);
}