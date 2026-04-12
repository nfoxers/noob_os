#ifndef SERIAL_H
#define SERIAL_H

#include "driver/tty/uart.h"
#include "io.h"

#define SRL_BUFSIZ 128

uint32_t init_serial(uint32_t freq);

void s_putchr(int c);
char s_getchr();

#endif