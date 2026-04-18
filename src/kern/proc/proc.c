#include "proc/proc.h"
#include "ams/termbits.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/spinlock.h"
#include "driver/keyboard.h"
#include "driver/tty.h"
#include "fs/vfs.h"
#include "lib/list.h"
#include "mem/mem.h"
#include "video/printf.h"
#include "syscall/syscall.h"
#include <lib/errno.h>
#include "crypt/crypt.h"
#include "video/video.h"

#define NOPROC 10

extern uint8_t __text_start__;
extern uint8_t __text_end__;

struct proc procs[NOPROC];
struct user root;

struct proc *volatile p_curproc;

struct run_queue rq;

#define MAXUSERLEN 32

#define STACKSIZ 512 // this'll be enough

uint8_t used_stacks[NOPROC];

static uint8_t alloc_esp(uint32_t *pointer) {
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

static void dealloc_esp(uint8_t idx) {
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

void enqueque_runqueue(struct run_queue *rq, struct proc *p) {
  list_add_tail(&p->p_runn, &rq->head);
}

void dequeque_runqueue(struct proc *p) {
  list_del(&p->p_runn);
}

struct proc *proc_next(struct run_queue *rq) {
  struct list_head *node = rq->head.next;
  return container_of(node, struct proc, p_runn);
}

void sleep_on(struct wait_queue *q, spinlock_t *lock) {
  struct proc *p = p_curproc;

  dequeque_runqueue(p);
  p->p_stat = P_SSLEEP;
  list_add_tail(&p->p_waitn, &q->head);

  spin_unlock(lock);
  sched_yield();
  spin_lock(lock);
}

void wake_up(struct wait_queue *q) { // todo: wake all
  struct list_head *n = q->head.next;
  struct proc *p = container_of(n, struct proc, p_waitn);
  list_del(&p->p_waitn);
  p->p_stat = P_SRUN;
  enqueque_runqueue(&rq, p);
}

void free_proc(struct proc *p) {
  dequeque_runqueue(p);
  p->p_stat = P_SFREE;
}

void block(struct proc *p) {
  dequeque_runqueue(p);
  p->p_stat = P_SSLEEP;
}

void exit_cur() {
  dequeque_runqueue(p_curproc); // this removes p_curproc->p_next

  p_curproc->p_stat = P_SFREE;
  p_curproc->p_runn.next = NULL;

  dealloc_esp(p_curproc->p_stidx);
  sched_yield();
}

#define EF_B1 0x02
#define EF_IF 0x200

extern void childret();

struct proc *alloc_proc(void (*f)(), uint16_t cs, void *args) {
  // todo: args!
  for (int i = 0; i < NOPROC; i++) {
    if (procs[i].p_stat == P_SFREE) {
      procs[i].p_stat = P_SRUNABLE;

      procs[i].p_addr = (uint32_t)f;
      procs[i].p_pid  = i;
      procs[i].p_ppid = 0;
      procs[i].p_user = &root;
      procs[i].p_args = args;

      uint32_t esp = 0x6210;
      procs[i].p_stidx = alloc_esp(&esp);
      esp -= sizeof(struct regs);
      struct regs *r = (struct regs *)esp;

      r->ss     = (cs == CS_U) ? DS_U : DS_K;
      r->ds     = r->ss;
      r->es     = r->ss;
      r->fs     = r->ss;
      r->gs     = r->ss;
      r->esp0   = esp;
      r->esp    = esp;
      r->cs     = cs;
      r->eip    = (uint32_t)f;
      r->eflags = EF_IF | EF_B1;

      procs[i].p_frame = (struct regs *)esp;
      procs[i].con.esp = esp;
      procs[i].con.eip = (uint32_t)childret;

      return &procs[i];
    }
  }
  return NULL;
}

struct proc tmp_proc;
static void task_switch(struct proc *next) {
  struct proc *prv;
  prv = (p_curproc == next) ? &tmp_proc : p_curproc;
  p_curproc = next;

  asm volatile(
      "pushfl\n"
      "pushl %%ebp\n"
      "movl %%esp, %[spprev]\n"
      "movl $1f, %[ipprev]\n"
      "movl %[spnext], %%esp\n"
      "pushl %[ipnext]\n"
      "ret\n"
      "1:\n"
      "popl %%ebp\n"
      "popfl\n"
      : [spprev] "=m"(prv->con.esp),
        [ipprev] "=m"(prv->con.eip)
      : [spnext] "m"(next->con.esp),
        [ipnext] "m"(next->con.eip)
      : "memory");
}

void schedule() {
  struct proc *prev = p_curproc;
  struct proc *next = proc_next(&rq);

  if(next == prev) return;

  if(prev->p_stat == P_SRUN) {
    enqueque_runqueue(&rq, prev);
  }
  dequeque_runqueue(next);

  task_switch(next);
}

void add_proc(struct proc *p) {
  init_list(&p->p_runn);
  init_list(&p->p_waitn);
  
  p->p_stat = P_SRUN;

  enqueque_runqueue(&rq, p);
}

void spawn_proc(void (*f)(), uint16_t cs, void *args) {
  CLI; // be advised this only works on kernel mode
  struct proc *p = alloc_proc(f, cs, args);
  add_proc(p);
  STI;
}

const char *gethostname() {
  static char buf[32];
  static char evoked = 0;

  if(evoked) return buf;

  int tmp = open("/etc/hostname", O_RDONLY);
  if(tmp == -1) {
    perror("open");
    return NULL;
  }
  read(tmp, buf, 32);
  for(int i = 0; i < 32; i++) {
    if(buf[i] == '\n') buf[i] = 0;
  }

  close(tmp);
  evoked++;
  return buf;
}

void login() {
  int nologin = 0;

  if(nologin) {
    strcpy(root.name, "root");
    strcpy(root.u_cdirname, "/home");
    root.cred.euid = 0;
    root.cred.egid = 0;
    goto done;
  } else {
    printkf("\n");

    loop:;
    char buf[MAXUSERLEN];
    char psw[MAXUSERLEN];

    struct userinfo *i = malloc(sizeof(*i));
    
    const char *hostname = gethostname();
    printkf("%s login: ", hostname);
    kgets(buf, MAXUSERLEN);

    int r = getuser(buf, 0, i);
    if(r && !(i->pass_req & 1)) goto yes;

    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~(ECHO);
    tcsetattr(0, TCSANOW, &t);

    printkf("password for %s: ", buf);
    kgets(psw, MAXUSERLEN);

    t.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &t);

    r = chkcred(buf, psw, i);
    if(r == 0) {
      printkf("\nlogin incorrect\n");
      free(i);
      goto loop;
    }
    
    yes:
    strcpy(root.name, i->username);

    root.cred.euid = i->uid;
    root.cred.egid = i->gid;

    free(i);

    printk("\n\n");

    if(chdir(i->home) == -1) {
      perror("home directory");
      chdir("/");
    }
  }

  done:
  //printkf("\nlogged as %s\n\n", root.name);
  return;
}

void init_root_proc() {
  init_list(&rq.head);

  struct proc *kproc = alloc_proc((void (*)())0x9400, CS_K, NULL);
  add_proc(kproc);
  p_curproc = kproc;

  kproc->p_size = &__text_end__ - &__text_start__;

  printkf("kernel text size: %d B\n", procs[0].p_size);

  // todo: properly initialize everything
  root.cred.egid  = 0;
  root.cred.euid  = 0;
  root.u_cdirname = malloc(CWD_MAXSIZ);
  root.name = malloc(MAXUSERLEN);
  strcpy(root.u_cdirname, "/");
  // cdir will handled by fs

  //register_ex(sys_yield, SYS_INTNO);
}
