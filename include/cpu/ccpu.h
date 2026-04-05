#ifndef CCPU_H
#define CCPU_H

#include "stdint.h"

struct cpu_capat {
  uint32_t ecx_leaf1;
  uint32_t edx_leaf1;

  uint32_t ebx_leaf7;
  uint32_t ecx_leaf7;
  uint32_t edx_leaf7;
};

void check_capat();
void rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void wrmsr(uint32_t msr, uint32_t lo, uint32_t hi);

void set_apic();
void fpu_init();

#endif