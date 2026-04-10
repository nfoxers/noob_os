#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <lib/errno.h>
#include <stddef.h>
#include <stdint.h>

extern struct inode rootinode;

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

int read_fs(struct inode *in, uint32_t off, size_t siz, uint8_t *b) {
  if (in && in->fops.read)
    return in->fops.read(in, b, off, siz);
  return -ENOSYS;
}

int write_fs(struct inode *in, uint32_t off, size_t siz, uint8_t *b) {
  if (in && in->fops.write)
    return in->fops.write(in, b, off, siz);
  return -ENOSYS;
}

int open_fs(struct inode *restrict in, const char *restrict pth, uint16_t mode) {
  if (in && in->fops.open)
    return in->fops.open(in, pth, mode);
  return -ENOSYS;
}

int close_fs(struct inode *in, int fd) {
  if (in && in->fops.close)
    return in->fops.close(in, fd);
  return -ENOSYS;
}

DIR *opendir_fs(struct inode *in) {
  if (in && in->ops.opendir) {
    return in->ops.opendir(in);
  }
  return (DIR *)-ENOSYS;
}

int closedir_fs(struct inode *in, DIR *d) {
  if (in && in->ops.closedir) {
    return in->ops.closedir(in, d);
  }
  return -ENOSYS;
}

int create_fs(struct inode *in, const char *name, mode_t mode) {
  if (in && in->ops.creat) {
    return in->ops.creat(in, name, mode);
  }
  return -ENOSYS;
}

int unlink_fs(struct inode *in, const char *name) {
  if (in && in->ops.creat) {
    return in->ops.unlink(in, name);
  }
  return -ENOSYS;
}

struct inode *lookup_fs(struct inode *in, const char *name) {
  if ((in->type == INODE_DIR) && in->ops.lookup) {
    return in->ops.lookup(in, name);
  }
  return NULL;
}

struct inode *lookup_vfs_i(char *pth) {
  int mdepth = 0;
  int depth  = 0; // skip the initial /
  for (size_t i = 0; i < strlen(pth); i++) {
    if (pth[i] == '/')
      mdepth++;
  }

  struct inode *inode = &rootinode;
  char         *sv;

  char *tok = strtok_r(pth, "/", &sv);

  if (!tok) { // effective path = / (root)
    struct inode *in = malloc(sizeof(struct inode));
    memcpy(in, inode, sizeof(struct inode));
    // free(pth);
    return in;
  }

  while (tok) {
    inode = lookup_fs(inode, tok);

    if (!inode) {
      printkf("lookup_vfs: file not found\n");
      // free(pth);
      return NULL;
    }

    depth++;

    if (depth == mdepth) {
      // free(pth);
      return inode;
    }
    free(inode);

    tok = strtok_r(NULL, "/", &sv);
  }

  // free(pth);
  return inode;
}

struct inode *lookup_vfs_r(const char *path, char **save) {
  char *cwd = p_curproc->p_user->u_cdirname;

  char *pth = path_canon(cwd, path);
  *save     = strdup(pth);

  // printkf("cwd: %s pth: %s ok: %s \n", cwd, path, *save);

  struct inode *ret = lookup_vfs_i(pth);
  free(pth);
  return ret;
}

struct inode *lookup_vfs(const char *path) {
  char         *sav;
  struct inode *ret = lookup_vfs_r(path, &sav);
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
  printkf("b: %s &%x, name: %s (%x)\n", b, b, *name, **name);

  struct inode *ret = lookup_vfs_i(b);
  if (!ret) {
    free(*tmp);
    return NULL;
  }
  return ret;
}

/* system call abstractions */
/* these syscals MUST NOT return NULL or zeroes for error. 0 is for ERR_OK (no errors at all!)*/
// as negatives are handled by the syscall interrupt handler (i.e by changing errno)

ssize_t fsys_read(int fd, void *buf, size_t count) {
  if (fd >= NOFILE)
    return -EBADF;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (f->inode->type & INODE_DIR)
    return -EISDIR;
  if (f->flags & O_WRONLY)
    return -EPERM;

  // printkf("fsysread %d\n", f->inode->fops.read);
  uint32_t read = read_fs(f->inode, f->position, count, buf);
  f->position += read;
  return read;
}

