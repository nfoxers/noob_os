#include "ams/sys/types.h"
#include "fs/devfs.h"
#include "fs/vfs.h"
#include "io.h"
#include "mem/mem.h"
#include <driver/tty.h>
#include <driver/tty/term.h>
#include <lib/errno.h>
#include <proc/proc.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall/syscall.h>
#include <video/printf.h>

#define VGA ((uint16_t *)0xb8000)

#define VGA_DEV_HEIGHT 25
#define VGA_DEV_WIDTH  80

#define VGA_SCROLL_HEIGHT 50

struct console_ops vga_cops;

void vga_setcursor(struct terminal *t, int x, int y) {
  uint16_t crs = y * t->width + x;
  outb(0x3d4, 0x0f);
  outb(0x3d5, (uint8_t)crs & 0xff);

  outb(0x3d4, 0x0e);
  outb(0x3d5, (uint8_t)(crs >> 8) & 0xff);
}

void vga_readcursor(struct terminal *t, int *x, int *y) {
  uint8_t hi, lo;
  outb(0x3d4, 0x0f);
  lo = inb(0x3d5);

  outb(0x3d4, 0x0e);
  hi = inb(0x3d5);

  uint16_t i = hi << 8 | lo;
  *x         = i % t->width;
  *y         = i / t->width;
}

extern void _putchr(char c);

void vga_putc(struct terminal *t, int x, int y, struct cell *c) {
  //_putchr(c->ch);
  VGA[y * t->width + x] = c->ch | (((t->fg << 8) & 0x0f00) | ((t->bg << 12) & 0xf000));
}

void vga_flush(struct terminal *t) {
  // ! absolutes kino
  for (int i = 0; i < t->width; i++) {
    for (int j = 0; j < t->dheight; j++) {
      if (j + t->scroll_top >= t->height)
        break;
      struct cell *c        = &t->cbuf[(j + t->scroll_top) * t->width + i];
      VGA[j * t->width + i] = c->ch | (((c->fg << 8) & 0x0f00) | ((c->bg << 12) & 0xf000));
    }
  }
}

void vga_clear(struct terminal *t) {
  t->cursor_x   = 0;
  t->cursor_y   = 0;
  t->scroll_top = 0;
  vga_setcursor(t, t->cursor_x, t->cursor_y);
  asm volatile(
      "pushl %%edi\n"
      "movl $0x0f000f00, %%eax\n"
      "movl $0xb8000, %%edi\n"
      "movl $1000, %%ecx\n"
      "rep stosl\n"
      "movl %[addr], %%edi\n"
      "movl %[len], %%ecx\n"
      "xor %%eax, %%eax\n"
      "rep stosl\n"
      "popl %%edi\n" ::[addr] "m"(t->cbuf),
      [len] "d"(sizeof(struct cell) * t->height * t->width / 4));

  for (int i = 0; i < t->width; i++) {
    for (int j = 0; j < t->dheight; j++) {
      if (j + t->scroll_top >= t->height)
        break;
      struct cell *c = &t->cbuf[(j + t->scroll_top) * t->width + i];
      c->fg          = 0x0f;
    }
  }
}

struct console_ops vga_cops = {
    .set_cursor = vga_setcursor,
    .putc       = vga_putc,
    .flush      = vga_flush,
    .clear      = vga_clear};

struct terminal vga_textterm;
struct vt       vga_vt;

ssize_t vga_ttywrite(struct tty *t, const char *buf, size_t c) {
  (void)t;
  for (size_t i = 0; i < c; i++) {
    vt_process(t->vt, buf[i]);
    vga_setcursor(t->vt->t, t->vt->t->cursor_x, t->vt->t->cursor_y - t->vt->t->scroll_top);
  }
  return c;
}

struct tty_ops vgttyops = {
    .write = vga_ttywrite};

struct tty    vgtty;
struct device vgdev;

void vgatext_init() {
  vga_textterm.cops = &vga_cops;
  vga_textterm.cbuf = malloc(sizeof(struct cell) * (VGA_SCROLL_HEIGHT * VGA_DEV_WIDTH));
  // memset(vga_textterm.cbuf, 0xff, sizeof(struct cell) * (VGA_SCROLL_HEIGHT * VGA_DEV_WIDTH));
  vga_textterm.dheight = VGA_DEV_HEIGHT;
  vga_textterm.height  = VGA_SCROLL_HEIGHT;
  vga_textterm.width   = VGA_DEV_WIDTH;

  vga_textterm.cursor_x = 0;
  vga_textterm.cursor_y = 0;

  vga_clear(&vga_textterm);

  vga_textterm.fg = 0x0f;
  vga_textterm.bg = 0x00;

  vga_vt.s = GROUND;
  vga_vt.t = &vga_textterm;

  // print_init("tty", "intializing /dev/tty...", 0);

  struct tty *t = alloc_tty(&vgttyops);
  memcpy(&vgtty, t, sizeof(*t));
  free(t);

  // ctty.termios.c_lflag &= ~(ECHO | ICANON);

  struct device *d = alloc_ttydev(&vgtty);
  memcpy(&vgdev, d, sizeof(*d));
  free(d);

  creat_devfs("tty", &vgdev, 1, 0);
  // printkf("stdin: %d\n", stdin);

  vgtty.vt                    = &vga_vt;
  p_curproc->signal->tty->tty = &vgtty;

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
