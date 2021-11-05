#ifndef MC_SKIN_H
#define MC_SKIN_H

#include "lib/global.h"

#include "lib/mcconfig.h"

#include "lib/tty/color.h"

/*** typedefs(not structures) and defined constants **********************************************/

/* Beware! When using Slang with color, not all the indexes are free.
   See color-slang.h (A_*) */

/* cache often used colors */
#define DEFAULT_COLOR             mc_skin_color__cache[0]
#define NORMAL_COLOR              mc_skin_color__cache[1]
#define MARKED_COLOR              mc_skin_color__cache[2]
#define SELECTED_COLOR            mc_skin_color__cache[3]
#define MARKED_SELECTED_COLOR     mc_skin_color__cache[4]
#define DISABLED_COLOR            mc_skin_color__cache[5]
#define REVERSE_COLOR             mc_skin_color__cache[6]
#define COMMAND_MARK_COLOR        mc_skin_color__cache[7]
#define HEADER_COLOR              mc_skin_color__cache[8]
#define SHADOW_COLOR              mc_skin_color__cache[9]

/* Dialog colors */
#define COLOR_NORMAL              mc_skin_color__cache[10]
#define COLOR_FOCUS               mc_skin_color__cache[11]
#define COLOR_HOT_NORMAL          mc_skin_color__cache[12]
#define COLOR_HOT_FOCUS           mc_skin_color__cache[13]
#define COLOR_TITLE               mc_skin_color__cache[14]

/* Error dialog colors */
#define ERROR_COLOR               mc_skin_color__cache[15]
#define ERROR_FOCUS               mc_skin_color__cache[16]
#define ERROR_HOT_NORMAL          mc_skin_color__cache[17]
#define ERROR_HOT_FOCUS           mc_skin_color__cache[18]
#define ERROR_TITLE               mc_skin_color__cache[19]

/* Menu colors */
#define MENU_ENTRY_COLOR          mc_skin_color__cache[20]
#define MENU_SELECTED_COLOR       mc_skin_color__cache[21]
#define MENU_HOT_COLOR            mc_skin_color__cache[22]
#define MENU_HOTSEL_COLOR         mc_skin_color__cache[23]
#define MENU_INACTIVE_COLOR       mc_skin_color__cache[24]

/* Popup menu colors */
#define PMENU_ENTRY_COLOR         mc_skin_color__cache[25]
#define PMENU_SELECTED_COLOR      mc_skin_color__cache[26]
#define PMENU_HOT_COLOR           mc_skin_color__cache[27]      /* unused: not implemented yet */
#define PMENU_HOTSEL_COLOR        mc_skin_color__cache[28]      /* unused: not implemented yet */
#define PMENU_TITLE_COLOR         mc_skin_color__cache[29]

#define BUTTONBAR_HOTKEY_COLOR    mc_skin_color__cache[30]
#define BUTTONBAR_BUTTON_COLOR    mc_skin_color__cache[31]

#define STATUSBAR_COLOR           mc_skin_color__cache[32]

/*
 * This should be selectable independently. Default has to be black background
 * foreground does not matter at all.
 */
#define GAUGE_COLOR               mc_skin_color__cache[33]
#define INPUT_COLOR               mc_skin_color__cache[34]
#define INPUT_UNCHANGED_COLOR     mc_skin_color__cache[35]
#define INPUT_MARK_COLOR          mc_skin_color__cache[36]
#define INPUT_HISTORY_COLOR       mc_skin_color__cache[37]
#define COMMAND_HISTORY_COLOR     mc_skin_color__cache[38]

#define HELP_NORMAL_COLOR         mc_skin_color__cache[39]
#define HELP_ITALIC_COLOR         mc_skin_color__cache[40]
#define HELP_BOLD_COLOR           mc_skin_color__cache[41]
#define HELP_LINK_COLOR           mc_skin_color__cache[42]
#define HELP_SLINK_COLOR          mc_skin_color__cache[43]
#define HELP_TITLE_COLOR          mc_skin_color__cache[44]


#define VIEW_NORMAL_COLOR         mc_skin_color__cache[45]
#define VIEW_BOLD_COLOR           mc_skin_color__cache[46]
#define VIEW_UNDERLINED_COLOR     mc_skin_color__cache[47]
#define VIEW_SELECTED_COLOR       mc_skin_color__cache[48]

/*
 * editor colors - only 4 for normal, search->found, select, and whitespace
 * respectively
 * Last is defined to view color.
 */
#define EDITOR_NORMAL_COLOR       mc_skin_color__cache[49]
#define EDITOR_BOLD_COLOR         mc_skin_color__cache[50]
#define EDITOR_MARKED_COLOR       mc_skin_color__cache[51]
#define EDITOR_WHITESPACE_COLOR   mc_skin_color__cache[52]
#define EDITOR_RIGHT_MARGIN_COLOR mc_skin_color__cache[53]
#define EDITOR_BACKGROUND         mc_skin_color__cache[54]
#define EDITOR_FRAME              mc_skin_color__cache[55]
#define EDITOR_FRAME_ACTIVE       mc_skin_color__cache[56]
#define EDITOR_FRAME_DRAG         mc_skin_color__cache[57]
/* color of left 8 char status per line */
#define LINE_STATE_COLOR          mc_skin_color__cache[58]
#define BOOK_MARK_COLOR           mc_skin_color__cache[59]
#define BOOK_MARK_FOUND_COLOR     mc_skin_color__cache[60]

/* Diff colors */
#define DFF_ADD_COLOR             mc_skin_color__cache[61]
#define DFF_CHG_COLOR             mc_skin_color__cache[62]
#define DFF_CHH_COLOR             mc_skin_color__cache[63]
#define DFF_CHD_COLOR             mc_skin_color__cache[64]
#define DFF_DEL_COLOR             mc_skin_color__cache[65]
#define DFF_ERROR_COLOR           mc_skin_color__cache[66]

#define MC_SKIN_COLOR_CACHE_COUNT 67

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct mc_skin_struct
{
    gchar *name;
    gchar *description;
    mc_config_t *config;
    GHashTable *colors;
    gboolean have_256_colors;
    gboolean have_true_colors;
} mc_skin_t;

/*** global variables defined in .c file *********************************************************/

extern int mc_skin_color__cache[];
extern mc_skin_t mc_skin__default;

/*** declarations of public functions ************************************************************/

gboolean mc_skin_init (const gchar * skin_override, GError ** error);
void mc_skin_deinit (void);

int mc_skin_color_get (const gchar * group, const gchar * name);

void mc_skin_lines_parse_ini_file (mc_skin_t * mc_skin);

gchar *mc_skin_get (const gchar * group, const gchar * key, const gchar * default_value);

GPtrArray *mc_skin_list (void);

#endif /* MC_SKIN_H */
