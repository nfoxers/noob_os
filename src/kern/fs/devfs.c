#include "fs/fat12.h"
#include "fs/vfs.h"
#include "lib/errno.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>
#include "fs/devfs.h"

#define MAXMAJ 5
#define MAXMIN 2

#define NODEVFS (MAXMAJ * MAXMIN)

struct device *chrdev_tab[MAXMAJ][MAXMIN];

struct inode_ops dev_iops;
struct file_ops dev_fops;

struct dir_data devfs_root;

void dir_add(struct dir_data *dir, const char *name, struct inode *in) {
  struct dir_entry *dent = dir->entry;
  while(dent->flg & DE_USED) dent++;
  dent->flg = DE_USED;
  dent->in = in;
  dir->count++;
  strcpy(dent->name, name);
}

struct inode *lookup_dev(struct inode *dir, const char *name) {
  //printkf("were here! %s\n", name);
  struct dir_data *data = dir->pdata;
  if(!data) return NULL;

  for(int i = 0; i < data->count; i++) {
    //printkf("%s vs %s\n", name, data->entry[i].name);
    if(!strcmp(name, data->entry[i].name)) {
      struct inode *in = malloc(sizeof(*in));
      memcpy(in, data->entry[i].in, sizeof(*in));
      return in;
    }
  }
  return NULL;
}

int register_dev(uint16_t maj, uint16_t min, struct device *dev) {
  if(chrdev_tab[maj][min]) return -1;
  chrdev_tab[maj][min] = dev;
  return 0;
}

struct inode *creat_devfs(const char *name, uint16_t maj, uint16_t min) {
  struct inode *in = malloc(sizeof(struct inode));

  in->type = INODE_CHARDEV;
  in->fops = &dev_fops;

  struct devidx *didx = malloc(sizeof(struct devidx));
  didx->maj = maj;
  didx->min = min;

  in->pdata = didx;
  dir_add(&devfs_root, name, in);
  return in;
}

struct device *getdev(struct inode *in) {
  struct devidx *d = in->pdata;
  return chrdev_tab[d->maj][d->min];
}

/* devfs ops */

ssize_t read_devfs(struct file *f, void *buf, size_t count) {
  struct device *d = getdev(f->inode);
  if(!d || !d->ops.read) return -ENOSYS;
  return d->ops.read(d, buf, count);
}

ssize_t write_devfs(struct file *f, const void *buf, size_t count) {
  struct device *d = getdev(f->inode);
  if(!d || !d->ops.read) return -ENOSYS;
  return d->ops.write(d, buf, count);
}

int open_devfs(struct inode *in, struct file *f) {
  struct device *d = getdev(in);
  if(d && d->ops.open) {
    return d->ops.open(in, f);
  }
  return 0;
}

int close_devfs(struct file *f) {
  struct device *d = getdev(f->inode);
  if(d && d->ops.close) {
    return d->ops.close(f);
  }
  return 0;
}

DIR *opendir_devfs(struct inode *dir) {
  (void)dir;

  struct dir_data *data = dir->pdata;

  DIR *d = malloc(sizeof(DIR) * NODEVFS);
  for(int i = 0; i < data->count && i < NODEVFS; i++) {
    strcpy(d[i].data, data->entry[i].name);
    d[i].in = data->entry[i].in;
    d[i].type = data->entry[i].in->type;
    d[i].count = data->count;
    d[i].size = data->entry[i].in->size;
  }

  return d;
}

int closedir_devfs(struct inode *dir, DIR *d) {
  free(d);
  (void)dir;
  return 0;
}

int ioctl_devfs(struct file *file, int op, void *arg) {
  struct device *d = getdev(file->inode);
  if(d && d->ops.ioctl) {
    return d->ops.ioctl(d, op, arg);
  }
  return -ENOSYS;
}

struct inode_ops dev_iops = {
  .lookup = lookup_dev,
  .opendir = opendir_devfs,
  .closedir = closedir_devfs
};

struct file_ops dev_fops = {
  .open = open_devfs,
  .read = read_devfs,
  .write = write_devfs,
  .ioctl = ioctl_devfs
};

/* syscall abstraction */

int fsys_ioctl(int fd, uint32_t op, void *arg) {
  if(fd >= NOFILE) return -EBADF;
  if(!arg) return -EFAULT;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (!(f->inode->type & INODE_CHARDEV))
    return -ENOTTY;

  if(f->fops && f->fops->ioctl) {
    return f->fops->ioctl(f, op, arg);
  }
  return -ENOSYS;
}
