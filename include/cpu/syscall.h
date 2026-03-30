#ifndef SYSCALL_H
#define SYSCALL_H

#include "stdint.h"

#define SYS_RSYS    0
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_WAITPID 7

#define SYS_YIELD  8 // no
#define SYS_CHDIR  9
#define SYS_MKDIR  10
#define SYS_UNLINK 11

void syscall_init();

uint32_t syscall(uint32_t nr, ...);

#endif