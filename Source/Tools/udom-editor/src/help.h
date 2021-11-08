/** \file help.h
 *  \brief Header: hypertext file browser
 *
 *  Implements the hypertext file viewer.
 *  The hypertext file is a file that may have one or more nodes.  Each
 *  node ends with a ^D character and starts with a bracket, then the
 *  name of the node and then a closing bracket. Right after the closing
 *  bracket a newline is placed. This newline is not to be displayed by
 *  the help viewer and must be skipped - its sole purpose is to faciliate
 *  the work of the people managing the help file template (xnc.hlp) .
 *
 *  Links in the hypertext file are specified like this: the text that
 *  will be highlighted should have a leading ^A, then it comes the
 *  text, then a ^B indicating that highlighting is done, then the name
 *  of the node you want to link to and then a ^C.
 *
 *  The file must contain a ^D at the beginning and at the end of the
 *  file or the program will not be able to detect the end of file.
 *
 *  Lazyness/widgeting attack: This file does use the dialog manager
 *  and uses mainly the dialog to achieve the help work.  there is only
 *  one specialized widget and it's only used to forward the mouse messages
 *  to the appropriate routine.
 *
 *  This file is included by help.c and man2hlp.c
 */

#ifndef MC__HELP_H
#define MC__HELP_H

/*** typedefs(not structures) and defined constants **********************************************/

/* Markers used in the help files */
#define CHAR_LINK_START         '\01'   /* Ctrl-A */
#define CHAR_LINK_POINTER       '\02'   /* Ctrl-B */
#define CHAR_LINK_END           '\03'   /* Ctrl-C */
#define CHAR_NODE_END           '\04'   /* Ctrl-D */
#define CHAR_ALTERNATE          '\05'   /* Ctrl-E */
#define CHAR_NORMAL             '\06'   /* Ctrl-F */
#define CHAR_VERSION            '\07'   /* Ctrl-G */
#define CHAR_FONT_BOLD          '\010'  /* Ctrl-H */
#define CHAR_FONT_NORMAL        '\013'  /* Ctrl-K */
#define CHAR_FONT_ITALIC        '\024'  /* Ctrl-T */

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

gboolean help_interactive_display (const gchar * event_group_name, const gchar * event_name,
                                   gpointer init_data, gpointer data);

/*** inline functions ****************************************************************************/
#endif /* MC__HELP_H */
