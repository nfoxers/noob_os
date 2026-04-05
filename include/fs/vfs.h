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

#define DT_BLK  0x01 // block
#define DT_CHR  0x02 // char
#define DT_DIR  0x04 // directory
#define DT_FIFO 0x08 // named fifo
#define DT_LNK  0x10 // symlink
#define DT_REG  0x20 // regular file
#define DT_SOCK 0x40 // unix sock
#define DT_UNK  0x80 // unknown

#define DT_MAXDIR 10

#define DIRENT_MAXSIZ 32
#define CWD_MAXSIZ    50

struct inode;
struct file;

typedef uint32_t off_t;

typedef struct dirstream {
  uint8_t count;
  size_t  size;
  uint8_t type;

  char data[DIRENT_MAXSIZ];
} DIR;

struct dirent {
  off_t    d_off;
  uint16_t d_reclen;
  uint8_t  d_type;

  char d_name[DIRENT_MAXSIZ];
};

struct inode_ops {
  struct inode *(*lookup)(struct inode *dir, const char *name);
  int (*creat)(struct inode *dir, const char *name, uint16_t flg);
  int (*unlink)(struct inode *dir, const char *name);

  DIR *(*opendir)(struct inode *dir);
  int (*closedir)(DIR *dir);
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

struct file {
  struct inode *inode;
  uint16_t      position;
  uint16_t      flags;
};

char *path_canon(const char *cwd, const char *path);

struct inode *lookup_vfs(const char *path);

DIR *opendir_ffs(const char *path);

#endif