#ifndef PROC_H
#define PROC_H

#include "stdint.h"

#define FAT_RDONLY 0x01
#define FAT_HIDDEN 0x02
#define FAT_SYSTEM 0x04
#define FAT_VOLLAB 0x08
#define FAT_SUBDIR 0x10
#define FAT_ARCHIV 0x20
#define FAT_DEVICE 0x40
#define FAT_RESERV 0x80

struct direntry {
  char fname[8];
  char ext[3];
  uint8_t fatt;
  uint8_t reserved;
  uint8_t create_cs;
  uint16_t create_time;
  uint16_t create_date;
  uint16_t la_date;
  uint16_t high_cluster;
  uint16_t lm_time;
  uint16_t lm_date;
  uint16_t low_cluster;
  uint32_t size;
} __attribute__((packed));

#define INODE_FILE 0x0001
#define INODE_DIR  0x0002
#define INODE_STDSTREAM 0x0003

#define F_RDONLY 0x0001
#define F_WRONLY 0x0002 // i'll just implement them later man
#define F_USED 0x0004

struct inode {
  uint16_t size;
  uint16_t cluster0;
  struct direntry *entaddr;
  uint16_t type;
};

struct file {
  struct inode *inode;
  uint16_t position;
  uint16_t flags;
};

#define P_SSLEEP 0x01
#define P_SRUN 0x02
#define P_SSTOP 0x03
#define P_SZOMB 0x04

#define NOFILE 10

struct proc {
  uint8_t p_stat;
  uint8_t p_next;
  uint8_t p_pri;
  uint8_t p_sig;

  uint16_t p_pid;
  uint16_t p_ppid;

  uint32_t p_addr;
  uint32_t p_saddr; // basically context-swtitchh esp
  uint32_t p_size;
  uint32_t p_wchan;
};

struct task {
  struct proc *t_proc;
  struct task *t_next;
  uint32_t t_esp;
} __attribute__((packed));

struct user {
  uint16_t u_uid;
  uint16_t u_gid;

  uint16_t u_err;
  uint32_t u_arg[5];
  uint32_t u_tsize;
  uint32_t u_dsize;
  uint32_t u_ssize;

  struct inode *u_cdir;
  struct inode *u_rdir;

  struct file (*u_ofile)[NOFILE];
};

void init_root_proc();
void yield();
void alloc_task(void (*f)());

#endif