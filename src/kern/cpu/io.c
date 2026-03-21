#include <stdint.h>

#include "io.h"

void outb(uint16_t a, uint8_t d) {
  asm volatile("outb %b0, %w1" ::"a"(d), "Nd"(a) : "memory");
}

void outw(uint16_t a, uint16_t d) {
  asm volatile("outw %w0, %w1" ::"a"(d), "Nd"(a) : "memory");
}

void outl(uint16_t a, uint32_t d) {
  asm volatile("outl %k0, %w1" ::"a"(d), "Nd"(a) : "memory");
}

uint8_t inb(uint16_t a) {
  uint8_t ret;
  asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(a) : "memory");
  return ret;
}

uint16_t inw(uint16_t a) {
  uint16_t ret;
  asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(a) : "memory");
  return ret;
}

uint32_t inl(uint16_t a) {
  uint32_t ret;
  asm volatile("inl %w1, %k0" : "=a"(ret) : "Nd"(a) : "memory");
  return ret;
}
