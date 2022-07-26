/**
 * @file error.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser errors.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_TOKENIZER_ERROR_H
#define PCHTML_HTML_TOKENIZER_ERROR_H

#include "config.h"
#include "private/array_obj.h"

#include "html/base.h"
#include "html/tokenizer.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    /* abrupt-closing-of-empty-comment */
    PCHTML_HTML_TOKENIZER_ERROR_ABCLOFEMCO         = 0x0000,
    /* abrupt-doctype-public-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_ABDOPUID           = 0x0001,
    /* abrupt-doctype-system-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_ABDOSYID           = 0x0002,
    /* absence-of-digits-in-numeric-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_ABOFDIINNUCHRE     = 0x0003,
    /* cdata-in-html-content */
    PCHTML_HTML_TOKENIZER_ERROR_CDINHTCO           = 0x0004,
    /* character-reference-outside-unicode-range */
    PCHTML_HTML_TOKENIZER_ERROR_CHREOUUNRA         = 0x0005,
    /* control-character-in-input-stream */
    PCHTML_HTML_TOKENIZER_ERROR_COCHININST         = 0x0006,
    /* control-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_COCHRE             = 0x0007,
    /* end-tag-with-attributes */
    PCHTML_HTML_TOKENIZER_ERROR_ENTAWIAT           = 0x0008,
    /* duplicate-attribute */
    PCHTML_HTML_TOKENIZER_ERROR_DUAT               = 0x0009,
    /* end-tag-with-trailing-solidus */
    PCHTML_HTML_TOKENIZER_ERROR_ENTAWITRSO         = 0x000A,
    /* eof-before-tag-name */
    PCHTML_HTML_TOKENIZER_ERROR_EOBETANA           = 0x000B,
    /* eof-in-cdata */
    PCHTML_HTML_TOKENIZER_ERROR_EOINCD             = 0x000C,
    /* eof-in-comment */
    PCHTML_HTML_TOKENIZER_ERROR_EOINCO             = 0x000D,
    /* eof-in-doctype */
    PCHTML_HTML_TOKENIZER_ERROR_EOINDO             = 0x000E,
    /* eof-in-script-html-comment-like-text */
    PCHTML_HTML_TOKENIZER_ERROR_EOINSCHTCOLITE     = 0x000F,
    /* eof-in-tag */
    PCHTML_HTML_TOKENIZER_ERROR_EOINTA             = 0x0010,
    /* incorrectly-closed-comment */
    PCHTML_HTML_TOKENIZER_ERROR_INCLCO             = 0x0011,
    /* incorrectly-opened-comment */
    PCHTML_HTML_TOKENIZER_ERROR_INOPCO             = 0x0012,
    /* invalid-character-sequence-after-doctype-name */
    PCHTML_HTML_TOKENIZER_ERROR_INCHSEAFDONA       = 0x0013,
    /* invalid-first-character-of-tag-name */
    PCHTML_HTML_TOKENIZER_ERROR_INFICHOFTANA       = 0x0014,
    /* missing-attribute-value */
    PCHTML_HTML_TOKENIZER_ERROR_MIATVA             = 0x0015,
    /* missing-doctype-name */
    PCHTML_HTML_TOKENIZER_ERROR_MIDONA             = 0x0016,
    /* missing-doctype-public-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_MIDOPUID           = 0x0017,
    /* missing-doctype-system-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_MIDOSYID           = 0x0018,
    /* missing-end-tag-name */
    PCHTML_HTML_TOKENIZER_ERROR_MIENTANA           = 0x0019,
    /* missing-quote-before-doctype-public-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOPUID       = 0x001A,
    /* missing-quote-before-doctype-system-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_MIQUBEDOSYID       = 0x001B,
    /* missing-semicolon-after-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_MISEAFCHRE         = 0x001C,
    /* missing-whitespace-after-doctype-public-keyword */
    PCHTML_HTML_TOKENIZER_ERROR_MIWHAFDOPUKE       = 0x001D,
    /* missing-whitespace-after-doctype-system-keyword */
    PCHTML_HTML_TOKENIZER_ERROR_MIWHAFDOSYKE       = 0x001E,
    /* missing-whitespace-before-doctype-name */
    PCHTML_HTML_TOKENIZER_ERROR_MIWHBEDONA         = 0x001F,
    /* missing-whitespace-between-attributes */
    PCHTML_HTML_TOKENIZER_ERROR_MIWHBEAT           = 0x0020,
    /* missing-whitespace-between-doctype-public-and-system-identifiers */
    PCHTML_HTML_TOKENIZER_ERROR_MIWHBEDOPUANSYID   = 0x0021,
    /* nested-comment */
    PCHTML_HTML_TOKENIZER_ERROR_NECO               = 0x0022,
    /* noncharacter-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_NOCHRE             = 0x0023,
    /* noncharacter-in-input-stream */
    PCHTML_HTML_TOKENIZER_ERROR_NOININST           = 0x0024,
    /* non-void-html-element-start-tag-with-trailing-solidus */
    PCHTML_HTML_TOKENIZER_ERROR_NOVOHTELSTTAWITRSO = 0x0025,
    /* null-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_NUCHRE             = 0x0026,
    /* surrogate-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_SUCHRE             = 0x0027,
    /* surrogate-in-input-stream */
    PCHTML_HTML_TOKENIZER_ERROR_SUININST           = 0x0028,
    /* unexpected-character-after-doctype-system-identifier */
    PCHTML_HTML_TOKENIZER_ERROR_UNCHAFDOSYID       = 0x0029,
    /* unexpected-character-in-attribute-name */
    PCHTML_HTML_TOKENIZER_ERROR_UNCHINATNA         = 0x002A,
    /* unexpected-character-in-unquoted-attribute-value */
    PCHTML_HTML_TOKENIZER_ERROR_UNCHINUNATVA       = 0x002B,
    /* unexpected-equals-sign-before-attribute-name */
    PCHTML_HTML_TOKENIZER_ERROR_UNEQSIBEATNA       = 0x002C,
    /* unexpected-null-character */
    PCHTML_HTML_TOKENIZER_ERROR_UNNUCH             = 0x002D,
    /* unexpected-question-mark-instead-of-tag-name */
    PCHTML_HTML_TOKENIZER_ERROR_UNQUMAINOFTANA     = 0x002E,
    /* unexpected-solidus-in-tag */
    PCHTML_HTML_TOKENIZER_ERROR_UNSOINTA           = 0x002F,
    /* unknown-named-character-reference */
    PCHTML_HTML_TOKENIZER_ERROR_UNNACHRE           = 0x0030,
    PCHTML_HTML_TOKENIZER_ERROR_LAST_ENTRY         = 0x0031,
}
pchtml_html_tokenizer_error_id_t;

typedef struct {
    const unsigned char              *pos;
    pchtml_html_tokenizer_error_id_t id;
}
pchtml_html_tokenizer_error_t;


pchtml_html_tokenizer_error_t *
pchtml_html_tokenizer_error_add(pcutils_array_obj_t *parse_errors,
                             const unsigned char *pos,
                             pchtml_html_tokenizer_error_id_t id) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TOKENIZER_ERROR_H */

