#include "driver/tty.h"
#include "asm/sys/ioctl.h"
#include "asm/termbits.h"
#include "cpu/spinlock.h"
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

const static cc_t	ttydefchars[NCCS] = {
  CINTR, CQUIT, CERASE, CKILL, CEOF, CTIME, CMIN, 0, CSTART, CSTOP, CSUSP, CEOL,
  CREPRINT, CDISCARD, CERASE, CLNEXT, CEOL
};

void update_tty(struct device *d) {
  (void)d;
}

uint8_t canon_pop(struct tty *t) {
  uint8_t c = t->canonbuf[t->canonr++];
  t->canonr %= TTYBUFSIZ;
  return c;
}

void canon_push(struct tty *t, uint8_t c) {
  t->canonbuf[t->canonw++] = c;
  t->canonw %= TTYBUFSIZ;
}

int canon_empty(struct tty *t) {
  return t->canonr == t->canonw;
}

int canon_full(struct tty *t) {
  return (t->canonw + 1) % TTYBUFSIZ == t->canonr;
}

uint8_t out_pop(struct tty *t) {
  uint8_t c = t->outbuf[t->outr++];
  t->outr %= TTYBUFSIZ;
  return c;
}

void out_push(struct tty *t, uint8_t c) {
  t->outbuf[t->outw++] = c;
  t->outw %= TTYBUFSIZ;
}

int out_empty(struct tty *t) {
  return t->outr == t->outw;
}

int out_full(struct tty *t) {
  return (t->outw + 1) % TTYBUFSIZ == t->outr;
}

ssize_t ttydev_write(struct tty *t, const char *b, size_t c) {
  if(t && t->ops && t->ops->write) {
    return t->ops->write(t, b, c);
  }
  printkf("no dev\n");
  return 0;
}

void tty_outputc(struct tty *t, char c) {
  out_push(t, c);
  
  if((t->termios.c_oflag & OPOST) && (t->termios.c_oflag & ONLCR) && c == '\n') {
    ttydev_write(t, "\r", 1);
  }

  ttydev_write(t, &c, 1);
}

void canon_tty(struct tty *tty, char c) {
  struct termios *t = &tty->termios;
  
  if(c == t->c_cc[VERASE]) {
    if(tty->line_len > 0) {
      tty->line_len--;

      if(t->c_lflag & ECHO) {
        tty_outputc(tty, '\b');
        tty_outputc(tty, ' ');
        tty_outputc(tty, '\b');
      }
    }      
    return ;
  }

  if(c == t->c_cc[VEOF]) {
    // todo : wakeup readers
    return;
  }

  tty->line_buf[tty->line_len++] = c;

  if(c == '\n') {
    for(size_t i = 0; i < tty->line_len; i++) canon_push(tty, tty->line_buf[i]);

    tty->line_len = 0;

    // todo : wakeup readers
  }
}

void tty_inputc(struct tty *t, char c) {
  //printkf("yes: %c\n", c);
  struct termios *tm = &t->termios;

  if((tm->c_iflag & ICRNL) && c == '\r') c = '\n';

  if((tm->c_lflag & ISIG) && c == tm->c_cc[VINTR]) {
    // todo : send kill()
    return;
  }

  if(tm->c_lflag & ICANON) {
    canon_tty(t, c);
  } else {
    canon_push(t, c);
    // todo : wakeup readers
  }

  if(tm->c_lflag & ECHO) {
    tty_outputc(t, c);
  }
}

/* things */

ssize_t read_tty(struct device *d, void *buf, size_t siz) {
  struct tty *tty = d->pdata;
  if (!tty)
    return -EFAULT;

  while(canon_empty(tty)) {
    return -EAGAIN;
  }

  int r = 0;

  while((uint32_t)r < siz && !canon_empty(tty)) {
    ((uint8_t*)buf)[r++] = canon_pop(tty);
    if(tty->termios.c_lflag & ICANON)
      if(((uint8_t*)buf)[r-1] == '\n') break;
  }

  return r;
}

