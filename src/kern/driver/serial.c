#include "driver/serial.h"
#include "ams/termbits.h"
#include "cpu/idt.h"
#include "driver/tty.h"
#include "driver/tty/uart.h"
#include "fs/devfs.h"
#include "fs/vfs.h"
#include "io.h"
#include "video/printf.h"
#include "video/video.h"
#include <mem/mem.h>

static uint32_t sfreq;

struct tty    uartty;
struct device uartdev;

int uart_rxavail() {
  return inb(COM1 + UART_LSTAT) & L_DR;
}

static void serial_isr(struct regs *r) {
  (void)r;

  while (uart_rxavail()) {
    char c = inb(COM1);
    tty_inputc(&uartty, c);
  }

  pic_eoi();
}

char s_getchr() {

  return 0;
}

void s_putchr(int c) {
  outb(COM1, c);
}

ssize_t write_ttyserial(struct tty *tty, const char *buf, size_t count) {
  (void)tty;
  for (size_t i = 0; i < count; i++) {
    s_putchr(buf[i]);
  }
  return count;
}

void update_serial(struct tty *tty) {
  struct termios *t = &tty->termios;

  uint8_t siz = t->c_cflag & CSIZE;
  switch (siz) {
    case CS5:
      siz = 0;
      break;
    case CS6:
      siz = 1;
      break;
    case CS7:
      siz = 2;
      break;
    case CS8:
      siz = 3;
      break;
  }

  uint8_t n = inb(COM1 + UART_LCTRL);
  n &= ~3;
  n |= siz;
  outb(COM1 + UART_LCTRL, n);

  // todo: things
}

struct tty_ops stty_ops = {
    .write = write_ttyserial,
    .update = update_serial};

void init_ttys0() {
  struct tty *tty_tmp = alloc_tty(&stty_ops);
  memcpy(&uartty, tty_tmp, sizeof(*tty_tmp));
  free(tty_tmp);

  struct device *dev_tmp = alloc_ttydev(&uartty);
  memcpy(&uartdev, dev_tmp, sizeof(*dev_tmp));
  free(dev_tmp);

  // register_dev(1, 0, &uartdev);
  creat_devfs("ttyS0", &uartdev, 1, 1); // dev (1, 0) has been used for the kbd
}

void set_baud(uint32_t freq) {
  uint32_t div = UART_FREQ / freq;
  if (div > UINT16_MAX) {
    div   = 0xffff;
    sfreq = UART_FREQ / div;
  }

  sfreq = UART_FREQ / div;

  uint8_t ints = 0;
  ints         = inb(COM1 + UART_INT);

  outb(COM1 + UART_INT, 0x00);

  uint8_t lctrl = inb(COM1 + UART_LCTRL) & ~LCTRL_DLAB;
  outb(COM1 + UART_LCTRL, 0x80);

  outb(COM1 + UART_DIVLO, div & 0xff);
  outb(COM1 + UART_DIVHI, (div >> 8) & 0xff);

  outb(COM1 + UART_LCTRL, lctrl);
  outb(COM1 + UART_INT, ints);
}

uint32_t init_serial(uint32_t freq) {
  outb(COM1 + UART_INT, 0x00);

  set_baud(freq);

  outb(COM1 + UART_LCTRL, DATAB8);
  outb(COM1 + UART_FCTRL, ITLB1 | FIFO_ENABLE | FIFO_RXCLR | FIFO_TXCLR);
  outb(COM1 + UART_MCTRL, M_OUT2 | M_DTR | M_RTS);
  outb(COM1 + UART_MCTRL, M_LOOP | M_OUT1 | M_DTR | M_RTS);

  outb(COM1, 0xAE);
  if (inb(COM1) != 0xAE) {
    print_init("srl", "initializing serial...", 1);
    return 1;
  }

  outb(COM1 + UART_MCTRL, M_OUT2 | M_DTR | M_RTS);
  // print_info("frq", 2, "baud rate: \e\x09%d baud * hz\e\x0f", UART_FREQ / div);

  outb(COM1 + UART_INT, INT_RXAVAIL);

  register_irq(serial_isr, 4);

  pic_cm(4);
  print_init("srl", "initializing serial...", 0);
  return 0;
}