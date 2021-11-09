/** \file layout.h
 *  \brief Header: panel layout module
 */

#ifndef MC__LAYOUT_H
#define MC__LAYOUT_H

#include "lib/global.h"
#include "lib/widget.h"

#include "panel.h"

/*** typedefs(not structures) and defined constants **********************************************/

typedef enum
{
    view_listing = 0,           /* Directory listing */
    view_info = 1,              /* Information panel */
    view_tree = 2,              /* Tree view */
    view_quick = 3,             /* Quick view */
    view_nothing = 4,           /* Undefined */
} panel_view_mode_t;

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct
{
    gboolean horizontal_split;

    /* vertical split */
    gboolean vertical_equal;
    int left_panel_size;

    /* horizontal split */
    gboolean horizontal_equal;
    int top_panel_size;
} panels_layout_t;

/*** global variables defined in .c file *********************************************************/

extern int output_lines;
extern gboolean command_prompt;
extern gboolean menubar_visible;
extern int output_start_y;
extern gboolean xterm_title;
extern gboolean free_space;
extern gboolean nice_rotating_dash;

extern int ok_to_refresh;

extern panels_layout_t panels_layout;

/*** declarations of public functions ************************************************************/
void layout_change (void);
void layout_box (void);
void panel_update_cols (Widget * widget, panel_display_t frame_size);
void setup_panels (void);
void panels_split_equal (void);
void panels_split_more (void);
void panels_split_less (void);
void destroy_panels (void);
void setup_cmdline (void);
void create_panel (int num, panel_view_mode_t type);
void swap_panels (void);
panel_view_mode_t get_panel_type (int idx);
panel_view_mode_t get_current_type (void);
panel_view_mode_t get_other_type (void);
int get_current_index (void);
int get_other_index (void);
const char *get_nth_panel_name (int num);

Widget *get_panel_widget (int idx);

WPanel *get_other_panel (void);

void save_panel_dir (int idx);
char *get_panel_dir_for (const WPanel * widget);

void set_hintbar (const char *str);

/* Rotating dash routines */
void use_dash (gboolean flag);  /* Disable/Enable rotate_dash routines */
void rotate_dash (gboolean show);

gboolean do_load_prompt (void);
int load_prompt (int fd, void *unused);

void update_xterm_title_path (void);

void title_path_prepare (char **path, char **login);

/*** inline functions ****************************************************************************/

#endif /* MC__LAYOUT_H */
