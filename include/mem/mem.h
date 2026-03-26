#ifndef MEM_H
#define MEM_H

#include "stddef.h"
#include "stdint.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define HEAP_SIZ 1024

// y'know what, i surrender living off below 1MB. a cool kernel can't just only use 25KB of memory (b)
#define EXT_MEM_BASE 0x00100000
#define EXT_MEM_SIZ ((size_t)1 << 20) // 1MB o' size

extern uint8_t __bss_start__;
extern uint8_t __bss_end__;

#define BSS_END   (&__bss_end__)
#define BSS_START (&__bss_start__)

struct bios_da {
  uint16_t com_port[4];
  uint16_t lpt_port[3];
  uint16_t ebda_addr;
  uint16_t flags;
  uint8_t  pcjr;
  uint16_t usable_memory;
} __attribute__((packed));

void parse_bda();

#if defined(__cplusplus)
  #define RES
#else
  #define RES restrict
#endif

void    kmemcpy(void *dst, const void *src, size_t siz);
uint8_t kmemcmp(const void *s1, const void *s2, size_t siz);
void    kmemset(void *dst, int c, size_t len);

void    kstrncpy(char *dst, const char *src, size_t siz);
void    kstrcpy(char *dst, const char *src);
uint8_t kstrncmp(const char *s1, const char *s2, size_t siz);
uint8_t kstrcmp(const char *s1, const char *s2);
size_t  kstrlen(const char *s);
char   *kstrtok(char *str, const char *delim);
char   *kstrrchr(const char *s, int c);

void zero_bss();

void  kmalloc_init();
void *kmalloc(size_t size);
void  kfree(void *ptr);

void *kmalloc_align(size_t siz, size_t align);
void kfree_align(void *ptr);

void smbios_scan(void);

#endif