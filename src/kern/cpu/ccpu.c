#include "cpu/ccpu.h"
#include "cpuid.h"
#include "video/printf.h"
#include "video/video.h"

struct cpu_capat c_capat;

static const char *syms[] = {
    "fpu", "vme", "de", "pse", "tsc", "msr", "pae",
    "me", "cx8", "apic", "sep", "mtrr", "pge", "mca",
    "cmov", "pat", "pse36", "psn", "clfsh", "ds", "acpi",
    "mmx", "fxsr", "sse", "sse2", "ss", "htt",
    "tm", "pbe"};

const uint8_t idxl[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
12, 13, 14,15 ,16, 17, 18, 19,
21, 22, 23, 24, 25, 26, 27, 28, 29,
31};

void check_capat() {
  uint32_t tmp;
  __get_cpuid(1, &tmp, &tmp, &c_capat.ecx_leaf1, &c_capat.edx_leaf1);

  printk("cpu extensions: \e\x0e");

  uint32_t edx = c_capat.edx_leaf1;
  for(uint32_t i = 0; i < sizeof(idxl); i++) {
    if(edx & (1 << idxl[i])) {
      printk((char*)syms[i]);
      putchr(' ');
    }
  }

  uint32_t ecx = c_capat.ecx_leaf1;
  if(ecx & bit_RDRND) {
    printk("rdrnd");
  }
  // todo: more leafes
  printk("\e\x0f\n");
}

void rdmsr(uint32_t msr, uint32_t *restrict lo, uint32_t *restrict hi) {
  asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void wrmsr(uint32_t msr, uint32_t lo, uint32_t hi) {
  asm volatile("wrmsr" ::"a"(lo), "d"(hi), "c"(msr));
}

#define APIC_MSR_BASE 0x1B
#define APIC_MSR_BSP  0x100
#define APIC_MSR_EN   0x800

#define APIC_REG(off) (*(volatile uint32_t *)(apic_base + (off)))

uint32_t apic_base = 0;

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

void apic_write(uint16_t reg, uint32_t data) {
  APIC_REG(reg) = data;
  (void)APIC_REG(0x20);
}

uint32_t apic_read(uint16_t reg) {
  return APIC_REG(reg);
}

void set_apic() {
  print_init("apic", "initializing the apic...", 0);

  set_apic_base(get_apic_base());
  apic_write(0xf0, apic_read(0xf0) | 0x100);
  // todo: set ioapic (qemu SOMEHOW doesnt give me acpi) and interrupts via apic
}

/* fpu things */

extern void set_fpu();

void fpu_init() {
  print_init("fpu", "intializing floating point unit...", 0);
  set_fpu();
}