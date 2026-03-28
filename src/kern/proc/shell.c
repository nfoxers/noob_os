#include "cpu/idt.h"
#include "driver/fat12.h"
#include "driver/keyboard.h"
#include "driver/pci.h"
#include "driver/time.h"
#include "kassert.h"
#include "mem/mem.h"
#include "video/printf.h"
#include "video/video.h"

#define ARGS_USELESS \
  (void)argv;        \
  (void)argc // supress clang's -Wunused-argument

#define NARGS 8

extern void enditall();

void bf() {
#define BF_BUFSIZ 128
  uint8_t dbuf[BF_BUFSIZ]; // data buffer
  uint8_t cbuf[BF_BUFSIZ]; // command buffer

  kmemset(dbuf, 0, BF_BUFSIZ);
  kmemset(cbuf, 0, BF_BUFSIZ);

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

int c_help(char **argv, int argc) {
  ARGS_USELESS;
  printk("supported commands: exit ls rm touch clear time bf lspci mkdir cd\n");
  return 0;
}

int c_exit(char **argv, int argc) {
  ARGS_USELESS;
  enditall();
  return 0; // this line is useless
}

int c_ls(char **argv, int argc) {
  if (argc == 1) {
    list_dir("");
    return 0;
  }
  list_dir(argv[1]);
  return 0;
}

int c_rm(char **argv, int argc) {
  if (argc == 1) {
    printk("specify thy path\n");
    return 2;
  }
  sys_unlink(argv[1]);
  return 0;
}

int c_touch(char **argv, int argc) {
  if (argc == 1) {
    printk("specify thy path\n");
    return 2;
  }
  int fd = sys_open(argv[1], O_CREAT);
  sys_close(fd);
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

  sys_mkdir(argv[1]);
  return 0;
}

int c_cd(char **argv, int argc) {
  if (argc == 1)
    return 0;
  sys_cd(argv[1]);
  return 0;
}

static int (*const ftab[])(char *argv[NARGS], int argc) = {
    c_help, c_exit, c_ls, c_rm, c_touch, c_clear, c_time, c_bf, c_lspci, c_mkdir, c_cd};

static char const *const cmds[] = {
    "help", "exit", "ls", "rm", "touch", "clear", "time", "bf", "lspci", "mkdir", "cd"};

void shell() {
  static char buf[128];
  static char argv[NARGS][16];

  kassert(sizeof(ftab) / sizeof(ftab[0]) != sizeof(cmds) / sizeof(cmds[0]));

  STI; // enable the keyboard an dstuff
  while (1) {
    printk("input> ");
    int rd = kgets(buf, sizeof buf);
    putchr('\n');

    if (rd == 0) {
      continue;
    }

    char   *tok = kstrtok(buf, " ");
    uint8_t idx = 0;
    while (tok) {
      kstrncpy(argv[idx++], tok, 16);
      tok = kstrtok(NULL, " ");
    }

    char *tmp[NARGS];
    for(int i = 0; i < idx; i++) {
      tmp[i] = argv[i];
    }

    int found = 0;
    for (uint32_t i = 0; i < sizeof(ftab) / sizeof(ftab[0]); i++) {
      if (!kstrcmp(buf, cmds[i])) {
        found++;
        ftab[i](tmp, idx);
      }
    }
    if(!found) {
      printkf("%s: command not found\n", argv[0]);
    }
  }
}