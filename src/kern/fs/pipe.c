#include "asm/sys/stat.h"
#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <lib/errno.h>
#include <stddef.h>
#include <stdint.h>

#define ofile p_curproc->p_user->u_ofile

int read_pipe(struct file *f, void *buf, size_t siz) {

  struct inode *in = f->inode;

  if (siz == 0)
    return 0;
  if (!(S_ISFIFO(in->mode)))
    return -EFAULT;

  struct pipe *p = (struct pipe *)f->pdata;
  if (p->readpos == p->writepos)
    return -EAGAIN;
  size_t r = 0;
  while (siz--) {
    *(uint8_t *)(buf++) = p->buf[p->readpos];
    p->readpos          = (p->readpos + 1) % PIPEBUFSIZ;
    //printkf("were here\n");
    r++;
  }
  return r;
}

int write_pipe(struct file *f, const void *buf, size_t siz) {
  if (siz == 0)
    return 0;

  //printkf("d: %x\n", *(uint8_t*)buf);

  struct inode *in = f->inode;

  if (!(S_ISFIFO(in->mode)))
    return -EFAULT;

  struct pipe *p = (struct pipe *)f->pdata;
  if ((p->writepos + 1) % PIPEBUFSIZ == p->readpos)
    return -EAGAIN;

  size_t w = 0;
  while (siz--) {
    p->buf[p->writepos] = *(uint8_t *)(buf++);
    p->writepos         = (p->writepos + 1) % PIPEBUFSIZ;
    w++;
  }
  return w;
}

int close_pipe(struct file *f) {
  free(f->pdata);
  return 0;
}

const static struct file_ops pipe_fops = {.read = read_pipe, .write = write_pipe, .close=close_pipe};

void set_pipe(int fd, int r, int rr) {
  static struct pipe *p = 0;

  ofile[fd] = malloc(sizeof(struct file));

  rr ? p = malloc(sizeof(struct pipe)) : 0;

  struct file *f       = ofile[fd];
  f->pdata = p;
  f->flags             = F_USED | (r ? O_RDONLY : O_WRONLY);
  f->inode             = malloc(sizeof(struct inode));
  f->inode->mode       = S_IFIFO | (r ? 0400 : 0200);
  memset(f->inode->entaddr, 0, sizeof(struct pipe));
  memcpy(&f->inode->fops, &pipe_fops, sizeof(struct file_ops));
}

int fsys_pipe(int fd[2]) {
  if (!fd)
    return -EFAULT;

  int rfd = findfreefd();
  if (rfd < 0)
    return -ENFILE;
  set_pipe(rfd, 1, 1);
  //printkf("fd[0]: %p\n", ofile[0]);

  int wfd = findfreefd();
  if (wfd < 0) {
    close(rfd);
    return -ENFILE;
  }
  set_pipe(wfd, 0, 0);

  *(fd++) = rfd;
  *fd     = wfd;

  return 0;
}
