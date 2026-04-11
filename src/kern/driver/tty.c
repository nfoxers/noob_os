#include "driver/tty.h"
#include "fs/vfs.h"
#include "mem/mem.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <fs/devfs.h>
#include <lib/errno.h>
#include <stddef.h>
#include <stdint.h>

int stdin;
int stdout;

int read_tty2(struct tty *tty, void *buf, size_t siz) {
  if (siz == 0)
    return 0;

  if (tty->readpos == tty->writepos)
    return -EAGAIN;
  size_t r = 0;
  while (siz--) {
    if (tty->readpos == tty->writepos) break;
    uint8_t c = tty->buf[tty->readpos++];

    *(uint8_t *)(buf++) = c;
    tty->readpos %= TTYBUFSIZ;
    r++;
  }
  return r;
}

int write_tty2(struct tty *tty, const void *buf, size_t siz) {
  if (siz == 0)
    return 0;

  // printkf("d: %x\n", *(uint8_t*)buf);

  if ((tty->writepos + 1) % TTYBUFSIZ == tty->readpos)
    return -EAGAIN;

  size_t w = 0;
  while (siz--) {
    if ((tty->writepos + 1) % PIPEBUFSIZ == tty->readpos) break;

    uint8_t c = *(uint8_t *)(buf++);

    if(tty->termios.c_iflag & ECHO)
      putchr(c);

    tty->buf[tty->writepos++] = c;
    tty->writepos %= PIPEBUFSIZ;
    w++;
  }
  return w;
}

ssize_t read_tty(struct device *d, void *buf, size_t siz) {
  struct tty *t = d->pdata;
  //printkf("t: %p\n", t);
  if (!t)
    return -EFAULT;
  return read_tty2(t, buf, siz);
}

int write_tty(struct device *d, const void *buf, size_t siz) {
  struct tty *t = d->pdata;
  if (!t)
    return -EFAULT;
  return write_tty2(t, buf, siz);
}

struct device tty_dev;

void tty_init() {
  struct tty *t = malloc(sizeof(struct tty));
  t->termios.c_iflag = 0;

  tty_dev.pdata = t;
  tty_dev.ops.read = read_tty;
  tty_dev.ops.write = write_tty;

  //tty_dev.ops.open = open_tty;

  register_dev(1, 0, &tty_dev);
  creat_devfs("tty", 1, 0);
  //printkf("stdin: %d\n", stdin);

  stdin = open("/dev/tty", O_RDONLY);

  if (stdin < 0) {
    perror("tty: open");
    return;
  }
  stdout = open("/dev/tty", O_WRONLY);
  if (stdout < 0) {
    close(stdin);
    perror("tty: open2");
    return;
  }


}