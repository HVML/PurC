#ifndef MC__TTY_LINEMODE_H
#define MC__TTY_LINEMODE_H

/*
 * Initializes the tty for line mode.
 * Returns the current encoding of the terminal, and the size of the terminal.
 */
const char *tty_linemode_init(int *rows, int *cols);

/*
 * Shutdown the ttye line mode.
 */
void tty_linemode_shutdown(void);

#endif /* MC_TTY_LINEMODE_H */
