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
#include <fs/fs.h>

extern struct super_ops ext2_sops;

extern void ext2_read(struct block_dev *bd, uint32_t block, uint32_t blkcount, void *buf);
extern void ext2_inoder(struct inode *in);

struct file_system_type ext2_fs_type;

int ext2_fill_super(struct file_system_type *type, struct super_block *sb) {
  (void)type;
  struct block_dev *bd = sb->s_bdev;

  uint8_t *buf = malloc(1024);
  struct ext2_superblock e2sb;

  bread(bd, 2, 2, buf);
  memcpy(&e2sb, buf, sizeof(e2sb));
  free(buf);

  if (e2sb.s_magic != EXT2_SUPER_MAGIC) {
    printkf("not an ext2 fs\n");
    return 1;
  }

  struct ext2_sb_info *info = malloc(sizeof(*info));
  uint32_t off = 0;
  
  bd->bd_sb = sb;
  sb->s_blocksize = 1024 << e2sb.s_log_block_siz;
  sb->s_bdev = bd;
  sb->s_dev = bd->bd_dev;
  sb->s_magic = EXT2_SUPER_MAGIC;
  sb->s_op = &ext2_sops;
  sb->s_inext = 3;
  sb->generic_sbp = info;
  info->s_block_size = sb->s_blocksize;
  info->s_blocks_per_group = e2sb.s_blocks_per_group;
  info->s_group_count = DIV_ROUND_UP(e2sb.s_blocks_count, e2sb.s_blocks_per_group);
  info->s_inode_size = e2sb.s_inode_siz;
  info->s_inodes_per_group = e2sb.s_inodes_per_group;
  
  uint8_t *buff = malloc(sb->s_blocksize);
  ext2_read(sb->s_bdev, info->s_block_size > 1024 ? 1 : 2, 1, buff);
  for(int i = 0; i < info->s_group_count; i++) {
    info->s_group_desc[i] = malloc(sizeof(struct ext2_blockgroup));
    info->s_inode_bitmap[i] = malloc(sb->s_blocksize);
    info->s_block_bitmap[i] = malloc(sb->s_blocksize);

    memcpy(info->s_group_desc[i], buff + off, sizeof(struct ext2_blockgroup));
    
    ext2_read(sb->s_bdev, info->s_group_desc[i]->bg_inode_bitmap, 1, info->s_inode_bitmap[i]);
    ext2_read(sb->s_bdev, info->s_group_desc[i]->bg_block_bitmap, 1, info->s_block_bitmap[i]);


    off += sizeof(struct ext2_blockgroup);
    if(off >= sb->s_blocksize) break;
  }
  free(buff);

  struct inode *root = iget(sb, 2);
  sb->s_root = root;

  return 0;
}

void ext2_putsb(struct file_system_type *type, struct super_block *sb) {
  (void)type;
  struct ext2_sb_info *inf = sb->generic_sbp;
  
  for(int i = 0; i < inf->s_loaded_block_bitmaps; i++) {
    free(inf->s_block_bitmap[i]);
    free(inf->s_inode_bitmap[i]);
    free(inf->s_group_desc[i]);
  }

  free(inf);
}

// ! todo: fix this
struct super_block *sget(struct file_system_type *fs_type, struct block_dev *bd) {
  if(bd->bd_sb && bd->bd_sb->s_refs != 0) {
    return bd->bd_sb;
  }

  struct super_block *sb = malloc(sizeof(*sb));
  sb->s_bdev = bd;
  bd->bd_sb = sb;
  //ext2_fill_super(sb, NULL);

  fs_type->read_super(fs_type, sb);

  sb->s_bdev = bd;
  sb->s_refs = 1;

  return sb;
}

void sput(struct super_block *sb) {
  // todo: fix this

  sb->s_refs--;

  if(sb->s_refs > 0) return;

  iput(sb->s_root);

  if(sb->s_type && sb->s_type->put_super) {
    sb->s_type->put_super(sb->s_type, sb);
  }

  free(sb);
}

struct inode *ext2_mount(struct file_system_type *fs_type, const char *src, void *data, int flg) {
  (void)fs_type;
  (void)flg;
  (void)data;

  struct block_dev *bd;
  if(flg & FT_DEVT) {
    bd = blkdev_get_dev(*(dev_t *)src);
  } else {
    bd = blkdev_get_path(src);
  }

  struct super_block *sb = sget(&ext2_fs_type, bd);

  return sb->s_root;
}

struct file_system_type ext2_fs_type = {.mount = ext2_mount, .read_super = ext2_fill_super, .put_super = ext2_putsb, .name = "ext2"};