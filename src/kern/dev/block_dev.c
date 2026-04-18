#include "fs/vfs.h"
#include <dev/block_dev.h>
#include <mem/mem.h>
#include <mm/mm.h>

#define PG_SIZ 4096

void bio_add_pg(struct bio *bio, struct page *page) {
  bio->b_vec[bio->b_veccount++] = (struct bio_vect) {
    .bv_page = page,
    .bv_len = PG_SIZ,
    .bv_off = 0
  };
}

uint32_t bmap(struct inode *in, uint32_t block) {
  if(in->aps && in->aps->bmap) {
    return in->aps->bmap(in, block);
  }
  return 0;
}

void submit_bio(struct bio *bio) {
  bio->b_sect += bio->b_bdev->bd_start;
  // enqueue_bio(bio);
}

int readpage(struct inode *in, struct page *page) {
  uint32_t idx = page->idx;
  uint32_t block = bmap(in, idx);
  
  struct bio *bio = malloc(sizeof(*bio));
  bio->b_bdev = in->sb->s_bdev;
  bio->b_sect = block;

  bio_add_pg(bio, page);

  // todo: add end_io callback


  submit_bio(bio);

  return 0;
}

int write_page(struct page *page) {
  uint32_t block = bmap(page->inode, page->idx);

  struct bio *bio = malloc(sizeof(*bio));
  bio->b_bdev = page->inode->sb->s_bdev;
  bio->b_sect = block;

  bio_add_pg(bio, page);

  bio->b_opf = BIO_WRITE;
  // end_io

  submit_bio(bio);
  return 0;
}