/*
 * uart.c — NS16550-compatible UART driver for the CV1800B.
 */
#include "uart.h"

/*
 * uart_init — set baud rate and frame format, enable TX/RX FIFOs.
 *
 * NS16550 baud rate programming sequence:
 *   1. Set DLAB (bit 7 of LCR) to expose the divisor latch registers.
 *   2. Write the 16-bit divisor: DLL = low byte, DLH = high byte.
 *      divisor = round(clk_hz / (16 × baud))
 *   3. Clear DLAB and set the desired frame format in LCR.
 *   4. Enable and reset the TX/RX FIFOs via FCR.
 */
void uart_init(unsigned long base, unsigned int clk_hz, unsigned int baud)
{
    /* Round to nearest integer to minimise baud rate error. */
    unsigned int div = (clk_hz + 8u * baud) / (16u * baud);

    UART_LCR(base) = LCR_DLAB;          /* unlock divisor registers    */
    UART_DLL(base) = div & 0xFFu;       /* divisor low byte            */
    UART_DLH(base) = (div >> 8) & 0xFFu;/* divisor high byte           */
    UART_LCR(base) = LCR_8N1;           /* 8-N-1, clears DLAB          */
    UART_FCR(base) = 0x01u;             /* enable TX and RX FIFOs      */
}

/*
 * uart_putc — transmit one character.
 *
 * Spins on LSR[5] (TX holding register empty) until the FIFO has room,
 * then writes the character to the TX holding register.
 */
void uart_putc(unsigned long base, char c)
{
    while (!(UART_LSR(base) & LSR_TX_EMPTY))
        ;
    UART_THR(base) = (unsigned char)c;
}

/*
 * uart_puts — transmit a NUL-terminated string.
 */
void uart_puts(unsigned long base, const char *s)
{
    while (*s)
        uart_putc(base, *s++);
}
