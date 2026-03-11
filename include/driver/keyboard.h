#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "cpu/idt.h"

void init_kbd();
uint8_t getch();

char *kgets(char *s, uint16_t siz);

#endif