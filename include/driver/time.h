#ifndef TIME_H
#define TIME_H

#include "stdint.h"
#include "stddef.h"

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

#define CLOCKS_PER_SEC 100

typedef long clock_t;
typedef long time_t;

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

/* unimplemented */
clock_t clock(void);
time_t time(time_t * tp);
double difftime(time_t time2, time_t time1);

char * asctime(const struct tm * tp);
char * ctime(const time_t * tp);
struct tm * gmtime(const time_t *tp);
struct tm *localtime(const time_t * tp);
size_t strftime(char * s, size_t smax, const char * fmt, const struct tm * tp);
void tzset(void);

/* implemented */
void read_time(struct tm *tim);
void init_pit(uint32_t freq);

uint32_t gettime(uint32_t *ms);
void print_time();
time_t mktime(struct tm * tp);

#endif