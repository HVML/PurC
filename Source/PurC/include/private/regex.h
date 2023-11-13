/*
 * @file regex.h
 * @author XueShuming
 * @date 2022/04/25
 * @brief The interfaces for regex.
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

#ifndef PURC_PRIVATE_REGEX_H
#define PURC_PRIVATE_REGEX_H

#include <stddef.h>
#include <stdint.h>

enum pcregex_compile_flags {
    PCREGEX_CASELESS          = 1 << 0,
    PCREGEX_MULTILINE         = 1 << 1,
    PCREGEX_DOTALL            = 1 << 2,
    PCREGEX_EXTENDED          = 1 << 3,
    PCREGEX_ANCHORED          = 1 << 4,
    PCREGEX_DOLLAR_ENDONLY    = 1 << 5,
    PCREGEX_UNGREEDY          = 1 << 9,
    PCREGEX_RAW               = 1 << 11,
    PCREGEX_NO_AUTO_CAPTURE   = 1 << 12,
    PCREGEX_OPTIMIZE          = 1 << 13,
    PCREGEX_FIRSTLINE         = 1 << 18,
    PCREGEX_DUPNAMES          = 1 << 19,
    PCREGEX_NEWLINE_CR        = 1 << 20,
    PCREGEX_NEWLINE_LF        = 1 << 21,
    PCREGEX_NEWLINE_CRLF      = PCREGEX_NEWLINE_CR | PCREGEX_NEWLINE_LF,
    PCREGEX_NEWLINE_ANYCRLF   = PCREGEX_NEWLINE_CR | 1 << 22,
    PCREGEX_BSR_ANYCRLF       = 1 << 23,
    // deprecated; PCREGEX_JAVASCRIPT_COMPAT = 1 << 25
};

enum pcregex_match_flags {
    PCREGEX_MATCH_ANCHORED         = 1 << 4,
    PCREGEX_MATCH_NOTBOL           = 1 << 7,
    PCREGEX_MATCH_NOTEOL           = 1 << 8,
    PCREGEX_MATCH_NOTEMPTY         = 1 << 10,
    PCREGEX_MATCH_PARTIAL          = 1 << 15,
    PCREGEX_MATCH_NEWLINE_CR       = 1 << 20,
    PCREGEX_MATCH_NEWLINE_LF       = 1 << 21,
    PCREGEX_MATCH_NEWLINE_CRLF     = PCREGEX_MATCH_NEWLINE_CR | PCREGEX_MATCH_NEWLINE_LF,
    PCREGEX_MATCH_NEWLINE_ANY      = 1 << 22,
    PCREGEX_MATCH_NEWLINE_ANYCRLF  = PCREGEX_MATCH_NEWLINE_CR | PCREGEX_MATCH_NEWLINE_ANY,
    PCREGEX_MATCH_BSR_ANYCRLF      = 1 << 23,
    PCREGEX_MATCH_BSR_ANY          = 1 << 24,
    PCREGEX_MATCH_PARTIAL_SOFT     = PCREGEX_MATCH_PARTIAL,
    PCREGEX_MATCH_PARTIAL_HARD     = 1 << 27,
    PCREGEX_MATCH_NOTEMPTY_ATSTART = 1 << 28
};


struct pcregex;
struct pcregex_match_info;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


/*
 * Scans for a match in string for pattern
 */
bool pcregex_is_match_ex(const char *pattern, const char *str,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options);

bool pcregex_is_match(const char *pattern, const char *str);

/*
 * Compiles the regular expression to an internal form
 *
 */
struct pcregex *pcregex_new_ex(const char *pattern,
        enum pcregex_compile_flags compile_options,
        enum pcregex_match_flags match_options);

struct pcregex *pcregex_new(const char *pattern);

void pcregex_destroy(struct pcregex *regex);


/*
 * Scans for a match in string for the pattern in regex.
 * The match_options are combined with the match options specified when
 * the regex structure was created.
 */
bool pcregex_match_ex(struct pcregex *regex, const char *str,
            enum pcregex_match_flags match_options,
            struct pcregex_match_info **match_info);

bool pcregex_match(struct pcregex *regex, const char *str,
            struct pcregex_match_info **match_info);

/*
 * Returns whether the previous match operation succeeded.
 */
bool pcregex_match_info_matches(const struct pcregex_match_info *match_info);

/*
 * Scans for the next match_info
 */
bool pcregex_match_info_next(const struct pcregex_match_info *match_info);

/*
 * Retrieves the text matching the match_num 'th capturing parentheses.
 * 0 is the full text of the match, 1 is the first paren set,
 * 2 the second, and so on.
 */
char *pcregex_match_info_fetch(const struct pcregex_match_info *match_info,
            int match_num);

void pcregex_match_info_destroy(struct pcregex_match_info *match_info);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_REGEX_H */

