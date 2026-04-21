#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/irq.h"
#include "driver/keyboard.h"
#include "driver/pci.h"
#include "driver/time.h"
#include "fs/fat12.h"
#include "fs/vfs.h"
#include "kassert.h"
#include "mem/mem.h"
#include "mem/paging.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <lib/errno.h>

#define ARGS_USELESS \
  (void)argv;        \
  (void)argc // supress clang's -Wunused-argument

#define NARGS 8

extern void enditall();

extern int stdin;
extern int stdout;

void bf() {
#define BF_BUFSIZ 128
  uint8_t dbuf[BF_BUFSIZ]; // data buffer
  uint8_t cbuf[BF_BUFSIZ]; // command buffer

  memset(dbuf, 0, BF_BUFSIZ);
  memset(cbuf, 0, BF_BUFSIZ);

  printk("enter yo brainfuh program:\n");

  // step ein: copy
  char     c;
  uint8_t *cbp = cbuf;
  while ((c = getch()) != EOF && (cbp - cbuf) <= BF_BUFSIZ) {
    putchr(c);
    *cbp++ = c;

    if (c == '\b') {
      *--cbp = 0;
      *--cbp = 0;
    }
  }
  uint16_t len = cbp - cbuf;
  putchr('\n');

  // step zwei: run :3
  uint8_t dp = 0;
  for (int i = 0; i < len; i++) {
    if (dp >= BF_BUFSIZ) {
      printk("data buffer overflow!\n");
      break;
    }
    switch (cbuf[i]) {
    case '>':
      dp++;
      break;
    case '<':
      dp--;
      break;
    case '+':
      dbuf[dp]++;
      break;
    case '-':
      dbuf[dp]--;
      break;
    case '.':
      putchr(dbuf[dp]);
      break;
    case ',':
      dbuf[dp] = getch();
      break;
    case '[':
      if (dbuf[dp] == 0) {
        int depth = 1;
        while (++i < BF_BUFSIZ && depth > 0) {
          if (cbuf[i] == '[')
            depth++;
          else if (cbuf[i] == ']')
            depth--;
        }
      }
      break;

    case ']':
      if (dbuf[dp] != 0) {
        int depth = 1;
        while (--i >= 0 && depth > 0) {
          if (cbuf[i] == ']')
            depth++;
          else if (cbuf[i] == '[')
            depth--;
        }
        i++;
      }
      break;
    default:
      break;
    }
  }

  putchr('\n');
}

int c_exit(char **argv, int argc) {
  ARGS_USELESS;

  return 0; // this line is useless
}

int c_ls(char **argv, int argc) {
  if (argc == 1) {
    return lsdir(".", 0);
  }

  char *pth = argv[1];
  if (*pth == '-') {
    if (*(pth + 1) == 'l') {
      if (argc == 2) {
        return lsdir(".", 3);
      } else {
        pth = argv[2];
        return lsdir(pth, 3);
      }

    } else
      return 2;
  }

  return lsdir(pth, 0);
}

int c_rm(char **argv, int argc) {
  if (argc == 1) {
    printk("specify thy path\n");
    return 2;
  }

  if (unlink(argv[1]))
    perror("rm");
  return 0;
}

int c_touch(char **argv, int argc) {
  if (argc == 1) {
    printk("specify thy path\n");
    return 2;
  }

  int fd = open(argv[1], O_CREAT);
  if (fd == -1) {
    perror("touch");
    return 1;
  }
  close(fd);
  return 0;
}

int c_clear(char **argv, int argc) {
  ARGS_USELESS;
  clr_scr();
  return 0;
}

int c_time(char **argv, int argc) {
  ARGS_USELESS;
  uint32_t ms = 0;
  uint32_t s  = gettime(&ms);
  printkf("time elapsed since boot: %ds %dms\n", s, ms);
  return 0;
}

int c_bf(char **argv, int argc) {
  ARGS_USELESS; // todo: add file options!
  bf();
  return 0;
}

int c_lspci(char **argv, int argc) {
  ARGS_USELESS;
  pci_enumerate();
  return 0;
}

int c_mkdir(char **argv, int argc) {
  if (argc == 1) {
    printk("specify thy path\n");
    return 2;
  }

  // sys_mkdir(argv[1]);
  int fd = mkdir(argv[1], 0);
  if (fd == -1) {
    perror("mkdir");
    return 1;
  }
  if (close(fd) == -1) {
    perror("close");
    return 1;
  }
  return 0;
}

