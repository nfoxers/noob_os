#include "fs/devfs.h"
#include "ams/sys/stat.h"
#include "dev/block_dev.h"
#include "fs/fat12.h"
#include "fs/vfs.h"
#include "lib/errno.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>
#include "crypt/crypt.h"

#define MAXMAJ 10
#define MAXMIN 5

#define NODEVFS (MAXMAJ * MAXMIN)

struct device *chrdev_tab[MAXMAJ][MAXMIN];

struct inode_ops dev_iops;
struct file_ops  dev_fops;

struct dir_data devfs_root;

struct super_block devblock;
struct inode devnode;
struct mount devmnt;

struct device dev_blk_dev = {.next = NULL};

int next_dev = 0;

void dir_add(struct dir_data *dir, const char *name, struct inode *in) {
  struct dir_entry *dent = dir->entry;
  while (dent->flg & DE_USED)
    dent++;
  dent->flg = DE_USED;
  dent->in  = in;
  dir->count++;
  strcpy(dent->name, name);
}

ino_t lookup_dev(struct inode *dir, const char *name) {
  struct dir_data *data = dir->pdata;
  if (!data)
    return -1;

  for (int i = 0; i < data->count; i++) {
    if (!strcmp(name, data->entry[i].name)) {
      return data->entry[i].in->ino;
    }
  }
  return -1;
}

int register_dev(uint16_t maj, uint16_t min, struct device *dev) {
  if (chrdev_tab[maj][min])
    return -1;
  chrdev_tab[maj][min] = dev;
  return 0;
}

struct inode *creat_devfs(const char *name, struct device *dev, uint16_t maj, uint16_t min) {
  register_dev(maj, min, dev);

  struct inode *in = malloc(sizeof(struct inode));

  in->mode = S_IFCHR | 0666;
  in->ino = next_dev++;
  in->sb = &devblock;
  in->fops = &dev_fops;
  in->refs = 1;

  in->rdev = MKDEV(maj, min);
  dev->devt = in->rdev;
  dir_add(&devfs_root, name, in);
  //iadd(in);
  return in;
}

struct inode *creat_blockdev(const char *name, struct device *dev, uint16_t maj, uint16_t min) {
  struct device *d = &dev_blk_dev;
  while(d->next) d = d->next;
  d->next = dev;
  
  struct inode *in = malloc(sizeof(*in));

  in->mode = S_IFBLK | 0666;
  in->ino = next_dev++;
  in->sb = &devblock;
  in->fops = &dev_fops;
  in->refs = 1;

  in->rdev = MKDEV(maj, min);
  dev->devt = in->rdev;
  dir_add(&devfs_root, name, in);
  return in;
}

struct device *device_get_devblk(dev_t dev) {
  struct device *d = &dev_blk_dev;
  while(d->next) {
    d = d->next;
    if(d->devt == dev) {
      return d;
    }
  }

  return 0;
}

struct block_dev *blkdev_get_dev(dev_t dev) {
  struct device *d = &dev_blk_dev;
  while(d->next) {
    d = d->next;
    if(d->devt == dev) {
      return d->pdata;
    }
    
  }

  return NULL;
}

struct block_dev *blkdev_get_path(const char *path) {
  struct inode *in = lookup_vfs(path);
  if(!S_ISBLK(in->mode)) {
    return NULL;
  }

  struct block_dev *bd = blkdev_get_dev(in->rdev);
  iput(in);

  return bd;
}

struct device *getdev(struct inode *in) {

  if(S_ISBLK(in->mode)) {
    return device_get_devblk(in->rdev);
  }

  uint16_t maj = MAJOR(in->rdev);
  uint16_t min = MINOR(in->rdev);
  return chrdev_tab[maj][min];
}

/* devfs ops */

ssize_t read_devfs(struct file *f, void *buf, size_t count) {
  struct device *d = getdev(f->inode);
  if (!d || !d->ops.read)
    return -ENOSYS;
  return d->ops.read(d, buf, count);
}

