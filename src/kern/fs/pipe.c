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

int read_pipe(struct inode *in, void *buf, size_t off, size_t siz) {
  if (siz == 0)
    return 0;
  if (!(in->type & INODE_PIPE))
    return -EFAULT;

  struct pipe *p = (struct pipe *)in->entaddr; // yes this seems bad i'll fix it later
  (void)off;
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

int write_pipe(struct inode *in, const void *buf, size_t off, size_t siz) {
  if (siz == 0)
    return 0;

  //printkf("d: %x\n", *(uint8_t*)buf);

  if (!(in->type & INODE_PIPE))
    return -EFAULT;

  struct pipe *p = (struct pipe *)in->entaddr;
  if ((p->writepos + 1) % PIPEBUFSIZ == p->readpos)
    return -EAGAIN;
  (void)off;

  size_t w = 0;
  while (siz--) {
    p->buf[p->writepos] = *(uint8_t *)(buf++);
    p->writepos         = (p->writepos + 1) % PIPEBUFSIZ;
    w++;
  }
  return w;
}

int close_pipe(struct inode *in, int fd) {
  (void)in;
  struct file *f = ofile[fd];
  if(!f) return -EBADF;
  if(!(f->flags & F_USED)) return -EBADF;

  if (f->inode->type != INODE_PIPE)
    return -EBADF;

  f->flags = 0;

  struct pipe *p = (void *)f->inode->entaddr;
  if(!(p->flg & P_FREED)) free(p);
  free(f->inode);
  free(f);
  return 0;
}

const static struct file_ops pipe_fops = {.read = read_pipe, .write = write_pipe, .close = close_pipe};

void set_pipe(int fd, int r, int rr) {
  static struct pipe *p = 0;

  ofile[fd] = malloc(sizeof(struct file));

  rr ? p = malloc(sizeof(struct pipe)) : 0;

  struct file *f       = ofile[fd];
  f->flags             = F_USED | (r ? O_RDONLY : O_WRONLY);
  f->inode             = malloc(sizeof(struct inode));
  f->inode->type       = INODE_PIPE;
  f->inode->permission = r ? 0400 : 0200;
  f->inode->entaddr    = (void *)p;
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
    close_pipe(NULL, rfd);
    return -ENFILE;
  }
  set_pipe(wfd, 0, 0);

  *(fd++) = rfd;
  *fd     = wfd;

  return 0;
}
