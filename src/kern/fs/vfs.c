#include "fs/vfs.h"
#include "ams/bits/struct_stat.h"
#include "ams/sys/stat.h"
#include "crypt/crypt.h"
#include "lib/list.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <lib/errno.h>
#include <stddef.h>
#include <stdint.h>

struct inode rootnode;
struct mount rootmnt;

extern struct super_block devblock;
extern struct inode       devnode;
extern struct mount       devmnt;
extern void init_devs();

#define MAX_PLEN 127

char *path_canon(const char *cwd, const char *path) {
  if (!cwd || !path)
    return NULL;

  size_t n   = strlen(path) + strlen(cwd) + 10;
  char  *tmp = malloc(n);
  char  *buf = malloc(n);

  if (*path == '/') {
    strncpy(tmp, path, n - 1);
  } else {
    snprintkf(tmp, n, "%s/%s", cwd, path);
  }
  tmp[n - 1] = 0;

  char *segs[MAX_PLEN / 2];
  int   top = 0;

  char *tok;
  char *rest = tmp;

  while ((tok = strtok_r(rest, "/", &rest))) {
    if (!strcmp(tok, "."))
      continue;
    else if (!strcmp(tok, "..")) {
      if (top > 0)
        top--;
    } else
      segs[top++] = tok;
  }

  size_t len = 0;
  buf[len++] = '/';

  for (int i = 0; i < top; i++) {
    size_t seglen = strlen(segs[i]);

    if (len + seglen + 1 > n) {
      free(tmp);
      return NULL;
    }

    memcpy(buf + len, segs[i], seglen);
    len += seglen;

    if (i < top - 1)
      buf[len++] = '/';
  }

  if (len == 0) {
    *buf++ = '/';
    *buf   = 0;
    free(tmp);
    return buf;
  }

  free(tmp);
  buf[len] = 0;
  return buf;
}

/* vfs internals */

void set_dev(struct super_block *b, struct inode *root) {
  b->s_root      = root;
  b->generic_sbp = NULL;

  root->dev = b->s_dev;
  root->sb  = b;
}

void set_special() {
  struct inode *dev_fs = lookup_vfs("/dev");
  imount(dev_fs, &devmnt);
}

/* safe guards */

int read_fs(struct file *restrict file, size_t siz, uint8_t *restrict b) {
  if (file->fops && file->fops->read)
    return file->fops->read(file, b, siz);
  return -ENOSYS;
}

int write_fs(struct file *restrict file, size_t siz, const uint8_t *restrict b) {
  if (file->fops && file->fops->write)
    return file->fops->write(file, b, siz);
  return -ENOSYS;
}

int open_fs(struct inode *restrict in, struct file *restrict file) {
  if (file->fops && file->fops->open)
    return file->fops->open(in, file);
  return -ENOSYS;
}

int close_fs(struct file *file) {
  if (file->fops && file->fops->close)
    return file->fops->close(file);
  return -ENOSYS;
}

int create_fs(struct inode *in, const char *name, mode_t mode) {
  if (in && in->ops->creat) {
    return in->ops->creat(in, name, mode);
  }
  return -ENOSYS;
}

int unlink_fs(struct inode *in, const char *name) {
  if (in && in->ops->creat) {
    return in->ops->unlink(in, name);
  }
  return -ENOSYS;
}

ino_t lookup_fs(struct inode *restrict in, const char *restrict name) {
  if ((S_ISDIR(in->mode)) && in->ops && in->ops->lookup) {
    return in->ops->lookup(in, name);
  }
  printkf("no lookup func isdir: %d\n", S_ISDIR(in->mode));
  return -1;
}

// vfs abstraction

struct inode *lookup_vfs_i(char *pth) {
  int mdepth = 0;
  int depth  = 0; // skip the initial /
  for (size_t i = 0; i < strlen(pth); i++) {
    if (pth[i] == '/')
      mdepth++;
  }

  if(rootnode.mnt == 0) {
    _printk("fatal error: / is not mounted\n");
    while(1) asm volatile("hlt");
  }

  struct inode *mroot = rootnode.mnt->sb->s_root;
  struct inode *inode = iget(mroot->sb, mroot->ino);
  char         *sv;

  char *tok = strtok_r(pth, "/", &sv);

