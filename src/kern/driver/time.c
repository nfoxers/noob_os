#include "driver/time.h"
#include "cpu/idt.h"
#include "io.h"
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

void read_time(struct time_s *tim) {
  if (read_cmos(RTC_STB) & 1) {
    printk("RTC is in binary!\n");

    tim->sec  = read_cmos(RTC_SEC);
    tim->min  = read_cmos(RTC_MIN);
    tim->hour = read_cmos(RTC_HOR);

    tim->day  = read_cmos(RTC_WDY);
    tim->mon  = read_cmos(RTC_MON);
    tim->year = read_cmos(RTC_JHR);

    printkf("%02d:%02d:%02d UTC+0 (hh:mm:ss)\n", tim->hour, tim->min, tim->sec);
    return;
  }

  tim->sec  = normal(read_cmos(RTC_SEC));
  tim->min  = normal(read_cmos(RTC_MIN));
  tim->hour = normal(read_cmos(RTC_HOR));

  tim->day  = normal(read_cmos(RTC_WDY));
  tim->mon  = normal(read_cmos(RTC_MON));
  tim->year = normal(read_cmos(RTC_JHR));

  printkf("%02d:%02d:%02d UTC+0 (hh:mm:ss)\n", tim->hour, tim->min, tim->sec);
}

// todo: lapic timer and probably hpet
// AND fix preemptive mulitasking

uint32_t          pit_freq;
volatile uint32_t counter;

void general_switch();

void timer_handler(struct regs *r) {
  counter++;
  pic_eoi();
  
  general_switch(); // uncomment when fixed
}

uint32_t gettime(uint32_t *ms) {
  uint32_t secs = counter/pit_freq;
  if(ms) *ms = (counter*1000)/pit_freq - secs*1000;
  return secs;
}

void init_pit(uint32_t freq) {
  pit_freq     = freq;
  counter      = 0;
  uint32_t div = (PIT_FREQ / freq);

  if (div > UINT16_MAX) {
    div      = 0xffff;
    pit_freq = PIT_FREQ / div;
  }

  outb(PIT_COMM, 0x36);
  outb(PIT_DATA, div & 0xff);
  outb(PIT_DATA, (div >> 8) & 0xff);

  register_irq(timer_handler, 0);
  pic_cm(0);
}