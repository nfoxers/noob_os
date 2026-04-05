#ifndef VFS_H
#define VFS_H

#include "stddef.h"
#include "stdint.h"

#define INODE_FILE     0x0001
#define INODE_DIR      0x0002
#define INODE_CHARDEV  0x0004
#define INODE_BLKDEV   0x0008
#define INODE_PIPE     0x0010
#define INODE_SYMLINK  0x0020
#define INODE_MNTPOINT 0x0040

#define F_RDONLY 0x0001
#define F_WRONLY 0x0002 // i'll just implement them later man
#define F_USED   0x0004
#define F_ADDR   0x0008

#define PRM_USR 0100
#define PRM_GRP 0010
#define PRM_OTH 0001

#define PRM_R 04
#define PRM_W 02
#define PRM_X 01

#define DIRENT_MAXSIZ 10
#define CWD_MAXSIZ    50

struct inode;
struct file;

typedef uint32_t (*readf_t)(struct inode *, uint32_t, uint32_t, uint8_t *);

struct inode_ops {
  struct inode *(*lookup)(struct inode *dir, const char *name);
  int (*creat)(struct inode *dir, const char *name, uint16_t flg);
  int (*unlink)(struct inode *dir, const char *name);
};

struct file_ops {
  int (*open)(struct inode *in, uint16_t flg);
  int (*read)(struct inode *in, void *buf, size_t off, size_t siz);
  int (*write)(struct inode *in, const void *buf, size_t off, size_t siz);
  int (*close)(struct inode *in);
};

struct inode {
  uint16_t size;
  uint16_t cluster0;
  uint16_t type;
  uint16_t permission;

  uint16_t uid;
  uint16_t gid;

  struct direntry *entaddr;
  struct inode_ops ops;
  struct file_ops  fops;
};

struct dentry {
  char          name[DIRENT_MAXSIZ];
  struct inode *in;
};

struct file {
  struct inode   *inode;
  uint16_t        position;
  uint16_t        flags;
};

char *path_canon(const char *cwd, const char *path);

struct inode *kopen(const char *path);

#endif