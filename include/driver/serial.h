#ifndef SERIAL_H
#define SERIAL_H

#define COM1 0x3f8 // todo: NOT hardwire this but instead do the bda stuff

#include "io.h"

uint32_t init_serial(uint32_t freq);

void s_putchr(int c);
char s_getchr();

#endif