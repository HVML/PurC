/*
 * @file helper.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of tools for all files in this directory.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "helper.h"

#if USE(GLIB)
#include <glib.h>
#endif

const char *pcdvobjs_remove_space (char *buffer)
{
    int i = 0;
    int j = 0;
    while (*(buffer + i) != 0x00) {
        if (*(buffer + i) != ' ') {
            *(buffer + j) = *(buffer + i);
            j++;
        }
        i++;
    }
    *(buffer + j) = 0x00;

    return buffer;
}

#if USE(GLIB)
bool pcdvobjs_wildcard_cmp (const char *str1, const char *pattern)
{
    GPatternSpec *glib_pattern = g_pattern_spec_new (pattern);

#if GLIB_CHECK_VERSION(2, 70, 0)
    gboolean result = g_pattern_spec_match_string (glib_pattern, str1);
#else
    gboolean result = g_pattern_match_string (glib_pattern, str1);
#endif

    g_pattern_spec_free (glib_pattern);

    return (bool)result;
}
#else
bool pcdvobjs_wildcard_cmp (const char *str1, const char *pattern)
{
    if (str1 == NULL)
        return false;
    if (pattern == NULL)
        return false;

    int len1 = strlen (str1);
    int len2 = strlen (pattern);
    int mark = 0;
    int p1 = 0;
    int p2 = 0;

    while ((p1 < len1) && (p2<len2)) {
        if (pattern[p2] == '?') {
            p1++;
            p2++;
            continue;
        }
        if (pattern[p2] == '*') {
            p2++;
            mark = p2;
            continue;
        }
        if (str1[p1] != pattern[p2]) {
            if (p1 == 0 && p2 == 0)
                return false;
            p1 -= p2 - mark - 1;
            p2 = mark;
            continue;
        }
        p1++;
        p2++;
    }
    if (p2 == len2) {
        if (p1 == len1)
            return true;
        if (pattern[p2 - 1] == '*')
            return true;
    }
    while (p2 < len2) {
        if (pattern[p2] != '*')
            return false;
        p2++;
    }
    return true;
}
#endif

purc_variant_t
purc_dvobj_make_from_methods(const struct purc_dvobj_method *methods,
        size_t size)
{
    size_t i = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (i = 0; i < size; i++) {
        val = purc_variant_make_dynamic(methods[i].getter, methods[i].setter);
        if (val == PURC_VARIANT_INVALID) {
            goto error;
        }

        if (!purc_variant_object_set_by_static_ckey(ret_var,
                    methods[i].name, val)) {
            goto error;
        }

        purc_variant_unref(val);
    }

    return ret_var;

error:
    purc_variant_unref(ret_var);
    return PURC_VARIANT_INVALID;
}