  if (!tok) { // effective path = / (root)
    return inode;
  }

  ino_t         ino;
  struct inode *tmp = inode;
  while (tok) {
    ino = lookup_fs(inode, tok);
  
    if ((int)ino == -1) {
      printk("lookup_vfs: file not found\n");
    
      return NULL;
    }
    inode = iget(inode->sb, ino);

    iput(tmp);

    if (inode->iflags & IN_MOUNT && inode->mnt && inode->mnt->sb) {
      struct inode *t = inode;
      inode           = iget(inode->mnt->sb, inode->mnt->sb->s_root->ino);
      iput(t);
    }
    tmp = inode;

    if (!inode)
      return NULL;

    depth++;

    if (depth == mdepth) {
      return inode;
    }

    tok = strtok_r(NULL, "/", &sv);
  }

  return inode;
}

struct inode *lookup_vfs_r(const char *path, char **save) {
  char *cwd = p_curproc->p_user->u_cdirname;

  char *pth = path_canon(cwd, path);
  *save     = strdup(pth);

  // printkf("cwd: %s pth: %s ok: %s \n", cwd, path, *save);
  struct inode *ret = lookup_vfs_i(pth);
  free(pth);
  if (!ret) {
    free(*save);
  }
  return ret;
}

struct inode *lookup_vfs(const char *path) {
  char         *sav;
  struct inode *ret = lookup_vfs_r(path, &sav);
  if (ret)
    free(sav);
  return ret;
}

char *get_comps(char *path) { // we tokenize by changing the last '/' to \0 (and few extra stuff)
  static char *save = 0;
  if (path) {
    char *l = strrchr(path, '/');
    if (!l) {
      save = NULL;
      return path;
    }
    if (!*(l + 1)) {
      *l = 0;
      l  = strrchr(path, '/');
    }
    *l = 0;

    save = l + 1;
    return path;
  }

  char *a = save;
  save    = 0;
  return a;
}

struct inode *look_bs(const char *path, char **name, char **tmp) {
  if (!path || !name || !tmp)
    return NULL;

  *tmp = path_canon(p_curproc->p_user->u_cdirname, path);
  if (**tmp == '/' && !*(*tmp + 1)) {
    // free(*tmp);
    *name = "/";
    return lookup_vfs_i("/");
  }

  char *b = get_comps(*tmp);
  *name   = get_comps(NULL);
  if (!*b)
    b = "/";
  // printkf("b: %s &%x, name: %s (%x)\n", b, b, *name, **name);

  struct inode *ret = lookup_vfs_i(b);
  if (!ret) {
    free(*tmp);
    return NULL;
  }
  return ret;
}

int check_fd(int fd, struct file **f) {
  if (fd > NOFILE)
    return -EBADF;

  *f = p_curproc->p_user->u_ofile[fd];

  if (!*f)
    return -EBADF;
  if (!((*f)->flags & F_USED))
    return -EBADF;
  if (!(*f)->inode)
    return -EBADF;
  return 0;
}

int check_perm(struct file *f, int mask) {
  const struct cred *c = f->cred;
  if (c->euid == 0)
    return 1;
  struct inode *in = f->inode;

  if (c->euid == in->uid) {
    return in->mode & (mask * S_IXUSR);
  }

  if (c->egid == in->gid) {
    return in->mode & (mask * S_IXGRP);
  }

  return in->mode & (mask * S_IXOTH);
}

/* system call abstractions */
/* these syscals MUST NOT return NULL or zeroes for error. 0 is for ERR_OK (no errors at all!)*/
// as negatives are handled by the syscall interrupt handler (i.e by changing errno)

ssize_t fsys_read(int fd, void *buf, size_t count) {
  if (fd >= NOFILE)
    return -EBADF;

  if (!count)
    return 0;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  // printkf("f: %x\n", f->fops);

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (S_ISDIR(f->inode->mode))
    return -EISDIR;
  if (O_ISW(f->flags))
    return -EPERM;

  int allow = check_perm(f, S_IROTH);
  if (!allow)
    return -EPERM;

  uint32_t read = read_fs(f, count, buf);
  return read;
}

