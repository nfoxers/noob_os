#ifndef VIDEO_H
#define VIDEO_H

#include "../io.h"

void printk(char *a);
void scroll_n(uint8_t n);
void putchr(char c);
void clr_scr();
void init_video();

#endif