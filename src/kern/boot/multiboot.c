#include "video/printf.h"
#include <stdint.h>
//#include <boot/multiboot2.h>

struct mb2_tag {
  uint32_t type;
  uint32_t size;
} __attribute__((packed));

struct mb2_info {
  uint32_t total_size;
  uint32_t res;
  struct mb2_tag tags[];
} __attribute__((packed));

struct mb2_mmap_entry {
  uint64_t base_addr;
  uint64_t len;
  uint32_t type;
  uint32_t res;
} __attribute__((packed));

struct mb2_mmap {
  struct mb2_tag h;
  uint32_t ent_siz;
  uint32_t ent_ver;

  struct mb2_mmap_entry entries[];

} __attribute__((packed));

void parse_multiboot2(void *ptr) {
  struct mb2_info *info = (struct mb2_info *)ptr;
  struct mb2_tag *tag = info->tags;

  while(tag->type != 0) {
    printkf("tpe: %d\n", tag->type);

    tag = (struct mb2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
  }
}