#define NANOPRINTF_IMPLEMENTATION
#include "video/printf.h"
#include "video/video.h"
#include "stdarg.h"


void wrapper(int c, void *ctx) {
  (void)ctx;
  putchr((char)c);
}

int printkf(const char *fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vpprintf(wrapper, NULL, fmt, a);
  va_end(a);
  return r;
}

int snprintkf(char *restrict buf, size_t siz, const char *restrict fmt, ...) {
  va_list a;
  va_start(a, fmt);
  int r = npf_vsnprintf(buf, siz, fmt, a);
  va_end(a);
  return r;
}

void print_init(const char *f, const char *s, int rc) {
  const char *ok = "\e\x0agood\e\x0f";
  const char *no = "\e\x0c""err \e\x0f";

  printkf("[ \e\x0c%-8s\e\x0f] %-59s [ %s ]", f, s, rc ? no : ok);
}

void print_info(const char *s, int mto, const char *fmt, ...) {
  va_list a;
  va_start(a, fmt);
  char buf[60];
  npf_vsnprintf(buf, 60, fmt, a);
  printkf("[  \e\x0c%c\e\x0B%-6s\e\x0f] %-59s\n", !mto ? 0xc0 : 0xc3, s, buf);
  va_end(a);
}