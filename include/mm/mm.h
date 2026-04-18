#ifndef MM_H
#define MM_H

#include "cpu/spinlock.h"
#include <stdint.h>
#include <stddef.h>
#include <fs/vfs.h>

// todo: change these into proper libc constants
#define PROT_READ 0x00
#define PROT_WRITE 0x01
#define PROT_EXEC 0x02

#define MAP_PRIVATE 0x00
#define MAP_SHARED 0x01

struct mm;
struct vm_area;
struct page;

struct addrspace_ops {
  int (*readpage)(struct inode *, struct page *);
  int (*writepage)(struct page *);
  uint32_t (*bmap)(struct inode *, uint32_t);
};

struct addrspace {
  struct inode *host;
  // todo: radix tree
  struct addrspace_ops *aps;
};

struct page {
  uint32_t      idx;
  struct inode *inode;

  void *data;

  int dirty;

  struct page *next;
};

struct vm_ops {
  int (*fault)(struct vm_area *vma, uintptr_t addr);
};

struct vm_area {
  uintptr_t start, end;

  struct mm *mm;

  int prot;
  int flags;

  struct file *file;
  struct addrspace *mapping;
  size_t off;

  struct vm_area *next;
};

struct mm {
  uint32_t *pgdir;

  struct vm_area *mmap;

  uintptr_t brk;
  uintptr_t stack_start;

  int refs;
};

#endif