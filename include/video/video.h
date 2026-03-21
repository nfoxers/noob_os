#ifndef VIDEO_H
#define VIDEO_H

#include "../io.h"

#define EOF 0x04

void printk(char *a);
void scroll_n(uint8_t n);
void putchr(char c);
void clr_scr();
void init_video();

int kisprint(int);

#endif