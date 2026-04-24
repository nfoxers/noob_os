#ifndef VIDEO_H
#define VIDEO_H

#include "io.h"

#define EOF 0x04

#define CL_BLACK   0
#define CL_BLUE    1
#define CL_GREEN   2
#define CL_CYAN    3
#define CL_RED     4
#define CL_MAGENTA 5
#define CL_BROWN   6
#define CL_GRAY    7

#define CL_YELLOW (CL_BROWN | 0x8)

#define CL_LIGHT(c) ((c | 0x08))

void printk(char *a);
void putchr(char c);
void clr_scr();
void init_video();

void vflush();
void vscroll_down();
void vscroll_up();

int  kisprint(int);
void mkadv();

#endif