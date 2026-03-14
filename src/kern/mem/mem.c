#include "mem/mem.h"
#include "video/printf.h"
#include <stdint.h>

#define BDA_ADDR ((struct bios_da *)0x400)

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

void kstrncpy(char *dst, const char *src, size_t siz) {
  while(siz--) {
    *dst++ = *src++;
  }
}

void kstrcpy(char *dst, const char *src) {
  while((*dst++ = *src++)) ; 
}

void kmemcpy(void *dst, const void *src, size_t siz) {
  for(; siz > 0; siz--) {
    *(uint8_t *)dst++ = *(uint8_t *)src++;
  }
}

uint8_t kmemcmp(const void *s1, const void *s2, size_t siz) {
  while(siz && (*(uint8_t*)s1 == *(uint8_t*)s2)) s1++, s2++, siz--;
  if(siz == 0) return 0;
  return *(uint8_t *)s1 - *(uint8_t *)s2;
}

void kmemset(void *s, int c, size_t len) {
  asm volatile("rep stosb" : "+D"(s), "+c"(len) : "a"((uint8_t)c) : "memory");
}

uint8_t kstrcmp(const char *s, const char *d) {
  while (*s == *d && *s)
    s++, d++;
  return *(uint8_t *)s - *(uint8_t *)d;
}

uint8_t kstrncmp(const char *s1, const char *s2, size_t siz) {

  while(siz && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
    siz--;
  }

  if(siz == 0) return 0;

  return *(uint8_t *)s1 - *(uint8_t *)s2;
}

char *kstrrchr(const char *s, int c) {
  const char *l = NULL;
  while(*s) {
    if(*s == (char)c) l = s;
    s++;
  }
  if(!c) return (char*)s;
  return (char*)l;
}

size_t kstrlen(const char *s) {
  const char *d = s;
  while((*d++));
  return d - s - 1;
}

void zero_bss() {
  uint8_t *p = &__bss_start__;
  while (p < &__bss_end__) {
    *p++ = 0;
  }
  heap_ptr = &__bss_end__;
}

void *kmalloc(size_t size) { // stupid allocator
  void *addr = heap_ptr;
  if((int)heap_ptr + size - __bss_end__ > HEAP_SIZ) return NULL;
  heap_ptr += size;
  return addr;
}

void stupidfree(size_t size) { // idiot free-er
  heap_ptr -= size;
}