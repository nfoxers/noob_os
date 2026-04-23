#include "fs/vfs.h"
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

extern struct inode rootnode;

#define HITAB_MAX 64

struct hlist_head hitab[HITAB_MAX];
struct inode      ilru = {.dev = 99, .sb = 0, .lru.next = &ilru.lru, .lru.prev = &ilru.lru};

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

dev_t nextdev = 0;

void set_dev(struct super_block *b, struct inode *root) {
  b->s_dev       = nextdev++;
  b->s_root      = root;
  b->generic_sbp = NULL;

  root->dev = b->s_dev;
  root->sb  = b;
}

uint32_t ihash(dev_t dev, ino_t ino) {
  uint32_t k = murmur3((uint8_t[]){dev, ino}, 2, dev + ino);
  return k % HITAB_MAX;
}

struct inode *inode_alloc(struct super_block *sb) {
  struct inode *in = malloc(sizeof(struct inode));
  in->dev          = sb->s_dev;
  return in;
}

void iadd(struct inode *in) {
  uint32_t h = ihash(in->sb->s_dev, in->ino);
  hlist_add_head(&in->hnode, &hitab[h]);
}

struct inode *iget(struct super_block *sb, fsino_t fsino) {
  if (!sb)
    return NULL;
  uint32_t h = ihash(sb->s_dev, fsino);
  // printkf("ino: %d ", fsino);
  //  printkf("dev: %d ", sb->s_dev);
  struct hlist_node *k = hitab[h].first;
  // printkf("hash: %d\n", h);

  while (k) {
    struct inode *in = container_of(k, struct inode, hnode);
    // printkf("cache bucket ");
    if ((in->sb->s_dev == sb->s_dev) && (in->ino == fsino)) {
      // printkf("cache hit\n");
      in->refs++;
      return in;
    }
    k = k->next;
  }

  // ok, maybe the inode isnt inside the hlist table, try the lru
  // printkf("cache miss (hash)\n");
  struct list_head *m = &ilru.lru;
  do {
    // printkf("lru cache ");
    struct inode *in = container_of(m, struct inode, lru);
    if (in->sb == 0) {
      m = m->next;
      continue;
    }
    if ((in->sb->s_dev == sb->s_dev) && (in->ino == fsino)) {
      // printkf("cache hit (lru)\n");
      if (in->refs == 0)
        list_del(&in->lru);
      in->refs++;
      hlist_add_head(&in->hnode, &hitab[h]);
      return in;
    }
    m = m->next;
  } while (m != &ilru.lru);

  // printkf("cache miss (all)\n");
  struct inode *in = malloc(sizeof(struct inode));
  in->ino          = fsino;
  in->dev          = sb->s_dev;
  in->sb           = sb;
  in->refs         = 1;
  init_list(&in->lru);

  if (sb->s_op && sb->s_op->read_inode)
    sb->s_op->read_inode(in);

  // printkf("mode: %x\n", in->mode);

  in->ino  = fsino;
  in->dev  = sb->s_dev;
  in->sb   = sb;
  in->refs = 1;

  hlist_add_head(&in->hnode, &hitab[h]);
  return in;
}

void free_inode(struct inode *in) {
  if (in->sb && in->sb->s_op && in->sb->s_op->put_inode)
    in->sb->s_op->put_inode(in);
  list_del(&in->lru);
  free(in);
}

void iput(struct inode *in) {
  if (in->refs == 0) {
    printkf("err: inode has 0 refs at iput\n");
    return;
  }

  in->refs--;

  if (in->refs > 0)
    return;

  if (in->iflags & IN_DIRTY) {
    if (in->sb->s_op->write_inode)
      in->sb->s_op->write_inode(0, in);
  }

  // printkf("added inode %x into lru\n", in);
  list_add(&in->lru, &ilru.lru);
  hlist_del(&in->hnode);
  // no free_inodes
}

void imount(struct inode *in, struct mount *mnt) {
  in->iflags |= IN_MOUNT;
  in->mnt = mnt;
}

void purge_lru() {
  struct list_head *m = &ilru.lru;
  while (m->next != m) {
    struct list_head *k  = m->next;
    struct inode     *in = container_of(k, struct inode, lru);
    free_inode(in);
  }
}

