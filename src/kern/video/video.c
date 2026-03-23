#include "io.h"
#include "video/printf.h"
#include <stdint.h>

#define VGA ((uint8_t *)0xb8000)

extern void scroll();
extern void clear();

uint16_t cursor = 0;

static uint8_t c_att = 0x00;
static uint8_t vflag = 0;

#define VF_ESCAPE 0x01

void setcursor() {
  outb(0x3d4, 0x0f);
  outb(0x3d5, (uint8_t)cursor & 0xff);

  outb(0x3d4, 0x0e);
  outb(0x3d5, (uint8_t)(cursor >> 8) & 0xff);
}

void readcursor() {
  uint8_t hi, lo;
  outb(0x3d4, 0x0f);
  lo = inb(0x3d5);

  outb(0x3d4, 0x0e);
  hi = inb(0x3d5);

  cursor = (hi << 8) | lo;
}

void scroll_once() {
  scroll();
  cursor -= 80;
  setcursor();
}

void backspace() {
  VGA[--cursor * 2] = 0;
  setcursor();
}

int kisprint(int c) {
  if((c >= ' ') && (c <= '~'))
    return 1;
  return 0;
}

void putchr(char c) {

  if(vflag & VF_ESCAPE) {
    // todo: proper ansi escape handling
    c_att = c;
    vflag &= ~VF_ESCAPE;
    return;
  }

  if (c == '\n') {
    cursor += 80 - (cursor % 80);
  } else if (c == '\b') {
    backspace();
  } else if (c == '\e') {
    vflag |= VF_ESCAPE;
    return;
  } else if(1 || kisprint(c)){
    VGA[cursor * 2]     = c;
    VGA[cursor * 2 + 1] = c_att;
    cursor++;
  }

  if (cursor >= 2000)
    scroll_once();

  setcursor();
}

void printk(char *a) {
  while (*a) {
    putchr(*a);
    a++;
  }
}

void scroll_n(uint8_t n) {
  for (; n > 0; n--) {
    if (cursor > 80)
      cursor -= 80;
    scroll();
  }
  setcursor();
}

void clr_scr() {
  cursor = 0;
  setcursor();
  clear();
}

void init_video() {
  readcursor();
  c_att = 0x0f;
  printkf("video initialized\n");
}