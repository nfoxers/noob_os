#include "fs/vfs.h"
#include "stddef.h"
#include "syscall/syscall.h"

long restart_syscall(void) { // unimplemented
  return syscall(SYS_RSYS);
}

void exit(int status) {
  syscall(SYS_EXIT, status);
}

pid_t fork() {
  return syscall(SYS_FORK);
}

ssize_t read(unsigned int fd, void *buf, size_t count) {
  if (!count)
    return 0;
  return syscall(SYS_READ, fd, buf, count);
}

ssize_t write(unsigned int fd, const void *buf, size_t count) {
  if (!count)
    return 0;
  return syscall(SYS_WRITE, fd, buf, count);
}

int open(const char *pathname, int flags) {
  return syscall(SYS_OPEN, pathname, flags);
}

int close(int fd) {
  return syscall(SYS_CLOSE, fd);
}

pid_t waitpid(pid_t pid, int *stat_addr, int options) {
  return syscall(SYS_WAITPID, pid, stat_addr, options);
}

// ill-defined unstandard section

int sched_yield() { // 158
  return syscall(SYS_YIELD);
}

int chdir(const char *path) { // 12
  return syscall(SYS_CHDIR, path);
}

int mkdir(const char *pathname, mode_t mode) { // 39
  return syscall(SYS_MKDIR, pathname, mode);
}

int unlink(const char *path) { // 10
  return syscall(SYS_UNLINK, path);
}

DIR *opendir(const char *path) {
  DIR *ret = (DIR *)syscall(SYS_OPENDIR, path);
  if ((int)ret == -1)
    return NULL;
  return ret;
}

int closedir(struct inode *in, DIR *d) {
  return syscall(SYS_CLOSEDIR, in, d);
}

int pipe(int fd[2]) { // 42
  return syscall(SYS_PIPE, fd);
}

int ioctl(int fd, int op, void *argp) {
  return syscall(SYS_IOCTL, fd, op, argp);
}