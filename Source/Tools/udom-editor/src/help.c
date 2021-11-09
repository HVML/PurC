/*
   Hypertext file browser.

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/** \file help.c
 *  \brief Source: hypertext file browser
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
 */


#include <config.h>

#include <errno.h>
#include <limits.h>             /* MB_LEN_MAX */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/skin.h"
#include "lib/strutil.h"
#include "lib/fileloc.h"
#include "lib/util.h"
#include "lib/widget.h"
#include "lib/event-types.h"

#include "keymap.h"
#include "help.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define MAXLINKNAME 80
#define HISTORY_SIZE 20
#define HELP_WINDOW_WIDTH MIN(80, COLS - 16)

#define STRING_LINK_START       "\01"
#define STRING_LINK_POINTER     "\02"
#define STRING_LINK_END         "\03"
#define STRING_NODE_END         "\04"

/*** file scope type declarations ****************************************************************/

/* Link areas for the mouse */
typedef struct Link_Area
{
    int x1, y1, x2, y2;
    const char *link_name;
} Link_Area;

/*** file scope variables ************************************************************************/

static char *fdata = NULL;      /* Pointer to the loaded data file */
static int help_lines;          /* Lines in help viewer */
static int history_ptr;         /* For the history queue */
static const char *main_node;   /* The main node */
static const char *last_shown = NULL;   /* Last byte shown in a screen */
static gboolean end_of_node = FALSE;    /* Flag: the last character of the node shown? */
static const char *currentpoint;
static const char *selected_item;

/* The widget variables */
static WDialog *whelp;

static struct
{
    const char *page;           /* Pointer to the selected page */
    const char *link;           /* Pointer to the selected link */
} history[HISTORY_SIZE];

static GSList *link_area = NULL;
static gboolean inside_link_area = FALSE;

static cb_ret_t help_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data);

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/** returns the position where text was found in the start buffer 
 * or 0 if not found
 */
static const char *
search_string (const char *start, const char *text)
{
    const char *result = NULL;
    char *local_text = g_strdup (text);
    char *d = local_text;
    const char *e = start;

    /* fmt sometimes replaces a space with a newline in the help file */
    /* Replace the newlines in the link name with spaces to correct the situation */
    while (*d != '\0')
    {
        if (*d == '\n')
            *d = ' ';
        str_next_char (&d);
    }

    /* Do search */
    for (d = local_text; *e; e++)
    {
        if (*d == *e)
            d++;
        else
            d = local_text;
        if (*d == '\0')
        {
            result = e + 1;
            break;
        }
    }

    g_free (local_text);
    return result;
}

/* --------------------------------------------------------------------------------------------- */
/** Searches text in the buffer pointed by start.  Search ends
 * if the CHAR_NODE_END is found in the text.
 * @return NULL on failure
 */

