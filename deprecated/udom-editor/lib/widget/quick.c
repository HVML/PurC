/*
   Widget based utility functions.

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Authors:
   Miguel de Icaza, 1994, 1995, 1996
   Radek Doulik, 1994, 1995
   Jakub Jelinek, 1995
   Andrej Borsenkow, 1995
   Andrew Borodin <aborodin@vmail.ru>, 2009, 2010, 2011, 2012, 2013

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

/** \file quick.c
 *  \brief Source: quick dialog engine
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>              /* fprintf() */

#include "lib/global.h"
#include "lib/strutil.h"        /* str_term_width1() */
#include "lib/util.h"           /* tilde_expand() */
#include "lib/widget.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#ifdef ENABLE_NLS
#define I18N(x) (x = x != NULL && *x != '\0' ? _(x) : x)
#else
#define I18N(x) (x = x)
#endif

/*** file scope type declarations ****************************************************************/

typedef struct
{
    Widget *widget;
    quick_widget_t *quick_widget;
} quick_widget_item_t;

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static WInput *
quick_create_input (int y, int x, const quick_widget_t * qw)
{
    WInput *in;

    in = input_new (y, x, input_colors, 8, qw->u.input.text, qw->u.input.histname,
                    qw->u.input.completion_flags);

    in->is_password = qw->u.input.is_passwd;
    in->strip_password = qw->u.input.strip_passwd;

    return in;
}

/* --------------------------------------------------------------------------------------------- */

