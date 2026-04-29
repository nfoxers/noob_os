#include "fs/ext2.h"
#include "fs/vfs.h"
#include "video/printf.h"
#include <dev/block_dev.h>
#include <mem/mem.h>
#include <mm/mm.h>
#include <fs/fs.h>

// maps filesystem blocks to partition relative blocks
uint32_t bmap(struct inode *in, uint32_t block) {
  if(in->aps && in->aps->bmap) {
    return in->aps->bmap(in, block);
  }
  return 0;
}

// bread
ssize_t bread(struct block_dev *bd, sector_t sect, sector_t count, void *buf) {
  if(!bd || !bd->bd_disk || !bd->bd_disk->bops || !bd->bd_disk->bops->read) {
    printkf("sdfsfsfs\n");
    return -1;
  }

  return bd->bd_disk->bops->read(sect + bd->bd_start, count, buf);
}

ssize_t bwrite(struct block_dev *bd, sector_t sect, sector_t count, const void *buf) {
  if(!bd || !bd->bd_disk || !bd->bd_disk->bops || !bd->bd_disk->bops->write) {
    return -1;
  }

  return bd->bd_disk->bops->write(sect + bd->bd_start, count, buf);
}