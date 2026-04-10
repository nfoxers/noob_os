#include "driver/keyboard.h"
#include "cpu/idt.h"
#include "lib/errno.h"
#include "syscall/syscall.h"
#include "video/printf.h"
#include "video/video.h"
#include <stdint.h>

#define K_SPECIAl 0xe0

#define K_S_PGDOWN 0x51
#define K_S_PGUP   0x49

#define K_ESC     0x01
#define K_BKSPACE 0x0e
#define K_TAB     0x0f
#define K_ENTER   0x1c
#define K_LCTRL   0x1d
#define K_LSHIFT  0x2A
#define K_RSHIFT  0x36
#define K_LALT    0x38

#define F_SHIFT 0x01
#define F_CTRL  0x02
#define F_SPCL  0x04

#define K_BUFSIZ 128

const uint8_t scancode_map[] = {
    000, 000, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 000,
    000, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 000, 000,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 000, '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 000, '*', 000, ' ', 000};

const uint8_t scancode_map_shift[] = {
    000, 000, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 000,
    000, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 000, 000,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 000, '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 000, '*', 000, ' ', 000};

volatile struct {
  uint8_t k_shift;
  uint8_t k_ctrl;
  uint8_t k_spcl;
} kbd_ctrl = {0};

volatile uint8_t kbd_buffer[K_BUFSIZ];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

uint8_t special_press(uint8_t scan) {
#define DP(k)                 \
  {                           \
    if (scan == ((k) | 0x80)) \
      return 1;               \
  }
#define SP(k)        \
  {                  \
    if (scan == (k)) \
      return 1;      \
  }

  SP(K_S_PGDOWN);
  SP(K_S_PGUP);
  return 0;
}

static int k_stdin  = 0;
static int k_stdout = 1;

uint8_t parse_char(uint8_t scan) {
  if (scan == K_SPECIAl) {
    kbd_ctrl.k_spcl++;
    return 0;
  }

  int press = !((scan & 0x80) >> 7);
  scan      = scan & 0x7f;

  if (scan > sizeof(scancode_map) && !special_press(scan)) {
    return 0;
  }

  if (kbd_ctrl.k_spcl) {
    kbd_ctrl.k_spcl &= 0;

    if (!press)
      return 0;

    if (scan == 0x49) {
      vscroll_up();
      return 0;
    }

    if (scan == 0x51) {
      vscroll_down();
      return 0;
    }
  }

  switch (scan) {
  case K_LSHIFT:
  case K_RSHIFT:
    kbd_ctrl.k_shift = press;
    return 0;
  case K_LCTRL:
    kbd_ctrl.k_ctrl = press;
    return 0;
  case K_ENTER:
    if (press)
      return '\n';
  case K_BKSPACE:
    if (press)
      return '\b';
  case K_TAB:
    if (press)
      return ' ';
  }

  if (!press)
    return 0;

  if (kbd_ctrl.k_shift) {
    return scancode_map_shift[scan];
  }

  if (kbd_ctrl.k_ctrl) {
    return scancode_map_shift[scan] - 'A' + 1; // utter genius.
  }

  return scancode_map[scan];
}

static void kbd_handler(struct regs *r) {
  (void)r;
  uint8_t scan = inb(0x60);
  // printkf("%02x ", scan);
  uint8_t ch = parse_char(scan);

  if (ch) {
    // uint8_t next = (head + 1) % K_BUFSIZ;
    // kbd_buffer[head] = ch;
    // head = next;
    if (write(k_stdout, &ch, 1) == -1) {
      perror("kbd: write");
    }
  }

  pic_eoi();
}

void init_ps2() {
  outb(0x64, 0xad); // disable devs
  outb(0x64, 0xa7);
  for (int i = 0; i < 10; i++)
    inb(0x60); // flush

  outb(0x64, 0x20);
  uint8_t ccb = inb(0x60);
  ccb         = 0b01000100;
  outb(0x64, 0x60);
  outb(0x60, ccb);

  outb(0x64, 0xaa); // self test
  uint8_t ret = inb(0x60);
  if (ret != 0x55) {
    printkf("ps/2 cont error %x %x\n", ret, inb(0x60));
    while (1)
      asm volatile("hlt");
  }

  outb(0x64, 0xae); // enable again
  outb(0x64, 0xa8);

  ccb = 0b01000101;
  outb(0x64, 0x60);
  outb(0x60, ccb);
}

int init_kbd() {

  int fd[2];
  if (pipe(fd) == -1) {
    print_init("kbd", "initializing the keyboard...", 1);
    return 1;
  }

  k_stdout = fd[1];
  k_stdin  = fd[0];

  init_ps2();
  register_irq(kbd_handler, 1);
  pic_cm(1);
  print_init("kbd", "intializing the keyboard...", 0);
  return 0;
}

uint8_t getch() {
  /*
  while (head == tail)
    ;

  CLI;
  uint8_t c = kbd_buffer[tail];

  tail = (tail + 1) % K_BUFSIZ;
  STI;
  */

  int c = 0;
  int r;
retry:
  r = read(k_stdin, &c, 1);
  if (r == -1) {
    if (errno == EAGAIN) {
      goto retry;
    }
    perror("read");
  }

  return c;
}

int kgets(char *s, uint16_t siz) {
  char     c   = 0;
  uint16_t idx = 0;
  while (((c = getch()) != '\n') && (c != EOF)) {
    if (idx >= siz)
      break;
    if (c != '\b') {
      s[idx++] = c;
      putchr(c);
    } else if (c == '\b' && idx > 0) {
      putchr('\b');
      idx--;
    }
  }
  s[idx] = 0;
  return idx;
}