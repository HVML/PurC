/*
 * @file regex.c
 * @author XueShuming
 * @date 2022/04/25
 * @brief The API for regex.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "private/regex.h"

#if HAVE(GLIB)

struct pcregex {
};

struct pcregex_match_info {
};

bool pcregex_is_match(const char *pattern, const char *str,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    UNUSED_PARAM(pattern);
    UNUSED_PARAM(str);
    UNUSED_PARAM(compile_options);
    UNUSED_PARAM(match_options);

    return false;
}

struct pcregex *pcregex_new(const char *pattern,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    UNUSED_PARAM(pattern);
    UNUSED_PARAM(compile_options);
    UNUSED_PARAM(match_options);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

void pcregex_destroy(struct pcregex *regex)
{
    UNUSED_PARAM(regex);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
}

bool pcregex_match(struct pcregex *regex, const char *str,
            enum pcregex_match_flags match_options,
            struct pcregex_match_info **match_info)
{
    UNUSED_PARAM(regex);
    UNUSED_PARAM(str);
    UNUSED_PARAM(match_options);
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

bool pcregex_match_info_matches(const struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

bool pcregex_match_info_next(const struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

char *pcregex_match_info_fetch(const struct pcregex_match_info *match_info,
            int match_num)
{
    UNUSED_PARAM(match_info);
    UNUSED_PARAM(match_num);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

void pcregex_match_info_desroy(struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
}

#else /* HAVA(GLIB) */

bool pcregex_is_match(const char *pattern, const char *str,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    UNUSED_PARAM(pattern);
    UNUSED_PARAM(str);
    UNUSED_PARAM(compile_options);
    UNUSED_PARAM(match_options);

    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

struct pcregex *pcregex_new(const char *pattern,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    UNUSED_PARAM(pattern);
    UNUSED_PARAM(compile_options);
    UNUSED_PARAM(match_options);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

void pcregex_destroy(struct pcregex *regex)
{
    UNUSED_PARAM(regex);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
}

bool pcregex_match(struct pcregex *regex, const char *str,
            enum pcregex_match_flags match_options,
            struct pcregex_match_info **match_info)
{
    UNUSED_PARAM(regex);
    UNUSED_PARAM(str);
    UNUSED_PARAM(match_options);
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

bool pcregex_match_info_matches(const struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

bool pcregex_match_info_next(const struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

char *pcregex_match_info_fetch(const struct pcregex_match_info *match_info,
            int match_num)
{
    UNUSED_PARAM(match_info);
    UNUSED_PARAM(match_num);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

void pcregex_match_info_desroy(struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
}

#endif /* HAVA(GLIB) */
