/*
 * term.h — simple command terminal over UART.
 */
#pragma once

/* Start the terminal: print the boot banner then enter the command loop forever. */
void term_run(void);
