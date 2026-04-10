#include "fs/vfs.h"
#include "mem/mem.h"
#include "fs/fat12.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>
#include "lib/errno.h"

#define NOFS 10

/* we set up /, /home, & /dev */

struct inode *lookup_root(struct inode *dir, const char *name);
DIR *opendir_root(struct inode *dir);
int closedir_root(struct inode *dir, DIR *d);
int create_root(struct inode *dir, const char *name, uint16_t flg);
int unlink_root(struct inode *dir, const char *name);
int open_root(struct inode *dir, const char *restrict pth, uint16_t flg);
int close_root(struct inode *dir, int fd);
int read_root(struct inode *in, void *buf, size_t off, size_t siz);
int write_root(struct inode *in, const void *buf, size_t off, size_t siz);

struct inode *finddir_fat(struct inode *in, const char *path);
DIR *opendir_fat(struct inode *in);
int closedir_fat(struct inode *in, DIR *d);
int create_fat(struct inode *dir, const char *name, uint16_t flg);
int unlink_fat(struct inode *dir, const char *name);
int open_fat(struct inode *dir, const char *restrict pth, uint16_t flg);
int close_fat(struct inode *dir, int fd);
int read_fat(struct inode *in, void *buf, size_t off, size_t siz);
int write_fat(struct inode *in, const void *buf, size_t off, size_t siz);

struct inode *lookup_dev(struct inode *dir, const char *name);
DIR *opendir_dev(struct inode *dir);
int closedir_dev(struct inode *dir, DIR *d);
int create_dev(struct inode *dir, const char *name, uint16_t flg);
int unlink_dev(struct inode *dir, const char *name);
int open_dev(struct inode *dir, const char *restrict pth, uint16_t flg);
int close_dev(struct inode *dir, int fd);
int read_dev(struct inode *in, void *buf, size_t off, size_t siz);
int write_dev(struct inode *in, const void *buf, size_t off, size_t siz);

#define ROOT_OPS \
  { lookup_root, create_root, unlink_root, opendir_root, closedir_root }

#define FAT_OPS \
  { finddir_fat, create_fat, unlink_fat, opendir_fat, closedir_fat }

#define DEV_OPS \
  { lookup_dev, create_dev, unlink_dev, opendir_dev, closedir_dev }


#define ROOT_FOPS \
  { open_root, read_root, write_root, close_root }

#define DEV_FOPS \
  { open_dev, read_dev, write_dev, close_dev }

#define FAT_FOPS \
  { open_fat, read_fat, write_fat, close_fat }

struct inode rootinode = {
  0, 0, INODE_DIR, 0766,
  0, 0,
  0, ROOT_OPS, ROOT_FOPS
};

struct inode rootfs[NOFS+1] = {
  {0, 2, INODE_DIR, 0755,
  0, 0,
  ROOT_ADDR+1, FAT_OPS, FAT_FOPS}, 

  {0, 0, INODE_DIR, 0755,
  0, 0,
ROOT_ADDR, FAT_OPS, FAT_FOPS},
  {0, 0, INODE_DIR, 0755,
  0, 0,
ROOT_ADDR, FAT_OPS, FAT_FOPS},
  {0, 0, INODE_DIR, 0777,
  0, 0,
ROOT_ADDR, FAT_OPS, FAT_FOPS},
  {0, 0, INODE_DIR, 0755,
  0, 0,
ROOT_ADDR, FAT_OPS, FAT_FOPS},

  {0, 0, INODE_DIR, 0755,
  0, 0,
0, DEV_OPS, DEV_FOPS},
  {0, 0, INODE_DIR, 0755,
  0, 0,
0, {}, {}},

};

char *rootname[NOFS+1] = {
  "home", "bin", "etc", "tmp", "var", "dev", "proc",
  NULL
};

DIR rootdirstream[] = {
  {7, INODE_DIR, 0, &rootfs[0], "home"},
  {0, INODE_DIR, 0, &rootfs[1], "bin"},
  {0, INODE_DIR, 0, &rootfs[2], "etc"},
  {0, INODE_DIR, 0, &rootfs[3], "tmp"},
  {0, INODE_DIR, 0, &rootfs[4], "var"},

  {0, INODE_DIR, 0, &rootfs[5], "dev"},
  {0, INODE_DIR, 0, &rootfs[6], "proc"}
};

struct inode *lookup_root(struct inode *dir, const char *name) {
  (void)dir;
  struct inode *in = malloc(sizeof(struct inode));
  for(uint32_t i = 0; i < sizeof(rootname)/sizeof(rootname[0]); i++) {
    if(!strcmp(name, rootname[i])) {
      memcpy(in, &rootfs[i], sizeof(rootfs[0]));
      return in;
    }
  }
  return NULL;
}

DIR *opendir_root(struct inode *dir) {
  (void)dir;
  return (DIR*)rootdirstream;
}

int closedir_root(struct inode *dir, DIR *d) {
  (void)dir; // we do nothing as everything is static
  (void)d;
  return 0;
}

int find_home(struct inode *buf);
int fat_lookup_from(const char *restrict path, const struct inode *restrict from, struct inode *restrict buf);

void init_rootfs() {
  print_init("fs", "initializing rootfs...", 0);
  for(uint32_t i = 0; i < sizeof(rootdirstream)/sizeof(rootdirstream[0])-2; i++) {
    struct inode buf;

    if(fat_lookup_from(rootname[i], NULL, &buf)) {
      printkf("rootfs: can't find %s\n", rootname[i]);
      continue;
    }

    memcpy(&rootfs[i], &buf, sizeof(struct inode));
  }
}

int create_root(struct inode *dir, const char *name, uint16_t flg) {
  (void)dir; (void)name; (void)flg;
  return -EPERM;
}

int unlink_root(struct inode *dir, const char *name) {
  (void)dir; (void)name;
  return -EPERM;
}

int open_root(struct inode *dir, const char *restrict pth, uint16_t flg) {
  (void)pth;
  (void)dir; (void)flg;
  return -ENOSYS;
}

int close_root(struct inode *dir, int fd) {
  (void)dir;
  (void)fd;
  return -ENOSYS;
}

int read_root(struct inode *in, void *buf, size_t off, size_t siz) {
  (void)in;
  (void)buf;
  (void)off;
  (void)siz;
  return -ENOSYS;
}

int write_root(struct inode *in, const void *buf, size_t off, size_t siz) {
  (void)in;
  (void)buf;
  (void)off;
  (void)siz;
  return -ENOSYS;
}