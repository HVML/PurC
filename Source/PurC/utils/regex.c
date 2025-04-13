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
#include <glib.h>
#endif

#if HAVE(GLIB)

struct pcregex {
    GRegex *g_regex;
};

struct pcregex_match_info {
    GMatchInfo *g_match_info;
};

static GRegexCompileFlags
to_g_regex_compile_flags(enum pcregex_compile_flags flags)
{
    int ret = 0;
    if (flags & PCREGEX_CASELESS) {
        ret |= G_REGEX_CASELESS;
    }
    if (flags & PCREGEX_MULTILINE) {
        ret |= G_REGEX_MULTILINE;
    }
    if (flags & PCREGEX_DOTALL) {
        ret |= G_REGEX_DOTALL;
    }
    if (flags & PCREGEX_EXTENDED) {
        ret |= G_REGEX_EXTENDED;
    }
    if (flags & PCREGEX_ANCHORED) {
        ret |= G_REGEX_ANCHORED;
    }
    if (flags & PCREGEX_DOLLAR_ENDONLY) {
        ret |= G_REGEX_DOLLAR_ENDONLY;
    }
    if (flags & PCREGEX_UNGREEDY) {
        ret |= G_REGEX_UNGREEDY;
    }
    if (flags & PCREGEX_RAW) {
        ret |= G_REGEX_RAW;
    }
    if (flags & PCREGEX_NO_AUTO_CAPTURE) {
        ret |= G_REGEX_NO_AUTO_CAPTURE;
    }
    if (flags & PCREGEX_OPTIMIZE) {
        ret |= G_REGEX_OPTIMIZE;
    }
    if (flags & PCREGEX_FIRSTLINE) {
        ret |= G_REGEX_FIRSTLINE;
    }
    if (flags & PCREGEX_DUPNAMES) {
        ret |= G_REGEX_DUPNAMES;
    }
    if (flags & PCREGEX_NEWLINE_CR) {
        ret |= G_REGEX_NEWLINE_CR;
    }
    if (flags & PCREGEX_NEWLINE_LF) {
        ret |= G_REGEX_NEWLINE_LF;
    }
    if (flags & PCREGEX_NEWLINE_CRLF) {
        ret |= G_REGEX_NEWLINE_CRLF;
    }
    if (flags & PCREGEX_NEWLINE_ANYCRLF) {
        ret |= G_REGEX_NEWLINE_ANYCRLF;
    }
    if (flags & PCREGEX_BSR_ANYCRLF) {
        ret |= G_REGEX_BSR_ANYCRLF;
    }
    /* deprecated
    if (flags & PCREGEX_JAVASCRIPT_COMPAT) {
        ret |= G_REGEX_JAVASCRIPT_COMPAT;
    }*/

    return (GRegexCompileFlags)ret;
}

static GRegexMatchFlags
to_g_regex_match_flags(enum pcregex_match_flags flags)
{
    int ret = 0;
    if (flags & PCREGEX_MATCH_ANCHORED) {
        ret |= G_REGEX_MATCH_ANCHORED;
    }
    if (flags & PCREGEX_MATCH_NOTBOL) {
        ret |= G_REGEX_MATCH_NOTBOL;
    }
    if (flags & PCREGEX_MATCH_NOTEOL) {
        ret |= G_REGEX_MATCH_NOTEOL;
    }
    if (flags & PCREGEX_MATCH_NOTEMPTY) {
        ret |= G_REGEX_MATCH_NOTEMPTY;
    }
    if (flags & PCREGEX_MATCH_PARTIAL) {
        ret |= G_REGEX_MATCH_PARTIAL;
    }
    if (flags & PCREGEX_MATCH_NEWLINE_CR) {
        ret |= G_REGEX_MATCH_NEWLINE_CR;
    }
    if (flags & PCREGEX_MATCH_NEWLINE_LF) {
        ret |= G_REGEX_MATCH_NEWLINE_LF;
    }
    if (flags & PCREGEX_MATCH_NEWLINE_CRLF) {
        ret |= G_REGEX_MATCH_NEWLINE_CRLF;
    }
    if (flags & PCREGEX_MATCH_NEWLINE_ANY) {
        ret |= G_REGEX_MATCH_NEWLINE_ANY;
    }
    if (flags & PCREGEX_MATCH_NEWLINE_ANYCRLF) {
        ret |= G_REGEX_MATCH_NEWLINE_ANYCRLF;
    }
    if (flags & PCREGEX_MATCH_BSR_ANYCRLF) {
        ret |= G_REGEX_MATCH_BSR_ANYCRLF;
    }
    if (flags & PCREGEX_MATCH_BSR_ANY) {
        ret |= G_REGEX_MATCH_BSR_ANY;
    }
    if (flags & PCREGEX_MATCH_PARTIAL_SOFT) {
        ret |= G_REGEX_MATCH_PARTIAL_SOFT;
    }
    if (flags & PCREGEX_MATCH_PARTIAL_HARD) {
        ret |= G_REGEX_MATCH_PARTIAL_HARD;
    }
    if (flags & PCREGEX_MATCH_NOTEMPTY_ATSTART) {
        ret |= G_REGEX_MATCH_NOTEMPTY_ATSTART;
    }
    return (GRegexMatchFlags)ret;
}

