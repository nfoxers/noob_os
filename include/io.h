#ifndef IO_H
#define IO_H

#include "stdint.h"

void outb(uint16_t a, uint8_t d);
void outw(uint16_t a, uint16_t d);
uint8_t inb(uint16_t a);
uint16_t inw(uint16_t a);

#endif