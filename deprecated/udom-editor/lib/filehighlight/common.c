/*
   File highlight plugin.
   Interface functions

   Copyright (C) 2009-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2009.

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

#include <config.h>

#include "lib/global.h"
#include "lib/filehighlight.h"
#include "internal.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

static void
mc_fhl_filter_free (void *data)
{
    mc_fhl_filter_t *filter = (mc_fhl_filter_t *) data;

    g_free (filter->fgcolor);
    g_free (filter->bgcolor);
    mc_search_free (filter->search_condition);
    g_free (filter);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_fhl_array_free (mc_fhl_t * fhl)
{
    if (fhl->filters != NULL)
    {
        g_ptr_array_foreach (fhl->filters, (GFunc) mc_fhl_filter_free, NULL);
        fhl->filters = (GPtrArray *) g_ptr_array_free (fhl->filters, TRUE);
    }
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

mc_fhl_t *
mc_fhl_new (gboolean need_auto_fill)
{
    mc_fhl_t *fhl;

    fhl = g_try_new0 (mc_fhl_t, 1);
    if (fhl == NULL)
        return NULL;

    if (!need_auto_fill)
        return fhl;

    if (!mc_fhl_init_from_standard_files (fhl))
    {
        g_free (fhl);
        return NULL;
    }

    if (!mc_fhl_parse_ini_file (fhl))
    {
        mc_fhl_free (&fhl);
        return NULL;
    }

    return fhl;
}

/* --------------------------------------------------------------------------------------------- */

void
mc_fhl_free (mc_fhl_t ** fhl)
{
    if (fhl == NULL || *fhl == NULL)
        return;

    mc_fhl_clear (*fhl);

    MC_PTR_FREE (*fhl);
}

/* --------------------------------------------------------------------------------------------- */

void
mc_fhl_clear (mc_fhl_t * fhl)
{
    if (fhl != NULL)
    {
        mc_config_deinit (fhl->config);
        mc_fhl_array_free (fhl);
    }
}

/* --------------------------------------------------------------------------------------------- */
