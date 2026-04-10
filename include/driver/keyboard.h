#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"

int init_kbd();
uint8_t getch();

int kgets(char *s, uint16_t siz);

#endif