/*
   Panel layout module for the uDOM Editor

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Janne Kukonlehto, 1995
   Miguel de Icaza, 1995
   Andrew Borodin <aborodin@vmail.ru>, 2011, 2012, 2013
   Slava Zanko <slavazanko@gmail.com>, 2013
   Avi Kelman <patcherton.fixesthings@gmail.com>, 2013

   This file is part of the uDOM Editor.

   The uDOM Editor is free software: you can redistribute it
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

/** \file layout.c
 *  \brief Source: panel layout module
 */

#include <config.h>

#include <pwd.h>                /* for username in xterm title */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib/global.h"
#include "lib/tty/tty.h"
#include "lib/skin.h"
#include "lib/tty/key.h"
#include "lib/tty/mouse.h"
#include "lib/mcconfig.h"
#include "lib/vfs/vfs.h"        /* For _vfs_get_cwd () */
#include "lib/strutil.h"
#include "lib/widget.h"
#include "lib/event.h"
#include "lib/util.h"           /* mc_time_elapsed() */

#include "src/consaver/cons.saver.h"
#include "src/setup.h"
#include "src/subshell/subshell.h"

#include "command.h"
#include "udommanager.h"
#include "layout.h"

/*** global variables ****************************************************************************/

panels_layout_t panels_layout = {
    /* Set if the panels are split horizontally */
    .horizontal_split = FALSE,

    /* vertical split */
    .vertical_equal = TRUE,
    .left_panel_size = 0,

    /* horizontal split */
    .horizontal_equal = TRUE,
    .top_panel_size = 0
};

/* Controls the display of the rotating dash on the verbose mode */
gboolean nice_rotating_dash = TRUE;

/* The number of output lines shown (if available) */
int output_lines = 0;

/* Set if the command prompt is to be displayed */
gboolean command_prompt = TRUE;

/* Set if the main menu is visible */
gboolean menubar_visible = TRUE;

/* Set to show current working dir in xterm window title */
gboolean xterm_title = TRUE;

/* Set to show free space on device assigned to current directory */
gboolean free_space = TRUE;

/* The starting line for the output of the subprogram */
int output_start_y = 0;

int ok_to_refresh = 1;

/*** file scope macro definitions ****************************************************************/

/* The maximum number of views managed by the create_panel routine */
/* Must be at least two (for current and other).  Please note that until */
/* Janne gets around this, we will only manage two of them :-) */
#define MAX_VIEWS 2

/* Width 12 for a wee Quick (Hex) View */
#define MINWIDTH 12
#define MINHEIGHT 5

#define B_2LEFT B_USER
#define B_2RIGHT (B_USER + 1)
#define B_PLUS (B_USER + 2)
#define B_MINUS (B_USER + 3)

#define LAYOUT_OPTIONS_COUNT  G_N_ELEMENTS (check_options)

/*** file scope type declarations ****************************************************************/

typedef struct
{
    gboolean menubar_visible;
    gboolean command_prompt;
    gboolean keybar_visible;
    gboolean message_visible;
    gboolean xterm_title;
    gboolean free_space;
    int output_lines;
} layout_t;

/*** file scope variables ************************************************************************/

static struct
{
    panel_view_mode_t type;
    Widget *widget;
    char *last_saved_dir;       /* last view_list working directory */
} panels[MAX_VIEWS] =
{
    /* *INDENT-OFF* */
    /* init MAX_VIEWS items */
    { view_listing, NULL, NULL},
    { view_listing, NULL, NULL}
    /* *INDENT-ON* */
};

static layout_t old_layout;
static panels_layout_t old_panels_layout;

static gboolean equal_split;
static int _output_lines;

static int height;

static WRadio *radio_widget;

static struct
{
    const char *text;
    gboolean *variable;
    WCheck *widget;
} check_options[] =
{
    /* *INDENT-OFF* */
    { N_("&Equal split"), &equal_split, NULL },
    { N_("&Menubar visible"), &menubar_visible, NULL },
    { N_("Command &prompt"), &command_prompt, NULL },
    { N_("&Keybar visible"), &mc_global.keybar_visible, NULL },
    { N_("H&intbar visible"), &mc_global.message_visible, NULL },
    { N_("&XTerm window title"), &xterm_title, NULL },
    { N_("&Show free space"), &free_space, NULL }
    /* *INDENT-ON* */
};

static const char *output_lines_label = NULL;
static int output_lines_label_len;

static WButton *bleft_widget, *bright_widget;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

/* don't use max() macro to avoid double call of str_term_width1() in widget width calculation */
#undef max

static int
max (int a, int b)
{
    return a > b ? a : b;
}

/* --------------------------------------------------------------------------------------------- */

