/*
 * main.c — application entry point.
 *
 * Initialises UART0 and starts the interactive terminal.
 * Serial port wiring (Milk-V Duo 40-pin header, default pin function):
 *   GP12 = UART0 TX  →  connect to laptop RX
 *   GP13 = UART0 RX  →  connect to laptop TX
 */
#include "uart.h"
#include "term.h"

/*
 * UART0 source clock: 25 MHz crystal oscillator (XTAL).
 * The Mask ROM does not switch the UART clock mux away from XTAL.
 *
 * Baud rate divisor = round(25 000 000 / (16 × 115 200)) = 14
 * Actual baud rate  = 25 000 000 / (16 × 14) = 111 607 baud (3.1 % error).
 */
#define UART_CLK_HZ  25000000u
#define UART_BAUD    115200u

void main(void)
{
    /*
     * The Mask ROM prints its boot log over UART0 just before jumping here.
     * Wait for its TX FIFO to drain before we reinitialise the peripheral,
     * otherwise the last bytes of the boot log corrupt our banner output.
     * At 25 MHz CPU speed, 10 000 000 nop iterations ≈ 400 ms.
     */
    volatile unsigned int i = 10000000u;
    while (i--)
        __asm__ volatile("nop");

    uart_init(UART0_BASE, UART_CLK_HZ, UART_BAUD);
    term_run();
}
