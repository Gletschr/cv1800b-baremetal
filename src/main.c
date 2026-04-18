/*
 * main.c — application entry point.
 *
 * Initialises UART0 and prints "Hello, World!" over the debug serial port.
 * Wiring (Milk-V Duo 40-pin header, default pin function — no pinmux needed):
 *   GP12 = UART0 TX  →  connect to laptop RX
 *   GP13 = UART0 RX  →  connect to laptop TX
 */
#include "uart.h"

/*
 * UART0 source clock: 25 MHz crystal oscillator (XTAL).
 *
 * The Mask ROM initialises the PLLs for DDR but does not switch the UART
 * clock mux away from XTAL, so the peripheral still runs at 25 MHz when
 * our code starts.
 *
 * Baud rate divisor = round(25 000 000 / (16 × 115 200)) = 14
 * Actual baud rate  = 25 000 000 / (16 × 14) = 111 607 baud  (3.1 % error,
 * within the ±5 % tolerance that UART receivers are required to accept).
 */
#define UART_CLK_HZ  25000000u
#define UART_BAUD    115200u

void main(void)
{
    /*
     * The Mask ROM prints its own boot log over UART0 at an unknown baud
     * rate just before jumping to us.  We wait here for its TX FIFO to drain
     * completely so those bytes do not bleed into our output when we
     * reinitialise the UART below.
     *
     * At 25 MHz CPU speed, 10 000 000 nop iterations ≈ 400 ms — long enough
     * for any reasonable boot log to finish transmitting.
     */
    volatile unsigned int i = 10000000u;
    while (i--)
        __asm__ volatile("nop");

    uart_init(UART0_BASE, UART_CLK_HZ, UART_BAUD);
    uart_puts(UART0_BASE, "Hello, World!\r\n");
}
