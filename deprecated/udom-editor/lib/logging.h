/** \file logging.h
 *  \brief Header: provides a log file to ease tracing the program
 */

#ifndef MC_LOGGING_H
#define MC_LOGGING_H

/*
   This file provides an easy-to-use function for writing all kinds of
   events into a central log file that can be used for debugging.
 */

/*** typedefs(not structures) and defined constants **********************************************/

#define mc_log_mark() mc_log("%s:%d\n",__FILE__,__LINE__)

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/* *INDENT-OFF* */
void mc_log (const char *fmt, ...) G_GNUC_PRINTF (1, 2);
void mc_always_log (const char *fmt, ...) G_GNUC_PRINTF (1, 2);
/* *INDENT-ON* */

/*** inline functions ****************************************************************************/

#endif
