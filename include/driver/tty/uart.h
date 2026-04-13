#ifndef UART_H
#define UART_H

#define COM1 0x3f8 // todo: NOT hardwire this but instead do the bda stuff

/* port offsets */
#define UART_RX    0x00 // receive buffer, read
#define UART_TX    0x00 // transmit buffer, write
#define UART_DIVLO 0x00 // dlab = 1

#define UART_DIVHI 0x01 // dlab = 1
#define UART_INT   0x01 // interrupt enable
#define UART_INTID 0x02 // interrupt id, read

#define UART_FCTRL 0x02 // fifo control, write
#define UART_LCTRL 0x03 // line control
#define UART_MCTRL 0x04 // modem control

#define UART_LSTAT 0x05 // line status, read
#define UART_MSTAT 0x06 // modem status, read
#define UART_SCRAT 0x07 // scratch

/* line control bit (masks) */
 
#define LCTRL_DATA 0x03
#define LCTRL_STOP 0x04
#define LCTRL_PART (0x07 << 3)
#define LCTRL_DLAB (0x01 << 7)

#define DATAB5 0x00
#define DATAB6 0x01
#define DATAB7 0x02
#define DATAB8 0x03

#define STOPB1  (0x00 << 2)
#define STOPB1h (0x01 << 2)

#define PAR_NONE  (0x00 << 3)
#define PAR_ODD   (0x01 << 3)
#define PAR_EVEN  (0x03 << 3)
#define PAR_MARK  (0x05 << 3)
#define PAR_SPACE (0x07 << 3)

/* interrupt enable bits */

#define INT_RXAVAIL 0x01
#define INT_TXEMPTY 0x02
#define INT_RXLSTAT 0x04
#define INT_MOSTATS 0x08 // modem status

/* FIFO control register bits */

#define FIFO_ENABLE 0x01
#define FIFO_RXCLR  0x02
#define FIFO_TXCLR  0x04
#define FIFO_DMA    0x08
#define FIFO_ITL    (3 << 6); // interrupt trigger level

#define ITLB1  (0x00 << 6)
#define ITLB4  (0x01 << 6)
#define ITLB8  (0x02 << 6)
#define ITLB14 (0x03 << 6)

/* interrupt identification */

#define INT_PENDING  0x01
#define INT_STATE    (3 << 1)
#define INT_TIMEOUT  0x08
#define INT_FIFOSTAT (3 << 6)

#define INTSTAT_MOD      0x00
#define INTSTAT_TXEMPTY  0x01
#define INTSTAT_RXAVAIL  0x02
#define INTSTAT_RECVSTAT 0x03

#define FIFOSTAT_NOF     0x00 // no fifo
#define FIFOSTAT_FBROKEN 0x01 // fifo unusable
#define FIFOSTAT_ENF     0x02 // fifo enabled

/* modem control */

#define M_DTR  (1 << 0)
#define M_RTS  (1 << 1)
#define M_OUT1 (1 << 2)
#define M_OUT2 (1 << 3)
#define M_LOOP (1 << 4)

/* line status register */

#define L_DR    (1 << 0) // data ready (to be read)
#define L_OE    (1 << 1) // overrun error
#define L_PE    (1 << 2) // parity error
#define L_FE    (1 << 3) // framing (stop bit) error
#define L_BI    (1 << 4) // break indicator
#define L_THRE  (1 << 5) // tx buffer is empty
#define L_TEMT  (1 << 6) // not doing any tx
#define L_IMPER (1 << 7) // impending doom

/* modem status reg */

#define MS_DCTS (1 << 0)
#define MS_DDSR (1 << 1)
#define MS_TERI (1 << 2)
#define MS_DDCD (1 << 3)
#define MS_CTS  (1 << 4)
#define MS_DSR  (1 << 5)
#define MS_RI   (1 << 6)
#define MS_DCD  (1 << 7)

#define UART_FREQ 115200

#endif