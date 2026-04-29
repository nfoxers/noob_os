#ifndef FS_H
#define FS_H

#include <ams/sys/types.h>
#include "cpu/spinlock.h"
#include "lib/list.h"
#include "stddef.h"
#include "stdint.h"

#include "ams/sys/stat.h"
#include "dev/block_dev.h"

#include <fs/fstype.h>
#include <ams/bits/fcntl-linux.h>

#define F_USED 0x1000

// inode flags
#define IN_DIRTY 0x0001
#define IN_MOUNT 0x0002

#define DT_MAXDIR 10

#define DIRENT_MAXSIZ 32
#define CWD_MAXSIZ    64

#define P_FREED    1

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define MKDEV(M, m) ((((M) & 0xfff) << 8) | (((m) & 0xff)) | (((m) & ~0xff) << 12))
#define MAJOR(d) (((d) >> 8) & 0xfff)
#define MINOR(d) (((d) & 0xff) | (((d) >> 12) & 0xfff00))

struct addrspace_ops;
struct nnux_dirent;
struct super_block;
struct file;
struct inode;
struct page;

struct inode_ops {
  ino_t (*lookup)(struct inode *parent, const char *name);
  int (*creat)(struct inode *dir, const char *name, uint16_t flg);
  int (*unlink)(struct inode *dir, const char *name);
};

struct file_ops {
  int (*open)(struct inode *in, struct file *file);
  int (*close)(struct file *file);

  ssize_t (*read)(struct file *file, void *buf, size_t siz);
  ssize_t (*write)(struct file *file, const void *buf, size_t siz);
  ssize_t (*readdir)(struct file *file, struct nnux_dirent *dirent, size_t count);

  off_t (*lseek)(struct file *file, off_t off, int whence);

  int (*ioctl)(struct file *file, int op, void *arg);
};

struct super_ops {
  void (*read_inode)(struct inode *);
  int (*write_inode)(int flags, struct inode *);
  void (*put_inode)(struct inode *);
  
  void (*read_super)(struct super_block *, void *data);
  void (*write_super)(struct super_block *);
  void (*put_super)(struct super_block *);

  void (*statfs)(struct super_block *, struct statfs *);
};

struct file_system_type {
  struct file_system_type *next;

  // todo: seperate fstype ops?
  struct inode *(*mount)(struct file_system_type *fs_type, const char *src, void *data, int flg);
  int (*read_super)(struct file_system_type *fs_type, struct super_block *sb);
  void (*put_super)(struct file_system_type *fs_type, struct super_block *sb);

  int fs_flags;
  const char *name;
};

struct super_block {
  uint32_t s_blocksize;
  uint16_t s_sbflags;
  uint32_t s_magic;

  dev_t s_dev;
  ino_t s_inext;

  uint32_t s_refs;

  struct inode *s_root;
  struct inode *s_mounted;

  struct super_ops *s_op;

  struct block_dev *s_bdev;
  struct file_system_type *s_type;
  void             *generic_sbp;
};

struct inode {
  mode_t mode;
  size_t size;

  unsigned short fsflags;
  unsigned int   iflags;

  ino_t ino;
  dev_t dev;
  dev_t rdev;

  uid_t uid;
  gid_t gid;

  blksize_t blksiz;
  blkcnt_t blkcount;

  unsigned short refs;

  struct super_block   *sb;
  struct inode_ops     *ops;
  struct file_ops      *fops;
  struct addrspace_ops *aps;
  struct mount         *mnt;

  struct hlist_node hnode;
  struct list_head  lru;

  void *pdata;
};

struct file {
  struct inode *inode;

  mode_t   mode;
  size_t   position;
  uint32_t refcont;
  uint32_t flags;

  void              *pdata;
  struct file_ops   *fops;
  const struct cred *cred;
};

char *path_canon(const char *cwd, const char *path);

/* useful stuff */

struct inode *lookup_vfs(const char *path);

// inode.c
struct inode *iget(struct super_block *sb, ino_t fsino);
void          iput(struct inode *in);
void          imount(struct inode *in, struct mount *mnt);

void purge_lru();
void print_caches();

/* syscalls */

ssize_t fsys_read(int fd, void *buf, size_t count);
ssize_t fsys_write(int fd, void *buf, size_t count);
int     fsys_open(const char *pathname, int flags);
int     fsys_close(int fd);
int     fsys_chdir(const char *path);
int     fsys_mkdir(const char *pathname, mode_t mode);
int     fsys_unlink(const char *pathname);
int     fsys_pipe(int fd[2]);
int     fsys_dup(int oldfd);
int     fsys_dup2(int oldfd, int newfd);
int     fsys_fstat(int fd, struct stat *statbuf);
int     fsys_getdents(int fd, struct nnux_dirent *dirp, size_t count);

void set_dev(struct super_block *b, struct inode *root);

/* library functions */
int lsdir(const char *path, int flg);
int nlsdir(const char *path, int flg);
int flstat(const char *name);
int findfreefd();

extern struct inode rootnode;

#endif