ssize_t fsys_write(int fd, void *buf, size_t count) {
  if (fd >= NOFILE)
    return -EBADF;

  struct file *f = p_curproc->p_user->u_ofile[fd];

  if (!f)
    return -EBADF;
  if (!(f->flags & F_USED))
    return -EBADF;
  if (!f->inode)
    return -EBADF;
  if (f->inode->type & INODE_DIR)
    return -EISDIR;
  if (f->flags & O_RDONLY)
    return -EPERM;

  // printkf("fsyswrite %d\n", f->inode->fops.read);
  uint32_t read = write_fs(f->inode, f->position, count, buf);
  f->position += read;
  return read;
}

int fsys_open(const char *pathname, int flags) {
  if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR)))
    return -EINVAL;

  char         *tmp = 0;
  char         *nam = 0;
  struct inode *in  = look_bs(pathname, &nam, &tmp);
  if (!in)
    return -ENOENT;
  // printkf("in: %x\n", in);
  if (!(in->type & INODE_DIR)) {
    free(in);
    free(tmp);
    return -ENOTDIR;
  }

  int ret = open_fs(in, nam, flags);
  free(in);
  free(tmp);
  return ret;
}

int fsys_close(int fd) {
  if (fd >= NOFILE)
    return -EBADF;

  struct file *i = p_curproc->p_user->u_ofile[fd];

  if (!i)
    return -EBADF;
  if (!(i->flags | F_USED))
    return -EBADF;
  if (!i->inode)
    return -EBADF;
  return close_fs(i->inode, fd);
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
  free(in);
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
  free(in);
  free(tmp);
  return ret;
}

DIR *fsys_opendir(const char *path) {
  if (!path)
    return (DIR *)-EFAULT;

  struct inode *in = lookup_vfs(path);
  if (!in)
    return (DIR *)-ENOENT;
  if (!(in->type & INODE_DIR)) {
    free(in);
    return (DIR *)-ENOTDIR;
  }

  DIR *ret = opendir_fs(in);
  free(in);

  return ret;
};

int fsys_closedir(struct inode *in, DIR *d) {
  // printkf("closedir on in %x\n", in);
  return closedir_fs(in, d);
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

  if (inp->type != INODE_DIR) {
    printkf("%s: not a directory\n", pth);

    free(inp);
    free(pth);
    return -ENOTDIR;
  }

  memcpy(&p_curproc->p_user->u_cdir, inp, sizeof(struct inode));
  strcpy(p_curproc->p_user->u_cdirname, pth);

  free(inp);
  free(pth);
  return 0;
}

/* convenience functions */
// now these can return whatever they want

char *permget(uint16_t type, uint16_t perm) {
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

  char c = type & INODE_DIR ? 'd' : type & INODE_CHARDEV ? 'c'
                                : type & INODE_BLKDEV    ? 'b'
                                                         : '-';

  s[0] = c;

  memcpy(s + 1, plist[(perm >> 6) & 7], 3);
  memcpy(s + 4, plist[(perm >> 3) & 7], 3);
  memcpy(s + 7, plist[(perm >> 0) & 7], 3);

  return s;
}

int lsdir(const char *path, int flg) {
  // printkf("cwd: %s\n", p_curproc->p_user->u_cdirname);

  DIR *const d = opendir(path);
  if (!d) {
    perror("ls: opendir");
    return 1;
  }

  if (flg & 1) {
    for (int i = 0; i < d->count; i++) {
      char *p = permget(d[i].type, d[i].in->permission);
      printkf("%s ", p);
      free(p);

      d[i].size ? printkf("% 4d ", d[i].size) : printkf("     ");
      printkf("%s\n", d[i].data);
    }
  } else {
    for (int i = 0; i < d->count; i++) {
      printkf("%s ", d[i].data);
    }
    putchr('\n');
  }

  // printkf("pat: %s\n", path);

  struct inode *in = lookup_vfs(path);

  if (closedir(in, d)) {
    perror("ls: closedir");
    free(in);
  }
  free(in);
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