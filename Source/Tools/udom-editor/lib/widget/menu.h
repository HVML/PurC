/*
   Header file for pulldown menu engine for Midnignt Commander
 */

/** \file menu.h
 *  \brief Header: pulldown menu code
 */

#ifndef MC__WIDGET_MENU_H
#define MC__WIDGET_MENU_H

/*** typedefs(not structures) and defined constants **********************************************/

#define MENUBAR(x) ((WMenuBar *)(x))

#define menu_separator_create() NULL

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

struct menu_entry_t;
typedef struct menu_entry_t menu_entry_t;

struct menu_t;
typedef struct menu_t menu_t;

/* The button bar menu */
typedef struct WMenuBar
{
    Widget widget;

    gboolean is_dropped;        /* If the menubar has dropped */
    GList *menu;                /* The actual menus */
    guint selected;             /* Selected menu on the top bar */
    unsigned long previous_widget;      /* Selected widget ID before activating menu */
} WMenuBar;

/*** global variables defined in .c file *********************************************************/

extern const global_keymap_t *menu_map;

/*** declarations of public functions ************************************************************/

menu_entry_t *menu_entry_create (const char *name, long command);
void menu_entry_free (menu_entry_t * me);

menu_t *create_menu (const char *name, GList * entries, const char *help_node);
void menu_set_name (menu_t * menu, const char *name);
void destroy_menu (menu_t * menu);

WMenuBar *menubar_new (GList * menu);
void menubar_set_menu (WMenuBar * menubar, GList * menu);
void menubar_add_menu (WMenuBar * menubar, menu_t * menu);
void menubar_arrange (WMenuBar * menubar);

WMenuBar *find_menubar (const WDialog * h);

void menubar_activate (WMenuBar * menubar, gboolean dropped, int which);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_MENU_H */