void print_caches() {
  int tmp = 0;
  printkf("HASH CACHE:\n");
  for (int h = 0; h < HITAB_MAX; h++) {
    tmp                  = 0;
    struct hlist_node *k = hitab[h].first;
    if (k) {
      printkf("[%02x]: ", h);
      tmp = 1;
    }
    while (k) {
      struct inode *in = container_of(k, struct inode, hnode);
      printkf("%d@%d.", in->ino, in->sb->s_dev);
      printkf("%d ", in->refs);
      k = k->next;
    }
    if (tmp)
      printkf("\n");
  }
  printkf("\nLRU CACHE:\n");
  struct list_head *m = &ilru.lru;
  do {
    struct inode *in = container_of(m, struct inode, lru);
    printkf("[%p]: %d@%d.%d\n", in, in->ino, in->dev, in->refs);
    m = m->next;
  } while (m != &ilru.lru);

  printkf("\nFILE DESCS\n");
  for (size_t i = 0; i < sizeof(p_curproc->p_user->u_ofile) / sizeof(p_curproc->p_user->u_ofile[0]); i++) {
    if (p_curproc->p_user->u_ofile[i]) {
      printkf("[%02d]: %p.%d\n", i, p_curproc->p_user->u_ofile[i]->inode, p_curproc->p_user->u_ofile[i]->refcont);
    }
  }
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

DIR *opendir_fs(struct inode *in) {
  if (in->ops && in->ops->opendir) {
    return in->ops->opendir(in);
  }
  return (DIR *)-ENOSYS;
}

int closedir_fs(struct inode *in, DIR *d) {
  if (in->ops && in->ops->closedir) {
    return in->ops->closedir(in, d);
  }
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

fsino_t lookup_fs(struct inode *restrict in, const char *restrict name) {
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

  struct inode *inode = iget(rootnode.sb, rootnode.ino);
  char         *sv;

  char *tok = strtok_r(pth, "/", &sv);

  if (!tok) { // effective path = / (root)
    // free(pth);
    iput(inode);
    return iget(rootnode.sb, rootnode.ino);
  }

  ino_t         ino;
  struct inode *tmp = inode;
  while (tok) {
    // printkf("searching for: %s\n", tok);
    //  printkf("in inode %x\n", inode->mode & S_IFREG);
    ino = lookup_fs(inode, tok);
    // printkf("ino (lookup): %d\n", ino);
    if ((int)ino == -1) {
      printkf("lookup_vfs: file not found\n");
      // free(pth);
      return NULL;
    }
    inode = iget(inode->sb, ino);

    iput(tmp);
    // printkf("inode (iget): %x freed: %x\n", inode, tmp);
    if (inode->iflags & IN_MOUNT) {
      // printkf("inode is a mount\n");
      // inode = inode->mnt->sb->s_root;
      struct inode *t = inode;
      inode           = iget(inode->mnt->sb, inode->mnt->sb->s_root->ino);
      iput(t);
    }
    tmp = inode;

    if (!inode)
      return NULL;

    depth++;

    if (depth == mdepth) {
      // free(pth);
      return inode;
    }

    // memcpy(&tmp, inode, sizeof(tmp));

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
  if (f->flags & O_WRONLY)
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
  if (f->flags & O_RDONLY)
    return -EPERM;

  int allow = check_perm(f, S_IWOTH);
  if (!allow)
    return -EPERM;

  uint32_t write = write_fs(f, count, buf);
  return write;
}

int fsys_open(const char *pathname, int flags) {
  if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR)))
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

DIR *fsys_opendir(const char *path) {
  if (!path)
    return (DIR *)-EFAULT;

  struct inode *in = lookup_vfs(path);
  if (!in)
    return (DIR *)-ENOENT;
  if (!(S_ISDIR(in->mode))) {
    iput(in);
    return (DIR *)-ENOTDIR;
  }

  DIR *ret = opendir_fs(in);
  iput(in);
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

  struct inode *in = f->inode;
  statbuf->st_ino = in->ino;
  statbuf->st_mode = in->mode;
  statbuf->st_size = in->size;
  // ! expand!!

  return 0;
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

int lsdir(const char *path, int flg) {
  // printkf("cwd: %s\n", p_curproc->p_user->u_cdirname);

  DIR *const d = opendir(path);
  if (!d) {
    perror("ls: opendir");
    return 1;
  }

  if (flg & 1) {
    for (int i = 0; i < d->count; i++) {

      if (flg & 2)
        printkf("% 8x ", d[i].in->ino);

      char *p = permget(d[i].in->mode, d[i].in->mode & 0777);
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
  // printkf("%x\n", in->hnode);
  if (closedir(in, d)) {
    perror("ls: closedir");
  }
  iput(in);
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