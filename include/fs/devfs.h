#ifndef DEVFS_H
#define DEVFS_H

#include "stddef.h"
#include "stdint.h"

#include "fs/vfs.h"

struct device;

struct dev_ops {
  ssize_t (*read)(struct device *d, void *buf, size_t count);
  ssize_t (*write)(struct device *d, const void *buf, size_t count);

  int (*open)(struct inode *in, struct file *f);
  int (*close)(struct file *f);

  int (*ioctl)(struct device *d, int op, void *arg);
};

struct devidx {
  uint16_t maj;
  uint16_t min;
};

struct device {
  const char *name;
  const struct devidx *idx;
  size_t off;
  void *pdata;

  struct dev_ops ops;

};

struct inode *creat_devfs(const char *name, struct device *d, uint16_t maj, uint16_t min);
int register_dev(uint16_t maj, uint16_t min, struct device *dev);

struct device *getdev(struct inode *in);
void init_devs();

/* device related syscalls */

int fsys_ioctl(int fd, uint32_t op, void *arg);

#endif