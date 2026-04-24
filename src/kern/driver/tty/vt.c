#include "video/printf.h"
#include <driver/tty/term.h>
#include <lib/ctype.h>
#include <video/video.h>

#define DEFAULT_FG 0x07
#define DEFAULT_BG 0x00

#define ATTR_DIM 0x01
#define ATTR_BRIGHT 0x02

int vt_vt2col(int col) {
  switch (col) {
    case 1: return CL_RED;
    case 3: return CL_YELLOW;
    case 4: return CL_BLUE;
    case 6: return CL_CYAN;
    default: return col;
  }
}

void vt_setattr(struct vt *vt) {
  struct terminal *term = vt->t;

  if(vt->params[0] == 0) {
    term->bg = DEFAULT_BG;
    term->fg = DEFAULT_FG;
    return;
  }

  uint8_t fg_pending, bg_pending;
  uint8_t flags = 0;

  for(int i = 0; i < vt->param_count; i++) {
    int p = vt->params[i];

    if(p == 1) flags |= ATTR_BRIGHT;
    // ! do more!

    if(p >= 30 && p <= 37) {
      fg_pending = p % 10;
    }

    if(p >= 40 && p <= 47) {
      bg_pending = p % 10;
    }
  }

  fg_pending = vt_vt2col(fg_pending);
  bg_pending = vt_vt2col(bg_pending);

  if(flags & ATTR_BRIGHT) {
    fg_pending |= 0x8;
    bg_pending |= 0x8;
  }

  term->fg = fg_pending;
  term->bg = bg_pending;
}

void vt_dispatch(struct vt *vt, char c) {
  switch (c) {
    case 'm': // attribute mode set
    vt_setattr(vt);
    break;
  }
}

extern void _putchr(char c);

void vt_process(struct vt *vt, char c) {
  switch(vt->s) {
    case GROUND:
      if(c == '\e') vt->s = ESCAPE;
      else term_putc(vt->t, c);
      break;
    case ESCAPE:
      if(c == '[') {
        vt->s = CSI_PARAM;
        vt->param_count = 0;
        vt->cur = 0;
      } else vt->s = GROUND;
      break;
    case CSI_PARAM:
      if(isdigit(c)) {
        vt->cur = vt->cur * 10 + (c - '0');
      } else if(c == ';') {
        vt->params[vt->param_count++] = vt->cur;
        vt->cur = 0;
      } else {
        vt->params[vt->param_count++] = vt->cur;
        vt_dispatch(vt, c);
        vt->s = GROUND;
      }
      break;
  }
}