static const char *
search_string_node (const char *start, const char *text)
{
    const char *d = text;
    const char *e = start;

    if (start != NULL)
        for (; *e && *e != CHAR_NODE_END; e++)
        {
            if (*d == *e)
                d++;
            else
                d = text;
            if (*d == '\0')
                return e + 1;
        }

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */
/** Searches the_char in the buffer pointer by start and searches
 * it can search forward (direction = 1) or backward (direction = -1)
 */

static const char *
search_char_node (const char *start, char the_char, int direction)
{
    const char *e;

    for (e = start; (*e != '\0') && (*e != CHAR_NODE_END); e += direction)
        if (*e == the_char)
            return e;

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */
/** Returns the new current pointer when moved lines lines */

static const char *
move_forward2 (const char *c, int lines)
{
    const char *p;
    int line;

    currentpoint = c;
    for (line = 0, p = currentpoint; (*p != '\0') && (*p != CHAR_NODE_END); str_cnext_char (&p))
    {
        if (line == lines)
            return currentpoint = p;

        if (*p == '\n')
            line++;
    }
    return currentpoint = c;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
move_backward2 (const char *c, int lines)
{
    const char *p;
    int line;

    currentpoint = c;
    for (line = 0, p = currentpoint; (*p != '\0') && ((int) (p - fdata) >= 0); str_cprev_char (&p))
    {
        if (*p == CHAR_NODE_END)
        {
            /* We reached the beginning of the node */
            /* Skip the node headers */
            while (*p != ']')
                str_cnext_char (&p);
            return currentpoint = p + 2;        /* Skip the newline following the start of the node */
        }

        if (*(p - 1) == '\n')
            line++;
        if (line == lines)
            return currentpoint = p;
    }
    return currentpoint = c;
}

/* --------------------------------------------------------------------------------------------- */

static void
move_forward (int i)
{
    if (!end_of_node)
        currentpoint = move_forward2 (currentpoint, i);
}

/* --------------------------------------------------------------------------------------------- */

static void
move_backward (int i)
{
    currentpoint = move_backward2 (currentpoint, ++i);
}

/* --------------------------------------------------------------------------------------------- */

static void
move_to_top (void)
{
    while (((int) (currentpoint > fdata) > 0) && (*currentpoint != CHAR_NODE_END))
        currentpoint--;

    while (*currentpoint != ']')
        currentpoint++;
    currentpoint = currentpoint + 2;    /* Skip the newline following the start of the node */
    selected_item = NULL;
}

/* --------------------------------------------------------------------------------------------- */

static void
move_to_bottom (void)
{
    while ((*currentpoint != '\0') && (*currentpoint != CHAR_NODE_END))
        currentpoint++;
    currentpoint--;
    move_backward (1);
}

/* --------------------------------------------------------------------------------------------- */

static const char *
help_follow_link (const char *start, const char *lc_selected_item)
{
    const char *p;

    if (lc_selected_item == NULL)
        return start;

    for (p = lc_selected_item; *p && *p != CHAR_NODE_END && *p != CHAR_LINK_POINTER; p++)
        ;
    if (*p == CHAR_LINK_POINTER)
    {
        int i;
        char link_name[MAXLINKNAME];

        link_name[0] = '[';
        for (i = 1; *p != CHAR_LINK_END && *p && *p != CHAR_NODE_END && i < MAXLINKNAME - 3;)
            link_name[i++] = *++p;
        link_name[i - 1] = ']';
        link_name[i] = '\0';
        p = search_string (fdata, link_name);
        if (p != NULL)
        {
            p += 1;             /* Skip the newline following the start of the node */
            return p;
        }
    }

    /* Create a replacement page with the error message */
    return _("Help file format error\n");
}

/* --------------------------------------------------------------------------------------------- */

static const char *
select_next_link (const char *current_link)
{
    const char *p;

    if (current_link == NULL)
        return NULL;

    p = search_string_node (current_link, STRING_LINK_END);
    if (p == NULL)
        return NULL;
    p = search_string_node (p, STRING_LINK_START);
    if (p == NULL)
        return NULL;
    return p - 1;
}

/* --------------------------------------------------------------------------------------------- */

static const char *
select_prev_link (const char *current_link)
{
    return current_link == NULL ? NULL : search_char_node (current_link - 1, CHAR_LINK_START, -1);
}

/* --------------------------------------------------------------------------------------------- */

static void
start_link_area (int x, int y, const char *link_name)
{
    Link_Area *la;

    if (inside_link_area)
        message (D_NORMAL, _("Warning"), "%s", _("Internal bug: Double start of link area"));

    /* Allocate memory for a new link area */
    la = g_new (Link_Area, 1);
    /* Save the beginning coordinates of the link area */
    la->x1 = x;
    la->y1 = y;
    /* Save the name of the destination anchor */
    la->link_name = link_name;
    link_area = g_slist_prepend (link_area, la);

    inside_link_area = TRUE;
}

/* --------------------------------------------------------------------------------------------- */

static void
end_link_area (int x, int y)
{
    if (inside_link_area)
    {
        Link_Area *la = (Link_Area *) link_area->data;
        /* Save the end coordinates of the link area */
        la->x2 = x;
        la->y2 = y;
        inside_link_area = FALSE;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
clear_link_areas (void)
{
    g_clear_slist (&link_area, g_free);
    inside_link_area = FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static void
help_print_word (WDialog * h, GString * word, int *col, int *line, gboolean add_space)
{
    if (*line >= help_lines)
        g_string_set_size (word, 0);
    else
    {
        int w;

        w = str_term_width1 (word->str);
        if (*col + w >= HELP_WINDOW_WIDTH)
        {
            *col = 0;
            (*line)++;
        }

        if (*line >= help_lines)
            g_string_set_size (word, 0);
        else
        {
            widget_gotoyx (h, *line + 2, *col + 2);
            tty_print_string (word->str);
            g_string_set_size (word, 0);
            *col += w;
        }
    }

    if (add_space)
    {
        if (*col < HELP_WINDOW_WIDTH - 1)
        {
            tty_print_char (' ');
            (*col)++;
        }
        else
        {
            *col = 0;
            (*line)++;
        }
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
help_show (WDialog * h, const char *paint_start)
{
    const char *p, *n;
    int col, line, c;
    gboolean painting = TRUE;
    gboolean acs;               /* Flag: Alternate character set active? */
    gboolean repeat_paint;
    int active_col, active_line;        /* Active link position */
    char buff[MB_LEN_MAX + 1];
    GString *word;

    word = g_string_sized_new (32);

    tty_setcolor (HELP_NORMAL_COLOR);
    do
    {
        line = col = active_col = active_line = 0;
        repeat_paint = FALSE;
        acs = FALSE;

        clear_link_areas ();
        if ((int) (selected_item - paint_start) < 0)
            selected_item = NULL;

        p = paint_start;
        n = paint_start;
        while ((n[0] != '\0') && (n[0] != CHAR_NODE_END) && (line < help_lines))
        {
            p = n;
            n = str_cget_next_char (p);
            memcpy (buff, p, n - p);
            buff[n - p] = '\0';

            c = (unsigned char) buff[0];
            switch (c)
            {
            case CHAR_LINK_START:
                if (selected_item == NULL)
                    selected_item = p;
                if (p != selected_item)
                    tty_setcolor (HELP_LINK_COLOR);
                else
                {
                    tty_setcolor (HELP_SLINK_COLOR);

                    /* Store the coordinates of the link */
                    active_col = col + 2;
                    active_line = line + 2;
                }
                start_link_area (col, line, p);
                break;
            case CHAR_LINK_POINTER:
                painting = FALSE;
                break;
            case CHAR_LINK_END:
                painting = TRUE;
                help_print_word (h, word, &col, &line, FALSE);
                end_link_area (col - 1, line);
                tty_setcolor (HELP_NORMAL_COLOR);
                break;
            case CHAR_ALTERNATE:
                acs = TRUE;
                break;
            case CHAR_NORMAL:
                acs = FALSE;
                break;
            case CHAR_VERSION:
                widget_gotoyx (h, line + 2, col + 2);
                tty_print_string (mc_global.mc_version);
                col += str_term_width1 (mc_global.mc_version);
                break;
            case CHAR_FONT_BOLD:
                tty_setcolor (HELP_BOLD_COLOR);
                break;
            case CHAR_FONT_ITALIC:
                tty_setcolor (HELP_ITALIC_COLOR);
                break;
            case CHAR_FONT_NORMAL:
                help_print_word (h, word, &col, &line, FALSE);
                tty_setcolor (HELP_NORMAL_COLOR);
                break;
            case '\n':
                if (painting)
                    help_print_word (h, word, &col, &line, FALSE);
                line++;
                col = 0;
                break;
            case ' ':
            case '\t':
                /* word delimiter */
                if (painting)
                {
                    help_print_word (h, word, &col, &line, c == ' ');
                    if (c == '\t')
                    {
                        col = (col / 8 + 1) * 8;
                        if (col >= HELP_WINDOW_WIDTH)
                        {
                            line++;
                            col = 8;
                        }
                    }
                }
                break;
            default:
                if (painting && (line < help_lines))
                {
                    if (!acs)
                        /* accumulate symbols in a word */
                        g_string_append (word, buff);
                    else if (col < HELP_WINDOW_WIDTH)
                    {
                        widget_gotoyx (h, line + 2, col + 2);

                        if ((c == ' ') || (c == '.'))
                            tty_print_char (c);
                        else
#ifndef HAVE_SLANG
                            tty_print_char (acs_map[c]);
#else
                            SLsmg_draw_object (WIDGET (h)->y + line + 2, WIDGET (h)->x + col + 2,
                                               c);
#endif
                        col++;
                    }
                }
            }
        }

        /* print last word */
        if (n[0] == CHAR_NODE_END)
            help_print_word (h, word, &col, &line, FALSE);

        last_shown = p;
        end_of_node = line < help_lines;
        tty_setcolor (HELP_NORMAL_COLOR);
        if ((int) (selected_item - last_shown) >= 0)
        {
            if ((link_area == NULL) || (link_area->data == NULL))
                selected_item = NULL;
            else
            {
                selected_item = ((Link_Area *) link_area->data)->link_name;
                repeat_paint = TRUE;
            }
        }
    }
    while (repeat_paint);

    g_string_free (word, TRUE);

    /* Position the cursor over a nice link */
    if (active_col)
        widget_gotoyx (h, active_line, active_col);
}

/* --------------------------------------------------------------------------------------------- */
/** show help */

static void
help_help (WDialog * h)
{
    const char *p;

    history_ptr = (history_ptr + 1) % HISTORY_SIZE;
    history[history_ptr].page = currentpoint;
    history[history_ptr].link = selected_item;

    p = search_string (fdata, "[How to use help]");
    if (p != NULL)
    {
        currentpoint = p + 1;   /* Skip the newline following the start of the node */
        selected_item = NULL;
        widget_draw (WIDGET (h));
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
help_index (WDialog * h)
{
    const char *new_item;

    new_item = search_string (fdata, "[Contents]");

    if (new_item == NULL)
        message (D_ERROR, MSG_ERROR, _("Cannot find node %s in help file"), "[Contents]");
    else
    {
        history_ptr = (history_ptr + 1) % HISTORY_SIZE;
        history[history_ptr].page = currentpoint;
        history[history_ptr].link = selected_item;

        currentpoint = new_item + 1;    /* Skip the newline following the start of the node */
        selected_item = NULL;
        widget_draw (WIDGET (h));
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
help_back (WDialog * h)
{
    currentpoint = history[history_ptr].page;
    selected_item = history[history_ptr].link;
    history_ptr--;
    if (history_ptr < 0)
        history_ptr = HISTORY_SIZE - 1;

    widget_draw (WIDGET (h));   /* FIXME: unneeded? */
}

/* --------------------------------------------------------------------------------------------- */

static void
help_next_link (gboolean move_down)
{
    const char *new_item;

    new_item = select_next_link (selected_item);
    if (new_item != NULL)
    {
        selected_item = new_item;
        if ((int) (selected_item - last_shown) >= 0)
        {
            if (move_down)
                move_forward (1);
            else
                selected_item = NULL;
        }
    }
    else if (move_down)
        move_forward (1);
    else
        selected_item = NULL;
}

/* --------------------------------------------------------------------------------------------- */

static void
help_prev_link (gboolean move_up)
{
    const char *new_item;

    new_item = select_prev_link (selected_item);
    selected_item = new_item;
    if ((selected_item == NULL) || (selected_item < currentpoint))
    {
        if (move_up)
            move_backward (1);
        else if ((link_area != NULL) && (link_area->data != NULL))
            selected_item = ((Link_Area *) link_area->data)->link_name;
        else
            selected_item = NULL;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
help_next_node (void)
{
    const char *new_item;

    new_item = currentpoint;
    while ((*new_item != '\0') && (*new_item != CHAR_NODE_END))
        new_item++;

    if (*++new_item == '[')
        while (*++new_item != '\0')
            if ((*new_item == ']') && (*++new_item != '\0') && (*++new_item != '\0'))
            {
                currentpoint = new_item;
                selected_item = NULL;
                break;
            }
}

/* --------------------------------------------------------------------------------------------- */

static void
help_prev_node (void)
{
    const char *new_item;

    new_item = currentpoint;
    while (((int) (new_item - fdata) > 1) && (*new_item != CHAR_NODE_END))
        new_item--;
    new_item--;
    while (((int) (new_item - fdata) > 0) && (*new_item != CHAR_NODE_END))
        new_item--;
    while (*new_item != ']')
        new_item++;
    currentpoint = new_item + 2;
    selected_item = NULL;
}

/* --------------------------------------------------------------------------------------------- */

static void
help_select_link (void)
{
    /* follow link */
    if (selected_item == NULL)
    {
#ifdef WE_WANT_TO_GO_BACKWARD_ON_KEY_RIGHT
        /* Is there any reason why the right key would take us
         * backward if there are no links selected?, I agree
         * with Torben than doing nothing in this case is better
         */
        /* If there are no links, go backward in history */
        history_ptr--;
        if (history_ptr < 0)
            history_ptr = HISTORY_SIZE - 1;

        currentpoint = history[history_ptr].page;
        selected_item = history[history_ptr].link;
#endif
    }
    else
    {
        history_ptr = (history_ptr + 1) % HISTORY_SIZE;
        history[history_ptr].page = currentpoint;
        history[history_ptr].link = selected_item;
        currentpoint = help_follow_link (currentpoint, selected_item);
    }

    selected_item = NULL;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
help_execute_cmd (long command)
{
    cb_ret_t ret = MSG_HANDLED;

    switch (command)
    {
    case CK_Help:
        help_help (whelp);
        break;
    case CK_Index:
        help_index (whelp);
        break;
    case CK_Back:
        help_back (whelp);
        break;
    case CK_Up:
        help_prev_link (TRUE);
        break;
    case CK_Down:
        help_next_link (TRUE);
        break;
    case CK_PageDown:
        move_forward (help_lines - 1);
        break;
    case CK_PageUp:
        move_backward (help_lines - 1);
        break;
    case CK_HalfPageDown:
        move_forward (help_lines / 2);
        break;
    case CK_HalfPageUp:
        move_backward (help_lines / 2);
        break;
    case CK_Top:
        move_to_top ();
        break;
    case CK_Bottom:
        move_to_bottom ();
        break;
    case CK_Enter:
        help_select_link ();
        break;
    case CK_LinkNext:
        help_next_link (FALSE);
        break;
    case CK_LinkPrev:
        help_prev_link (FALSE);
        break;
    case CK_NodeNext:
        help_next_node ();
        break;
    case CK_NodePrev:
        help_prev_node ();
        break;
    case CK_Quit:
        dlg_stop (whelp);
        break;
    default:
        ret = MSG_NOT_HANDLED;
    }

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
help_handle_key (WDialog * h, int key)
{
    Widget *w = WIDGET (h);
    long command;

    command = widget_lookup_key (w, key);
    if (command == CK_IgnoreKey)
        return MSG_NOT_HANDLED;

    return help_execute_cmd (command);
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
help_bg_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_DRAW:
        frame_callback (w, NULL, MSG_DRAW, 0, NULL);
        help_show (DIALOG (w->owner), currentpoint);
        return MSG_HANDLED;

    default:
        return frame_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
help_resize (WDialog * h)
{
    Widget *w = WIDGET (h);
    WButtonBar *bb;
    WRect r;

    help_lines = MIN (LINES - 4, MAX (2 * LINES / 3, 18));
    rect_init (&r, w->y, w->x, help_lines + 4, HELP_WINDOW_WIDTH + 4);
    dlg_default_callback (w, NULL, MSG_RESIZE, 0, &r);
    bb = find_buttonbar (h);
    widget_set_size (WIDGET (bb), LINES - 1, 0, 1, COLS);

    return MSG_HANDLED;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
help_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WDialog *h = DIALOG (w);

    switch (msg)
    {
    case MSG_RESIZE:
        return help_resize (h);

    case MSG_KEY:
        {
            cb_ret_t ret;

            ret = help_handle_key (h, parm);
            if (ret == MSG_HANDLED)
                widget_draw (w);

            return ret;
        }

    case MSG_ACTION:
        /* Handle shortcuts and buttonbar. */
        return help_execute_cmd (parm);

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
interactive_display_finish (void)
{
    clear_link_areas ();
}

/* --------------------------------------------------------------------------------------------- */
/** translate help file into terminal encoding */

static void
translate_file (char *filedata)
{
    GIConv conv;
    GString *translated_data;

    /* initial allocation for largest whole help file */
    translated_data = g_string_sized_new (32 * 1024);

    conv = str_crt_conv_from ("UTF-8");

    if (conv == INVALID_CONV)
        g_string_free (translated_data, TRUE);
    else
    {
        g_free (fdata);

        if (str_convert (conv, filedata, translated_data) != ESTR_FAILURE)
            fdata = g_string_free (translated_data, FALSE);
        else
        {
            fdata = NULL;
            g_string_free (translated_data, TRUE);
        }
        str_close_conv (conv);
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
md_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_RESIZE:
        widget_default_callback (w, NULL, MSG_RESIZE, 0, data);
        w->lines = help_lines;
        return MSG_HANDLED;

    default:
        return widget_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
help_mouse_callback (Widget * w, mouse_msg_t msg, mouse_event_t * event)
{
    int x, y;
    GSList *current_area;

    if (msg != MSG_MOUSE_CLICK)
        return;

    if ((event->buttons & GPM_B_RIGHT) != 0)
    {
        /* Right button click */
        help_back (whelp);
        return;
    }

    /* Left bytton click */

    /* The event is relative to the dialog window, adjust it: */
    x = event->x - 1;
    y = event->y - 1;

    /* Test whether the mouse click is inside one of the link areas */
    for (current_area = link_area; current_area != NULL; current_area = g_slist_next (current_area))
    {
        Link_Area *la = (Link_Area *) current_area->data;

        /* Test one line link area */
        if (y == la->y1 && x >= la->x1 && y == la->y2 && x <= la->x2)
            break;

        /* Test two line link area */
        if (la->y1 + 1 == la->y2)
        {
            /* The first line || The second line */
            if ((y == la->y1 && x >= la->x1) || (y == la->y2 && x <= la->x2))
                break;
        }
        /* Mouse will not work with link areas of more than two lines */
    }

    /* Test whether a link area was found */
    if (current_area != NULL)
    {
        Link_Area *la = (Link_Area *) current_area->data;

        /* The click was inside a link area -> follow the link */
        history_ptr = (history_ptr + 1) % HISTORY_SIZE;
        history[history_ptr].page = currentpoint;
        history[history_ptr].link = la->link_name;
        currentpoint = help_follow_link (currentpoint, la->link_name);
        selected_item = NULL;
    }
    else if (y < 0)
        move_backward (help_lines - 1);
    else if (y >= help_lines)
        move_forward (help_lines - 1);
    else if (y < help_lines / 2)
        move_backward (1);
    else
        move_forward (1);

    /* Show the new node */
    widget_draw (WIDGET (w->owner));
}

/* --------------------------------------------------------------------------------------------- */

static Widget *
mousedispatch_new (int y, int x, int yl, int xl)
{
    Widget *w;

    w = g_new0 (Widget, 1);
    widget_init (w, y, x, yl, xl, md_callback, help_mouse_callback);
    w->options |= WOP_SELECTABLE | WOP_WANT_CURSOR;

    return w;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* event callback */
gboolean
help_interactive_display (const gchar * event_group_name, const gchar * event_name,
                          gpointer init_data, gpointer data)
{
    const dlg_colors_t help_colors = {
        HELP_NORMAL_COLOR,      /* common text color */
        0,                      /* unused in help */
        HELP_BOLD_COLOR,        /* bold text color */
        0,                      /* unused in help */
        HELP_TITLE_COLOR        /* title color */
    };

    Widget *wh;
    WGroup *g;
    WButtonBar *help_bar;
    Widget *md;
    char *hlpfile = NULL;
    char *filedata;
    ev_help_t *event_data = (ev_help_t *) data;

    (void) event_group_name;
    (void) event_name;
    (void) init_data;

    if (event_data->filename != NULL)
        g_file_get_contents (event_data->filename, &filedata, NULL, NULL);
    else
        filedata = load_mc_home_file (mc_global.share_data_dir, MC_HELP, &hlpfile, NULL);

    if (filedata == NULL)
        message (D_ERROR, MSG_ERROR, _("Cannot open file %s\n%s"),
                 event_data->filename ? event_data->filename : hlpfile, unix_error_string (errno));

    g_free (hlpfile);

    if (filedata == NULL)
        return TRUE;

    translate_file (filedata);

    g_free (filedata);

    if (fdata == NULL)
        return TRUE;

    if ((event_data->node == NULL) || (*event_data->node == '\0'))
        event_data->node = "[main]";

    main_node = search_string (fdata, event_data->node);

    if (main_node == NULL)
    {
        message (D_ERROR, MSG_ERROR, _("Cannot find node %s in help file"), event_data->node);

        /* Fallback to [main], return if it also cannot be found */
        main_node = search_string (fdata, "[main]");
        if (main_node == NULL)
        {
            interactive_display_finish ();
            return TRUE;
        }
    }

    help_lines = MIN (LINES - 4, MAX (2 * LINES / 3, 18));

    whelp =
        dlg_create (TRUE, 0, 0, help_lines + 4, HELP_WINDOW_WIDTH + 4, WPOS_CENTER | WPOS_TRYUP,
                    FALSE, help_colors, help_callback, NULL, "[Help]", _("Help"));
    wh = WIDGET (whelp);
    g = GROUP (whelp);
    wh->keymap = help_map;
    widget_want_tab (wh, TRUE);
    /* draw background */
    whelp->bg->callback = help_bg_callback;

    selected_item = search_string_node (main_node, STRING_LINK_START) - 1;
    currentpoint = main_node + 1;       /* Skip the newline following the start of the node */

    for (history_ptr = HISTORY_SIZE; history_ptr;)
    {
        history_ptr--;
        history[history_ptr].page = currentpoint;
        history[history_ptr].link = selected_item;
    }

    help_bar = buttonbar_new ();
    WIDGET (help_bar)->y -= wh->y;
    WIDGET (help_bar)->x -= wh->x;

    md = mousedispatch_new (1, 1, help_lines, HELP_WINDOW_WIDTH - 2);

    group_add_widget (g, md);
    group_add_widget (g, help_bar);     /* FIXME */

    buttonbar_set_label (help_bar, 1, Q_ ("ButtonBar|Help"), wh->keymap, NULL);
    buttonbar_set_label (help_bar, 2, Q_ ("ButtonBar|Index"), wh->keymap, NULL);
    buttonbar_set_label (help_bar, 3, Q_ ("ButtonBar|Prev"), wh->keymap, NULL);
    buttonbar_set_label (help_bar, 4, "", wh->keymap, NULL);
    buttonbar_set_label (help_bar, 5, "", wh->keymap, NULL);
    buttonbar_set_label (help_bar, 6, "", wh->keymap, NULL);
    buttonbar_set_label (help_bar, 7, "", wh->keymap, NULL);
    buttonbar_set_label (help_bar, 8, "", wh->keymap, NULL);
    buttonbar_set_label (help_bar, 9, "", wh->keymap, NULL);
    buttonbar_set_label (help_bar, 10, Q_ ("ButtonBar|Quit"), wh->keymap, NULL);

    dlg_run (whelp);
    interactive_display_finish ();
    widget_destroy (wh);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