static void
check_split (panels_layout_t * layout)
{
    if (layout->horizontal_split)
    {
        if (layout->horizontal_equal)
            layout->top_panel_size = height / 2;
        else if (layout->top_panel_size < MINHEIGHT)
            layout->top_panel_size = MINHEIGHT;
        else if (layout->top_panel_size > height - MINHEIGHT)
            layout->top_panel_size = height - MINHEIGHT;
    }
    else
    {
        int md_cols = CONST_WIDGET (filemanager)->cols;

        if (layout->vertical_equal)
            layout->left_panel_size = md_cols / 2;
        else if (layout->left_panel_size < MINWIDTH)
            layout->left_panel_size = MINWIDTH;
        else if (layout->left_panel_size > md_cols - MINWIDTH)
            layout->left_panel_size = md_cols - MINWIDTH;
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
update_split (const WDialog * h)
{
    /* Check split has to be done before testing if it changed, since
       it can change due to calling check_split() as well */
    check_split (&panels_layout);

    if (panels_layout.horizontal_split)
        check_options[0].widget->state = panels_layout.horizontal_equal;
    else
        check_options[0].widget->state = panels_layout.vertical_equal;
    widget_draw (WIDGET (check_options[0].widget));

    tty_setcolor (check_options[0].widget->state ? DISABLED_COLOR : COLOR_NORMAL);

    widget_gotoyx (h, 6, 5);
    if (panels_layout.horizontal_split)
        tty_printf ("%03d", panels_layout.top_panel_size);
    else
        tty_printf ("%03d", panels_layout.left_panel_size);

    widget_gotoyx (h, 6, 17);
    if (panels_layout.horizontal_split)
        tty_printf ("%03d", height - panels_layout.top_panel_size);
    else
        tty_printf ("%03d", CONST_WIDGET (filemanager)->cols - panels_layout.left_panel_size);

    widget_gotoyx (h, 6, 12);
    tty_print_char ('=');
}

/* --------------------------------------------------------------------------------------------- */

static int
b_left_right_cback (WButton * button, int action)
{
    (void) action;

    if (button == bright_widget)
    {
        if (panels_layout.horizontal_split)
            panels_layout.top_panel_size++;
        else
            panels_layout.left_panel_size++;
    }
    else
    {
        if (panels_layout.horizontal_split)
            panels_layout.top_panel_size--;
        else
            panels_layout.left_panel_size--;
    }

    update_split (DIALOG (WIDGET (button)->owner));
    layout_change ();
    do_refresh ();
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static int
bplus_cback (WButton * button, int action)
{
    (void) button;
    (void) action;

    if (_output_lines < 99)
        _output_lines++;
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static int
bminus_cback (WButton * button, int action)
{
    (void) button;
    (void) action;

    if (_output_lines > 0)
        _output_lines--;
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
layout_bg_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_DRAW:
        frame_callback (w, NULL, MSG_DRAW, 0, NULL);

        old_layout.output_lines = -1;

        update_split (DIALOG (w->owner));

        if (old_layout.output_lines != _output_lines)
        {
            old_layout.output_lines = _output_lines;
            tty_setcolor (mc_global.tty.console_flag != '\0' ? COLOR_NORMAL : DISABLED_COLOR);
            widget_gotoyx (w, 9, 5);
            tty_print_string (output_lines_label);
            widget_gotoyx (w, 9, 5 + 3 + output_lines_label_len);
            tty_printf ("%02d", _output_lines);
        }
        return MSG_HANDLED;

    default:
        return frame_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
layout_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    WDialog *h = DIALOG (w);

    switch (msg)
    {
    case MSG_POST_KEY:
        {
            const Widget *mw = CONST_WIDGET (filemanager);
            gboolean _menubar_visible, _command_prompt, _keybar_visible, _message_visible;

            _menubar_visible = check_options[1].widget->state;
            _command_prompt = check_options[2].widget->state;
            _keybar_visible = check_options[3].widget->state;
            _message_visible = check_options[4].widget->state;

            if (mc_global.tty.console_flag == '\0')
                height =
                    mw->lines - (_keybar_visible ? 1 : 0) - (_command_prompt ? 1 : 0) -
                    (_menubar_visible ? 1 : 0) - _output_lines - (_message_visible ? 1 : 0);
            else
            {
                int minimum;

                if (_output_lines < 0)
                    _output_lines = 0;
                height =
                    mw->lines - (_keybar_visible ? 1 : 0) - (_command_prompt ? 1 : 0) -
                    (_menubar_visible ? 1 : 0) - _output_lines - (_message_visible ? 1 : 0);
                minimum = MINHEIGHT * (1 + (panels_layout.horizontal_split ? 1 : 0));
                if (height < minimum)
                {
                    _output_lines -= minimum - height;
                    height = minimum;
                }
            }

            if (old_layout.output_lines != _output_lines)
            {
                old_layout.output_lines = _output_lines;
                tty_setcolor (mc_global.tty.console_flag != '\0' ? COLOR_NORMAL : DISABLED_COLOR);
                widget_gotoyx (h, 9, 5 + 3 + output_lines_label_len);
                tty_printf ("%02d", _output_lines);
            }
        }
        return MSG_HANDLED;

    case MSG_NOTIFY:
        if (sender == WIDGET (radio_widget))
        {
            if ((panels_layout.horizontal_split ? 1 : 0) == radio_widget->sel)
                update_split (h);
            else
            {
                int eq;

                panels_layout.horizontal_split = radio_widget->sel != 0;

                if (panels_layout.horizontal_split)
                {
                    eq = panels_layout.horizontal_equal;
                    if (eq)
                        panels_layout.top_panel_size = height / 2;
                }
                else
                {
                    eq = panels_layout.vertical_equal;
                    if (eq)
                        panels_layout.left_panel_size = CONST_WIDGET (filemanager)->cols / 2;
                }

                widget_disable (WIDGET (bleft_widget), eq);
                widget_disable (WIDGET (bright_widget), eq);

                update_split (h);
                layout_change ();
                do_refresh ();
            }

            return MSG_HANDLED;
        }

        if (sender == WIDGET (check_options[0].widget))
        {
            gboolean eq;

            if (panels_layout.horizontal_split)
            {
                panels_layout.horizontal_equal = check_options[0].widget->state;
                eq = panels_layout.horizontal_equal;
            }
            else
            {
                panels_layout.vertical_equal = check_options[0].widget->state;
                eq = panels_layout.vertical_equal;
            }

            widget_disable (WIDGET (bleft_widget), eq);
            widget_disable (WIDGET (bright_widget), eq);

            update_split (h);
            layout_change ();
            do_refresh ();

            return MSG_HANDLED;
        }

        {
            gboolean ok = TRUE;

            if (sender == WIDGET (check_options[1].widget))
                menubar_visible = check_options[1].widget->state;
            else if (sender == WIDGET (check_options[2].widget))
                command_prompt = check_options[2].widget->state;
            else if (sender == WIDGET (check_options[3].widget))
                mc_global.keybar_visible = check_options[3].widget->state;
            else if (sender == WIDGET (check_options[4].widget))
                mc_global.message_visible = check_options[4].widget->state;
            else if (sender == WIDGET (check_options[5].widget))
                xterm_title = check_options[5].widget->state;
            else if (sender == WIDGET (check_options[6].widget))
                free_space = check_options[6].widget->state;
            else
                ok = FALSE;

            if (ok)
            {
                update_split (h);
                layout_change ();
                do_refresh ();
                return MSG_HANDLED;
            }
        }

        return MSG_NOT_HANDLED;

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static WDialog *
layout_dlg_create (void)
{
    WDialog *layout_dlg;
    WGroup *g;
    int l1 = 0, width;
    int b1, b2, b;
    size_t i;

    const char *title1 = N_("Panel split");
    const char *title2 = N_("Console output");
    const char *title3 = N_("Other options");

    const char *s_split_direction[2] = {
        N_("&Vertical"),
        N_("&Horizontal")
    };

    const char *ok_button = N_("&OK");
    const char *cancel_button = N_("&Cancel");

    output_lines_label = _("Output lines:");

    /* radiobuttons */
    i = G_N_ELEMENTS (s_split_direction);
    while (i-- != 0)
        l1 = max (l1, str_term_width1 (s_split_direction[i]) + 7);
    /* checkboxes */
    for (i = 0; i < (size_t) LAYOUT_OPTIONS_COUNT; i++)
        l1 = max (l1, str_term_width1 (check_options[i].text) + 7);
    /* groupboxes */
    l1 = max (l1, str_term_width1 (title1) + 4);
    l1 = max (l1, str_term_width1 (title2) + 4);
    l1 = max (l1, str_term_width1 (title3) + 4);
    /* label + "+"/"-" buttons */
    output_lines_label_len = str_term_width1 (output_lines_label);
    l1 = max (l1, output_lines_label_len + 12);
    /* buttons */
    b1 = str_term_width1 (ok_button) + 5;       /* default button */
    b2 = str_term_width1 (cancel_button) + 3;
    b = b1 + b2 + 1;
    /* dialog width */
    width = max (l1 * 2 + 7, b);

    layout_dlg =
        dlg_create (TRUE, 0, 0, 15, width, WPOS_CENTER, FALSE, dialog_colors, layout_callback, NULL,
                    "[Layout]", _("Layout"));
    g = GROUP (layout_dlg);

    /* draw background */
    layout_dlg->bg->callback = layout_bg_callback;

#define XTRACT(i) (*check_options[i].variable != 0), check_options[i].text

    /* "Panel split" groupbox */
    group_add_widget (g, groupbox_new (2, 3, 6, l1, title1));

    radio_widget = radio_new (3, 5, 2, s_split_direction);
    radio_widget->sel = panels_layout.horizontal_split ? 1 : 0;
    group_add_widget (g, radio_widget);

    check_options[0].widget = check_new (5, 5, XTRACT (0));
    group_add_widget (g, check_options[0].widget);

    equal_split = panels_layout.horizontal_split ?
        panels_layout.horizontal_equal : panels_layout.vertical_equal;

    bleft_widget = button_new (6, 8, B_2LEFT, NARROW_BUTTON, "&<", b_left_right_cback);
    widget_disable (WIDGET (bleft_widget), equal_split);
    group_add_widget (g, bleft_widget);

    bright_widget = button_new (6, 14, B_2RIGHT, NARROW_BUTTON, "&>", b_left_right_cback);
    widget_disable (WIDGET (bright_widget), equal_split);
    group_add_widget (g, bright_widget);

    /* "Console output" groupbox */
    {
        widget_state_t disabled;
        Widget *w;

        disabled = mc_global.tty.console_flag != '\0' ? 0 : WST_DISABLED;

        w = WIDGET (groupbox_new (8, 3, 3, l1, title2));
        w->state |= disabled;
        group_add_widget (g, w);

        w = WIDGET (button_new (9, output_lines_label_len + 5, B_PLUS,
                                NARROW_BUTTON, "&+", bplus_cback));
        w->state |= disabled;
        group_add_widget (g, w);

        w = WIDGET (button_new (9, output_lines_label_len + 5 + 5, B_MINUS,
                                NARROW_BUTTON, "&-", bminus_cback));
        w->state |= disabled;
        group_add_widget (g, w);
    }

    /* "Other options" groupbox */
    group_add_widget (g, groupbox_new (2, 4 + l1, 9, l1, title3));

    for (i = 1; i < (size_t) LAYOUT_OPTIONS_COUNT; i++)
    {
        check_options[i].widget = check_new (i + 2, 6 + l1, XTRACT (i));
        group_add_widget (g, check_options[i].widget);
    }

#undef XTRACT

    group_add_widget (g, hline_new (11, -1, -1));
    /* buttons */
    group_add_widget (g, button_new (12, (width - b) / 2, B_ENTER, DEFPUSH_BUTTON, ok_button, 0));
    group_add_widget (g,
                      button_new (12, (width - b) / 2 + b1 + 1, B_CANCEL, NORMAL_BUTTON,
                                  cancel_button, 0));

    widget_select (WIDGET (radio_widget));

    return layout_dlg;
}

/* --------------------------------------------------------------------------------------------- */

static void
panel_do_cols (int idx)
{
    if (get_panel_type (idx) == view_listing)
        set_panel_formats (PANEL (panels[idx].widget));
    else
        panel_update_cols (panels[idx].widget, frame_half);
}

/* --------------------------------------------------------------------------------------------- */
/** Save current list_view widget directory into panel */

static Widget *
restore_into_right_dir_panel (int idx, gboolean last_was_panel, int y, int x, int lines, int cols)
{
    WPanel *new_widget;
    const char *p_name;

    p_name = get_nth_panel_name (idx);

    if (last_was_panel)
    {
        vfs_path_t *saved_dir_vpath;

        saved_dir_vpath = vfs_path_from_str (panels[idx].last_saved_dir);
        new_widget = panel_sized_with_dir_new (p_name, y, x, lines, cols, saved_dir_vpath);
        vfs_path_free (saved_dir_vpath, TRUE);
    }
    else
        new_widget = panel_sized_new (p_name, y, x, lines, cols);

    return WIDGET (new_widget);
}

/* --------------------------------------------------------------------------------------------- */

static void
layout_save (void)
{
    old_layout.menubar_visible = menubar_visible;
    old_layout.command_prompt = command_prompt;
    old_layout.keybar_visible = mc_global.keybar_visible;
    old_layout.message_visible = mc_global.message_visible;
    old_layout.xterm_title = xterm_title;
    old_layout.free_space = free_space;
    old_layout.output_lines = -1;

    _output_lines = output_lines;

    old_panels_layout = panels_layout;
}

/* --------------------------------------------------------------------------------------------- */

static void
layout_restore (void)
{
    menubar_visible = old_layout.menubar_visible;
    command_prompt = old_layout.command_prompt;
    mc_global.keybar_visible = old_layout.keybar_visible;
    mc_global.message_visible = old_layout.message_visible;
    xterm_title = old_layout.xterm_title;
    free_space = old_layout.free_space;
    output_lines = old_layout.output_lines;

    panels_layout = old_panels_layout;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
layout_change (void)
{
    setup_panels ();
    /* update the main menu, because perhaps there was a change in the way
       how the panel are split (horizontal/vertical),
       and a change of menu visibility. */
    update_menu ();
    load_hint (TRUE);
}

/* --------------------------------------------------------------------------------------------- */

void
layout_box (void)
{
    WDialog *layout_dlg;

    layout_save ();

    layout_dlg = layout_dlg_create ();

    if (dlg_run (layout_dlg) == B_ENTER)
    {
        size_t i;

        for (i = 0; i < (size_t) LAYOUT_OPTIONS_COUNT; i++)
            if (check_options[i].widget != NULL)
                *check_options[i].variable = check_options[i].widget->state;

        output_lines = _output_lines;
    }
    else
        layout_restore ();

    widget_destroy (WIDGET (layout_dlg));
    layout_change ();
    do_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

void
panel_update_cols (Widget * widget, panel_display_t frame_size)
{
    const Widget *mw = CONST_WIDGET (filemanager);
    int cols, x;

    /* don't touch panel if it is not in dialog yet */
    /* if panel is not in dialog it is not in widgets list
       and cannot be compared with get_panel_widget() result */
    if (widget->owner == NULL)
        return;

    if (panels_layout.horizontal_split)
    {
        widget->cols = mw->cols;
        return;
    }

    if (frame_size == frame_full)
    {
        cols = mw->cols;
        x = mw->x;
    }
    else if (widget == get_panel_widget (0))
    {
        cols = panels_layout.left_panel_size;
        x = mw->x;
    }
    else
    {
        cols = mw->cols - panels_layout.left_panel_size;
        x = mw->x + panels_layout.left_panel_size;
    }

    widget->cols = cols;
    widget->x = x;
}

/* --------------------------------------------------------------------------------------------- */

void
setup_panels (void)
{
    /* File manager screen layout:
     *
     * +---------------------------------------------------------------+
     * | Menu bar                                                      |
     * +-------------------------------+-------------------------------+
     * |                               |                               |
     * |                               |                               |
     * |                               |                               |
     * |                               |                               |
     * |         Left panel            |         Right panel           |
     * |                               |                               |
     * |                               |                               |
     * |                               |                               |
     * |                               |                               |
     * +-------------------------------+-------------------------------+
     * | Hint (message) bar                                            |
     * +---------------------------------------------------------------+
     * |                                                               |
     * |                        Console content                        |
     * |                                                               |
     * +--------+------------------------------------------------------+
     * | Prompt | Command line                                         |
     * | Key (button) bar                                              |
     * +--------+------------------------------------------------------+
     */

    Widget *mw = WIDGET (filemanager);
    int start_y;
    gboolean active;

    active = widget_get_state (mw, WST_ACTIVE);

    /* lock the group to avoid many redraws */
    if (active)
        widget_set_state (mw, WST_SUSPENDED, TRUE);

    /* iniitial height of panels */
    height =
        mw->lines - (menubar_visible ? 1 : 0) - (mc_global.message_visible ? 1 : 0) -
        (command_prompt ? 1 : 0) - (mc_global.keybar_visible ? 1 : 0);

    if (mc_global.tty.console_flag != '\0')
    {
        int minimum;

        if (output_lines < 0)
            output_lines = 0;
        else
            height -= output_lines;
        minimum = MINHEIGHT * (1 + (panels_layout.horizontal_split ? 1 : 0));
        if (height < minimum)
        {
            output_lines -= minimum - height;
            height = minimum;
        }
    }

    widget_set_size (WIDGET (the_menubar), mw->y, mw->x, 1, mw->cols);
    widget_set_visibility (WIDGET (the_menubar), menubar_visible);

    check_split (&panels_layout);
    start_y = mw->y + (menubar_visible ? 1 : 0);

    /* update columns first... */
    panel_do_cols (0);
    panel_do_cols (1);

    /* ...then rows and origin */
    if (panels_layout.horizontal_split)
    {
        widget_set_size (panels[0].widget, start_y, mw->x, panels_layout.top_panel_size,
                         panels[0].widget->cols);
        widget_set_size (panels[1].widget, start_y + panels_layout.top_panel_size, mw->x,
                         height - panels_layout.top_panel_size, panels[1].widget->cols);
    }
    else
    {
        widget_set_size (panels[0].widget, start_y, mw->x, height, panels[0].widget->cols);
        widget_set_size (panels[1].widget, start_y, panels[1].widget->x, height,
                         panels[1].widget->cols);
    }

    widget_set_size (WIDGET (the_hint), height + start_y, mw->x, 1, mw->cols);
    widget_set_visibility (WIDGET (the_hint), mc_global.message_visible);

    /* Output window */
    if (mc_global.tty.console_flag != '\0' && output_lines != 0)
    {
        unsigned char end_line;

        end_line = mw->lines - (mc_global.keybar_visible ? 1 : 0) - 1;
        output_start_y = end_line - (command_prompt ? 1 : 0) - output_lines + 1;
        show_console_contents (output_start_y, end_line - output_lines, end_line);
    }

    if (command_prompt)
    {
        if (!mc_global.tty.use_subshell || !do_load_prompt ())
            setup_cmdline ();
    }
    else
    {
        /* make invisible */
        widget_hide (WIDGET (cmdline));
        widget_hide (WIDGET (the_prompt));
    }

    widget_set_size (WIDGET (the_bar), mw->lines - 1, mw->x, 1, mw->cols);
    widget_set_visibility (WIDGET (the_bar), mc_global.keybar_visible);

    update_xterm_title_path ();

    /* unlock */
    if (active)
    {
        widget_set_state (mw, WST_ACTIVE, TRUE);
        widget_draw (mw);
    }
}

/* --------------------------------------------------------------------------------------------- */

void
panels_split_equal (void)
{
    if (panels_layout.horizontal_split)
        panels_layout.horizontal_equal = TRUE;
    else
        panels_layout.vertical_equal = TRUE;

    layout_change ();
    do_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

void
panels_split_more (void)
{
    if (panels_layout.horizontal_split)
    {
        panels_layout.horizontal_equal = FALSE;
        panels_layout.top_panel_size++;
    }
    else
    {
        panels_layout.vertical_equal = FALSE;
        panels_layout.left_panel_size++;
    }

    layout_change ();
}

/* --------------------------------------------------------------------------------------------- */

void
panels_split_less (void)
{
    if (panels_layout.horizontal_split)
    {
        panels_layout.horizontal_equal = FALSE;
        panels_layout.top_panel_size--;
    }
    else
    {
        panels_layout.vertical_equal = FALSE;
        panels_layout.left_panel_size--;
    }

    layout_change ();
}

/* --------------------------------------------------------------------------------------------- */

void
setup_cmdline (void)
{
    const Widget *mw = CONST_WIDGET (filemanager);
    int prompt_width;
    int y;
    char *tmp_prompt = (char *) mc_prompt;

    if (!command_prompt)
        return;

    if (mc_global.tty.use_subshell)
    {
        /* Workaround: avoid crash on FreeBSD (see ticket #4213 for details)  */
        if (subshell_prompt != NULL)
            tmp_prompt = g_string_free (subshell_prompt, FALSE);
        else
            tmp_prompt = g_strdup (mc_prompt);
        (void) strip_ctrl_codes (tmp_prompt);
    }

    prompt_width = str_term_width1 (tmp_prompt);

    /* Check for prompts too big */
    if (mw->cols > 8 && prompt_width > mw->cols - 8)
    {
        int prompt_len;

        prompt_width = mw->cols - 8;
        prompt_len = str_offset_to_pos (tmp_prompt, prompt_width);
        tmp_prompt[prompt_len] = '\0';
    }

    if (mc_global.tty.use_subshell)
    {
        subshell_prompt = g_string_new (tmp_prompt);
        g_free (tmp_prompt);
        mc_prompt = subshell_prompt->str;
    }

    y = mw->lines - 1 - (mc_global.keybar_visible ? 1 : 0);

    widget_set_size (WIDGET (the_prompt), y, mw->x, 1, prompt_width);
    label_set_text (the_prompt, mc_prompt);
    widget_set_size (WIDGET (cmdline), y, mw->x + prompt_width, 1, mw->cols - prompt_width);

    widget_show (WIDGET (the_prompt));
    widget_show (WIDGET (cmdline));
}

/* --------------------------------------------------------------------------------------------- */

void
use_dash (gboolean flag)
{
    if (flag)
        ok_to_refresh++;
    else
        ok_to_refresh--;
}

/* --------------------------------------------------------------------------------------------- */

void
set_hintbar (const char *str)
{
    label_set_text (the_hint, str);
    if (ok_to_refresh > 0)
        mc_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

void
rotate_dash (gboolean show)
{
    static gint64 timestamp = 0;
    /* update with 10 FPS rate */
    static const gint64 delay = G_USEC_PER_SEC / 10;

    const Widget *w = CONST_WIDGET (filemanager);

    if (!nice_rotating_dash || (ok_to_refresh <= 0))
        return;

    if (show && !mc_time_elapsed (&timestamp, delay))
        return;

    widget_gotoyx (w, menubar_visible ? 1 : 0, w->cols - 1);
    tty_setcolor (NORMAL_COLOR);

    if (!show)
        tty_print_alt_char (ACS_URCORNER, FALSE);
    else
    {
        static const char rotating_dash[4] = "|/-\\";
        static size_t pos = 0;

        tty_print_char (rotating_dash[pos]);
        pos = (pos + 1) % sizeof (rotating_dash);
    }

    mc_refresh ();
}

/* --------------------------------------------------------------------------------------------- */

const char *
get_nth_panel_name (int num)
{
    if (num == 0)
        return "New Left Panel";

    if (num == 1)
        return "New Right Panel";

    {
        static char buffer[BUF_SMALL];

        g_snprintf (buffer, sizeof (buffer), "%ith Panel", num);
        return buffer;
    }
}

/* --------------------------------------------------------------------------------------------- */
/* I wonder if I should start to use the folding mode than Dugan uses */
/*                                                                     */
/* This is the centralized managing of the panel display types         */
/* This routine takes care of destroying and creating new widgets      */
/* Please note that it could manage MAX_VIEWS, not just left and right */
/* Currently nothing in the code takes advantage of this and has hard- */
/* coded values for two panels only                                    */

/* Set the num-th panel to the view type: type */
/* This routine also keeps at least one WPanel object in the screen */
/* since a lot of routines depend on the current_panel variable */

void
create_panel (int num, panel_view_mode_t type)
{
    int x = 0, y = 0, cols = 0, lines = 0;
    unsigned int the_other = 0; /* Index to the other panel */
    Widget *new_widget = NULL, *old_widget = NULL;
    panel_view_mode_t old_type = view_listing;

    if (num >= MAX_VIEWS)
    {
        fprintf (stderr, "Cannot allocate more that %d views\n", MAX_VIEWS);
        abort ();
    }
    /* Check that we will have a WPanel * at least */
    if (type != view_listing)
    {
        the_other = num == 0 ? 1 : 0;

        if (panels[the_other].type != view_listing)
            return;
    }

    /* Get rid of it */
    if (panels[num].widget != NULL)
    {
        Widget *w = panels[num].widget;
        WPanel *panel = PANEL (w);

        x = w->x;
        y = w->y;
        cols = w->cols;
        lines = w->lines;
        old_widget = w;
        old_type = panels[num].type;

        if (old_type == view_listing && panel->frame_size == frame_full && type != view_listing)
        {
            int md_cols = CONST_WIDGET (filemanager)->cols;

            if (panels_layout.horizontal_split)
            {
                cols = md_cols;
                x = 0;
            }
            else
            {
                cols = md_cols - panels_layout.left_panel_size;
                if (num == 1)
                    x = panels_layout.left_panel_size;
            }
        }
    }

    /* Restoring saved path from panels.ini for nonlist panel */
    /* when it's first creation (for example view_info) */
    if (old_widget == NULL && type != view_listing)
        panels[num].last_saved_dir = _vfs_get_cwd ();

    switch (type)
    {
    case view_nothing:
    case view_listing:
        {
            gboolean last_was_panel;

            last_was_panel = old_widget != NULL && get_panel_type (num) != view_listing;
            new_widget = restore_into_right_dir_panel (num, last_was_panel, y, x, lines, cols);
            break;
        }

    default:
        break;
    }

    if (type != view_listing)
        /* Must save dir, for restoring after change type to */
        /* view_listing */
        save_panel_dir (num);

    panels[num].type = type;
    panels[num].widget = new_widget;

    /* We use replace to keep the circular list of the dialog in the */
    /* same state.  Maybe we could just kill it and then replace it  */
    if (old_widget != NULL)
    {
        if (old_type == view_listing)
        {
            /* save and write directory history of panel
             * ... and other histories of filemanager  */
            dlg_save_history (filemanager);
        }

        widget_replace (old_widget, new_widget);
    }

    if (type == view_listing)
    {
        WPanel *panel = PANEL (new_widget);

        /* if existing panel changed type to view_listing, then load history */
        if (old_widget != NULL)
        {
            ev_history_load_save_t event_data = { NULL, new_widget };

            mc_event_raise (filemanager->event_group, MCEVENT_HISTORY_LOAD, &event_data);
        }

        if (num == 0)
            left_panel = panel;
        else
            right_panel = panel;

        /* forced update format after set new sizes */
        set_panel_formats (panel);
    }

    /* Prevent current_panel's value from becoming invalid.
     * It's just a quick hack to prevent segfaults. Comment out and
     * try following:
     * - select left panel
     * - invoke menue left/tree
     * - as long as you stay in the left panel almost everything that uses
     *   current_panel causes segfault, e.g. C-Enter, C-x c, ...
     */
    if ((type != view_listing) && (current_panel == PANEL (old_widget)))
        current_panel = num == 0 ? right_panel : left_panel;

    g_free (old_widget);
}

/* --------------------------------------------------------------------------------------------- */
/** This routine is deeply sticked to the two panels idea.
   What should it do in more panels. ANSWER - don't use it
   in any multiple panels environment. */

void
swap_panels (void)
{
    WPanel *panel1, *panel2;
    Widget *tmp_widget;

    panel1 = PANEL (panels[0].widget);
    panel2 = PANEL (panels[1].widget);

    if (panels[0].type == view_listing && panels[1].type == view_listing &&
        !mc_config_get_bool (mc_global.main_config, CONFIG_PANELS_SECTION, "simple_swap", FALSE))
    {
        WPanel panel;

#define panelswap(x) panel.x = panel1->x; panel1->x = panel2->x; panel2->x = panel.x;
        /* Change content and related stuff */
        panelswap (active);
#undef panelswap

        if (current_panel == panel1)
            current_panel = panel2;
        else
            current_panel = panel1;

        if (widget_is_active (panels[0].widget))
            widget_select (panels[1].widget);
        else if (widget_is_active (panels[1].widget))
            widget_select (panels[0].widget);
    }
    else
    {
        WPanel *tmp_panel;
        int x, y, cols, lines;
        int tmp_type;

        tmp_panel = right_panel;
        right_panel = left_panel;
        left_panel = tmp_panel;

        if (panels[0].type == view_listing)
        {
            if (strcmp (panel1->name, get_nth_panel_name (0)) == 0)
            {
                g_free (panel1->name);
                panel1->name = g_strdup (get_nth_panel_name (1));
            }
        }
        if (panels[1].type == view_listing)
        {
            if (strcmp (panel2->name, get_nth_panel_name (1)) == 0)
            {
                g_free (panel2->name);
                panel2->name = g_strdup (get_nth_panel_name (0));
            }
        }

        x = panels[0].widget->x;
        y = panels[0].widget->y;
        cols = panels[0].widget->cols;
        lines = panels[0].widget->lines;

        panels[0].widget->x = panels[1].widget->x;
        panels[0].widget->y = panels[1].widget->y;
        panels[0].widget->cols = panels[1].widget->cols;
        panels[0].widget->lines = panels[1].widget->lines;

        panels[1].widget->x = x;
        panels[1].widget->y = y;
        panels[1].widget->cols = cols;
        panels[1].widget->lines = lines;

        tmp_widget = panels[0].widget;
        panels[0].widget = panels[1].widget;
        panels[1].widget = tmp_widget;
        tmp_type = panels[0].type;
        panels[0].type = panels[1].type;
        panels[1].type = tmp_type;

        /* force update formats because of possible changed sizes */
        if (panels[0].type == view_listing)
            set_panel_formats (PANEL (panels[0].widget));
        if (panels[1].type == view_listing)
            set_panel_formats (PANEL (panels[1].widget));
    }
}

/* --------------------------------------------------------------------------------------------- */

panel_view_mode_t
get_panel_type (int idx)
{
    return panels[idx].type;
}

/* --------------------------------------------------------------------------------------------- */

Widget *
get_panel_widget (int idx)
{
    return panels[idx].widget;
}

/* --------------------------------------------------------------------------------------------- */

int
get_current_index (void)
{
    return (panels[0].widget == WIDGET (current_panel) ? 0 : 1);
}

/* --------------------------------------------------------------------------------------------- */

int
get_other_index (void)
{
    return (get_current_index () == 0 ? 1 : 0);
}

/* --------------------------------------------------------------------------------------------- */

WPanel *
get_other_panel (void)
{
    return PANEL (get_panel_widget (get_other_index ()));
}

/* --------------------------------------------------------------------------------------------- */
/** Returns the view type for the current panel/view */

panel_view_mode_t
get_current_type (void)
{
    return (panels[0].widget == WIDGET (current_panel) ? panels[0].type : panels[1].type);
}

/* --------------------------------------------------------------------------------------------- */
/** Returns the view type of the unselected panel */

panel_view_mode_t
get_other_type (void)
{
    return (panels[0].widget == WIDGET (current_panel) ? panels[1].type : panels[0].type);
}

/* --------------------------------------------------------------------------------------------- */
/** Save current list_view widget directory into panel */

void
save_panel_dir (int idx)
{
    panel_view_mode_t type;

    type = get_panel_type (idx);
    if (type == view_listing)
    {
        WPanel *p;

        p = PANEL (get_panel_widget (idx));
        if (p != NULL)
        {
            g_free (panels[idx].last_saved_dir);        /* last path no needed */
            /* Because path can be nonlocal */
            panels[idx].last_saved_dir = g_strdup (vfs_path_as_str (p->cwd_vpath));
        }
    }
}

/* --------------------------------------------------------------------------------------------- */
/** Return working dir, if it's view_listing - cwd,
   but for other types - last_saved_dir */

char *
get_panel_dir_for (const WPanel * widget)
{
    int i;

    for (i = 0; i < MAX_VIEWS; i++)
        if (PANEL (get_panel_widget (i)) == widget)
            break;

    if (i >= MAX_VIEWS)
        return g_strdup (".");

    if (get_panel_type (i) == view_listing)
    {
        vfs_path_t *cwd_vpath;

        cwd_vpath = PANEL (get_panel_widget (i))->cwd_vpath;
        return g_strdup (vfs_path_as_str (cwd_vpath));
    }

    return g_strdup (panels[i].last_saved_dir);
}

/* --------------------------------------------------------------------------------------------- */

gboolean
do_load_prompt (void)
{
    gboolean ret = FALSE;

    if (!read_subshell_prompt ())
        return ret;

    /* Don't actually change the prompt if it's invisible */
    if (top_dlg != NULL && DIALOG (top_dlg->data) == filemanager && command_prompt)
    {
        setup_cmdline ();

        /* since the prompt has changed, and we are called from one of the
         * tty_get_event channels, the prompt updating does not take place
         * automatically: force a cursor update and a screen refresh
         */
        widget_update_cursor (WIDGET (filemanager));
        mc_refresh ();
        ret = TRUE;
    }
    update_subshell_prompt = TRUE;
    return ret;
}

/* --------------------------------------------------------------------------------------------- */

int
load_prompt (int fd, void *unused)
{
    (void) fd;
    (void) unused;

    if (should_read_new_subshell_prompt)
        do_load_prompt ();
    else
        flush_subshell (0, QUIETLY);

    return 0;
}

/* --------------------------------------------------------------------------------------------- */

void
title_path_prepare (char **path, char **login)
{
    char host[BUF_TINY];
    struct passwd *pw = NULL;
    int res = 0;

    *path =
        vfs_path_to_str_flags (current_panel->cwd_vpath, 0, VPF_STRIP_HOME | VPF_STRIP_PASSWORD);

    res = gethostname (host, sizeof (host));
    if (res != 0)
        host[0] = '\0';
    else
        host[sizeof (host) - 1] = '\0';

    pw = getpwuid (getuid ());
    if (pw != NULL)
        *login = g_strdup_printf ("%s@%s", pw->pw_name, host);
    else
        *login = g_strdup (host);
}

/* --------------------------------------------------------------------------------------------- */

/** Show current directory in the xterm title */
void
update_xterm_title_path (void)
{
    if (mc_global.tty.xterm_flag && xterm_title)
    {
        char *p;
        char *path;
        char *login;

        title_path_prepare (&path, &login);

        p = g_strdup_printf ("mc [%s]:%s", login, path);
        g_free (login);
        g_free (path);

        fprintf (stdout, "\33]0;%s\7", str_term_form (p));
        g_free (p);

        if (!mc_global.tty.alternate_plus_minus)
            numeric_keypad_mode ();
        (void) fflush (stdout);
    }
}

/* --------------------------------------------------------------------------------------------- */
