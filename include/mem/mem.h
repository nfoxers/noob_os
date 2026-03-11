#ifndef MEM_H
#define MEM_H

#include "stdint.h"
#include "stddef.h"

struct bios_da {
  uint16_t com_port[4];
  uint16_t lpt_port[3];
  uint16_t ebda_addr;
  uint16_t flags;
  uint8_t pcjr;
  uint16_t usable_memory;
} __attribute__((packed));

void parse_bda();

char *kstrtok(char *str, const char *delim);
void kstrncpy(const char *s, char *d);
uint8_t kstrcmp(const char *s, char *d);
void zero_bss();

#endif