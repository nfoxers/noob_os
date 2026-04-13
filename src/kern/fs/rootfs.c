#include "asm/sys/stat.h"
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
int open_root(struct inode *in, struct file *file);
int close_root(struct file *file);
ssize_t read_root(struct file *file, void *buf, size_t siz);
ssize_t write_root(struct file *file, const void *buf, size_t siz);

struct inode *finddir_fat(struct inode *in, const char *path);
DIR *opendir_fat(struct inode *in);
int closedir_fat(struct inode *in, DIR *d);
int create_fat(struct inode *dir, const char *name, uint16_t flg);
int unlink_fat(struct inode *dir, const char *name);
int open_fat(struct inode *in, struct file *file);
int close_fat(struct file *file);
ssize_t read_fat(struct file *file, void *buf, size_t siz);
ssize_t write_fat(struct file *file, const void *buf, size_t siz);

struct inode *lookup_dev(struct inode *dir, const char *name);
DIR *opendir_dev(struct inode *dir);
int closedir_dev(struct inode *dir, DIR *d);
int create_dev(struct inode *dir, const char *name, uint16_t flg);
int unlink_dev(struct inode *dir, const char *name);
int open_dev(struct inode *in, struct file *file);
int close_dev(struct file *file);
ssize_t read_dev(struct file *file, void *buf, size_t siz);
ssize_t write_dev(struct file *file, const void *buf, size_t siz);

const char *rootname[] = {
  "home", "bin", "etc", "tmp", "var", "dev"
};

struct inode rootinode;
struct inode devinode;
struct dir_data data;

extern struct dir_data devfs_root;

extern struct file_ops  fat_fops;
extern struct inode_ops fat_iops;

static struct inode_ops root_iops;
static struct file_ops root_fops;

extern struct inode_ops dev_iops;
extern struct file_ops dev_fops;

struct inode *lookup_root(struct inode *dir, const char *name) {
  //printkf("rootlok: %s\n", name);
  struct dir_data *data = dir->pdata;
  if(!data) {
    printkf("err: zero data\n");
    return NULL;
  }
  for(int i = 0; i < data->count; i++) {
    if(!strcmp(name, data->entry[i].name)) {
      struct inode *in = malloc(sizeof(struct inode));
      //printkf("s: %x\n", data->entry[i].in);
      memcpy(in, data->entry[i].in, sizeof(struct inode));
      //printkf("enk: %x\n", in->entaddr);
      return in;
    }
  }
  return NULL;
}

DIR *opendir_root(struct inode *dir) {
  (void)dir;

  struct dir_data *data = dir->pdata;

  DIR *d = malloc(sizeof(DIR) * NOFS);
  for(int i = 0; i < data->count && i < NOFS; i++) {
    strcpy(d[i].data, data->entry[i].name);
    d[i].in = data->entry[i].in;
    d[i].type = data->entry[i].in->mode;
    d[i].count = data->count;
    d[i].size = data->entry[i].in->size;
  }

  return d;
}

int closedir_root(struct inode *dir, DIR *d) {
  free(d);
  (void)dir;
  return 0;
}

int find_home(struct inode *buf);
int fat_lookup_from(const char *restrict path, const struct inode *restrict from, struct inode *restrict buf);

void init_rootfs() {
  print_init("fs", "initializing rootfs...", 0);

  rootinode.fops = &root_fops;
  rootinode.ops = &root_iops;
  rootinode.pdata = &data;
  rootinode.mode = S_IFDIR;

  uint32_t i = 0;
  for(i = 0; i < sizeof(rootname)/sizeof(rootname[0])-1; i++) {
    struct inode buf;

    if(fat_lookup_from(rootname[i], NULL, &buf)) {
      printkf("rootfs: can't find %s\n", rootname[i]);
      continue;
    }

    data.entry[i].in = malloc(sizeof(struct inode));
    memcpy(data.entry[i].in, &buf, sizeof(struct inode));
    strcpy(data.entry[i].name, rootname[i]);
  }

  devinode.fops = &dev_fops;
  devinode.ops = &dev_iops;
  devinode.ino = 1;
  devinode.mode = S_IFDIR | 0755;
  devinode.pdata = &devfs_root;

  strcpy(data.entry[i].name, "dev");
  data.entry[i].in = &devinode;

  data.count = i + 1;

}

int create_root(struct inode *dir, const char *name, uint16_t flg) {
  (void)dir; (void)name; (void)flg;
  return -EPERM;
}

int unlink_root(struct inode *dir, const char *name) {
  (void)dir; (void)name;
  return -EPERM;
}

int open_root(struct inode *in, struct file *file) {
  (void)in;
  (void)file;
  return -ENOSYS;
}



static struct inode_ops root_iops = {
  .opendir = opendir_root,
  .closedir = closedir_root,
  .lookup = lookup_root,
  .unlink = unlink_root
};

static struct file_ops root_fops = {
    .open  = 0,
    .close = 0,
    .read  = 0,
    .write = 0};