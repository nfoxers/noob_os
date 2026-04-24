#ifndef TTY_H
#define TTY_H

#include "cpu/spinlock.h"
#include "fs/vfs.h"
#include "stddef.h"
#include "stdint.h"

#define TTYBUFSIZ  32
#define TTYLINESIZ 32

#include "ams/termbits.h"

struct tty;

struct tty_ops {
  ssize_t (*write)(struct tty *tty, const char *buf, size_t count);

  void (*update)(struct tty *tty);
};

struct tty {
  char canonbuf[TTYBUFSIZ];
  int  canonr;
  int  canonw;

  char outbuf[TTYBUFSIZ];
  int  outr;
  int  outw;

  char   line_buf[TTYLINESIZ];
  size_t line_len;

  struct tty_ops *ops;
  struct termios  termios;
  struct vt *vt;

  spinlock_t      lock;
};

struct tty_struct {
  struct tty *tty;
};

extern int stdin;
extern int stdout;

void tty_inputc(struct tty *t, char c);
void tty_outputc(struct tty *t, char c);

struct tty    *alloc_tty(struct tty_ops *ops);
struct device *alloc_ttydev(struct tty *t);

speed_t b2speed(uint32_t baud);


int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions,
              const struct termios *termios_p);

int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);

void cfmakeraw(struct termios *termios_p);

speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);

int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
int cfsetspeed(struct termios *termios_p, speed_t speed);

#endif