static void
quick_create_labeled_input (GArray * widgets, int *y, int x, quick_widget_t * quick_widget,
                            int *width)
{
    quick_widget_item_t in, label;

    label.quick_widget = g_new0 (quick_widget_t, 1);
    label.quick_widget->widget_type = quick_label;
    label.quick_widget->options = quick_widget->options;
    label.quick_widget->state = quick_widget->state;
    /* FIXME: this should be turned in depend of label_location */
    label.quick_widget->pos_flags = quick_widget->pos_flags;

    switch (quick_widget->u.input.label_location)
    {
    case input_label_above:
        label.widget = WIDGET (label_new (*y, x, I18N (quick_widget->u.input.label_text)));
        *y += label.widget->lines - 1;
        g_array_append_val (widgets, label);

        in.widget = WIDGET (quick_create_input (++(*y), x, quick_widget));
        in.quick_widget = quick_widget;
        g_array_append_val (widgets, in);

        *width = MAX (label.widget->cols, in.widget->cols);
        break;

    case input_label_left:
        label.widget = WIDGET (label_new (*y, x, I18N (quick_widget->u.input.label_text)));
        g_array_append_val (widgets, label);

        in.widget = WIDGET (quick_create_input (*y, x + label.widget->cols + 1, quick_widget));
        in.quick_widget = quick_widget;
        g_array_append_val (widgets, in);

        *width = label.widget->cols + in.widget->cols + 1;
        break;

    case input_label_right:
        in.widget = WIDGET (quick_create_input (*y, x, quick_widget));
        in.quick_widget = quick_widget;
        g_array_append_val (widgets, in);

        label.widget =
            WIDGET (label_new
                    (*y, x + in.widget->cols + 1, I18N (quick_widget->u.input.label_text)));
        g_array_append_val (widgets, label);

        *width = label.widget->cols + in.widget->cols + 1;
        break;

    case input_label_below:
        in.widget = WIDGET (quick_create_input (*y, x, quick_widget));
        in.quick_widget = quick_widget;
        g_array_append_val (widgets, in);

        label.widget = WIDGET (label_new (++(*y), x, I18N (quick_widget->u.input.label_text)));
        *y += label.widget->lines - 1;
        g_array_append_val (widgets, label);

        *width = MAX (label.widget->cols, in.widget->cols);
        break;

    default:
        return;
    }

    INPUT (in.widget)->label = LABEL (label.widget);
    /* cross references */
    label.quick_widget->u.label.input = in.quick_widget;
    in.quick_widget->u.input.label = label.quick_widget;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

int
quick_dialog_skip (quick_dialog_t * quick_dlg, int nskip)
{
    int len;
    int blen = 0;
    int x, y;                   /* current positions */
    int y1 = 0;                 /* bottom of 1st column in case of two columns */
    int y2 = -1;                /* start of two columns */
    int width1 = 0;             /* width of single column */
    int width2 = 0;             /* width of each of two columns */
    gboolean have_groupbox = FALSE;
    gboolean two_columns = FALSE;
    gboolean put_buttons = FALSE;

    /* x position of 1st column is 3 */
    const int x1 = 3;
    /* x position of 2nd column is 4 and it will be fixed later, after creation of all widgets */
    int x2 = 4;

    GArray *widgets;
    size_t i;
    quick_widget_t *quick_widget;
    WGroupbox *g = NULL;
    WDialog *dd;
    GList *input_labels = NULL; /* Widgets not directly requested by the user. */
    int return_val;

    len = str_term_width1 (I18N (quick_dlg->title)) + 6;
    quick_dlg->cols = MAX (quick_dlg->cols, len);

    y = 1;
    x = x1;

    /* create widgets */
    widgets = g_array_sized_new (FALSE, FALSE, sizeof (quick_widget_item_t), 8);

    for (quick_widget = quick_dlg->widgets; quick_widget->widget_type != quick_end; quick_widget++)
    {
        quick_widget_item_t item = { NULL, quick_widget };
        int width = 0;

        switch (quick_widget->widget_type)
        {
        case quick_checkbox:
            item.widget =
                WIDGET (check_new
                        (++y, x, *quick_widget->u.checkbox.state,
                         I18N (quick_widget->u.checkbox.text)));
            g_array_append_val (widgets, item);
            width = item.widget->cols;
            if (g != NULL)
                width += 2;
            if (two_columns)
                width2 = MAX (width2, width);
            else
                width1 = MAX (width1, width);
            break;

        case quick_button:
            /* single button */
            item.widget = WIDGET (button_new (++y, x, quick_widget->u.button.action,
                                              quick_widget->u.button.action == B_ENTER ?
                                              DEFPUSH_BUTTON : NORMAL_BUTTON,
                                              I18N (quick_widget->u.button.text),
                                              quick_widget->u.button.callback));
            g_array_append_val (widgets, item);
            width = item.widget->cols;
            if (g != NULL)
                width += 2;
            if (two_columns)
                width2 = MAX (width2, width);
            else
                width1 = MAX (width1, width);
            break;

        case quick_input:
            *quick_widget->u.input.result = NULL;
            y++;
            if (quick_widget->u.input.label_location != input_label_none)
            {
                quick_create_labeled_input (widgets, &y, x, quick_widget, &width);
                input_labels = g_list_prepend (input_labels, quick_widget->u.input.label);
            }
            else
            {
                item.widget = WIDGET (quick_create_input (y, x, quick_widget));
                g_array_append_val (widgets, item);
                width = item.widget->cols;
            }
            if (g != NULL)
                width += 2;
            if (two_columns)
                width2 = MAX (width2, width);
            else
                width1 = MAX (width1, width);
            break;

        case quick_label:
            item.widget = WIDGET (label_new (++y, x, I18N (quick_widget->u.label.text)));
            g_array_append_val (widgets, item);
            y += item.widget->lines - 1;
            width = item.widget->cols;
            if (g != NULL)
                width += 2;
            if (two_columns)
                width2 = MAX (width2, width);
            else
                width1 = MAX (width1, width);
            break;

        case quick_radio:
            {
                WRadio *r;
                char **items = NULL;

                /* create the copy of radio_items to avoid mwmory leak */
                items = g_new (char *, quick_widget->u.radio.count + 1);
                for (i = 0; i < (size_t) quick_widget->u.radio.count; i++)
                    items[i] = g_strdup (_(quick_widget->u.radio.items[i]));
                items[i] = NULL;

                r = radio_new (++y, x, quick_widget->u.radio.count, (const char **) items);
                r->pos = r->sel = *quick_widget->u.radio.value;
                g_strfreev (items);
                item.widget = WIDGET (r);
                g_array_append_val (widgets, item);
                y += item.widget->lines - 1;
                width = item.widget->cols;
                if (g != NULL)
                    width += 2;
                if (two_columns)
                    width2 = MAX (width2, width);
                else
                    width1 = MAX (width1, width);
            }
            break;

        case quick_start_groupbox:
            I18N (quick_widget->u.groupbox.title);
            len = str_term_width1 (quick_widget->u.groupbox.title);
            g = groupbox_new (++y, x, 1, len + 4, quick_widget->u.groupbox.title);
            item.widget = WIDGET (g);
            g_array_append_val (widgets, item);
            have_groupbox = TRUE;
            break;

        case quick_stop_groupbox:
            if (g != NULL)
            {
                Widget *w = WIDGET (g);

                y++;
                w->lines = y + 1 - w->y;
                g = NULL;

                g_array_append_val (widgets, item);
            }
            break;

        case quick_separator:
            y++;
            if (quick_widget->u.separator.line)
            {
                item.widget = WIDGET (hline_new (y, x, 1));
                g_array_append_val (widgets, item);
            }
            break;

        case quick_start_columns:
            y2 = y;
            g_array_append_val (widgets, item);
            two_columns = TRUE;
            break;

        case quick_next_column:
            x = x2;
            y1 = y;
            y = y2;
            break;

        case quick_stop_columns:
            x = x1;
            y = MAX (y1, y);
            g_array_append_val (widgets, item);
            two_columns = FALSE;
            break;

        case quick_buttons:
            /* start put several buttons in bottom line */
            if (quick_widget->u.separator.space)
            {
                y++;

                if (quick_widget->u.separator.line)
                    item.widget = WIDGET (hline_new (y, 1, -1));
            }

            g_array_append_val (widgets, item);

            /* several buttons in bottom line */
            y++;
            quick_widget++;
            for (; quick_widget->widget_type == quick_button; quick_widget++)
            {
                item.widget = WIDGET (button_new (y, x++, quick_widget->u.button.action,
                                                  quick_widget->u.button.action == B_ENTER ?
                                                  DEFPUSH_BUTTON : NORMAL_BUTTON,
                                                  I18N (quick_widget->u.button.text),
                                                  quick_widget->u.button.callback));
                item.quick_widget = quick_widget;
                g_array_append_val (widgets, item);
                blen += item.widget->cols + 1;
            }

            /* stop dialog build here */
            blen--;
            quick_widget->widget_type = quick_end;
            quick_widget--;
            break;

        default:
            break;
        }
    }

    /* adjust dialog width */
    quick_dlg->cols = MAX (quick_dlg->cols, blen + 6);
    if (have_groupbox)
    {
        if (width1 != 0)
            width1 += 2;
        if (width2 != 0)
            width2 += 2;
    }
    if (width2 == 0)
        len = width1 + 6;
    else
    {
        len = width2 * 2 + 7;
        if (width1 != 0)
            len = MAX (len, width1 + 6);
    }

    quick_dlg->cols = MAX (quick_dlg->cols, len);
    width1 = quick_dlg->cols - 6;
    width2 = (quick_dlg->cols - 7) / 2;

    if (quick_dlg->x == -1 || quick_dlg->y == -1)
        dd = dlg_create (TRUE, 0, 0, y + 3, quick_dlg->cols, WPOS_CENTER | WPOS_TRYUP, FALSE,
                         dialog_colors, quick_dlg->callback, quick_dlg->mouse_callback,
                         quick_dlg->help, quick_dlg->title);
    else
        dd = dlg_create (TRUE, quick_dlg->y, quick_dlg->x, y + 3, quick_dlg->cols,
                         WPOS_KEEP_DEFAULT, FALSE, dialog_colors, quick_dlg->callback,
                         quick_dlg->mouse_callback, quick_dlg->help, quick_dlg->title);

    /* add widgets into the dialog */
    x2 = x1 + width2 + 1;
    g = NULL;
    two_columns = FALSE;
    x = (WIDGET (dd)->cols - blen) / 2;

    for (i = 0; i < widgets->len; i++)
    {
        quick_widget_item_t *item;
        int column_width;

        item = &g_array_index (widgets, quick_widget_item_t, i);
        column_width = two_columns ? width2 : width1;

        /* adjust widget width and x position */
        switch (item->quick_widget->widget_type)
        {
        case quick_label:
            {
                quick_widget_t *input = item->quick_widget->u.label.input;

                if (input != NULL && input->u.input.label_location == input_label_right)
                {
                    /* location of this label will be adjusted later */
                    break;
                }
            }
            MC_FALLTHROUGH;
        case quick_checkbox:
        case quick_radio:
            if (item->widget->x != x1)
                item->widget->x = x2;
            if (g != NULL)
                item->widget->x += 2;
            break;

        case quick_button:
            if (!put_buttons)
            {
                if (item->widget->x != x1)
                    item->widget->x = x2;
                if (g != NULL)
                    item->widget->x += 2;
            }
            else
            {
                item->widget->x = x;
                x += item->widget->cols + 1;
            }
            break;

        case quick_input:
            {
                Widget *label = WIDGET (INPUT (item->widget)->label);
                int width = column_width;

                if (g != NULL)
                    width -= 4;

                switch (item->quick_widget->u.input.label_location)
                {
                case input_label_left:
                    /* label was adjusted before; adjust input line */
                    item->widget->x = label->x + label->cols + 1 - WIDGET (label->owner)->x;
                    item->widget->cols = width - label->cols - 1;
                    break;

                case input_label_right:
                    if (item->widget->x != x1)
                        item->widget->x = x2;
                    if (g != NULL)
                        item->widget->x += 2;
                    item->widget->cols = width - label->cols - 1;
                    label->x = item->widget->x + item->widget->cols + 1;
                    break;

                default:
                    if (item->widget->x != x1)
                        item->widget->x = x2;
                    if (g != NULL)
                        item->widget->x += 2;
                    item->widget->cols = width;
                    break;
                }

                /* forced update internal variables of inpuit line */
                widget_set_size (item->widget, item->widget->y, item->widget->x, 1,
                                 item->widget->cols);
            }
            break;

        case quick_start_groupbox:
            g = GROUPBOX (item->widget);
            if (item->widget->x != x1)
                item->widget->x = x2;
            item->widget->cols = column_width;
            break;

        case quick_stop_groupbox:
            g = NULL;
            break;

        case quick_separator:
            if (item->widget != NULL)
            {
                if (g != NULL)
                {
                    Widget *wg = WIDGET (g);

                    HLINE (item->widget)->auto_adjust_cols = FALSE;
                    item->widget->x = wg->x + 1 - WIDGET (wg->owner)->x;
                    item->widget->cols = wg->cols;
                }
                else if (two_columns)
                {
                    HLINE (item->widget)->auto_adjust_cols = FALSE;
                    if (item->widget->x != x1)
                        item->widget->x = x2;
                    item->widget->x--;
                    item->widget->cols = column_width + 2;
                }
                else
                    HLINE (item->widget)->auto_adjust_cols = TRUE;
            }
            break;

        case quick_start_columns:
            two_columns = TRUE;
            break;

        case quick_stop_columns:
            two_columns = FALSE;
            break;

        case quick_buttons:
            /* several buttons in bottom line */
            put_buttons = TRUE;
            break;

        default:
            break;
        }

        if (item->widget != NULL)
        {
            unsigned long id;

            /* add widget into dialog */
            item->widget->options |= item->quick_widget->options;       /* FIXME: cannot reset flags, setup only */
            item->widget->state |= item->quick_widget->state;   /* FIXME: cannot reset flags, setup only */
            id = group_add_widget_autopos (GROUP (dd), item->widget, item->quick_widget->pos_flags,
                                           NULL);
            if (item->quick_widget->id != NULL)
                *item->quick_widget->id = id;
        }
    }

    /* skip frame widget */
    if (dd->bg != NULL)
        nskip++;

    while (nskip-- != 0)
        group_set_current_widget_next (GROUP (dd));

    return_val = dlg_run (dd);

    /* Get the data if we found something interesting */
    if (return_val != B_CANCEL)
        for (i = 0; i < widgets->len; i++)
        {
            quick_widget_item_t *item;

            item = &g_array_index (widgets, quick_widget_item_t, i);

            switch (item->quick_widget->widget_type)
            {
            case quick_checkbox:
                *item->quick_widget->u.checkbox.state = CHECK (item->widget)->state;
                break;

            case quick_input:
                if ((item->quick_widget->u.input.completion_flags & INPUT_COMPLETE_CD) != 0)
                    *item->quick_widget->u.input.result =
                        tilde_expand (INPUT (item->widget)->buffer);
                else
                    *item->quick_widget->u.input.result = g_strdup (INPUT (item->widget)->buffer);
                break;

            case quick_radio:
                *item->quick_widget->u.radio.value = RADIO (item->widget)->sel;
                break;

            default:
                break;
            }
        }

    widget_destroy (WIDGET (dd));

    g_list_free_full (input_labels, g_free);    /* destroy input labels created before */
    g_array_free (widgets, TRUE);

    return return_val;
}

/* --------------------------------------------------------------------------------------------- */
