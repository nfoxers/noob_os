#include "ams/sys/stat.h"
#include "fs/devfs.h"
#include "fs/vfs.h"
#include "video/printf.h"
#include <dev/block_dev.h>
#include <driver/disk/ide.h>
#include <fs/ext2.h>
#include <mem/mem.h>
#include <proc/proc.h>

// ! todo: partition support

extern struct gendisk atamaster;

struct ext2_superblock ext2_sb;
struct ext2_blockgroup ext2_bg;

struct super_block sb_ext2;

struct ext2info_sb {
  uint32_t block_siz;
};

struct ext2info_sb e2sb;

/* vfs interface */

struct super_block  g_ext2_sb;
extern struct inode rootnode;

struct inode_ops ext2_iops;
struct file_ops  ext2_fops;
struct super_ops ext2_sops;

extern struct proc *volatile p_curproc;

extern struct super_block devblock;
extern struct inode       devnode;
extern struct mount       devmnt;

void ext2_read(struct gendisk *gd, uint32_t block, uint32_t blkcount, void *buf) {
  uint32_t siz   = e2sb.block_siz;
  uint32_t lba   = block * siz / 512;
  uint32_t count = blkcount * siz / 512;
  // printkf("lba: %d - %d\n", lba, count);
  gd->bops->read(lba, count, buf);
}

int ext2_bitmap_test(struct gendisk *gd, uint32_t index) {
  uint32_t byte = index >> 3;
  uint32_t bit  = index & 7;
  uint8_t *buf  = malloc(e2sb.block_siz);
  // printkf("blk: %d\n", ext2_bg.bg_free_inodes_count);
  ext2_read(gd, ext2_bg.bg_inode_bitmap, 1, buf);
  int v = (buf[byte] >> bit) & 1;
  free(buf);

  // printkf("status for ino %d: %d\n", index, v);
  return v;
}

int ext2_read_ino(struct gendisk *gd, int32_t ino, struct ext2_inode *out) {
  uint32_t itab = ext2_bg.bg_inode_table;
  uint32_t isiz = ext2_sb.s_inode_siz;
  uint32_t off  = (ino - 1) * isiz;

  uint32_t blk  = itab + (off / e2sb.block_siz);
  uint32_t boff = off % e2sb.block_siz;

  uint8_t *buf = malloc(e2sb.block_siz);
  ext2_read(gd, blk, 1, buf);
  memcpy(out, buf + boff, sizeof(*out));
  free(buf);
  return 0;
};

ino_t ext2_lookup(struct inode *in, const char *name) {
  ino_t ino = in->ino;
  if (!ext2_bitmap_test(in->sb->s_disk, ino)) {
    return -1;
  }
  struct ext2_inode tmp;
  ext2_read_ino(in->sb->s_disk, ino, &tmp);

  uint8_t *buf = malloc(e2sb.block_siz);
  ext2_read(in->pdata, tmp.i_block[0], 1, buf);
  ino_t ino_found = -1;

  for (int i = 0; i < 12; i++) {
    if (tmp.i_block[i] == 0)
      continue;

    uint32_t off = 0;
    while (off < e2sb.block_siz) {
      struct ext_direntry *ent = (struct ext_direntry *)(buf + off);

      if (ent->d_ino != 0) {
        // printkf("%s\n", ent->name);
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

  if (off > in->size)
    return 0;

  if (off + siz > in->size)
    siz = in->size - off;

  size_t read = 0;

  struct ext2_inode ein;
  ext2_read_ino(in->sb->s_disk, in->ino, &ein);

  uint8_t *buff = malloc(e2sb.block_siz);
  while (read < siz) {
    uint32_t pos      = off + read;
    uint32_t boff     = pos % e2sb.block_siz;
    uint8_t  blockptr = pos / e2sb.block_siz;
    uint32_t blk      = ein.i_block[blockptr];

    if (blk == 0)
      break;

    ext2_read(in->sb->s_disk, blk, 1, buff);
    uint32_t dcount = e2sb.block_siz - boff;
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

  struct ext2_inode e2in;
  ext2_read_ino(in->sb->s_disk, in->ino, &e2in);

  uint8_t *buf     = malloc(e2sb.block_siz);
  uint8_t  blk_ptr = 0;

  size_t read = 0;
  while (blk_ptr < 12) {
    if (e2in.i_block[blk_ptr] == 0) {
      blk_ptr++;
      continue;
    }
    ext2_read(in->sb->s_disk, e2in.i_block[blk_ptr], 1, buf);
    size_t off = 0;
    while (off < e2sb.block_siz && read < count) {
      struct ext_direntry *dirent = (struct ext_direntry *)(buf + off);
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
  ext2_read_ino(in->sb->s_disk, ino, &tmp);

  in->gid   = tmp.i_gid;
  in->uid   = tmp.i_uid;
  in->mode  = tmp.i_mode;
  in->pdata = &atamaster;
  in->sb    = &g_ext2_sb;
  in->fops  = &ext2_fops;
  in->ops   = &ext2_iops;
  in->size  = tmp.i_size;
}

struct inode_ops ext2_iops = {
    .lookup = ext2_lookup};

struct file_ops ext2_fops = {
    .read    = ext2_lread,
    .readdir = ext2_readdir};

struct super_ops ext2_sops = {
    .read_inode = ext2_inoder};

void set_special() {
  init_devs();

  fsino_t dev_fs = ext2_lookup(&rootnode, "dev");
  // printkf("devfs ino: %d\n", dev_fs);
  struct inode *in = iget(&g_ext2_sb, dev_fs);
  imount(in, &devmnt);
}

void ext2_init(struct gendisk *gd) {
  uint8_t *buf = malloc(1024);
  gd->bops->read(2, 2, (void *)buf);
  memcpy(&ext2_sb, buf, sizeof(ext2_sb));
  free(buf);

  if (ext2_sb.s_magic != EXT2_SUPER_MAGIC) {
    printkf("not an ext2 fs\n");
    return;
  }

  e2sb.block_siz = 1024 << ext2_sb.s_log_block_siz;

  buf = malloc(e2sb.block_siz);
  ext2_read(gd, e2sb.block_siz > 1024 ? 1 : 2, 1, buf);
  memcpy(&ext2_bg, buf, sizeof(ext2_bg));
  free(buf);

  // printkf("free inodes: %d\n", ext2_bg.bg_free_inodes_count);
  struct ext2_inode root;
  ext2_read_ino(gd, 2, &root);

  // printkf("block: %d\n", S_ISDIR(root.i_mode));

  g_ext2_sb.s_dev  = 0;
  g_ext2_sb.s_op   = &ext2_sops;
  g_ext2_sb.s_disk = gd;

  rootnode.sb    = &g_ext2_sb;
  rootnode.gid   = 0;
  rootnode.uid   = 0;
  rootnode.ino   = 2;
  rootnode.size  = 0;
  rootnode.mode  = S_IFDIR | 0766;
  rootnode.pdata = gd;
  rootnode.ops   = &ext2_iops;
  rootnode.fops  = &ext2_fops;

  set_dev(&g_ext2_sb, &rootnode);
  memcpy(&p_curproc->p_user->u_cdir, &rootnode, sizeof(struct inode));

  set_special();
}