ssize_t fsys_write(int fd, void *buf, size_t count) {
  if (fd >= NOFILE)
    return -EBADF;

  if (!count)
    return 0;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (S_ISDIR(f->inode->mode))
    return -EISDIR;
  if (O_ISR(f->flags))
    return -EPERM; 

  int allow = check_perm(f, S_IWOTH);
  if (!allow)
    return -EPERM;

  uint32_t write = write_fs(f, count, buf);
  return write;
}

int fsys_open(const char *pathname, int flags) {
  if (((flags & O_ACCMODE) == 3))
    return -EINVAL;

  struct inode *in = lookup_vfs(pathname);
  if (!in)
    return -ENOENT;

  // printkf("in: %p\n", in->fops);

  int fd = findfreefd();
  if (fd == -1) {
    iput(in);
    return -ENFILE;
  }

  struct file *f = malloc(sizeof(struct file));

  f->inode    = in;
  f->fops     = in->fops;
  f->mode     = in->mode;
  f->cred     = (const struct cred *)&p_curproc->p_user->cred;
  f->position = 0;
  f->flags    = flags | F_USED;
  f->refcont  = 1;

  int r = 0;
  if (f->fops && f->fops->open) {
    if ((r = f->fops->open(in, f)) < 0) {
      free(f);
      iput(in);
      return r;
    }
  }

  p_curproc->p_user->u_ofile[fd] = f;

  return fd;
}

int fsys_close(int fd) {
  if (fd >= NOFILE)
    return -EBADF;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  if (!f)
    return -EBADF;
  if (!(f->flags | F_USED))
    return -EBADF;

  p_curproc->p_user->u_ofile[fd] = NULL;

  f->refcont--;

  if (f->refcont == 0) {
    if (f->fops && f->fops->close)
      f->fops->close(f);
    iput(f->inode);
    free(f);
  }

  return 0;
}

int fsys_mkdir(const char *path, mode_t mode) {
  (void)(path + mode);
  return -ENOSYS;
}

int fsys_unlink(const char *pathname) {
  char         *tmp = 0;
  char         *nam = 0;
  struct inode *in  = look_bs(pathname, &nam, &tmp);
  if (!in)
    return -EFAULT;

  int ret = unlink_fs(in, nam);
  iput(in);
  free(tmp);
  return ret;
}

int fsys_creat(const char *pathname, mode_t mode) {
  char         *tmp;
  char         *nam;
  struct inode *in = look_bs(pathname, &nam, &tmp);
  if (!in)
    return -EFAULT;

  int ret = create_fs(in, nam, mode);
  iput(in);
  free(tmp);
  return ret;
}

int fsys_chdir(const char *path) {
  if (!path)
    return -EFAULT;
  char *pth;

  struct inode *inp = lookup_vfs_r(path, &pth);
  if (!inp) {
    printkf("%s: no such file\n", pth);
    return -ENOENT;
  }

  if (!S_ISDIR(inp->mode)) {
    printkf("%s: not a directory\n", pth);

    free(pth);
    return -ENOTDIR;
  }

  memcpy(&p_curproc->p_user->u_cdir, inp, sizeof(struct inode));
  strcpy(p_curproc->p_user->u_cdirname, pth);

  iput(inp);
  free(pth);
  return 0;
}

int fsys_dup2(int oldfd, int newfd) {
  int          r;
  struct file *f;
  if ((r = check_fd(oldfd, &f)) < 0)
    return r;

  struct file *n;
  if ((r = check_fd(newfd, &n)) >= 0) {
    // newfd exists
    close(newfd);
  }

  f->refcont++;
  p_curproc->p_user->u_ofile[newfd] = f;

  return newfd;
}

int fsys_dup(int oldfd) {
  int newfd = findfreefd();
  if (newfd == -1)
    return -ENFILE;

  fsys_dup2(oldfd, newfd);

  return newfd;
}

int fsys_fstat(int fd, struct stat *statbuf) {
  if (fd >= NOFILE)
    return -EBADF;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  // printkf("f: %x\n", f->fops);

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;

  memset(statbuf, 0, sizeof *statbuf);

  struct inode *in = f->inode;
  statbuf->st_ino  = in->ino;
  statbuf->st_mode = in->mode;
  statbuf->st_size = in->size;
  statbuf->st_uid = in->uid;
  statbuf->st_gid = in->gid;
  statbuf->st_dev = in->sb->s_dev;
  statbuf->st_rdev = in->rdev;
  statbuf->st_blksize = in->sb->s_blocksize;
  statbuf->st_blocks = in->blkcount;
  // ! expand!!

  return 0;
}

