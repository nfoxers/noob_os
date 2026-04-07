#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "video/printf.h"
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

uint32_t read_fs(struct inode *in, uint32_t off, size_t siz, uint8_t *b) {
  if (in->fops.read)
    return in->fops.read(in, b, off, siz);
  return 0;
}

uint32_t write_fs(struct inode *in, uint32_t off, size_t siz, uint8_t *b) {
  if (in->fops.write)
    return in->fops.write(in, b, off, siz);
  return 0;
}

void open_fs(struct inode *in, uint16_t mode) {
  if (in->fops.open)
    in->fops.open(in, mode);
}

void close_fs(struct inode *in) {
  if (in->fops.close)
    in->fops.close(in);
}

DIR *opendir_fs(struct inode *in) {
  if (in && in->ops.opendir) {
    return in->ops.opendir(in);
  }
  return NULL;
}

int closedir_fs(struct inode *in, DIR *d) {
  if (in && in->ops.closedir) {
    return in->ops.closedir(in, d);
  }
  return -ENOSYS;
}

struct inode *lookup_fs(struct inode *in, const char *name) {
  if ((in->type == INODE_DIR) && in->ops.lookup) {
    return in->ops.lookup(in, name);
  }
  return NULL;
}

struct inode *lookup_vfs_r(const char *path, char **save) {
  char *cwd = p_curproc->p_user->u_cdirname;

  char *pth = path_canon(cwd, path);
  *save     = strdup(pth);

  // printkf("cwd: %s pth: %s ok: %s \n", cwd, path, *save);

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
    free(pth);
    return in;
  }

  while (tok) {
    inode = lookup_fs(inode, tok);

    if (!inode) {
      printkf("lookup_vfs: file not found\n");
      free(pth);
      return NULL;
    }

    depth++;

    if (depth == mdepth) {
      free(pth);
      return inode;
    }
    free(inode);

    tok = strtok_r(NULL, "/", &sv);
  }

  free(pth);
  return inode;
}

struct inode *lookup_vfs(const char *path) {
  char         *sav;
  struct inode *ret = lookup_vfs_r(path, &sav);
  free(sav);
  return ret;
}

/* system call abstractions */
/* these syscals MUST NOT return NULL or zeroes for error. 0 is for ERR_OK (no errors at all!)*/
// as negatives are handled by the syscall interrupt handler (changes errno)

ssize_t fsys_read(int fd, void *buf, size_t count) {
  (void)(fd + buf + count);
  return -ENOSYS;
}

int fsys_open(const char *pathname, int flags) {
  (void)(pathname + flags);
  return -ENOSYS;
}

int fsys_close(int fd) {
  (void)fd;
  return -ENOSYS;
}

int fsys_mkdir(const char *path, mode_t mode) {
  (void)(path + mode);
  return -ENOSYS;
}

int fsys_unlink(const char *pathname) {
  (void)pathname;
  return -ENOSYS;
}

DIR *fsys_opendir(const char *path) {
  if (!path)
    return (DIR *)-EFAULT;

  struct inode *in = lookup_vfs(path);

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

const char *plist[] = {
    "---",
    "--x",
    "-w-",
    "-wx",
    "r--",
    "r-x",
    "rw-",
    "rwx"};

char *permget(uint16_t type, uint16_t perm) {
#define SIZ_s 3 * 3 + 1 + 1
  char *s = malloc(SIZ_s);
  memset(s, 0, SIZ_s);

  s[0] = type & INODE_DIR ? 'd' : '-';
  memcpy(s + 1, plist[(perm >> 6) & 7], 3);
  memcpy(s + 4, plist[(perm >> 3) & 7], 3);
  memcpy(s + 7, plist[(perm >> 0) & 7], 3);

  return s;
}

int lsdir(const char *path) {
  // printkf("cwd: %s\n", p_curproc->p_user->u_cdirname);

  DIR *const d = opendir(path);
  if (!d) {
    perror("ls: opendir");
    return 1;
  }

  for (int i = 0; i < d->count; i++) {
    char *p = permget(d[i].type, d[i].in->permission);
    printkf("%s ", p);
    free(p);

    d[i].size ? printkf("% 4d ", d[i].size) : printkf("     ");    
    printkf("%s\n", d[i].data);
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