static void
set_error_code_from_gerror(GError *err)
{
    if (!err) {
        return;
    }

    int error_code = PURC_ERROR_OK;

    switch (err->code) {
    case G_REGEX_ERROR_COMPILE:
    case G_REGEX_ERROR_OPTIMIZE:
    case G_REGEX_ERROR_REPLACE:
    case G_REGEX_ERROR_MATCH:
    case G_REGEX_ERROR_INTERNAL:
    case G_REGEX_ERROR_STRAY_BACKSLASH:
    case G_REGEX_ERROR_MISSING_CONTROL_CHAR:
    case G_REGEX_ERROR_UNRECOGNIZED_ESCAPE:
    case G_REGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER:
    case G_REGEX_ERROR_QUANTIFIER_TOO_BIG:
    case G_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS:
    case G_REGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS:
    case G_REGEX_ERROR_RANGE_OUT_OF_ORDER:
    case G_REGEX_ERROR_NOTHING_TO_REPEAT:
    case G_REGEX_ERROR_UNRECOGNIZED_CHARACTER:
    case G_REGEX_ERROR_POSIX_NAMED_CLASS_OUTSIDE_CLASS:
    case G_REGEX_ERROR_UNMATCHED_PARENTHESIS:
    case G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE:
    case G_REGEX_ERROR_UNTERMINATED_COMMENT:
    case G_REGEX_ERROR_EXPRESSION_TOO_LARGE:
    case G_REGEX_ERROR_MEMORY_ERROR:
    case G_REGEX_ERROR_VARIABLE_LENGTH_LOOKBEHIND:
    case G_REGEX_ERROR_MALFORMED_CONDITION:
    case G_REGEX_ERROR_TOO_MANY_CONDITIONAL_BRANCHES:
    case G_REGEX_ERROR_ASSERTION_EXPECTED:
    case G_REGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME:
    case G_REGEX_ERROR_POSIX_COLLATING_ELEMENTS_NOT_SUPPORTED:
    case G_REGEX_ERROR_HEX_CODE_TOO_LARGE:
    case G_REGEX_ERROR_INVALID_CONDITION:
    case G_REGEX_ERROR_SINGLE_BYTE_MATCH_IN_LOOKBEHIND:
    case G_REGEX_ERROR_INFINITE_LOOP:
    case G_REGEX_ERROR_MISSING_SUBPATTERN_NAME_TERMINATOR:
    case G_REGEX_ERROR_DUPLICATE_SUBPATTERN_NAME:
    case G_REGEX_ERROR_MALFORMED_PROPERTY:
    case G_REGEX_ERROR_UNKNOWN_PROPERTY:
    case G_REGEX_ERROR_SUBPATTERN_NAME_TOO_LONG:
    case G_REGEX_ERROR_TOO_MANY_SUBPATTERNS:
    case G_REGEX_ERROR_INVALID_OCTAL_VALUE:
    case G_REGEX_ERROR_TOO_MANY_BRANCHES_IN_DEFINE:
    case G_REGEX_ERROR_DEFINE_REPETION:
    case G_REGEX_ERROR_INCONSISTENT_NEWLINE_OPTIONS:
    case G_REGEX_ERROR_MISSING_BACK_REFERENCE:
    case G_REGEX_ERROR_INVALID_RELATIVE_REFERENCE:
    case G_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_FORBIDDEN:
    case G_REGEX_ERROR_UNKNOWN_BACKTRACKING_CONTROL_VERB :
    case G_REGEX_ERROR_NUMBER_TOO_BIG:
    case G_REGEX_ERROR_MISSING_SUBPATTERN_NAME:
    case G_REGEX_ERROR_MISSING_DIGIT:
    case G_REGEX_ERROR_INVALID_DATA_CHARACTER:
    case G_REGEX_ERROR_EXTRA_SUBPATTERN_NAME:
    case G_REGEX_ERROR_BACKTRACKING_CONTROL_VERB_ARGUMENT_REQUIRED:
    case G_REGEX_ERROR_INVALID_CONTROL_CHAR:
    case G_REGEX_ERROR_MISSING_NAME:
    case G_REGEX_ERROR_NOT_SUPPORTED_IN_CLASS:
    case G_REGEX_ERROR_TOO_MANY_FORWARD_REFERENCES:
    case G_REGEX_ERROR_NAME_TOO_LONG:
    case G_REGEX_ERROR_CHARACTER_VALUE_TOO_LARGE:
        error_code = PURC_ERROR_INVALID_VALUE;
        break;
    }

    purc_set_error_with_info(error_code, "%s", err->message);
    g_error_free(err);
}

