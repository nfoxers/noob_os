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

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_SEC 0x00
#define RTC_MIN 0x02
#define RTC_HOR 0x04

#define RTC_WDY 0x06
#define RTC_DAY 0x07
#define RTC_MON 0x08
#define RTC_JHR 0x09

#define RTC_STB 0x0B

#define NMI_DISABLE 0x80

void read_time(struct time_s *tim);
void init_pit(uint32_t freq);

uint32_t gettime(uint32_t *ms);
void print_time();

#endif