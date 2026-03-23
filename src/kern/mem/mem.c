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
  while (siz--) {
    *dst++ = *src++;
  }
}

void kstrcpy(char *dst, const char *src) {
  while ((*dst++ = *src++))
    ;
}

void kmemcpy(void *dst, const void *src, size_t siz) {
  for (; siz > 0; siz--) {
    *(uint8_t *)dst++ = *(uint8_t *)src++;
  }
}

uint8_t kmemcmp(const void *s1, const void *s2, size_t siz) {
  while (siz && (*(uint8_t *)s1 == *(uint8_t *)s2))
    s1++, s2++, siz--;
  if (siz == 0)
    return 0;
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

  while (siz && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
    siz--;
  }

  if (siz == 0)
    return 0;

  return *(uint8_t *)s1 - *(uint8_t *)s2;
}

char *kstrrchr(const char *s, int c) {
  const char *l = NULL;
  while (*s) {
    if (*s == (char)c)
      l = s;
    s++;
  }
  if (!c)
    return (char *)s;
  return (char *)l;
}

size_t kstrlen(const char *s) {
  const char *d = s;
  while ((*d++))
    ;
  return d - s - 1;
}

void zero_bss() {
  uint8_t *p = &__bss_start__;
  while (p < &__bss_end__) {
    *p++ = 0;
  }
  heap_ptr = &__bss_end__;
}

#define MAX_ORD 20
#define MIN_ORD 5

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

struct block {
  struct block *next;
};

struct alloc {
  uint8_t *base;
  size_t siz;
  struct block *free[MAX_ORD + 1];
};

struct alloc_hdr {
  uint8_t ord;
};

static inline size_t ord_siz(uint8_t ord) {
  return (size_t)1 << ord;
}

static int size2ord(size_t siz) {
  int ord = MIN_ORD;
  size_t s = (size_t)1 << ord;
  while (s < siz) {
    s <<= 1;
    ord++;
  }
  return ord;
}

static inline uintptr_t ptr2off(struct alloc *a, void *ptr) {
  return (uintptr_t)((uint8_t *)ptr - a->base);
}

static inline void *off2ptr(struct alloc *a, uintptr_t off) {
  return (void *)(a->base + off);
}

static int remove_block(struct block **head, struct block *target) {
  struct block **cur = head;
  while (*cur) {
    if (*cur == target) {
      *cur = target->next;
      return 1;
    }
    cur = &(*cur)->next;
  }
  return 0;
}

static void insert_block(struct block **head, struct block *b) {
  struct block **cur = head;
  while(*cur && *cur < b) {
    cur = &(*cur)->next;
  }

  b->next = *cur;
  *cur = b;
}

void km_init(struct alloc *a, void *mem, size_t siz) {
  for (int i = 0; i <= MAX_ORD; i++)
    a->free[i] = NULL;

  int max_ord = MAX_ORD;
  while (((size_t)1 << max_ord) > siz)
    max_ord--;

  size_t max_block = (size_t)1 << max_ord;

  // align in worsdt case scenario
  uintptr_t base = (uintptr_t)mem;
  uintptr_t aligned = ALIGN_UP(base, max_block);
  size_t adjust = aligned - base;

  if (adjust >= siz) {
    a->base = NULL;
    a->siz = 0;
    return;
  }

  siz -= adjust;
  siz &= ~(max_block - 1); // truncate to multiple

  a->base = (uint8_t *)aligned;
  a->siz = siz;

  struct block *init = (struct block *)a->base;
  init->next = NULL;

  a->free[max_ord] = init;
}

void *km_alloc(struct alloc *a, size_t siz) {
  siz = ALIGN_UP(siz, sizeof(void*));

  size_t total = siz + sizeof(struct alloc_hdr);
  int ord = size2ord(total);

  if (ord > MAX_ORD)
    return NULL;

  int cur = ord;
  while (cur <= MAX_ORD && a->free[cur] == NULL)
    cur++;

  if (cur > MAX_ORD)
    return NULL;

  struct block *bl = a->free[cur];
  a->free[cur] = bl->next;

  while (cur > ord) {
    cur--;
    size_t splitsiz = ord_siz(cur);

    struct block *bud =
        (struct block *)((uint8_t *)bl + splitsiz);

    insert_block(&a->free[cur], bud);
  }

  struct alloc_hdr *hdr = (struct alloc_hdr *)bl;
  hdr->ord = ord;

  return (void *)(hdr + 1);
}

void km_free(struct alloc *a, void *ptr) {
  if (!ptr)
    return;

  struct alloc_hdr *h = ((struct alloc_hdr *)ptr) - 1;
  int ord = h->ord;

  uintptr_t off = ptr2off(a, (void *)h);

  while (ord < MAX_ORD) {
    uintptr_t size = ord_siz(ord);
    uintptr_t boff = off ^ size;

    if (boff >= a->siz)
      break;

    struct block *bud =
        (struct block *)off2ptr(a, boff);

    if (!remove_block(&a->free[ord], bud))
      break;

    if (boff < off)
      off = boff;

    ord++;
  }

  struct block *bl =
      (struct block *)off2ptr(a, off);

  insert_block(&a->free[ord], bl);
}

struct alloc kalloc;

void kmalloc_init() {
  void *mem = (void *)EXT_MEM_BASE;
  size_t siz = EXT_MEM_SIZ;
  km_init(&kalloc, mem, siz);
}

void *kmalloc(size_t siz) { // todo: low-address priority allocation
  return km_alloc(&kalloc, siz);
}

void kfree(void *ptr) {
  km_free(&kalloc, ptr);
}
