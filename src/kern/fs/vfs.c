#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "video/printf.h"
#include <stddef.h>
#include <stdint.h>

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

struct inode *finddir_fs(struct inode *in, const char *name) {
  if ((in->type == INODE_DIR) && in->ops.lookup) {
    return in->ops.lookup(in, name);
  }
  return NULL;
}

#define MAX_PLEN 127

char *path_canon(const char *cwd, const char *path) {
  if (!cwd || !path)
    return NULL;

  size_t n   = strlen(path) + strlen(cwd) + 10;
  char  *tmp = malloc(n);
  char  *buf = malloc(n);

  if (*path == '/') {
    strncpy(tmp, path, MAX_PLEN - 1);
  } else {
    snprintkf(tmp, MAX_PLEN, "%s/%s", cwd, path);
  }
  tmp[MAX_PLEN - 1] = 0;

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

    if (len + seglen + 1 > n){
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

struct inode *kopen(const char *path) {
  char *cwd = p_curproc->p_user->u_cdirname;
  char *pth = path_canon(cwd, path);

  int mdepth = 0;
  int depth = 0; // skip the initial /
  for(size_t i = 0; i < strlen(pth); i++) {
    if(pth[i] == '/') mdepth++;
  }

  struct inode *inode = &root_dir;
  char *sv;

  char *tok = strtok_r(pth, "/", &sv);
  while (tok) {
    inode = finddir_fs(inode, tok);

    if (!inode) {
      free(pth);
      return NULL;
    }

    depth++;
    
    if(depth == mdepth) {
      free(pth);
      return inode;
    }
    free(inode);

    tok = strtok_r(NULL, "/", &sv);
  }

  free(pth);
  return inode;
}