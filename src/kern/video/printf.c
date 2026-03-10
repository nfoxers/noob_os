#define NANOPRINTF_IMPLEMENTATION
#include "video/printf.h"
#include "stdarg.h"
#include "io.h"

void wrapper(int c, void *ctx) {
  (void)ctx;
  putchar((char)c);
}

int printkf(const char *fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vpprintf(wrapper, NULL, fmt, a);
  va_end(a);
  return r;
}