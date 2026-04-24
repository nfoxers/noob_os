#ifndef PROC_H
#define PROC_H

#include "cpu/idt.h"
#include "fs/vfs.h"
#include "lib/list.h"
#include "stdint.h"

#define P_SFREE    0x00
#define P_SSLEEP   0x01
#define P_SRUN     0x02
#define P_SRUNABLE 0x03
#define P_SSTOP    0x04
#define P_SZOMB    0x05

#define NOFILE  5
#define NGROUPS 5

#define MAX_CMTLEN 32

struct tty_struct;

struct cred {
  uid_t uid, euid, suid;
  gid_t gid, egid, sgid;

  gid_t groups[NGROUPS];
  int   ngroups;
};

struct user {
  char *name;

  char        *u_cdirname;
  struct inode u_cdir;

  struct cred  cred;
  struct file *u_ofile[NOFILE];
};

struct sigpending {
  struct list_head list;
  sigset_t signal;
};

struct signal_struct {
  struct sigpending shared_pending;
  struct tty_struct *tty;
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

  struct signal_struct *signal;
  // sighand *

  struct list_head p_runn;  // run node
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

#define UI_ALLOCATED 0x80

struct userinfo {
  char  username[MAX_CMTLEN];
  uid_t uid;
  gid_t gid;
  char  comment[MAX_CMTLEN];
  char  home[MAX_CMTLEN];
  char  interp[MAX_CMTLEN];

  uint8_t pass_req;
};

extern struct proc *volatile p_curproc;

void general_switch();
void schedule();

struct proc *alloc_proc(void (*f)(), uint16_t cs, void *args);

void login();
void init_root_proc();

const char *gethostname();

void spawn_proc(void (*f)(), uint16_t cs, void *args);
void exit_cur();


#endif