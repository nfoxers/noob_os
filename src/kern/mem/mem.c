#include "mem/mem.h"
#include "video/printf.h"
#include <stdint.h>

#define BDA_ADDR ((struct bios_da *)0x400)

extern uint8_t __bss_start__;
extern uint8_t __bss_end__;

void *heap_ptr = 0;

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

void kstrncpy(const char *s, char *d, uint16_t siz) {
  while(siz--) {
    *d++ = *s++;
  }
}

void kstrcpy(const char *s, char *d) {
  while((*d++ = *s++)) ; 
}

void kmemcpy(const void *s, void *d, uint16_t siz) {
  for(; siz > 0; siz--) {
    *(uint8_t *)d++ = *(uint8_t *)s++;
  }
}

uint8_t kmemcmp(const void *s, const void *d, uint16_t siz) {
  while(siz && (*(uint8_t*)s == *(uint8_t*)d)) s++, d++, siz--;
  if(siz == 0) return 0;
  return *(uint8_t *)s - *(uint8_t *)d;
}

void kmemset(void *s, uint8_t c, size_t len) {
  asm volatile("rep stosb" : "+D"(s), "+c"(len) : "a"(c) : "memory");
}

uint8_t kstrcmp(const char *s, const char *d) {
  while (*s == *d && *s)
    s++, d++;
  return *(uint8_t *)s - *(uint8_t *)d;
}

uint8_t kstrncmp(const char *s, const char *d, uint16_t size) {

  while(size && *s && (*s == *d)) {
    s++;
    d++;
    size--;
  }

  if(size == 0) return 0;

  return *(uint8_t *)s - *(uint8_t *)d;
}

size_t kstrlen(const char *s) {
  const char *d = s;
  while((*d++));
  return d - s - 1;
}

void zero_bss() {
  heap_ptr = &__bss_end__;
  uint8_t *p = &__bss_start__;
  while (p < &__bss_end__) {
    *p++ = 0;
  }
}

void *kmalloc(uint16_t size) { // stupid allocator
  void *addr = heap_ptr;
  heap_ptr += size;
  return addr;
}

void stupidfree(uint16_t size) { // idiot free-er
  heap_ptr -= size;
}