int fsys_getdents(int fd, struct nnux_dirent *dirp, size_t count) {
  if (!count)
    return 0;

  struct file *f;
  ;
  int r;
  if ((r = check_fd(fd, &f)) < 0)
    return r;

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (!S_ISDIR(f->inode->mode))
    return -ENOTDIR;

  if (f->fops && f->fops->readdir) {
    return f->fops->readdir(f, dirp, count);
  }
  return -ENOSYS;
}

/* convenience functions */
// now these can return whatever they want

char *permget(mode_t mode, uint16_t perm) {
#define SIZ_s 3 * 3 + 1 + 1
  char *s = malloc(SIZ_s);
  memset(s, 0, SIZ_s);

  static const char *plist[] = {
      "---",
      "--x",
      "-w-",
      "-wx",
      "r--",
      "r-x",
      "rw-",
      "rwx"};

  char c = S_ISDIR(mode) ? 'd' : S_ISCHR(mode) ? 'c'
                             : S_ISBLK(mode)   ? 'b'
                                               : '-';

  s[0] = c;

  memcpy(s + 1, plist[(perm >> 6) & 7], 3);
  memcpy(s + 4, plist[(perm >> 3) & 7], 3);
  memcpy(s + 7, plist[(perm >> 0) & 7], 3);

  return s;
}

int nlsdir(const char *path, int flg) {
  if (!path)
    return 1;

  int fd = open(path, O_RDONLY);

  uint8_t *buf = malloc(1024);

  ssize_t nread = getdents(fd, (void *)buf, 1024);

  if (nread == -1) {
    return 2;
  }

  size_t off = 0;

  while (off < (size_t)nread) {
    struct nnux_dirent *d = (struct nnux_dirent *)(buf + off);
    if (flg & 1) {
      printkf("%8u  ", d->d_ino);
      int d_type = d->d_type;
      printkf("%-10s  ", (d_type == DT_REG) ? "regular" : (d_type == DT_DIR) ? "directory"
                                                     : (d_type == DT_FIFO)  ? "FIFO"
                                                     : (d_type == DT_SOCK)  ? "socket"
                                                     : (d_type == DT_LNK)   ? "symlink"
                                                     : (d_type == DT_BLK)   ? "block dev"
                                                     : (d_type == DT_CHR)   ? "char dev"
                                                                            : "???");
      printkf("%4d %4d  %s\n", d->d_reclen, d->d_off, d->name);

    } else
      printkf("%s ", d->name);

    off += d->d_reclen;
  }
  if(!(flg & 1)) printkf("\n");

  free(buf);

  close(fd);
  return 0;
}

int flstat(const char *name) {
  int fd = open(name, O_RDONLY);
  if(fd < 0) {
    perror("cant stat:");
    return 1;
  }

  struct stat stat;
  fstat(fd, &stat);

  char *perm = permget(stat.st_mode, stat.st_mode & 0777);

  printkf("  file: %s\n", name);
  printkf("  size: %u     blocks: %d\n", stat.st_size, stat.st_blocks);
  printkf("device: %d,%d", MAJOR(stat.st_dev), MINOR(stat.st_dev));
  printkf("  inode: %d", stat.st_ino);

  if(S_ISCHR(stat.st_mode) || S_ISBLK(stat.st_mode)) {
    printkf("  device type: %d,%d", MAJOR(stat.st_rdev), MINOR(stat.st_rdev));
  }
  printkf("\n");

  printkf("access: (0%03.3o/%s)  uid: %d gid: %d\n", stat.st_mode & 0777, perm, stat.st_uid, stat.st_gid);

  free(perm);
  close(fd);
  return 0;
}

int findfreefd() {
  int           fd = 0;
  struct file **f  = p_curproc->p_user->u_ofile;
  // printkf("of: %p\n", f[fd]);
  while (fd < NOFILE && (f[fd] && f[fd]->flags & F_USED)) {
    // printkf("of: %p\n", f[fd]);
    fd++;
  }
  if (fd == NOFILE)
    return -1;
  return fd;
}