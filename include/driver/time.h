#ifndef TIME_H
#define TIME_H

#include "stdint.h"

struct time_s {
  uint8_t sec;
  uint8_t min;
  uint8_t hour;

  uint8_t day;
  uint8_t mon;
  uint8_t year;
};

void read_time(struct time_s *tim);

#endif