int c_cd(char **argv, int argc) {
  if (argc == 1)
    return 0;
  if (chdir(argv[1])) {
    perror("cd");
    return 1;
  }
  return 0;
}

int printfile(const char *file) {
  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    perror("cat: open");
    return 1;
  }

  int r = 0;
  int c = 0;
  int count = 1;
  while((r = read(fd, &c, count)) == count) {
    if(write(stdout, &c, r) == -1) perror("write");
  }

  close(fd);
  return 0;
}

int c_cat(char **argv, int argc) {
  if (argc == 1) {
    printkf("specify your file please good sir\n");
    return 2;
  }

  return printfile(argv[1]);
}

int c_mall(char **argv, int argc) {
  ARGS_USELESS;
  void *a = malloc(1);
  printkf("addr: %p\n", a);
  free(a);
  return 0;
}

int c_h(char **argv, int argc) {
  ARGS_USELESS;
  printkf("this will erase the content of cr0 and kill the kernel. continue? [n] ");
  char c = getch();
  putchr('\n');
  if (c != 'y')
    return 2;

  asm volatile(
      "mov %%cr0, %%eax\n"
      "mov $0x86763726, %%eax\n"
      "mov %%eax, %%cr0\n" ::: "eax");
  return 0;
}

int c_mused(char **argv, int argc) {
  ARGS_USELESS;
  printkf("used dynamic memory: %dB\n", getused());
  printkf("used dynamic memory max: %dB\n", getmaxused());
  printkf("used dynamic memory eff: %dB\n", gettrueused());
  return 0;
}

int c_icach(char **argv, int argc) {
  ARGS_USELESS;
  print_caches();
  return 0;
}

int c_ipurge(char **argv, int argc) {
  ARGS_USELESS;
  purge_lru();
  return 0;
}

int c_lsirq(char **argv, int argc) {
  ARGS_USELESS;
  show_interrupts();
  return 0;
}

static char const *const cmds[] = {
    "help", "exit", "ls", "rm", "touch", "clear", "time", "bf", "lspci", "mkdir", "cd",
    "cat", "mall", "h", "mused", "icach", "ipurge", "lsirq"};

int c_help(char **argv, int argc) {
  ARGS_USELESS;
  printk("supported commands: ");

  for (uint32_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
    printkf("%s ", cmds[i]);
  }

  putchr('\n');
  return 0;
}

static int (*const ftab[])(char *argv[NARGS], int argc) = {
    c_help, c_exit, c_ls, c_rm, c_touch, c_clear, c_time, c_bf, c_lspci, c_mkdir, c_cd,
    c_cat, c_mall, c_h, c_mused, c_icach, c_ipurge, c_lsirq};

void shell() {
  printkf("note: creds are root/toor or user w no pass\n");
  login();
  printk("\e\x07");
  printfile("/etc/motd");

  const char *hostname = gethostname();

  static char buf[128];

  kassert(sizeof(ftab) / sizeof(ftab[0]) != sizeof(cmds) / sizeof(cmds[0]));

  STI; // enable the keyboard an dstuff
  while (1) {
    printkf("\e\x02%s@%s\e\x07:\e\x03%s\e\x07$ ", p_curproc->p_user->name, hostname, p_curproc->p_user->u_cdirname);
    int rd = kgets(buf, sizeof buf);
    // putchr('\n');

    if (rd == 0) {
      continue;
    }

    char   *sv;
    char   *tok = strtok_r(buf, " ", &sv);
    uint8_t idx = 0;
    char   *argv[NARGS];
    while (tok) {
      argv[idx] = tok;
      tok       = strtok_r(NULL, " ", &sv);
      idx++;
      if (idx >= NARGS)
        break;
    }

    int found = 0;
    int ret   = 0;
    for (uint32_t i = 0; i < sizeof(ftab) / sizeof(ftab[0]); i++) {
      if (!strcmp(buf, cmds[i])) {
        found++;
        ret = ftab[i](argv, idx);
        break;
      }
    }
    if (!found) {
      printkf("%s: command not found\n", argv[0]);
    }
    if (ret) {
      printkf("nonzero return: %d\n", ret);
    }
  }
}