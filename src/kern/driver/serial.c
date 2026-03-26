#include "driver/serial.h"
#include "cpu/idt.h"
#include "io.h"
#include "video/printf.h"
#include "video/video.h"

#define UART_FREQ 115200
#define SRL_BUFSIZ 128

static uint32_t sfreq;

volatile uint8_t charbuf[SRL_BUFSIZ];
static volatile uint16_t head;
static volatile uint16_t tail;

static void serial_isr(struct regs *r) {
  (void)r;
  uint16_t next = (head + 1) % SRL_BUFSIZ;
  char c = inb(COM1);

  if(c == 'u') {
    vscroll_up();
    goto isr_done;
  }

  if(c == 'v') {
    vscroll_down();
    goto isr_done;
  }

  if(next != tail) {
    if(c == '\r') c = '\n';
    charbuf[head] = c;
    s_putchr(c);
    head = next;
  }
isr_done:
  pic_eoi();
}

char s_getchr() {
  while(head == tail);

  char c = charbuf[tail];
  tail = (tail + 1) % SRL_BUFSIZ;
  return c;
}

void s_putchr(int c) {
  if(c == '\n') {
    outb(COM1, '\n');
    outb(COM1, '\r');
    return;
  }
  if(c == '\b') {
    outb(COM1, '\b');
    outb(COM1, ' ');
    outb(COM1, '\b');
    return;
  }
  outb(COM1, c);
}

uint32_t init_serial(uint32_t freq) {
  uint32_t div = UART_FREQ / freq;
  if (div > UINT16_MAX) {
    div   = 0xffff;
    sfreq = UART_FREQ / div;
  }

  sfreq = UART_FREQ / div;

  outb(COM1 + 1, 0x00);

  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, div & 0xff);
  outb(COM1 + 1, (div >> 8) & 0xff);

  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xc7);
  outb(COM1 + 4, 0x0b);
  outb(COM1 + 4, 0x1e);

  outb(COM1 + 0, 0xAE);
  if (inb(COM1 + 0) != 0xAE) 
    return 1;

  outb(COM1 + 4, 0x0f);
  print_info("frq", 2, "baud rate: \e\x09%d baud * hz\e\x0f", UART_FREQ / div);

  outb(COM1 + 1, 1);
  
  register_irq(serial_isr, 4);

  pic_cm(4);
  return 0;
}