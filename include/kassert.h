#ifndef KASSERT_H
#define KASSERT_H

#include "stdint.h"

extern void _kassert(const char *restrict expr, const char *restrict file, uint32_t line);

#define kassert(expr)                      \
  {                                        \
    if (expr)                              \
      _kassert(#expr, __FILE__, __LINE__); \
  }

#endif