bool pcregex_is_match_ex(const char *pattern, const char *str,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    if (!pattern || !pattern[0] || !str || !str[0]) {
        return false;
    }
    return g_regex_match_simple(pattern, str,
            to_g_regex_compile_flags(compile_options),
            to_g_regex_match_flags(match_options));
}

bool pcregex_is_match(const char *pattern, const char *str)
{
    return pcregex_is_match_ex(pattern, str, 0, 0);
}

struct pcregex *pcregex_new_ex(const char *pattern,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    struct pcregex *regex = (struct pcregex *) malloc(sizeof(struct pcregex));
    if (!regex) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    GError *err = NULL;
    regex->g_regex = g_regex_new(pattern,
            to_g_regex_compile_flags(compile_options),
            to_g_regex_match_flags(match_options),
            &err);
    if (regex->g_regex) {
        return regex;
    }

    free(regex);
    set_error_code_from_gerror(err);
    return NULL;
}

struct pcregex *pcregex_new(const char *pattern)
{
    return pcregex_new_ex(pattern, 0, 0);
}

void pcregex_destroy(struct pcregex *regex)
{
    if (!regex) {
        return;
    }
    g_regex_unref(regex->g_regex);
    free(regex);
}

bool pcregex_match_ex(struct pcregex *regex, const char *str,
            enum pcregex_match_flags match_options,
            struct pcregex_match_info **match_info)
{
    if (!regex || !str) {
        if (match_info) {
            *match_info = NULL;
        }
        return false;
    }

    bool ret;
    GMatchInfo *g_match_info = NULL;
    if (match_info) {
        ret = g_regex_match(regex->g_regex, str,
                to_g_regex_match_flags(match_options),
                &g_match_info);
    }
    else {
        ret = g_regex_match(regex->g_regex, str,
                to_g_regex_match_flags(match_options),
                NULL);
    }

    if (ret && match_info) {
        *match_info = (struct pcregex_match_info *)malloc(
                sizeof(struct pcregex_match_info));
        if (*match_info == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            g_match_info_free(g_match_info);
            return false;
        }
        (*match_info)->g_match_info = g_match_info;
    }
    else if (g_match_info) {
        g_match_info_free(g_match_info);
        if (match_info) {
            *match_info = NULL;
        }
    }
    return ret;
}

bool pcregex_match(struct pcregex *regex, const char *str,
            struct pcregex_match_info **match_info)
{
    return pcregex_match_ex(regex, str, 0, match_info);
}

bool pcregex_match_info_matches(const struct pcregex_match_info *match_info)
{
    if (!match_info) {
        return false;
    }

    return g_match_info_matches(match_info->g_match_info);
}

bool pcregex_match_info_next(const struct pcregex_match_info *match_info)
{
    if (!match_info) {
        return false;
    }
    GError *err = NULL;
    bool ret = g_match_info_next(match_info->g_match_info, &err);
    if (!ret) {
        set_error_code_from_gerror(err);
    }
    return ret;
}

char *pcregex_match_info_fetch(const struct pcregex_match_info *match_info,
            int match_num)
{
    if (!match_info) {
        return NULL;
    }
    char *word = g_match_info_fetch(match_info->g_match_info, match_num);
    if (!word) {
        return NULL;
    }

    char *ret = strdup(word);
    g_free(word);
    return ret;
}

void pcregex_match_info_destroy(struct pcregex_match_info *match_info)
{
    if (match_info) {
        g_match_info_free(match_info->g_match_info);
        free(match_info);
    }
}

#else /* HAVA(GLIB) */

bool pcregex_is_match_ex(const char *pattern, const char *str,
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

bool pcregex_is_match(const char *pattern, const char *str)
{
    return pcregex_is_match_ex(pattern, str, 0, 0);
}

struct pcregex *pcregex_new_ex(const char *pattern,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options)
{
    UNUSED_PARAM(pattern);
    UNUSED_PARAM(compile_options);
    UNUSED_PARAM(match_options);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

struct pcregex *pcregex_new(const char *pattern)
{
    return pcregex_new_ex(pattern, 0, 0);
}

void pcregex_destroy(struct pcregex *regex)
{
    UNUSED_PARAM(regex);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
}

bool pcregex_match_ex(struct pcregex *regex, const char *str,
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

void pcregex_match_info_destroy(struct pcregex_match_info *match_info)
{
    UNUSED_PARAM(match_info);
    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
}

#endif /* HAVA(GLIB) */
