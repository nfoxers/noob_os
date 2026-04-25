#ifndef TERM_H
#define TERM_H

#include <stdint.h>

struct terminal;
struct cell;

// all takes *DEVICE* cursors
struct console_ops {
  void (*putc)(struct terminal *t, int x, int y, struct cell *c);
  void (*flush)(struct terminal *t);
  void (*clear)(struct terminal *t);

  void (*set_cursor)(struct terminal *t, int x, int y);
};

struct cell {
  uint8_t ch;
  uint8_t fg, bg;
};

struct terminal {
  int cursor_x, cursor_y; // cursors in buffer space

  int width, height; // logical dimension
  int dheight;       // device height

  int scroll_top;

  struct cell        *cbuf;
  struct console_ops *cops;

  uint8_t fg, bg;
};

void term_set_cursor(struct terminal *t, int x, int y);
void term_putc(struct terminal *t, char c);
void term_newline(struct terminal *t);
void term_bkspace(struct terminal *t);

void term_scroll_down(struct terminal *t);
void term_scroll_up(struct terminal *t);

/* VT100 subsystem */

enum vt_state {
  GROUND,
  ESCAPE,
  CSI_PARAM,
};

struct vt {
  enum vt_state    s;
  struct terminal *t;

  int params[8];
  int param_count;
  int cur;
};

void vt_process(struct vt *vt, char c);

#endif