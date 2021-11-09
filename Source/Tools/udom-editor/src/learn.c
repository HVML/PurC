/*
   Learn keys

   Copyright (C) 1995-2021
   Free Software Foundation, Inc.

   Written by:
   Jakub Jelinek, 1995
   Andrew Borodin <aborodin@vmail.ru>, 2012, 2013

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

/** \file learn.c
 *  \brief Source: learn keys module
 */

#include <config.h>

#include <stdlib.h>

#include "lib/global.h"

#include "lib/tty/tty.h"
#include "lib/tty/key.h"
#include "lib/mcconfig.h"
#include "lib/strescape.h"
#include "lib/strutil.h"
#include "lib/util.h"           /* convert_controls() */
#include "lib/widget.h"

#include "setup.h"
#include "learn.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define UX 4
#define UY 2

#define ROWS     13
#define COLSHIFT 23

/*** file scope type declarations ****************************************************************/

typedef struct
{
    Widget *button;
    Widget *label;
    gboolean ok;
    char *sequence;
} learnkey_t;

/*** file scope variables ************************************************************************/

static WDialog *learn_dlg;
static const char *learn_title = N_("Learn keys");

static learnkey_t *learnkeys = NULL;
static int learn_total;
static int learnok;
static gboolean learnchanged = FALSE;

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static int
learn_button (WButton * button, int action)
{
    WDialog *d;
    char *seq;

    (void) button;

    d = create_message (D_ERROR, _("Teach me a key"),
                        _("Please press the %s\n"
                          "and then wait until this message disappears.\n\n"
                          "Then, press it again to see if OK appears\n"
                          "next to its button.\n\n"
                          "If you want to escape, press a single Escape key\n"
                          "and wait as well."), _(key_name_conv_tab[action - B_USER].longname));
    mc_refresh ();
    if (learnkeys[action - B_USER].sequence != NULL)
        MC_PTR_FREE (learnkeys[action - B_USER].sequence);

    seq = learn_key ();
    if (seq != NULL)
    {
        /* Esc hides the dialog and do not allow definitions of
         * regular characters
         */
        gboolean seq_ok = FALSE;

        if (*seq != '\0' && strcmp (seq, "\\e") != 0 && strcmp (seq, "\\e\\e") != 0
            && strcmp (seq, "^m") != 0 && strcmp (seq, "^i") != 0
            && (seq[1] != '\0' || *seq < ' ' || *seq > '~'))
        {
            learnchanged = TRUE;
            learnkeys[action - B_USER].sequence = seq;
            seq = convert_controls (seq);
            seq_ok = define_sequence (key_name_conv_tab[action - B_USER].code, seq, MCKEY_NOACTION);
        }

        if (!seq_ok)
            message (D_NORMAL, _("Cannot accept this key"), _("You have entered \"%s\""), seq);

        g_free (seq);
    }

    dlg_run_done (d);
    widget_destroy (WIDGET (d));

    widget_select (learnkeys[action - B_USER].button);

    return 0;                   /* Do not kill learn_dlg */
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
learn_move (gboolean right)
{
    int i, totalcols;

    totalcols = (learn_total - 1) / ROWS + 1;
    for (i = 0; i < learn_total; i++)
        if (learnkeys[i].button == WIDGET (GROUP (learn_dlg)->current->data))
        {
            if (right)
            {
                if (i < learn_total - ROWS)
                    i += ROWS;
                else
                    i %= ROWS;
            }
            else
            {
                if (i / ROWS != 0)
                    i -= ROWS;
                else if (i + (totalcols - 1) * ROWS >= learn_total)
                    i += (totalcols - 2) * ROWS;
                else
                    i += (totalcols - 1) * ROWS;
            }
            widget_select (learnkeys[i].button);
            return TRUE;
        }

    return FALSE;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
learn_check_key (int c)
{
    int i;

    for (i = 0; i < learn_total; i++)
    {
        if (key_name_conv_tab[i].code != c || learnkeys[i].ok)
            continue;

        widget_select (learnkeys[i].button);
        /* TRANSLATORS: This label appears near learned keys.  Keep it short.  */
        label_set_text (LABEL (learnkeys[i].label), _("OK"));
        learnkeys[i].ok = TRUE;
        learnok++;
        if (learnok >= learn_total)
        {
            learn_dlg->ret_value = B_CANCEL;
            if (learnchanged)
            {
                if (query_dialog (learn_title,
                                  _
                                  ("It seems that all your keys already\n"
                                   "work fine. That's great."), D_ERROR, 2,
                                  _("&Save"), _("&Discard")) == 0)
                    learn_dlg->ret_value = B_ENTER;
            }
            else
            {
                message (D_ERROR, learn_title, "%s",
                         _
                         ("Great! You have a complete terminal database!\n"
                          "All your keys work well."));
            }
            dlg_stop (learn_dlg);
        }
        return TRUE;
    }

    switch (c)
    {
    case KEY_LEFT:
    case 'h':
        return learn_move (FALSE);
    case KEY_RIGHT:
    case 'l':
        return learn_move (TRUE);
    case 'j':
        group_select_next_widget (GROUP (learn_dlg));
        return TRUE;
    case 'k':
        group_select_prev_widget (GROUP (learn_dlg));
        return TRUE;
    default:
        break;
    }

    /* Prevent from disappearing if a non-defined sequence is pressed
       and contains a button hotkey.  Only recognize hotkeys with ALT.  */
    return (c < 255 && g_ascii_isalnum (c));
}

/* --------------------------------------------------------------------------------------------- */

static cb_ret_t
learn_callback (Widget * w, Widget * sender, widget_msg_t msg, int parm, void *data)
{
    switch (msg)
    {
    case MSG_KEY:
        return learn_check_key (parm) ? MSG_HANDLED : MSG_NOT_HANDLED;

    default:
        return dlg_default_callback (w, sender, msg, parm, data);
    }
}

/* --------------------------------------------------------------------------------------------- */

static void
init_learn (void)
{
    WGroup *g;

    const int dlg_width = 78;
    const int dlg_height = 23;

    /* buttons */
    int bx0, bx1;
    const char *b0 = N_("&Save");
    const char *b1 = N_("&Cancel");
    int bl0, bl1;

    int x, y, i;
    const key_code_name_t *key;

#ifdef ENABLE_NLS
    static gboolean i18n_flag = FALSE;
    if (!i18n_flag)
    {
        learn_title = _(learn_title);
        i18n_flag = TRUE;
    }

    b0 = _(b0);
    b1 = _(b1);
#endif /* ENABLE_NLS */

    do_refresh ();

    learn_dlg =
        dlg_create (TRUE, 0, 0, dlg_height, dlg_width, WPOS_CENTER, FALSE, dialog_colors,
                    learn_callback, NULL, "[Learn keys]", learn_title);
    g = GROUP (learn_dlg);

    /* find first unshown button */
    for (key = key_name_conv_tab, learn_total = 0;
         key->name != NULL && strcmp (key->name, "kpleft") != 0; key++, learn_total++)
        ;

    learnok = 0;
    learnchanged = FALSE;

    learnkeys = g_new (learnkey_t, learn_total);

    x = UX;
    y = UY;

    /* add buttons and "OK" labels */
    for (i = 0; i < learn_total; i++)
    {
        char buffer[BUF_TINY];
        const char *label;
        int padding;

        learnkeys[i].ok = FALSE;
        learnkeys[i].sequence = NULL;

        label = _(key_name_conv_tab[i].longname);
        padding = 16 - str_term_width1 (label);
        padding = MAX (0, padding);
        g_snprintf (buffer, sizeof (buffer), "%s%*s", label, padding, "");

        learnkeys[i].button =
            WIDGET (button_new (y, x, B_USER + i, NARROW_BUTTON, buffer, learn_button));
        learnkeys[i].label = WIDGET (label_new (y, x + 19, ""));
        group_add_widget (g, learnkeys[i].button);
        group_add_widget (g, learnkeys[i].label);

        y++;
        if (y == UY + ROWS)
        {
            x += COLSHIFT;
            y = UY;
        }
    }

    group_add_widget (g, hline_new (dlg_height - 8, -1, -1));
    group_add_widget (g, label_new (dlg_height - 7, 5,
                                    _
                                    ("Press all the keys mentioned here. After you have done it, check\n"
                                     "which keys are not marked with OK. Press space on the missing\n"
                                     "key, or click with the mouse to define it. Move around with Tab.")));
    group_add_widget (g, hline_new (dlg_height - 4, -1, -1));
    /* buttons */
    bl0 = str_term_width1 (b0) + 5;     /* default button */
    bl1 = str_term_width1 (b1) + 3;     /* normal button */
    bx0 = (dlg_width - (bl0 + bl1 + 1)) / 2;
    bx1 = bx0 + bl0 + 1;
    group_add_widget (g, button_new (dlg_height - 3, bx0, B_ENTER, DEFPUSH_BUTTON, b0, NULL));
    group_add_widget (g, button_new (dlg_height - 3, bx1, B_CANCEL, NORMAL_BUTTON, b1, NULL));
}

/* --------------------------------------------------------------------------------------------- */

static void
learn_done (void)
{
    widget_destroy (WIDGET (learn_dlg));
    repaint_screen ();
}

/* --------------------------------------------------------------------------------------------- */

static void
learn_save (void)
{
    int i;
    char *section;
    gboolean profile_changed = FALSE;

    section = g_strconcat ("terminal:", getenv ("TERM"), (char *) NULL);

    for (i = 0; i < learn_total; i++)
        if (learnkeys[i].sequence != NULL)
        {
            char *esc_str;

            esc_str = strutils_escape (learnkeys[i].sequence, -1, ";\\", TRUE);
            mc_config_set_string_raw_value (mc_global.main_config, section,
                                            key_name_conv_tab[i].name, esc_str);
            g_free (esc_str);

            profile_changed = TRUE;
        }

    /* On the one hand no good idea to save the complete setup but 
     * without 'Auto save setup' the new key-definitions will not be 
     * saved unless the user does an 'Options/Save Setup'. 
     * On the other hand a save-button that does not save anything to 
     * disk is much worse.
     */
    if (profile_changed)
        mc_config_save_file (mc_global.main_config, NULL);

    g_free (section);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

void
learn_keys (void)
{
    gboolean save_old_esc_mode = old_esc_mode;
    gboolean save_alternate_plus_minus = mc_global.tty.alternate_plus_minus;
    int result;

    /* old_esc_mode cannot work in learn keys dialog */
    old_esc_mode = 0;

    /* don't translate KP_ADD, KP_SUBTRACT and
       KP_MULTIPLY to '+', '-' and '*' in
       correct_key_code */
    mc_global.tty.alternate_plus_minus = TRUE;
    application_keypad_mode ();

    init_learn ();
    result = dlg_run (learn_dlg);

    old_esc_mode = save_old_esc_mode;
    mc_global.tty.alternate_plus_minus = save_alternate_plus_minus;

    if (!mc_global.tty.alternate_plus_minus)
        numeric_keypad_mode ();

    if (result == B_ENTER)
        learn_save ();

    learn_done ();
}

/* --------------------------------------------------------------------------------------------- */
