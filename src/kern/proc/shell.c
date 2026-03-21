#include "cpu/idt.h"
#include "driver/fat12.h"
#include "driver/keyboard.h"
#include "driver/pci.h"
#include "driver/time.h"
#include "mem/mem.h"
#include "video/printf.h"
#include "video/video.h"

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

void shell() {
  static char buf[128];
  static char argv[8][16];

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

    if (!kstrcmp(buf, "help")) {
      printk("supported commands: exit ls rm touch clear time bf lspci\n");
    } else if (!kstrcmp(buf, "exit")) {
      printk("exiting...\n");
      break;
    } else if (!kstrcmp(buf, "ls")) {
      list_dir();
    } else if (!kstrcmp(buf, "rm")) {
      if (idx == 1) {
        printk("specify your file path!\n");
        continue;
      }
      sys_unlink(argv[1]);
    } else if (!kstrcmp(buf, "touch")) {
      if (idx == 1) {
        printk("specify your file path!\n");
        continue;
      }
      (void)sys_open(argv[1], O_CREAT);
    } else if (!kstrcmp(buf, "clear")) {
      clr_scr();
    } else if (!kstrcmp(buf, "time")) {
      uint32_t ms = 0;
      uint32_t s  = gettime(&ms);
      printkf("time elapsed since boot: %ds %dms\n", s, ms);
    }

    else if (!kstrcmp(buf, "bf")) {
      bf();
    }

    else if(!kstrcmp(buf, "lspci")) {
      pci_enumerate();
    }

    else {
      int fd = sys_open(argv[0], 0);
      if (!fd) {
        printkf("unrecognized command '%s'\n", argv[0]);
        continue;
      } // todo: load and run if not INODE_DIR
    }
  }
}