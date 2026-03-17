#include "proc/proc.h"
#include "video/printf.h"

#define NOPROC 10

extern uint8_t __text_start__;
extern uint8_t __text_end__;

struct proc procs[NOPROC];
struct user root;

struct task tasks[NOPROC];
struct task *cur;
struct task *next;

struct inode root_dir;

extern struct file root_fd[NOFILE];

void init_root_proc() {
  procs[0].p_addr = 0x9400;
  procs[0].p_stat = P_SRUN;
  procs[0].p_pid = 0;
  procs[0].p_ppid = 0;
  procs[0].p_pri = 0;
  procs[0].p_size = &__text_end__ - &__text_start__;

  cur = &tasks[0];
  cur->t_proc = &procs[0];
  cur->t_next = &tasks[0];

  printkf("kernel text size: %d\n", procs[0].p_size);

  root.u_uid = 0;
  root.u_rdir = &root_dir;
  root.u_cdir = &root_dir;
  root.u_gid = 0;
  root.u_ofile = &root_fd;
} 

struct task *schedule() {
  struct task *r = cur;
  for(;;) {
    r = r->t_next;
    if(r->t_proc->p_stat == P_SRUN) break;
  }
  return r;
}

extern uint32_t get_ef();
extern void c_switch(uint32_t *esp1, uint32_t *esp2);
extern void int40();

void yield() {
  next = schedule();
  if(next != cur) {
    //c_switch(&prev->p_saddr, &next->p_saddr);
    int40();
  }
}

void test_task() {
  printkf("hello from task2\n");
  yield();
  while(1);
}

void alloc_task(void (*f)()) {
  uint32_t *esp = (uint32_t*)0x6210; 
  // INSERT CS HERE!
  *(--esp) = get_ef(); // eflag
  *(--esp) = 0x08; // cs
  *(--esp) = (uint32_t)test_task;

  *(--esp) = 0; // eax
  *(--esp) = 0; // ecx
  *(--esp) = 0; // edx
  *(--esp) = 0; // ebx
  *(--esp) = 0; // esp (IGNORED!)
  *(--esp) = 0x6210; // ebp
  *(--esp) = 0; // esi
  *(--esp) = 0; // edi
  
  // regs hierr
  procs[1].p_next = 0;
  procs[1].p_stat = P_SRUN;
  procs[1].p_saddr = (uint32_t)esp;
  tasks[0].t_next = &tasks[1];
  tasks[1].t_next = &tasks[0];
  tasks[1].t_esp = (uint32_t)esp;
  tasks[1].t_proc = &procs[1];
}

