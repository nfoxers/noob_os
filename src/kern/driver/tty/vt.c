#include <driver/tty/term.h>
#include <lib/ctype.h>

void vt_dispatch(char c, int *params, int pcount) {
  // ! ...
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
        vt_dispatch(c, vt->params, vt->param_count);
        vt->s = GROUND;
      }
      break;
  }
}