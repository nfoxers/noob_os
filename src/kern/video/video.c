#include "io.h"
#include "video/printf.h"
#include <stdint.h>

#define VGA ((uint8_t *)0xb8000)

extern void scroll();
extern void clear();

uint16_t cursor = 0;

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

void putchr(char c) {
  if (c == '\n') {
    cursor += 80 - (cursor % 80);
  } else {
    VGA[cursor * 2] = c;
    VGA[cursor * 2 + 1] = 0x0f;
    cursor++;
  }
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
  clear();
  cursor = 0;
}

void init_video() {
  readcursor();
  printkf("video initialized\n");
}