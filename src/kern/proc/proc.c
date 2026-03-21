#include "proc/proc.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "mem/mem.h"
#include "video/printf.h"

#define NOPROC 10

extern uint8_t __text_start__;
extern uint8_t __text_end__;

struct proc procs[NOPROC];
struct user root;

struct proc *volatile cur;
struct proc *volatile prev;
struct proc *volatile next;

struct proc *run_head;

struct inode root_dir;

extern struct file root_fd[NOFILE];

extern uint32_t get_ef();

#define STACKSIZ 512 // this'll be enough

uint8_t used_stacks[NOPROC];

uint8_t alloc_esp(uint32_t *pointer) {
  int i = 0;
  for (; i < NOPROC; i++) {
    if (!used_stacks[i]) {
      used_stacks[i] = 1;
      *pointer       = (uint32_t)(i * (STACKSIZ + 1) + BSS_END + HEAP_SIZ);
      return i;
    }
  }
  return 0xff;
}

void dealloc_esp(uint8_t idx) {
  used_stacks[idx] = 0;
}

volatile struct proc *find_freeproc() {
  for (int i = 0; i < NOPROC; i++) {
    if (procs[i].p_stat == P_SFREE) {
      return &procs[i];
    }
  }
  return NULL;
}

void rq_add(struct proc *p) {
  if (!run_head) {
    run_head  = p;
    p->p_next = p;
    p->p_prev = p;
    return;
  }

  struct proc *tail = run_head->p_prev;
  tail->p_next      = p;
  p->p_prev         = tail;

  p->p_next        = run_head;
  run_head->p_prev = p;
}

void rq_remove(struct proc *p) {
  if (p->p_next == p) {
    run_head = NULL;
  } else {
    p->p_prev->p_next = p->p_next;
    p->p_next->p_prev = p->p_prev;

    if (run_head == p)
      run_head = p->p_next;
  }

  p->p_next = NULL;
  p->p_prev = NULL;
}

void free_proc(struct proc *p) {
  rq_remove(p);
  p->p_stat = P_SFREE;
}

void block(struct proc *p) {
  rq_remove(p);
  p->p_stat = P_SSLEEP;
}

void exit_cur() {
  next = cur->p_next;
  prev = cur;
  rq_remove(cur);
  cur->p_stat = P_SFREE;
  cur->p_next = next; // compatibility with int40
  general_switch();
}

struct proc *alloc_proc(void (*f)(), uint16_t cs) {
  for (int i = 0; i < NOPROC; i++) {
    if (procs[i].p_stat == P_SFREE) {
      procs[i].p_stat = P_SRUNABLE;

      procs[i].p_addr = (uint32_t)f;
      procs[i].p_pid  = i;
      procs[i].p_ppid = 0;
      procs[i].p_user = &root;

      uint32_t esp = 0x6210;
      alloc_esp(&esp);
      esp -= sizeof(struct regs);
      struct regs *r = (struct regs *)esp;
      r->ss          = (cs == CS_U) ? DS_U : DS_K;
      r->esp0        = esp;
      r->esp         = esp;
      r->cs          = cs;
      r->eip         = (uint32_t)f;
      r->eflags      = get_ef();

      procs[i].p_frame = (struct regs *)esp;

      return &procs[i];
    }
  }
  return NULL;
}

extern void c_switch3(volatile struct regs *volatile prev, volatile struct regs *volatile next);

void schedule_int(struct regs *cur_frame) {
  if (!run_head)
    return;

  if (!cur) {
    next = run_head;
    cur  = run_head;

    cur->p_frame = cur_frame;
  } else if (cur == cur->p_next)
    return;
  next = cur->p_next;
  prev = cur;
  cur  = next;

  prev->p_frame = cur_frame;
  pic_eoi();
  c_switch3(prev->p_frame, next->p_frame);
}

void general_switch() {
  int40();
}

void spawn_proc(void (*f)(), uint16_t cs) {
  CLI; // be advised this only works on kernel mode
  struct proc *p = alloc_proc(f, cs);
  rq_add(p);
  STI;
}

void init_root_proc() {
  struct proc *kproc = alloc_proc((void (*)())0x9400, CS_K);
  rq_add(kproc);
  cur = kproc;

  kproc->p_size = &__text_end__ - &__text_start__;

  printkf("kernel text size: %d B\n", procs[0].p_size);

  root.u_uid  = 0;
  root.u_rdir = &root_dir;
  root.u_cdir = &root_dir;
  root.u_gid  = 0;

  register_ex(schedule_int, 40);
}