ssize_t write_devfs(struct file *f, const void *buf, size_t count) {
  struct device *d = getdev(f->inode);
  if (!d || !d->ops.read)
    return -ENOSYS;
  return d->ops.write(d, buf, count);
}

int open_devfs(struct inode *in, struct file *f) {
  struct device *d = getdev(in);
  if (d && d->ops.open) {
    return d->ops.open(in, f);
  }
  return 0;
}

int close_devfs(struct file *f) {
  struct device *d = getdev(f->inode);
  if (d && d->ops.close) {
    return d->ops.close(f);
  }
  return 0;
}

int ioctl_devfs(struct file *file, int op, void *arg) {
  struct device *d = getdev(file->inode);
  if (d && d->ops.ioctl) {
    return d->ops.ioctl(d, op, arg);
  }
  return -ENOSYS;
}

void dev_inoder(struct inode *in) {
  if(in->ino == 0) {
    memcpy(in, &devnode, sizeof(*in));
    return;
  }
  for(int i = 0; i < 16; i++) {
    struct inode *n = devfs_root.entry[i].in;
    if(n && n->ino == in->ino) {
        memcpy(in, n, sizeof(*n));
        break;
    }
  }
} 

struct inode_ops dev_iops = {
    .lookup   = lookup_dev,
};

struct file_ops dev_fops = {
    .open  = open_devfs,
    .read  = read_devfs,
    .write = write_devfs,
    .ioctl = ioctl_devfs};

struct super_ops dev_sops = {
  .read_inode = dev_inoder
};

/* some kind of inits */

ssize_t read_null(struct device *d, void *buf, size_t count) {
  //printkf("min: %d\n", d->idx->min);
  uint16_t min = MINOR(d->devt);

  if(min == 1) {
    return 0;
  }
  if(min == 2) {
    memset(buf, 0, count);
    return count;
  }
  if(min == 3) {
    size_t siz = count;
    size_t off = 0;
    while(siz > 4) {
      get_rand((uint8_t *)buf + off, 4);
      siz -= 4;
      off += 4;
    }
    get_rand((uint8_t *)buf + off, siz);
    return count;
  }
  return 0;
}

ssize_t write_null(struct device *d, const void *buf, size_t count) {
  (void)d;
  (void)buf;
  (void)count;
  return 0;
}

struct device nulldev = {
    .name = "null dev",
    .ops  = {
         .read = read_null,
        .write=write_null}};

struct device zerodev = {
    .name = "zero dev",
    .ops  = {
         .read = read_null,
        .write=write_null}};

struct device randdev = {
    .name = "rand dev",
    .ops  = {
         .read = read_null,
        .write=write_null}};

void init_devs() {
  devmnt.sb = &devblock;

  devnode.ino = 0;
  devnode.mode = S_IFDIR | 0766;
  devnode.ops = &dev_iops;
  devnode.fops = &dev_fops;
  devnode.refs = 1;
  devnode.pdata = &devfs_root;

  devblock.s_op = &dev_sops;
  devblock.s_blocksize = 4096;
  devblock.s_magic = 'DEV ';
  devblock.s_dev = 0;
  set_dev(&devblock, &devnode);

  next_dev++;

  creat_devfs("null", &nulldev, 1, 1);
  creat_devfs("zero", &zerodev, 1, 2);
  creat_devfs("random", &randdev, 1, 3);
}

/* syscall abstraction */

int fsys_ioctl(int fd, uint32_t op, void *arg) {
  if (fd >= NOFILE)
    return -EBADF;
  if (!arg)
    return -EFAULT;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (!(S_ISCHR(f->inode->mode)))
    return -ENOTTY;

  if (f->fops && f->fops->ioctl) {
    return f->fops->ioctl(f, op, arg);
  }
  return -ENOSYS;
}



