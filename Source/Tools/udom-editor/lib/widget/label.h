
/** \file label.h
 *  \brief Header: WLabel widget
 */

#ifndef MC__WIDGET_LABEL_H
#define MC__WIDGET_LABEL_H

/*** typedefs(not structures) and defined constants **********************************************/

#define LABEL(x) ((WLabel *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    Widget widget;
    gboolean auto_adjust_cols;  /* compute widget.cols from strlen(text)? */
    char *text;
    gboolean transparent;       /* Paint in the default color fg/bg */
} WLabel;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WLabel *label_new (int y, int x, const char *text);
void label_set_text (WLabel * label, const char *text);
/* *INDENT-OFF* */
void label_set_textv (WLabel * label, const char *format, ...) G_GNUC_PRINTF (2, 3);
/* *INDENT-ON* */

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_LABEL_H */
