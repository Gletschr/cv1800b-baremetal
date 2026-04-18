/*
 * uart.h — NS16550-compatible UART driver for the CV1800B.
 *
 * The CV1800B has five UART controllers.  UART0 (base 0x04140000) is the
 * dedicated debug port, wired to GP12 (TX) and GP13 (RX) by default at
 * power-on — no pinmux configuration is required.
 *
 * Register layout: each logical register is 8 bits wide but occupies a
 * 32-bit word (4-byte stride, reg-shift = 2).  Offsets below are in bytes.
 *
 *  Offset  DLAB=0 name   DLAB=1 name   Direction
 *  0x00    THR           DLL           W  / W   (TX data / baud divisor low)
 *  0x00    RBR           DLL           R  / W   (RX data / baud divisor low)
 *  0x04    IER           DLH           RW / W   (interrupt enable / divisor high)
 *  0x08    FCR / IIR     —             W  / R   (FIFO control / interrupt ID)
 *  0x0C    LCR           —             RW       (line control; bit 7 = DLAB)
 *  0x14    LSR           —             R        (line status)
 */
#pragma once

#define UART0_BASE  0x04140000UL

/* Register accessors — base is an unsigned long holding the peripheral address. */
#define UART_THR(b)  (*(volatile unsigned int *)((b) + 0x00))  /* TX holding  (DLAB=0) */
#define UART_DLL(b)  (*(volatile unsigned int *)((b) + 0x00))  /* divisor low (DLAB=1) */
#define UART_DLH(b)  (*(volatile unsigned int *)((b) + 0x04))  /* divisor hi  (DLAB=1) */
#define UART_FCR(b)  (*(volatile unsigned int *)((b) + 0x08))  /* FIFO control (W only) */
#define UART_LCR(b)  (*(volatile unsigned int *)((b) + 0x0C))  /* line control */
#define UART_LSR(b)  (*(volatile unsigned int *)((b) + 0x14))  /* line status  (R only) */

/* LCR bit fields */
#define LCR_DLAB     (1u << 7)  /* enable divisor latch registers at 0x00/0x04 */
#define LCR_8N1      0x03u      /* 8 data bits, no parity, 1 stop bit; DLAB=0  */

/* LSR bit fields */
#define LSR_TX_EMPTY (1u << 5)  /* TX holding register empty — safe to write THR */

/*
 * uart_init  — configure baud rate and frame format, enable the TX/RX FIFOs.
 *   base    : peripheral base address (e.g. UART0_BASE)
 *   clk_hz  : UART source clock in Hz (25 000 000 for XTAL on CV1800B)
 *   baud    : desired baud rate (e.g. 115200)
 */
void uart_init(unsigned long base, unsigned int clk_hz, unsigned int baud);

/* uart_putc — transmit a single character, blocking until the FIFO has room. */
void uart_putc(unsigned long base, char c);

/* uart_puts — transmit a NUL-terminated string character by character. */
void uart_puts(unsigned long base, const char *s);
