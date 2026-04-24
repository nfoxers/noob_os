#include "lib/ctype.h"
#include "video/printf.h"
#include "video/video.h"
#include <driver/tty/term.h>
#include <stdint.h>
#include <mem/mem.h>

extern void _putchr(char c);

void term_set_cursor(struct terminal *t, int x, int y) {
  if(x > t->width) return;
  if(y - t->scroll_top > t->dheight) return;
  if(y < t->scroll_top) return;

  t->cursor_x = x;
  t->cursor_y = y;
  if (t->cops && t->cops->set_cursor) {
    t->cops->set_cursor(t, x, y - t->scroll_top);
  }
}

void term_scroll_up(struct terminal *t) {
  if(t->scroll_top + t->dheight >= t->height) return;

  t->scroll_top++;
  if (t->cops && t->cops->flush) {
    t->cops->flush(t);
    term_set_cursor(t, t->cursor_x, t->cursor_y);
  }
}

void term_scroll_down(struct terminal *t) {
  if(t->scroll_top <= 0) return;

  t->scroll_top--;
  if (t->cops && t->cops->flush) {
    t->cops->flush(t);
    term_set_cursor(t, t->cursor_x, t->cursor_y);
  }
}

void term_putc_raw(struct terminal *t, char c) {
  if (!t->cops || !t->cops->putc) {
    return;
  }

  uint8_t k = t->scroll_top;

  if (t->cursor_x >= t->width) {
    if (t->cursor_y - t->scroll_top >= t->dheight) {
      term_scroll_up(t);
      t->cursor_y--;
    }
    t->cursor_x = 0;
    t->cursor_y++;
  }
  
  
  struct cell cc = {.ch = c, .fg = t->fg, .bg = t->bg};

  t->cops->putc(t, t->cursor_x, t->cursor_y - t->scroll_top, &cc);
  memcpy(&t->cbuf[t->cursor_y * t->width + t->cursor_x], &cc, sizeof(cc));
  //_putchr('s');
  t->cursor_x++;
}

void term_newline(struct terminal *t) {
  if (t->cursor_y - t->scroll_top >= t->dheight) {
    term_scroll_up(t);
    return;
  };
  t->cursor_y++;
  //_putchr(toascii(t->cursor_y));
}

void term_bkspace(struct terminal *t) {
  if(t->cursor_x <= 0) return;
  t->cursor_x--;
  if(t->cops && t->cops->set_cursor) {
    t->cops->set_cursor(t, t->cursor_x, t->cursor_y);
  }
  return;
}

extern struct terminal vga_textterm;
void clr_scr() {
  if(vga_textterm.cops && vga_textterm.cops->clear) {
    vga_textterm.cops->clear(&vga_textterm);
  }
}

void term_putc(struct terminal *t, char c) {
  //_putchr('s');

  switch (c) {
  case '\n':
    term_newline(t);
    break;
  case '\r':
    t->cursor_x = 0;
    break;
  case '\b':
    term_bkspace(t);
    break;
  default:
    term_putc_raw(t, c);
  }

  // if(t->cursor_y >= t->scroll_top + t->height) t->scroll_top++;
}