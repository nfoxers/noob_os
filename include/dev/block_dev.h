#ifndef BLOCKDEV_H
#define BLOCKDEV_H

#include "ams/sys/types.h"
#include "cpu/spinlock.h"
#include "lib/list.h"
#include "stddef.h"
#include "stdint.h"

#include "ams/sys/stat.h"

#define DISKNAME 32

#define BIO_READ  0
#define BIO_WRITE 1

#define DPART_MAXPART 4

typedef uint32_t sector_t;

struct block_dev;

struct hd_struct {
  sector_t          start_sect;
  sector_t          nr_sect;
  struct block_dev *bd;
};

struct dpart_tbl {
  uint8_t           pad;
  struct hd_struct *part[DPART_MAXPART];
};

struct bd_ops {
  ssize_t (*read)(uint32_t, size_t, uint8_t *);
  ssize_t (*write)(uint32_t, size_t, const uint8_t *);
};

struct bio_vect {
  struct page *bv_page;
  uint32_t     bv_len;
  uint32_t     bv_off;
};

struct request {
  struct block_dev *bdev;

  uint32_t sect;
  uint32_t nr_sect;

  struct bio *bios;

  struct request *next;
};

struct req_queue {
  struct request *head;
};

struct bio {
  struct block_dev *b_bdev;
  sector_t          b_sect;

  struct bio_vect *b_vec;
  uint32_t         b_veccount;
  uint32_t         b_opf;
};

struct gendisk {
  int maj;
  int minor;
  int minor_cont;

  char name[DISKNAME];

  struct req_queue *queue;
  struct bd_ops    *bops;
  struct dpart_tbl *part_tbl;
};

struct block_dev {
  dev_t               bd_dev;
  struct inode       *bd_inode;
  struct super_block *bd_sb;

  struct gendisk   *bd_disk;
  struct req_queue *bd_queue;
  struct block_dev *bd_parent;
  sector_t          bd_start;
};

ssize_t bread(struct block_dev *bd, sector_t sect, sector_t count, void *buf);
ssize_t bwrite(struct block_dev *bd, sector_t sect, sector_t count, const void *buf);
uint32_t bmap(struct inode *in, uint32_t block);

#endif