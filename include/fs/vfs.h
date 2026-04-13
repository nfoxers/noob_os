#ifndef VFS_H
#define VFS_H

#include "cpu/spinlock.h"
#include "stddef.h"
#include "stdint.h"
#include <asm/sys/types.h>

#include "asm/sys/stat.h"

#define F_USED 0x1000
#define F_ADDR 0x0008

#define O_RDONLY               0x0001
#define O_WRONLY               0x0002
#define O_RDWR                 0x0004
#define O_CREAT                0x0008
#define O_JUSTGIVEMETHEADDRESS 0x0010
/*
#define INODE_FILE     S_IFREG
#define INODE_DIR      S_IFDIR
#define INODE_CHARDEV  S_IFCHR
#define INODE_BLKDEV   S_IFBLK
#define INODE_PIPE     0
#define INODE_SYMLINK  0x0020
#define INODE_MNTPOINT 0x0040
*/

#define DT_MAXDIR 10

#define DIRENT_MAXSIZ 32
#define CWD_MAXSIZ    64

#define PIPEBUFSIZ 128
#define P_FREED    1

struct inode;
struct file;

// typedef uint32_t off_t;
typedef int      pid_t;
typedef int      ssize_t;
typedef uint32_t mode_t;

typedef struct dirstream {
  uint8_t count;
  mode_t  type;
  size_t  size;

  struct inode *in;

  char data[DIRENT_MAXSIZ];
} DIR;

struct super_block;
struct inode;

struct statfs {
};

// extra comments so i dont waste dynamic memory
struct inode_ops {
  // *must a dynamic inode *
  struct inode *(*lookup)(struct inode *dir, const char *name);
  int (*creat)(struct inode *dir, const char *name, uint16_t flg);
  int (*unlink)(struct inode *dir, const char *name);

  // *must return a dynamic DIR *
  DIR *(*opendir)(struct inode *dir);
  int (*closedir)(struct inode *dir, DIR *d);
};

struct file_ops {
  int (*open)(struct inode *in, struct file *file);
  int (*close)(struct file *file);

  ssize_t (*read)(struct file *file, void *buf, size_t siz);
  ssize_t (*write)(struct file *file, const void *buf, size_t siz);
  off_t (*lseek)(struct file *file, off_t off, int whence);

  int (*ioctl)(struct file *file, int op, void *arg);
};

struct super_ops {
  void (*read_inode)(struct inode *);
  int (*write_inode)(int flags, struct inode *);
  void (*put_inode)(struct inode *);
  void (*put_super)(struct super_block *);
  void (*write_super)(struct super_block *);
  void (*statfs)(struct super_block *, struct statfs *);
};

struct pipe {
  uint8_t  buf[PIPEBUFSIZ];
  uint16_t flg;
  size_t   readpos;
  size_t   writepos;

  spinlock_t lock;
};

struct dentry;

struct mount {
  struct super_block *sb;

  struct dentry *mntpoint;
  struct dentry *root;
  struct mount  *parent;
  int            ref;
};

struct dentry {
  struct inode  *in;
  struct dentry *parent;
  const char    *name;
  struct mount  *mnt;
};

struct super_block {
  uint32_t s_blocksize;
  uint16_t s_sbflags;
  uint32_t s_magic;

  struct inode     *s_mounted;
  struct super_ops *s_op;

  void *generic_sbp;
};

struct inode {
  mode_t   mode;
  uint16_t fsflags;
  uint32_t iflags;
  size_t   size;
  uint32_t ino;

  uint16_t uid;
  uint16_t gid;

  struct super_block *sb;
  struct direntry    *entaddr;
  struct inode_ops   *ops;
  struct file_ops    *fops;

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

char *path_canon(const char *cwd, const char *path);

struct inode *lookup_vfs(const char *path);

/* syscalls */

ssize_t fsys_read(int fd, void *buf, size_t count);
ssize_t fsys_write(int fd, void *buf, size_t count);
DIR    *fsys_opendir(const char *path);
int     fsys_closedir(struct inode *in, DIR *d);
int     fsys_open(const char *pathname, int flags);
int     fsys_close(int fd);
int     fsys_chdir(const char *path);
int     fsys_mkdir(const char *pathname, mode_t mode);
int     fsys_unlink(const char *pathname);
int     fsys_pipe(int fd[2]);
int     fsys_dup(int oldfd);
int     fsys_dup2(int oldfd, int newfd);

void init_rootfs();

/* library functions */
int lsdir(const char *path, int flg);
int findfreefd();

#endif