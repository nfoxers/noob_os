#ifndef SYSCALL_H
#define SYSCALL_H

#include "stddef.h"
#include "stdint.h"

#include "fs/vfs.h"

#define SYS_RSYS    0
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_WAITPID 7

#define SYS_YIELD    8 // no
#define SYS_CHDIR    9
#define SYS_MKDIR    10
#define SYS_UNLINK   11
#define SYS_OPENDIR  12
#define SYS_CLOSEDIR 13
#define SYS_PIPE     14
#define SYS_IOCTL    15
#define SYS_DUP      16
#define SYS_DUP2     17

void     syscall_init();
uint32_t syscall(uint32_t nr, ...);

// wrappers

long  restart_syscall(void);
void  exit(int status);
pid_t fork();

ssize_t read(unsigned int fd, void *buf, size_t count);
ssize_t write(unsigned int fd, const void *buf, size_t count);

int   open(const char *pathname, int flags);
int   close(int fd);
pid_t waitpid(pid_t pid, int *stat_addr, int options);

// ill-defined unstandard (yet) section

int sched_yield();
int chdir(const char *path);
int mkdir(const char *pathname, mode_t mode);
int unlink(const char *path);
int pipe(int fd[2]);
int ioctl(int fd, int op, void *argp);

// doesnt even exist in linux syscall table

DIR *opendir(const char *path);
int  closedir(struct inode *in, DIR *d);

#endif