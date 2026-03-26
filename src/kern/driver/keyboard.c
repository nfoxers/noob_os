#include "driver/keyboard.h"
#include "cpu/idt.h"
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

volatile uint8_t kbd_flg = 0;

volatile uint8_t kbd_buffer[K_BUFSIZ];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

uint8_t special_depress(uint8_t scan) {
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

  DP(K_RSHIFT);
  DP(K_LSHIFT);
  DP(K_LCTRL);

  SP(K_SPECIAl);
  SP(K_S_PGDOWN);
  SP(K_S_PGUP);
  return 0;
}

uint8_t parse_char(uint8_t scan) {
  if (scan > sizeof(scancode_map) && !special_depress(scan)) {
    return 0;
  }

  if (kbd_flg & F_SPCL) {
    kbd_flg &= ~F_SPCL;

    if (scan == 0x49) {
      vscroll_up();
      return 0;
    }

    if (scan == 0x51) {
      vscroll_down();
      return 0;
    }
  }

  if (scan == K_LSHIFT || scan == K_RSHIFT) {
    kbd_flg |= F_SHIFT;
    return 0;
  }

  if (scan == (0x80 | K_LSHIFT) || scan == (0x80 | K_RSHIFT)) {
    kbd_flg &= ~F_SHIFT;
    return 0;
  }

  if (scan == K_LCTRL) {
    kbd_flg |= F_CTRL;
    return 0;
  }

  if (scan == (0x80 | K_LCTRL)) {
    kbd_flg &= ~F_CTRL;
    return 0;
  }

  if (scan == K_SPECIAl) {
    kbd_flg |= F_SPCL;
    return 0;
  }

  if (scan == K_ENTER) {
    return '\n';
  }

  if (scan == K_BKSPACE) {
    return '\b';
  }

  if (scan == K_TAB) {
    return ' ';
  }

  if (kbd_flg & F_SHIFT) {
    return scancode_map_shift[scan];
  }

  if (kbd_flg & F_CTRL) {
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
    uint8_t next = (head + 1) % K_BUFSIZ;

    kbd_buffer[head] = ch;

    head = next;
  }

  pic_eoi();
}

void init_kbd() {
  register_irq(kbd_handler, 1);
  pic_cm(1);
}

uint8_t getch() {
  while (head == tail)
    ;

  CLI;
  uint8_t c = kbd_buffer[tail];

  tail = (tail + 1) % K_BUFSIZ;
  STI;
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