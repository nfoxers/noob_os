#include "ams/sys/stat.h"
#include "fs/devfs.h"
#include "fs/vfs.h"
#include "video/printf.h"
#include "video/video.h"
#include <dev/block_dev.h>
#include <driver/disk/ide.h>
#include <fs/ext2.h>
#include <mem/mem.h>
#include <proc/proc.h>

// ! todo: partition support
/* vfs interface */

struct inode_ops ext2_iops;
struct file_ops  ext2_fops;
struct super_ops ext2_sops;

struct ext2_sb_info *e2sbinfo(struct block_dev *bd) {
  if(!bd || !bd->bd_sb || !bd->bd_sb->generic_sbp) {
    return NULL;
  }

  return bd->bd_sb->generic_sbp;
}

struct ext2_inode_info *e2ininfo(struct inode *in) {
  if(!in || !in->pdata) {
    return NULL;
  }

  return in->pdata;
}

void ext2_read(struct block_dev *bd, uint32_t block, uint32_t blkcount, void *buf) {
  uint32_t siz   = bd->bd_sb->s_blocksize;
  uint32_t lba   = block * siz / 512;
  uint32_t count = blkcount * siz / 512;
  // printkf("lba: %d - %d\n", lba, count);
  bread(bd, lba, count, buf);
}

int ext2_bitmap_test(struct block_dev *bd, uint32_t index) {
  uint32_t group  = index / e2sbinfo(bd)->s_inodes_per_group;
  index %= e2sbinfo(bd)->s_inodes_per_group;

  uint32_t byte = index >> 3;
  uint32_t bit  = index & 7;
  uint8_t *buf  = e2sbinfo(bd)->s_block_bitmap[group];

  int v = (buf[byte] >> bit) & 1;

  //printkf("status for ino %d: %d\n", index, v);
  return v;
}

int ext2_read_ino(struct block_dev *bd, int32_t ino, struct ext2_inode *out) {

  //ext2_bitmap_test(bd, ino);

  struct ext2_sb_info *sb = e2sbinfo(bd);
  uint32_t group = ino / sb->s_inodes_per_group;
  ino %= sb->s_inodes_per_group;

  uint32_t itab = sb->s_group_desc[group]->bg_inode_table;
  
  uint32_t isiz = sb->s_inode_size;
  uint32_t off  = (ino - 1) * isiz;

  uint32_t blk  = itab + (off / sb->s_block_size);
  uint32_t boff = off % sb->s_block_size;

  uint8_t *buf = malloc(sb->s_block_size);
  ext2_read(bd, blk, 1, buf);
  memcpy(out, buf + boff, sizeof(*out));
  free(buf);

  return 0;
};

ino_t ext2_lookup(struct inode *in, const char *name) {
  struct ext2_inode_info *t = e2ininfo(in);
  //struct ext2_sb_info *sb = in->sb;

  uint32_t blocksiz = in->sb->s_blocksize;
  uint8_t *buf = malloc(blocksiz);
  ino_t ino_found = -1;

  for (int i = 0; i < 12; i++) {
    if (t->i_block[i] == 0) {

      continue;
    }
      
    ext2_read(in->sb->s_bdev, t->i_block[i], 1, buf);

    uint32_t off = 0;
    while (off < blocksiz) {
      struct ext2_direntry *ent = (struct ext2_direntry *)(buf + off);

      if (ent->d_ino != 0) {
        if (ent->d_name_len == strlen(name) && memcmp(ent->name, name, ent->d_name_len) == 0) {
          ino_found = ent->d_ino;
          goto done;
        }
      }

      off += ent->d_rec_len;
    }
  }
done:
  free(buf);

  // printkf("fond: %d\n", ino_found);
  return ino_found;
}

// ! todo: bio cache
ssize_t ext2_lread(struct file *file, void *buf, size_t siz) {
  struct inode *in = file->inode;

  uint32_t off = file->position;
  uint32_t blocksiz = in->sb->s_blocksize;

  if (off > in->size)
    return 0;

  if (off + siz > in->size)
    siz = in->size - off;

  size_t read = 0;

  struct ext2_inode ein;
  ext2_read_ino(in->sb->s_bdev, in->ino, &ein);

  uint8_t *buff = malloc(blocksiz);
  while (read < siz) {
    uint32_t pos      = off + read;
    uint32_t boff     = pos % blocksiz;
    uint8_t  blockptr = pos / blocksiz;
    uint32_t blk      = ein.i_block[blockptr];

    if (blk == 0)
      break;

    ext2_read(in->sb->s_bdev, blk, 1, buff);
    uint32_t dcount = blocksiz - boff;
    if (dcount > (siz - read))
      dcount = siz - read;

    memcpy(buf + read, buff + boff, dcount);

    read += dcount;
  }

  free(buff);
  file->position += read;
  return read;
}

ssize_t ext2_readdir(struct file *file, struct nnux_dirent *dirp, size_t count) {
  struct inode *in = file->inode;

  struct ext2_inode_info *inf = e2ininfo(in);

  uint8_t *buf     = malloc(in->sb->s_blocksize);
  uint32_t blocksiz = in->sb->s_blocksize;
  uint8_t  blk_ptr = 0;

  size_t read = 0;
  while (blk_ptr < 12) {
    if (inf->i_block[blk_ptr] == 0) {
      blk_ptr++;
      continue;
    }

    ext2_read(in->sb->s_bdev, inf->i_block[blk_ptr], 1, buf);
    size_t off = 0;
    while (off < blocksiz && read < count) {
      struct ext2_direntry *dirent = (struct ext2_direntry *)(buf + off);
      struct nnux_dirent  *cur    = (struct nnux_dirent *)((uint8_t *)dirp + read);

      if (dirent->d_ino != 0) {
        cur->d_ino = dirent->d_ino;
        cur->d_off = off;
        strncpy(cur->name, dirent->name, dirent->d_name_len);
        cur->d_type   = dirent->d_file_type;
        cur->d_reclen = sizeof(*cur) + dirent->d_name_len + 1;
        read += cur->d_reclen;
      }
      off += dirent->d_rec_len;
    }
    blk_ptr++;
  }

  free(buf);
  return read;
}

void ext2_inoder(struct inode *in) {
  ino_t             ino = in->ino;
  struct ext2_inode tmp;
  ext2_read_ino(in->sb->s_bdev, ino, &tmp);

  struct ext2_inode_info *inf = malloc(sizeof(*inf));
  memcpy(inf->i_block, tmp.i_block, sizeof(tmp.i_block));
  inf->i_blocks = tmp.i_blocks;

  in->gid      = tmp.i_gid;
  in->uid      = tmp.i_uid;
  in->mode     = tmp.i_mode;
  in->pdata    = inf;
  in->sb       = in->sb;
  in->fops     = &ext2_fops;
  in->ops      = &ext2_iops;
  in->size     = tmp.i_size;
  in->blkcount = tmp.i_blocks;
  in->blksiz   = in->sb->s_blocksize;
}

void ext2_putin(struct inode *in) {
  free(in->pdata);
}

struct inode_ops ext2_iops = {
    .lookup = ext2_lookup};

struct file_ops ext2_fops = {
    .read    = ext2_lread,
    .readdir = ext2_readdir};

struct super_ops ext2_sops = {
    .read_inode = ext2_inoder,
  .put_inode = ext2_putin};
