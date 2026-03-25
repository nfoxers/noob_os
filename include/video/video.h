#ifndef VIDEO_H
#define VIDEO_H

#include "../io.h"

#define EOF 0x04

void printk(char *a);
void scroll_n(uint8_t n);
void putchr(char c);
void clr_scr();
void init_video();

void vflush();
void vscroll_down();
void vscroll_up();

int kisprint(int);

#endif