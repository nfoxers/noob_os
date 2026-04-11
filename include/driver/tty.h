#ifndef TTY_H
#define TTY_H

#include "stddef.h"
#include "stdint.h"

#define TTYBUFSIZ 128

#include "asm2/termbits.h"

struct tty {
  char buf[TTYBUFSIZ];
  int  readpos;
  int  writepos;

  struct termios termios;
};

extern int stdin;
extern int stdout;

void tty_init();

#endif