/*
 * term.c — simple command terminal over UART.
 *
 * Provides a readline loop with echo and backspace support, basic argument
 * parsing (including double-quoted strings), and a small command dispatch
 * table.  All I/O goes through UART0.
 *
 * Adding a new command:
 *   1. Write a handler: static void cmd_foo(int argc, char *argv[])
 *   2. Add an else-if branch in dispatch() below.
 */
#include "term.h"
#include "uart.h"

#define PROMPT    "duo> "
#define LINE_MAX  128     /* maximum input line length including NUL */
#define ARGS_MAX  16      /* maximum number of tokens per line       */

/* -------------------------------------------------------------------------
 * String helpers (no libc available in freestanding builds)
 * ---------------------------------------------------------------------- */

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

/* -------------------------------------------------------------------------
 * Command handlers
 * ---------------------------------------------------------------------- */

/*
 * echo — print all arguments separated by spaces.
 * Usage: echo [text ...]
 *        echo "text with spaces"
 *
 * Quoted arguments have their quotes stripped by the parser before we see
 * them, so `echo "Hello World"` and `echo Hello World` both reach us as
 * two separate argv entries when unquoted, or one entry when quoted.
 */
static void cmd_echo(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++) {
        if (i > 1)
            uart_putc(UART0_BASE, ' ');
        uart_puts(UART0_BASE, argv[i]);
    }
    uart_puts(UART0_BASE, "\r\n");
}

/*
 * help — list available commands.
 */
static void cmd_help(int argc, char *argv[])
{
    (void)argc; (void)argv;
    uart_puts(UART0_BASE,
        "Available commands:\r\n"
        "  echo [text ...]   print arguments to the terminal\r\n"
        "  help              show this message\r\n"
    );
}

/* -------------------------------------------------------------------------
 * Command dispatch
 * ---------------------------------------------------------------------- */

static void dispatch(int argc, char *argv[])
{
    if (argc == 0)
        return;

    if      (str_eq(argv[0], "echo")) cmd_echo(argc, argv);
    else if (str_eq(argv[0], "help")) cmd_help(argc, argv);
    else {
        uart_puts(UART0_BASE, argv[0]);
        uart_puts(UART0_BASE, ": command not found (try 'help')\r\n");
    }
}

/* -------------------------------------------------------------------------
 * Argument parser
 *
 * Splits a mutable line buffer into tokens in-place (inserts NUL bytes).
 * Supports double-quoted strings: `"hello world"` becomes one token with
 * the quotes stripped.  Returns the number of tokens found.
 * ---------------------------------------------------------------------- */

static int parse_args(char *line, char *argv[], int max_args)
{
    int   argc = 0;
    char *p    = line;

    while (*p) {
        /* skip whitespace between tokens */
        while (*p == ' ' || *p == '\t')
            p++;

        if (!*p || argc >= max_args)
            break;

        if (*p == '"') {
            /* quoted token — strip the surrounding quotes */
            p++;
            argv[argc++] = p;
            while (*p && *p != '"')
                p++;
            if (*p)
                *p++ = '\0';  /* overwrite closing quote with NUL */
        } else {
            /* unquoted token — ends at the next whitespace */
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t')
                p++;
            if (*p)
                *p++ = '\0';  /* overwrite space with NUL */
        }
    }

    return argc;
}

/* -------------------------------------------------------------------------
 * Line reader
 *
 * Reads characters from UART until Enter is pressed, echoing each one back
 * so the user can see what they typed.  Handles backspace/DEL for editing.
 * Returns the number of characters in buf (excluding the NUL terminator).
 * ---------------------------------------------------------------------- */

static int readline(char *buf, int max)
{
    int len = 0;

    for (;;) {
        char c = uart_getc(UART0_BASE);

        if (c == '\r' || c == '\n') {
            /* Enter pressed — finish the line */
            uart_puts(UART0_BASE, "\r\n");
            buf[len] = '\0';
            return len;
        }

        if ((c == '\b' || c == 0x7F) && len > 0) {
            /* Backspace / DEL — erase the last character on screen:
             * move cursor back one, print a space to blank it, move back again. */
            uart_puts(UART0_BASE, "\b \b");
            len--;
            continue;
        }

        if (c >= 0x20 && c < 0x7F && len < max - 1) {
            /* Printable ASCII — store and echo */
            buf[len++] = c;
            uart_putc(UART0_BASE, c);
        }
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void term_run(void)
{
    char  line[LINE_MAX];
    char *argv[ARGS_MAX];
    int   argc;

    uart_puts(UART0_BASE,
        "\r\n"
        "  Milk-V Duo v1.2\r\n"
        "  CPU : T-Head C906  RV64IMAC  1 GHz\r\n"
        "  RAM : 64 MB DDR\r\n"
        "\r\n"
    );

    for (;;) {
        uart_puts(UART0_BASE, PROMPT);
        readline(line, LINE_MAX);
        argc = parse_args(line, argv, ARGS_MAX);
        dispatch(argc, argv);
    }
}
