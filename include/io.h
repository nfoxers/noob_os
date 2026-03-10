#include "stdint.h"

#ifndef IO_H
#define IO_H

void printk(char *a);
void scroll_n(uint8_t n);
void putchar(char c);
void clr_scr();

void outb(uint16_t a, uint8_t d);
void outw(uint16_t a, uint16_t d);
uint8_t inb(uint16_t a);
uint16_t inw(uint16_t a);

#endif