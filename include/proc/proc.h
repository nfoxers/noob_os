#ifndef PROC_H
#define PROC_H

#include "cpu/idt.h"
#include "lib/list.h"
#include "stdint.h"

#include "fs/vfs.h"

#define P_SFREE    0x00
#define P_SSLEEP   0x01
#define P_SRUN     0x02
#define P_SRUNABLE 0x03
#define P_SSTOP    0x04
#define P_SZOMB    0x05

#define NOFILE 5

struct user {
  uint16_t u_uid;
  uint16_t u_gid;

  // uint32_t u_arg[5];

  struct inode u_cdir;
  char        *u_cdirname;

  // struct inode *u_rdir;

  struct file *u_ofile[NOFILE];
};

struct context {
  uint32_t esp;
  uint32_t eip;
} __attribute__((packed));

struct proc {
  uint8_t p_stat;
  uint8_t p_stidx;
  uint8_t p_sig;

  uint16_t p_pid;
  uint16_t p_ppid;

  uint32_t p_addr;
  uint32_t p_size;

  struct list_head p_runn; // run node
  struct list_head p_waitn; // wait node

  volatile struct context con;
  volatile struct regs *volatile p_frame;

  struct user *p_user;
  void        *p_args;
};

struct wait_queue {
  struct list_head head;
};

struct run_queue {
  struct list_head head;
};

extern struct proc *volatile p_curproc;
extern struct inode root_dir;

void general_switch();
void schedule();

struct proc *alloc_proc(void (*f)(), uint16_t cs, void *args);

void init_root_proc();

void spawn_proc(void (*f)(), uint16_t cs, void *args);
void exit_cur();

#endif