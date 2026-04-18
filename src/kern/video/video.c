#include "video/video.h"
#include "io.h"
#include "mem/mem.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include <stdint.h>

#define VGA ((uint8_t *)0xb8000)

static uint8_t *vbuf;

static uint16_t cursor     = 0;
static uint16_t bottom     = 0;
static uint16_t bottom_max = 0;

static uint8_t c_att = 0x00;
static uint8_t vflag = 0;

#define VF_ESCAPE 0x01
#define VF_ADV 0x02

#define V_MAXPAGE 2

void setcursor() {
  if ((int16_t)cursor - bottom * 80 < 0)
    return;

  uint16_t crs = cursor - bottom * 80;
  outb(0x3d4, 0x0f);
  outb(0x3d5, (uint8_t)crs & 0xff);

  outb(0x3d4, 0x0e);
  outb(0x3d5, (uint8_t)(crs >> 8) & 0xff);
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
  cursor -= 80;
  memcpy(VGA, VGA + 80 * 2, 80 * 25 * 2);
  memcpy(vbuf, vbuf + 80 * 2, 80 * 25 * 2 * V_MAXPAGE);
  vflush();
}

void backspace() {
  vbuf[--cursor * 2]                = 0;
  VGA[cursor * 2 - bottom * 80 * 2] = 0;
  setcursor();
}

void _putchr(char c) {
  if (vflag & VF_ESCAPE) {
    // todo: proper ansi escape handling
    c_att = c;
    vflag &= ~VF_ESCAPE;
    return;
  }

  if (c == '\n') {
    cursor += 80 - (cursor % 80);
    if (cursor >= 2000 + bottom * 80) {
      for (int i = bottom; i <= bottom_max; i++)
        vscroll_down();
    }
  } else if(c == '\r') {
    cursor -= (cursor % 80);
  } else if (c == '\b') {
    backspace();
  } else if (c == '\e') {
    vflag |= VF_ESCAPE;
    return;
  } else {
    vbuf[cursor * 2]                      = c;
    vbuf[cursor * 2 + 1]                  = c_att;
    VGA[cursor * 2 - bottom * 80 * 2]     = c;
    VGA[cursor * 2 + 1 - bottom * 80 * 2] = c_att;
    cursor++;
  }

  if (cursor >= 2000 * 2) {
    scroll_once();
  }

  if (cursor >= 2000 + bottom_max * 80) {
    bottom_max++;
  }

  setcursor();
  //for(int i = 0; i < 100; i++)vflush();
}

void putchr(char c) {
  if(vflag & VF_ADV) {
    write(1, &c, 1);
  } else {
    _putchr(c);
  }
}

void mkadv() {
  vflag |= VF_ADV;
}

void printk(char *a) {
  while (*a) {
    putchr(*a);
    a++;
  }
}

void clr_scr() {
  bottom = 0;
  cursor = 0;
  setcursor();
  asm volatile(
      "pushl %%edi\n"
      "movl $0x0f000f00, %%eax\n"
      "movl $0xb8000, %%edi\n"
      "movl $1000, %%ecx\n"
      "rep stosl\n"
      "movl %[addr], %%edi\n"
      "movl %[len], %%ecx\n"
      "rep stosl\n"
      "popl %%edi\n" ::[addr] "m"(vbuf),
      [len] "d"(80 * 25 * 2 * V_MAXPAGE / 4));
}

void vflush() {
  memcpy(VGA, vbuf + bottom * 2 * 80, 80 * 25 * 2);
}

void vscroll_down() {
  if (bottom == 25 * (V_MAXPAGE - 1))
    return;
  bottom++;

  vflush();
  setcursor();
}

void vscroll_up() {
  if (bottom == 0)
    return;
  bottom--;

  vflush();
  setcursor();
}

void init_video() {
  vbuf = malloc(80 * 25 * 2 * V_MAXPAGE); // capacity of V_MAXPAGE 'pages' (i.e one full vga screen)
  memcpy(vbuf, VGA, 80 * 25 * 2);
  // clr_scr();

  readcursor();
  c_att = 0x0f;
  printkf("video initialized\n");
}