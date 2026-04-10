#include "driver/time.h"
#include "cpu/idt.h"
#include "io.h"
#include "proc/proc.h"
#include "video/printf.h"
#include "video/video.h"

#define PIT_DATA 0x40 // only for channel 0 >:3c
#define PIT_COMM 0x43

#define PIT_FREQ (uint32_t)1193182

int8_t timezone = 0; // todo: fix timezone and ADD COMMANDS!

uint8_t read_cmos(uint8_t addr) {
  outb(CMOS_ADDR, NMI_DISABLE | addr);
  io_wait();
  return inb(CMOS_DATA);
}

uint8_t normal(uint8_t bcd) {
  return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F);
}

void read_time(struct tm *tim) {
  if (read_cmos(RTC_STB) & 1) {
    printk("RTC is in binary!\n");

    tim->tm_sec  = read_cmos(RTC_SEC);
    tim->tm_min  = read_cmos(RTC_MIN);
    tim->tm_hour = read_cmos(RTC_HOR);

    tim->tm_yday = read_cmos(RTC_DAY);
    tim->tm_mon  = read_cmos(RTC_MON);
    tim->tm_year = read_cmos(RTC_JHR);

    return;
  }

  tim->tm_sec  = normal(read_cmos(RTC_SEC));
  tim->tm_min  = normal(read_cmos(RTC_MIN));
  tim->tm_hour = normal(read_cmos(RTC_HOR));

  tim->tm_yday = normal(read_cmos(RTC_DAY));
  tim->tm_mon  = normal(read_cmos(RTC_MON));
  tim->tm_year = normal(read_cmos(RTC_JHR));
}

void print_time() {
  struct tm tim;
  read_time(&tim);
  printkf("20%02d/%02d/%02d, %02d:%02d:%02d UTC+0 (hh:mm:ss)\n", tim.tm_year, tim.tm_mon, tim.tm_yday, tim.tm_hour, tim.tm_min, tim.tm_sec);
}

// todo: lapic timer and probably hpet

uint32_t          pit_freq;
volatile uint32_t counter;

void general_switch();

void timer_handler(struct regs *r) {
  (void)r;

  counter++;
  pic_eoi();

  // it is fixed now Ü
  schedule();
}

uint32_t gettime(uint32_t *ms) {
  uint32_t secs = counter / pit_freq;
  if (ms)
    *ms = (counter * 1000) / pit_freq - secs * 1000;
  return secs;
}

void init_pit(uint32_t freq) {
  print_init("pit", "intializing the pit...", 0);

  pit_freq     = freq;
  counter      = 0;
  uint32_t div = (PIT_FREQ / freq);

  if (div > UINT16_MAX) {
    div      = 0xffff;
    pit_freq = PIT_FREQ / div;
  }

  print_info("frq", 0, "actual pit freq: \e\x09%d mHz\e\x0f", PIT_FREQ * 1000 / div);
  outb(PIT_COMM, 0x36);
  outb(PIT_DATA, div & 0xff);
  outb(PIT_DATA, (div >> 8) & 0xff);

  register_irq(timer_handler, 0);
  pic_cm(0);
}

/* note from nfoxers: code below taken from the linux kernel 0.01 source */

/*
 * This isn't the library routine, it is only used in the kernel.
 * as such, we don't care about years<1970 etc, but assume everything
 * is ok. Similarly, TZ etc is happily ignored. We just do everything
 * as easily as possible. Let's find something public for the library
 * routines (although I think minix times is public).
 */
/*
 * PS. I hate whoever though up the year 1970 - couldn't they have gotten
 * a leap-year instead? I also hate Gregorius, pope or no. I'm grumpy.
 */
#define MINUTE 60
#define HOUR   (60 * MINUTE)
#define DAY    (24 * HOUR)
#define YEAR   (365 * DAY)

/* interestingly, we assume leap-years */
static const int month[12] = {
    0,
    DAY * (31),
    DAY * (31 + 29),
    DAY * (31 + 29 + 31),
    DAY * (31 + 29 + 31 + 30),
    DAY * (31 + 29 + 31 + 30 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};

long mktime(struct tm *tm) {
  long res;
  int  year;

  year = tm->tm_year - 70;
  /* magic offsets (y+1) needed to get leapyears right.*/
  res = YEAR * year + DAY * ((year + 1) / 4);
  res += month[tm->tm_mon];
  /* and (y+2) here. If it wasn't a leap-year, we have to adjust */
  if (tm->tm_mon > 1 && ((year + 2) % 4))
    res -= DAY;
  res += DAY * (tm->tm_mday - 1);
  res += HOUR * tm->tm_hour;
  res += MINUTE * tm->tm_min;
  res += tm->tm_sec;
  return res;
}

struct tm *gmtime_r(const time_t *restrict timep, struct tm *restrict result) {
  time_t t = *timep;
  (void)t;
  return result;
}
