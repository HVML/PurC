
/** \file button.h
 *  \brief Header: WButton widget
 */

#ifndef MC__WIDGET_BUTTON_H
#define MC__WIDGET_BUTTON_H

/*** typedefs(not structures) and defined constants **********************************************/

#define BUTTON(x) ((WButton *)(x))

struct WButton;

/* button callback */
/* return 0 to continue work with dialog, non-zero to close */
typedef int (*bcback_fn) (struct WButton * button, int action);

/*** enums ***************************************************************************************/

typedef enum
{
    HIDDEN_BUTTON = 0,
    NARROW_BUTTON = 1,
    NORMAL_BUTTON = 2,
    DEFPUSH_BUTTON = 3
} button_flags_t;

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct WButton
{
    Widget widget;
    int action;                 /* what to do when pressed */

    button_flags_t flags;       /* button flags */
    hotkey_t text;              /* text of button, contain hotkey too */
    int hotpos;                 /* offset hot KEY char in text */
    bcback_fn callback;         /* callback function */
} WButton;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WButton *button_new (int y, int x, int action, button_flags_t flags, const char *text,
                     bcback_fn callback);
char *button_get_text (const WButton * b);
void button_set_text (WButton * b, const char *text);
int button_get_len (const WButton * b);

cb_ret_t button_default_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm,
                                  void *data);
void button_mouse_default_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_BUTTON_H */
