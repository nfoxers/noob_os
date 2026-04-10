#include "fs/fat12.h"
#include "fs/vfs.h"
#include "lib/errno.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>

#define NFILES 10

struct inode *lookup_dev(struct inode *dir, const char *name);

DIR *opendir_dev(struct inode *dir);
int  closedir_dev(struct inode *dir, DIR *d);
int  create_dev(struct inode *dir, const char *name, uint16_t flg);
int  unlink_dev(struct inode *dir, const char *name);
int  open_dev(struct inode *dir, const char *restrict pth, uint16_t flg);
int  close_dev(struct inode *dir, int fd);
int  read_dev(struct inode *in, void *buf, size_t off, size_t siz);
int  write_dev(struct inode *in, const void *buf, size_t off, size_t siz);

#define DEV_OPS \
  { lookup_dev, create_dev, unlink_dev, opendir_dev, closedir_dev }

#define DEV_FOPS \
  { open_dev, read_dev, write_dev, close_dev }

struct inode devinode[NFILES] = {
    {.type = INODE_CHARDEV, .permission = 0666, .ops = DEV_OPS, .fops = DEV_FOPS, .cluster0 = 0},
    {.type = INODE_CHARDEV, .permission = 0666, .ops = DEV_OPS, .fops = DEV_FOPS, .cluster0 = 1},
    {.type = INODE_CHARDEV, .permission = 0666, .ops = DEV_OPS, .fops = DEV_FOPS, .cluster0 = 2}};

char *devname[NFILES] = {
    "null", "zero", "random", NULL};

struct inode *lookup_dev(struct inode *dir, const char *name) {
  (void)dir;
  struct inode *in = malloc(sizeof(struct inode));
  // printkf("done malloced\n");
  for (uint32_t i = 0; devname[i] != NULL; i++) {
    if (!strcmp(name, devname[i])) {
      memcpy(in, &devinode[i], sizeof(devinode[0]));
      return in;
    }
  }
  return NULL;
}

DIR *opendir_dev(struct inode *dir) {
  (void)dir;
  int i = 0;
  while (*(devname + i))
    i++;
  int j = i;

  DIR *d = malloc(sizeof(DIR) * i);
  i--;
  while (i + 1) {
    DIR k = {j, INODE_CHARDEV, 0, &devinode[i], {0}};
    strcpy(k.data, devname[i]);
    memcpy(&d[i], &k, sizeof(DIR));
    i--;
  }

  return d;
}

int closedir_dev(struct inode *dir, DIR *d) {
  free(d);
  (void)dir;
  return 0;
}

int create_dev(struct inode *dir, const char *name, uint16_t flg) {
  (void)dir;
  (void)name;
  (void)flg;
  return -EPERM;
}

int unlink_dev(struct inode *dir, const char *name) {
  (void)dir;
  (void)name;
  return -EPERM;
}

int open_dev(struct inode *dir, const char *restrict pth, uint16_t flg) {
  struct inode *in = lookup_dev(dir, pth);
  if (!in)
    return -ENOENT;

  if (in->type & INODE_DIR) {
    free(in);
    return -EISDIR;
  }

  int fd = findfreefd();
  if(fd == -1) {
    free(in);
    return -ENFILE;
  }

  p_curproc->p_user->u_ofile[fd] = malloc(sizeof(struct file));

  p_curproc->p_user->u_ofile[fd]->flags = flg | F_USED;
  p_curproc->p_user->u_ofile[fd]->position = 0;
  p_curproc->p_user->u_ofile[fd]->inode = in;

  return fd;
}

int close_dev(struct inode *dir, int fd) {
  (void)dir;
  struct file *f = p_curproc->p_user->u_ofile[fd];

  if(!f) return -EBADF;
  if(!(f->inode)) return -EBADF;
  if(!(f->flags & F_USED)) return -EBADF;

  free(f->inode);
  free(f);
  return 0;
}

int read_dev(struct inode *in, void *buf, size_t off, size_t siz) {
  (void)off;
  if (in->cluster0 == 0) { // null read
    memset(buf, 0x04, siz);
    return siz;
  }

  if (in->cluster0 == 1) { // zero read
    memset(buf, 0x00, siz);
    return siz;
  }

  if (in->cluster0 == 2) { // random read
    memcpy(buf, (void *)0x7c00, siz);
    return siz;
  }

  return 0;
}

int write_dev(struct inode *in, const void *buf, size_t off, size_t siz) {
  (void)in;
  (void)buf;
  (void)off;
  (void)siz;
  return 0;
}