int write_tty(struct device *d, const void *buf, size_t siz) {
  struct tty *tty = d->pdata;
  if (!tty)
    return -EFAULT;

  size_t w = 0;

  while(w < siz) {
    if(out_full(tty)) {
      return w ? w : -EAGAIN;
    }
    tty_outputc(tty, ((uint8_t *)buf)[w++]);
  }

  return w;
}

int ioctl_tty(struct device *d, int op, void *arg) {
  struct tty *t = d->pdata;
  if (!t)
    return -EFAULT;

  if(!arg) return -EFAULT;

  switch(op) {
    case TCGETS:
      memcpy(arg, &((struct tty *)d->pdata)->termios, sizeof(struct termios));
      return 0;
    case TCSETS:
      memcpy(&((struct tty *)d->pdata)->termios, arg, sizeof(struct termios));
      update_tty(d);
      return 0;
  }

  return -EINVAL;
}

/* termios functions */

speed_t b2speed(uint32_t baud) {
  switch (baud) {
  case B0:
    return 0;
  case B50:
    return 50;
  case B75:
    return 75;
  case B110:
    return 110;
  case B134:
    return 134;
  case B150:
    return 150;
  case B200:
    return 200;
  case B300:
    return 300;
  case B600:
    return 600;
  case B1200:
    return 1200;
  case B1800:
    return 1800;
  case B2400:
    return 2400;
  case B4800:
    return 4800;
  case B9600:
    return 9600;
  case B19200:
    return 19200;
  case B38400:
    return 38400;
  }
  return -1;
}

uint32_t speed2b(speed_t speed) {
  switch (speed) {
  case 0:
    return B0;
  case 9600:
    return B9600;
  }
  return -1;
}

int tcgetattr(int fd, struct termios *termios_p) {
  return ioctl(fd, TCGETS, termios_p);
}

int tcsetattr(int fd, int op, const struct termios *termios_p) {
  switch (op) {
  case TCSANOW:
    return ioctl(fd, TCSETS, (void *)termios_p);
  case TCSADRAIN:
    return ioctl(fd, TCSETSW, (void *)termios_p);
  case TCSAFLUSH:
    return ioctl(fd, TCSETSF, (void *)termios_p);
  }
  return -EINVAL;
}

int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);

void cfmakeraw(struct termios *termios_p) {
  termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  termios_p->c_oflag &= ~OPOST;
  termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  termios_p->c_cflag &= ~(CSIZE | PARENB);
  termios_p->c_cflag |= CS8;
}

speed_t cfgetispeed(const struct termios *termios_p) {
  uint32_t ispeed = ((termios_p->c_cflag & CIBAUD) >> 16);
  return b2speed(ispeed);
}

speed_t cfgetospeed(const struct termios *termios_p) {
  uint32_t ospeed = termios_p->c_cflag & CBAUD;
  return b2speed(ospeed);
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
  termios_p->c_cflag &= ~CIBAUD;
  termios_p->c_cflag |= speed2b(speed) << 16;
  return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
  termios_p->c_cflag &= CBAUD;
  termios_p->c_cflag |= speed2b(speed);
  return 0;
}

int cfsetspeed(struct termios *termios_p, speed_t speed) {
  cfsetispeed(termios_p, speed);
  cfsetospeed(termios_p, speed);
  return 0;
}

/* init and misc */

struct device tty_dev;
struct tty_ops tty_ops;

struct tty *alloc_tty(struct tty_ops *ops) {
  struct tty *t      = malloc(sizeof(struct tty));
  t->termios.c_iflag = TTYDEF_IFLAG;
  t->termios.c_cflag = TTYDEF_CFLAG;
  t->termios.c_lflag = TTYDEF_LFLAG;
  t->termios.c_oflag = TTYDEF_OFLAG;
  t->termios.c_cflag |= TTYDEF_SPEED;       // ospeed
  t->termios.c_cflag |= TTYDEF_SPEED << 16; // ispeed
  t->ops = ops;

  memcpy(t->termios.c_cc, ttydefchars, sizeof(ttydefchars));

  return t;
}

struct device *alloc_ttydev(struct tty *t) {
  struct device *d = malloc(sizeof(*d));
  d->pdata     = t;
  d->ops.read  = read_tty;
  d->ops.write = write_tty;
  d->ops.ioctl = ioctl_tty;
  return d;
}