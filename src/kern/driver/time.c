#include "driver/time.h"
#include "io.h"
#include "video/printf.h"
#include "video/video.h"

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

uint8_t read_cmos(uint8_t addr) {
  outb(CMOS_ADDR, NMI_DISABLE | addr);
  io_wait();
  return inb(CMOS_DATA);
}

uint8_t normal(uint8_t bcd) {
  return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F);
}

void read_time(struct time_s *tim) {
  if(read_cmos(RTC_STB) & 1) {
    printk("RTC is in binary!\n");

    tim->sec = read_cmos(RTC_SEC);
    tim->min = read_cmos(RTC_MIN);
    tim->hour = read_cmos(RTC_HOR);

    tim->day = read_cmos(RTC_WDY);
    tim->mon = read_cmos(RTC_MON);
    tim->year = read_cmos(RTC_JHR);

    
    printkf("%02d:%02d:%02d (hh:mm:ss)\n", tim->hour, tim->min, tim->sec);
    return;
  }

  tim->sec = normal(read_cmos(RTC_SEC));
  tim->min = normal(read_cmos(RTC_MIN));
  tim->hour = normal(read_cmos(RTC_HOR));

  tim->day = normal(read_cmos(RTC_WDY));
  tim->mon = normal(read_cmos(RTC_MON));
  tim->year = normal(read_cmos(RTC_JHR));

    
  printkf("%02d:%02d:%02d (hh:mm:ss)\n", tim->hour, tim->min, tim->sec);
}

// todo: lapic timer and probably hpet or just pit if im lazy
// AND preemptive (not sure if i spelled it right) multitasking