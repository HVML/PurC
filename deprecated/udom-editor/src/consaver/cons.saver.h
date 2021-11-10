/** \file cons.saver.h
 *  \brief Header: general purpose Linux console screen save/restore server
 *
 *  This code does _not_ need to be setuid root. However, it needs
 *  read/write access to /dev/vcsa* (which is priviledged
 *  operation). You should create user vcsa, make cons.saver setuid
 *  user vcsa, and make all vcsa's owned by user vcsa.
 *  Seeing other peoples consoles is bad thing, but believe me, full
 *  root is even worse.
 */

#ifndef MC__CONS_SAVER_H
#define MC__CONS_SAVER_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

typedef enum
{
    CONSOLE_INIT = '1',
    CONSOLE_DONE,
    CONSOLE_SAVE,
    CONSOLE_RESTORE,
    CONSOLE_CONTENTS
} console_action_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

#ifndef LINUX_CONS_SAVER_C
/* Used only in mc, not in cons.saver */
extern int cons_saver_pid;
#endif /* !LINUX_CONS_SAVER_C */

/*** declarations of public functions ************************************************************/

#ifndef LINUX_CONS_SAVER_C
/* Used only in mc, not in cons.saver */
void show_console_contents (int starty, unsigned char begin_line, unsigned char end_line);
void handle_console (console_action_t action);
#endif /* !LINUX_CONS_SAVER_C */

/*** inline functions ****************************************************************************/
#endif /* MC__CONS_SAVER_H */
