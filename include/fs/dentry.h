#ifndef DENTRY_H
#define DENTRY_H

#include <stdint.h>
#include <ams/bits/dirent.h>

struct dentry {
  struct inode  *in;
  struct dentry *parent;
  const char    *name;
  struct mount  *mnt;
};

#define DE_USED 0x0001

struct dir_entry {
  char          name[32];
  uint16_t      flg;
  struct inode *in;
};

struct dir_data {
  struct dir_entry entry[16];
  int              count;
};

// cognate to linux-dirent
struct nnux_dirent {
  unsigned long d_ino;
  unsigned long d_off;
  unsigned long d_reclen;
  char          pad;
  char          d_type;
  char          name[];
};

#endif