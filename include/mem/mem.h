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
void kstrncpy(const char *s, char *d, uint16_t siz);

void kmemcpy(const void *s, void *d, uint16_t siz);
uint8_t kmemcmp(const void *s, const void *d, uint16_t siz);
void kmemset(void *s, uint8_t c, size_t len);

void kstrcpy(const char *s, char *d);
uint8_t kstrncmp(const char *s, const char *d, uint16_t siz);
uint8_t kstrcmp(const char *s, const char *d);
size_t kstrlen(const char *s);
void zero_bss();

void *kmalloc(uint16_t size);
void stupidfree(uint16_t size);

#endif