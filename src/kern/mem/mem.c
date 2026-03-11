#include "mem/mem.h"
#include "video/printf.h"
#include <stdint.h>

#define BDA_ADDR ((struct bios_da *)0x400)

extern uint8_t __bss_start__;
extern uint8_t __bss_end__;

void parse_bda() {
  printkf("com1 port: %x\n", BDA_ADDR->com_port[0]);
  printkf("lpt1 port: %x\n", BDA_ADDR->lpt_port[0]);
  printkf("usable memory: %d KB\n", BDA_ADDR->usable_memory);
  printkf("ebda address: 0x%p\n", BDA_ADDR->ebda_addr << 4);
}

uint8_t dlim(char c, const char *delim) {
  while (*delim) {
    if (c == *delim)
      return 1;
    delim++;
  }
  return 0;
}

char *kstrtok(char *str, const char *delim) {
  static char *next;
  char *start;

  if (str != NULL)
    next = str;
  if (next == NULL)
    return NULL;

  while (*next && dlim(*next, delim))
    next++;

  if (*next == '\0') {
    next = NULL;
    return NULL;
  }

  start = next;

  while (*next && !dlim(*next, delim))
    next++;

  if (*next) {
    *next = '\0';
    next++;
  } else {
    next = NULL;
  }

  return start;
}

void kstrncpy(const char *s, char *d) {
  while ((*d++ = *s++))
    ;
}

uint8_t kstrcmp(const char *s, char *d) {
  while (*s == *d && *s)
    s++, d++;
  return *(uint8_t *)s - *(uint8_t *)d;
}

void kmemset(uint8_t *s, uint8_t c, size_t len) {
  asm volatile("rep stosb" : "+D"(s), "+c"(len) : "a"(c) : "memory");
}

void zero_bss() {
  uint8_t *p = &__bss_start__;
  while (p < &__bss_end__) {
    *p++ = 0;
  }
}